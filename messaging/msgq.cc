#include <iostream>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <cstdlib>


#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>


#include <stdio.h>

#include "msgq.hpp"


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
    ;
  }

  return;
}



int msgq_new_queue(msgq_queue_t * q, const char * path, size_t size){
  assert(size < 0xFFFFFFFF); // Buffer must be smaller than 2^32 bytes

  const char * prefix = "/dev/shm/";
  char * full_path = new char[strlen(path) + strlen(prefix) + 1];
  strcpy(full_path, prefix);
  strcat(full_path, path);

  auto fd = open(full_path, O_RDWR | O_CREAT, 0777);
  delete[] full_path;

  assert(fd >= 0); // TODO: properly handle exit codes
  if (fd < 0)
    return -1;

  int rc = ftruncate(fd, size + sizeof(msgq_header_t));
  assert(rc == 0); // TODO: properly handle exit codes
  if (rc < 0)
    return -1;

  char * mem = (char*)mmap(NULL, size + sizeof(msgq_header_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  assert(mem != NULL); // TODO: properly handle exit codes
  if (mem == NULL)
    return -1;

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
  q->read_fifo = -1;

  return 0;
}

void msgq_close_queue(msgq_queue_t *q){
  if (q->read_fifo >= 0){
    close(q->read_fifo);
    remove(q->read_fifo_path.c_str());
  }

  for (uint64_t i = 0; i < NUM_READERS; i++){
    if (q->read_fifos[i] >= 0){
      close(q->read_fifos[i]);
    }
  }

  if (q->mmap_p != NULL){
    munmap(q->mmap_p, q->size + sizeof(msgq_header_t));
  }
}


void msgq_init_publisher(msgq_queue_t * q) {
  std::cout << "Starting publisher" << std::endl;

  uint64_t uid = getpid();

  *q->write_uid = uid;
  *q->num_readers = 0;

  for (size_t i = 0; i < NUM_READERS; i++){
    *q->read_valids[i] = false;
    q->read_fifos[i] = -1;
    q->read_fifos_uid[i] = 0;
    *q->read_uids[i] = 0;
  }

  q->write_uid_local = uid;
}

void msgq_init_subscriber(msgq_queue_t * q) {
  assert(q != NULL);
  assert(q->num_readers != NULL);

  uint64_t uid = getpid();

  // Get reader id
  while (true){
    uint64_t cur_num_readers = *q->num_readers;
    uint64_t new_num_readers = cur_num_readers + 1;

    // No more slots available. Reset all subscribers to kick out inactive ones
    if (new_num_readers > NUM_READERS){
      std::cout << "Warning, evicting all subscribers!" << std::endl;
      *q->num_readers = 0;

      for (size_t i = 0; i < NUM_READERS; i++){
        *q->read_valids[i] = false;
        *q->read_uids[i] = 0;
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

  for (size_t i = 0; i < NUM_READERS; i++){
    q->read_fifos[i] = -1;
  }

  q->read_fifo_path = "/dev/shm/fifo-";
  q->read_fifo_path += std::to_string(q->read_uid_local);

  std::cout << q->read_fifo_path << std::endl;
  int r = mkfifo(q->read_fifo_path.c_str(), 0777);
  if (r != 0)
    perror("Fifo: ");
  assert(r == 0);

  q->read_fifo = open(q->read_fifo_path.c_str(), O_RDWR | O_NONBLOCK);

  // Fysnc so the fifo shows up in the directory
  auto shm_fd = open("/dev/shm", O_RDONLY);
  fsync(shm_fd);
  close(shm_fd);

  std::cout << "New subscriber id: " << q->reader_id << " uid: " << q->read_uid_local << " " << q->endpoint << std::endl;
  msgq_reset_reader(q);
}

int msgq_msg_send(msgq_msg_t * msg, msgq_queue_t *q){
  // Die if we are no longer the active publisher
  if (q->write_uid_local != *q->write_uid){
    std::cout << "Killing old publisher: " << q->endpoint << std::endl;
    assert(q->write_uid_local == *q->write_uid);
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

    // Open fifo when not set, or when reader changes
    if (q->read_fifos[i] == -1 || q->read_fifos_uid[i] != reader_uid){
      // Close old reader fifo
      if (q->read_fifos[i] >= 0){
        close(q->read_fifos[i]);
      }

      q->read_fifos_uid[i] = reader_uid;

      std::string path = "/dev/shm/fifo-";
      path += std::to_string(reader_uid);

      q->read_fifos[i] = open(path.c_str(), O_RDWR | O_NONBLOCK);
      if(q->read_fifos[i] < 0){
        std::cout << "Fifo: " << path << std::endl;
        perror("Error opening fifo");
      }
    }

    uint8_t m = 1;
    write(q->read_fifos[i], &m, 1);
  }

  return msg->size;
}

int msgq_get_fd(msgq_queue_t * q){
  int id = q->reader_id;
  assert(id >= 0); // Make sure subscriber is initialized

  if (q->read_uid_local != *q->read_uids[id]){
    std::cout << q->endpoint << ": Reader was evicted, reconnecting" << std::endl;
    msgq_init_subscriber(q);
  }

  return q->read_fifo;
}


int msgq_msg_ready(msgq_queue_t * q){
 start:
  int id = q->reader_id;
  assert(id >= 0); // Make sure subscriber is initialized

  if (q->read_uid_local != *q->read_uids[id]){
    std::cout << q->endpoint << ": Reader was evicted, reconnecting" << std::endl;
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

  // Check if new message is available
  return (read_pointer != write_pointer);
}

int msgq_msg_recv(msgq_msg_t * msg, msgq_queue_t * q){
 start:
  int id = q->reader_id;
  assert(id >= 0); // Make sure subscriber is initialized

  if (q->read_uid_local != *q->read_uids[id]){
    std::cout << q->endpoint << ": Reader was evicted, reconnecting" << std::endl;
    msgq_init_subscriber(q);
    goto start;
  }

  // Read one byte from fifo
  char buf[1];
  read(q->read_fifo, buf, 1);

  // Check valid
  if (!*q->read_valids[id]){
    msgq_reset_reader(q);
    goto start;
  }

  uint32_t read_cycles, read_pointer;
  UNPACK64(read_cycles, read_pointer, *q->read_pointers[id]);

  uint32_t write_cycles, write_pointer;
  UNPACK64(write_cycles, write_pointer, *q->write_pointer);

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
  assert(timeout >= 0);

  int num = 0;
  struct pollfd * fds = (struct pollfd *)calloc(nitems, sizeof(struct pollfd));

  // Build poll structure
  for (size_t i = 0; i < nitems; i++){
    fds[i].fd = msgq_get_fd(items[i].q);
    fds[i].events = POLLIN;

    // Check if message is ready in case we get out of sync with the pipe
    if (msgq_msg_ready(items[i].q)){
      items[i].revents = 1;
      timeout = 0; // No timeout if a message is ready
      num++;
    } else {
      items[i].revents = 0;
    }
  }

  poll(fds, nitems, timeout);

  // Read poll results
  for (size_t i = 0; i < nitems; i++){
    if (fds[i].revents && !items[i].revents){
      // Don't add it if it was already added
      num++;
      items[i].revents = 1;
    }
  }

  free(fds);
  return num;
}
