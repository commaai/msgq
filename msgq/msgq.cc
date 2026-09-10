#include <iostream>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <csignal>
#include <random>
#include <string>
#include <limits>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <stdio.h>

#include "msgq/msgq.h"

#ifdef _WIN32
// Readers wait on a per-thread named event; publishers signal it by thread id, the low 32 bits of a reader uid
static std::string msgq_event_name(uint32_t tid) {
  return "Local\\msgq_tid_" + std::to_string(tid);
}

static HANDLE msgq_thread_event() {
  struct ThreadEvent {
    HANDLE h = CreateEventA(NULL, FALSE, FALSE, msgq_event_name(GetCurrentThreadId()).c_str());
    ~ThreadEvent() { if (h != NULL) CloseHandle(h); }
  };
  thread_local ThreadEvent ev;
  return ev.h;
}
#else
void sigusr2_handler(int signal) {
  assert(signal == SIGUSR2);
}
#endif

std::string msgq_shm_dir() {
#ifdef __APPLE__
  return "/tmp";
#else
  return "/dev/shm";
#endif
}

uint64_t msgq_get_uid(void){
  std::random_device rd("/dev/urandom");
  std::uniform_int_distribution<uint64_t> distribution(0, std::numeric_limits<uint32_t>::max());

  #ifdef __APPLE__
    // TODO: this doesn't work
    uint64_t uid = distribution(rd) << 32 | getpid();
  #elif defined(_WIN32)
    uint64_t uid = distribution(rd) << 32 | GetCurrentThreadId();
  #else
    uint64_t uid = distribution(rd) << 32 | syscall(SYS_gettid);
  #endif

  return uid;
}

int msgq_msg_init_size(msgq_msg_t * msg, size_t size){
  msg->size = size;
  msg->data = new(std::nothrow) char[size];

  return (msg->data == NULL) ? -1 : 0;
}


int msgq_msg_init_data(msgq_msg_t * msg, char * data, size_t size) {
  int r = msgq_msg_init_size(msg, size);

  if (r == 0)
    memcpy(msg->data, data, size);

  return r;
}

int msgq_msg_close(msgq_msg_t * msg){
  if (msg->size > 0)
    delete[] msg->data;

  msg->size = 0;

  return 0;
}

void msgq_reset_reader(msgq_queue_t * q){
  int id = q->reader_id;
  q->read_valids[id]->store(true);
  q->read_pointers[id]->store(*q->write_pointer);
}

void msgq_wait_for_subscriber(msgq_queue_t *q){
  while (*q->num_readers == 0){
    // wait for subscriber
  }
}

int msgq_new_queue(msgq_queue_t * q, const char * path, size_t size){
  assert(size < 0xFFFFFFFF); // Buffer must be smaller than 2^32 bytes
#ifndef _WIN32
  std::signal(SIGUSR2, sigusr2_handler);
#endif

#ifdef _WIN32
  std::string base_path = "Local\\msgq_";  // a named section, kept by the kernel while anyone holds it
#else
  std::string base_path = msgq_shm_dir() + "/msgq_";
#endif
  const char* prefix = std::getenv("OPENPILOT_PREFIX");
  if (prefix) {
    base_path += std::string(prefix) + "/";
  }
  std::string full_path = base_path + path;

#ifdef _WIN32
  HANDLE section = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)(size + sizeof(msgq_header_t)), full_path.c_str());
  if (section == NULL) {
    std::cout << "Warning, could not open: " << full_path << std::endl;
    return -1;
  }

  char * mem = (char*)MapViewOfFile(section, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (mem == NULL){
    CloseHandle(section);
    return -1;
  }
  q->section = section;  // the name only lives while a handle does, so keep it until the queue closes
#else
  auto fd = open(full_path.c_str(), O_RDWR | O_CREAT, 0664);
  if (fd < 0) {
    std::cout << "Warning, could not open: " << full_path << std::endl;
    return -1;
  }

  int rc = ftruncate(fd, size + sizeof(msgq_header_t));
  if (rc < 0){
    close(fd);
    return -1;
  }

  int mmap_flags = MAP_SHARED;
#ifdef MAP_POPULATE
  if (std::getenv("MSGQ_PREALLOC")) {
    mmap_flags |= MAP_POPULATE;
  }
#endif

  char * mem = (char*)mmap(NULL, size + sizeof(msgq_header_t), PROT_READ | PROT_WRITE, mmap_flags, fd, 0);
  close(fd);

  if (mem == MAP_FAILED){
    return -1;
  }
#endif

  q->mmap_p = mem;

  msgq_header_t *header = (msgq_header_t *)mem;

  // Setup pointers to header segment
  q->num_readers = reinterpret_cast<std::atomic<uint64_t>*>(&header->num_readers);
  q->write_pointer = reinterpret_cast<std::atomic<uint64_t>*>(&header->write_pointer);
  q->write_uid = reinterpret_cast<std::atomic<uint64_t>*>(&header->write_uid);

  for (size_t i = 0; i < NUM_READERS; i++){
    q->read_pointers[i] = reinterpret_cast<std::atomic<uint64_t>*>(&header->read_pointers[i]);
    q->read_valids[i] = reinterpret_cast<std::atomic<uint64_t>*>(&header->read_valids[i]);
    q->read_uids[i] = reinterpret_cast<std::atomic<uint64_t>*>(&header->read_uids[i]);
  }

  q->data = mem + sizeof(msgq_header_t);
  q->size = size;
  q->reader_id = -1;

  q->endpoint = path;
  q->read_conflate = false;

  return 0;
}

void msgq_close_queue(msgq_queue_t *q){
  if (q->mmap_p != NULL){
#ifdef _WIN32
    UnmapViewOfFile(q->mmap_p);
    CloseHandle(q->section);
#else
    munmap(q->mmap_p, q->size + sizeof(msgq_header_t));
#endif
  }
}


void msgq_init_publisher(msgq_queue_t * q) {
  //std::cout << "Starting publisher" << std::endl;
  uint64_t uid = msgq_get_uid();

  *q->write_uid = uid;
  *q->num_readers = 0;

  for (size_t i = 0; i < NUM_READERS; i++){
    *q->read_valids[i] = false;
    *q->read_uids[i] = 0;
  }

  q->write_uid_local = uid;
}

static void thread_signal(uint32_t tid) {
  #ifdef __APPLE__
    // macOS doesn't have tkill, rely on polling instead
    (void)tid;
  #elif defined(_WIN32)
    HANDLE ev = OpenEventA(EVENT_MODIFY_STATE, FALSE, msgq_event_name(tid).c_str());
    if (ev != NULL) {
      SetEvent(ev);
      CloseHandle(ev);
    }
  #elif !defined(SYS_tkill)
    // fallback for systems without tkill
    kill(tid, SIGUSR2);
  #else
    syscall(SYS_tkill, tid, SIGUSR2);
  #endif
}

void msgq_init_subscriber(msgq_queue_t * q) {
  assert(q != NULL);
  assert(q->num_readers != NULL);

  uint64_t uid = msgq_get_uid();

  // Get reader id
  while (true){
    uint64_t cur_num_readers = *q->num_readers;
    uint64_t new_num_readers = cur_num_readers + 1;

    // No more slots available. Reset all subscribers to kick out inactive ones
    if (new_num_readers > NUM_READERS){
      //std::cout << "Warning, evicting all subscribers!" << std::endl;
      *q->num_readers = 0;

      for (size_t i = 0; i < NUM_READERS; i++){
        *q->read_valids[i] = false;

        uint64_t old_uid = *q->read_uids[i];
        *q->read_uids[i] = 0;

        // Wake up reader in case they are in a poll
        thread_signal(old_uid & 0xFFFFFFFF);
      }

      continue;
    }

    // Use atomic compare and swap to handle race condition
    // where two subscribers start at the same time
    if (std::atomic_compare_exchange_strong(q->num_readers,
                                            &cur_num_readers,
                                            new_num_readers)){
      q->reader_id = cur_num_readers;
      q->read_uid_local = uid;

      // We start with read_valid = false,
      // on the first read the read pointer will be synchronized with the write pointer
      *q->read_valids[cur_num_readers] = false;
      *q->read_pointers[cur_num_readers] = 0;
      *q->read_uids[cur_num_readers] = uid;
      break;
    }
  }

  //std::cout << "New subscriber id: " << q->reader_id << " uid: " << q->read_uid_local << " " << q->endpoint << std::endl;
  msgq_reset_reader(q);
}

int msgq_msg_send(msgq_msg_t * msg, msgq_queue_t *q){
  // Die if we are no longer the active publisher
  if (q->write_uid_local != *q->write_uid){
    std::cout << "Killing old publisher: " << q->endpoint << std::endl;
    errno = EADDRINUSE;
    return -1;
  }

  uint64_t total_msg_size = ALIGN(msg->size + sizeof(int64_t));

  // We need to fit at least three messages in the queue,
  // then we can always safely access the last message
  assert(3 * total_msg_size <= q->size);

  uint64_t num_readers = *q->num_readers;

  uint32_t write_cycles, write_pointer;
  UNPACK64(write_cycles, write_pointer, *q->write_pointer);

  char *p = q->data + write_pointer; // add base offset

  // Check remaining space
  // Always leave space for a wraparound tag for the next message, including alignment
  int64_t remaining_space = q->size - write_pointer - total_msg_size - sizeof(int64_t);
  if (remaining_space <= 0){
    // Write -1 size tag indicating wraparound
    *(int64_t*)p = -1;

    // Invalidate all readers that are beyond the write pointer
    // TODO: should we handle the case where a new reader shows up while this is running?
    for (uint64_t i = 0; i < num_readers; i++){
      uint64_t read_pointer = *q->read_pointers[i];
      uint64_t read_cycles = read_pointer >> 32;
      read_pointer &= 0xFFFFFFFF;

      if ((read_pointer > write_pointer) && (read_cycles != write_cycles)) {
        *q->read_valids[i] = false;
      }
    }

    // Update global and local copies of write pointer and write_cycles
    write_pointer = 0;
    write_cycles = write_cycles + 1;
    PACK64(*q->write_pointer, write_cycles, write_pointer);

    // Set actual pointer to the beginning of the data segment
    p = q->data;
  }

  // Invalidate readers that are in the area that will be written
  uint64_t start = write_pointer;
  uint64_t end = ALIGN(start + sizeof(int64_t) + msg->size);

  for (uint64_t i = 0; i < num_readers; i++){
    uint32_t read_cycles, read_pointer;
    UNPACK64(read_cycles, read_pointer, *q->read_pointers[i]);

    if ((read_pointer >= start) && (read_pointer < end) && (read_cycles != write_cycles)) {
      *q->read_valids[i] = false;
    }
  }


  // Write size tag
  std::atomic<int64_t> *size_p = reinterpret_cast<std::atomic<int64_t>*>(p);
  *size_p = msg->size;

  // Copy data
  memcpy(p + sizeof(int64_t), msg->data, msg->size);
  __sync_synchronize();

  // Update write pointer
  uint32_t new_ptr = ALIGN(write_pointer + msg->size + sizeof(int64_t));
  PACK64(*q->write_pointer, write_cycles, new_ptr);

  // Notify readers
  for (uint64_t i = 0; i < num_readers; i++){
    uint64_t reader_uid = *q->read_uids[i];
    thread_signal(reader_uid & 0xFFFFFFFF);
  }

  return msg->size;
}


int msgq_msg_ready(msgq_queue_t * q){
 start:
  int id = q->reader_id;
  assert(id >= 0); // Make sure subscriber is initialized

  if (q->read_uid_local != *q->read_uids[id]){
    //std::cout << q->endpoint << ": Reader was evicted, reconnecting" << std::endl;
    msgq_init_subscriber(q);
    goto start;
  }

  // Check valid
  if (!*q->read_valids[id]){
    msgq_reset_reader(q);
    goto start;
  }

  uint32_t read_cycles, read_pointer;
  UNPACK64(read_cycles, read_pointer, *q->read_pointers[id]);
  UNUSED(read_cycles);

  uint32_t write_cycles, write_pointer;
  UNPACK64(write_cycles, write_pointer, *q->write_pointer);
  UNUSED(write_cycles);

  // Check if new message is available
  return (read_pointer != write_pointer);
}

int msgq_msg_recv(msgq_msg_t * msg, msgq_queue_t * q){
 start:
  int id = q->reader_id;
  assert(id >= 0); // Make sure subscriber is initialized

  if (q->read_uid_local != *q->read_uids[id]){
    //std::cout << q->endpoint << ": Reader was evicted, reconnecting" << std::endl;
    msgq_init_subscriber(q);
    goto start;
  }

  // Check valid
  if (!*q->read_valids[id]){
    msgq_reset_reader(q);
    goto start;
  }

  uint32_t read_cycles, read_pointer;
  UNPACK64(read_cycles, read_pointer, *q->read_pointers[id]);

  uint32_t write_cycles, write_pointer;
  UNPACK64(write_cycles, write_pointer, *q->write_pointer);
  UNUSED(write_cycles);

  char * p = q->data + read_pointer;

  // Check if new message is available
  if (read_pointer == write_pointer) {
    msg->size = 0;
    return 0;
  }

  // Read potential message size
  std::atomic<int64_t> *size_p = reinterpret_cast<std::atomic<int64_t>*>(p);
  std::int64_t size = *size_p;

  // Check if the size that was read is valid
  if (!*q->read_valids[id]){
    msgq_reset_reader(q);
    goto start;
  }

  // If size is -1 the buffer was full, and we need to wrap around
  if (size == -1){
    read_cycles++;
    PACK64(*q->read_pointers[id], read_cycles, 0);
    goto start;
  }

  // crashing is better than passing garbage data to the consumer
  // the size will have weird value if it was overwritten by data accidentally
  assert((uint64_t)size < q->size);
  assert(size > 0);

  uint32_t new_read_pointer = ALIGN(read_pointer + sizeof(std::int64_t) + size);

  // If conflate is true, check if this is the latest message, else start over
  if (q->read_conflate){
    if (new_read_pointer != write_pointer){
      // Update read pointer
      PACK64(*q->read_pointers[id], read_cycles, new_read_pointer);
      goto start;
    }
  }

  // Copy message
  if (msgq_msg_init_size(msg, size) < 0)
    return -1;

  __sync_synchronize();
  memcpy(msg->data, p + sizeof(int64_t), size);
  __sync_synchronize();

  // Update read pointer
  PACK64(*q->read_pointers[id], read_cycles, new_read_pointer);

  // Check if the actual data that was copied is valid
  if (!*q->read_valids[id]){
    msgq_msg_close(msg);
    msgq_reset_reader(q);
    goto start;
  }


  return msg->size;
}



int msgq_poll(msgq_pollitem_t * items, size_t nitems, int timeout){
  int num = 0;

  // Check if messages ready
  for (size_t i = 0; i < nitems; i++) {
    items[i].revents = msgq_msg_ready(items[i].q);
    if (items[i].revents) num++;
  }

  int ms = (timeout == -1) ? 100 : timeout;
  auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
#ifdef __APPLE__
  ms = std::min(ms, 10);  // signals can't interrupt nanosleep on macOS, so poll more frequently
#endif

  while (num == 0) {
#ifdef _WIN32
    WaitForSingleObject(msgq_thread_event(), ms);
#else
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000 * 1000};
    nanosleep(&ts, NULL);
#endif

    // Check if messages ready
    for (size_t i = 0; i < nitems; i++) {
      if (items[i].revents == 0 && msgq_msg_ready(items[i].q)){
        num += 1;
        items[i].revents = 1;
      }
    }

    if (timeout != -1) {
      auto left = std::chrono::ceil<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
      if (left <= 0) break;
      ms = std::min<int>(ms, left);
    }
  }

  return num;
}

bool msgq_all_readers_updated(msgq_queue_t *q) {
  uint64_t num_readers = *q->num_readers;
  for (uint64_t i = 0; i < num_readers; i++) {
    if (*q->read_valids[i] && *q->write_pointer != *q->read_pointers[i]) {
      return false;
    }
  }
  return num_readers > 0;
}
