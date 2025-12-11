#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <filesystem>
#include <vector>

#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "msgq/event.h"

#ifndef __APPLE__
#include <sys/eventfd.h>
#endif

void event_state_shm_mmap(std::string endpoint, std::string identifier, char **shm_mem, std::string *shm_path) {
  const char* op_prefix = std::getenv("OPENPILOT_PREFIX");

  #ifdef __APPLE__
  std::string full_path = "/tmp/";
  #else
  std::string full_path = "/dev/shm/";
  #endif

  if (op_prefix) {
    full_path += std::string(op_prefix) + "/";
  }
  full_path += CEREAL_EVENTS_PREFIX + "/";
  if (identifier.size() > 0) {
    full_path += identifier + "/";
  }
  std::filesystem::create_directories(full_path);
  full_path += endpoint;

  int shm_fd = open(full_path.c_str(), O_RDWR | O_CREAT, 0664);
  if (shm_fd < 0) {
    throw std::runtime_error("Could not open shared memory file.");
  }

  int rc = ftruncate(shm_fd, sizeof(EventState));
  if (rc < 0){
    close(shm_fd);
    throw std::runtime_error("Could not truncate shared memory file.");
  }

  char * mem = (char*)mmap(NULL, sizeof(EventState), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  close(shm_fd);
  if (mem == nullptr) {
    throw std::runtime_error("Could not map shared memory file.");
  }

  if (shm_mem != nullptr)
    *shm_mem = mem;
  if (shm_path != nullptr)
    *shm_path = full_path;
}

SocketEventHandle::SocketEventHandle(std::string endpoint, std::string identifier, bool override) {
  char *mem;
  event_state_shm_mmap(endpoint, identifier, &mem, &this->shm_path);

  this->state = (EventState*)mem;
  if (override) {
    #ifdef __APPLE__
    if (pipe(&this->state->fds[0]) < 0 || pipe(&this->state->fds[2]) < 0) {
       throw std::runtime_error("pipe creation failed");
    }
    // flags
    for (int i=0; i<4; ++i) {
      int flags = fcntl(this->state->fds[i], F_GETFL, 0);
      fcntl(this->state->fds[i], F_SETFL, flags | O_NONBLOCK);
    }
    #else
    this->state->fds[0] = eventfd(0, EFD_NONBLOCK);
    this->state->fds[1] = eventfd(0, EFD_NONBLOCK);
    #endif
  }
}

SocketEventHandle::~SocketEventHandle() {
  #ifdef __APPLE__
  close(this->state->fds[0]);
  close(this->state->fds[1]);
  close(this->state->fds[2]);
  close(this->state->fds[3]);
  #else
  close(this->state->fds[0]);
  close(this->state->fds[1]);
  #endif
  munmap(this->state, sizeof(EventState));
  unlink(this->shm_path.c_str());
}

bool SocketEventHandle::is_enabled() {
  return this->state->enabled;
}

void SocketEventHandle::set_enabled(bool enabled) {
  this->state->enabled = enabled;
}

Event SocketEventHandle::recv_called() {
  #ifdef __APPLE__
  return Event(this->state->fds[0], this->state->fds[1]);
  #else
  return Event(this->state->fds[0]);
  #endif
}

Event SocketEventHandle::recv_ready() {
  #ifdef __APPLE__
  return Event(this->state->fds[2], this->state->fds[3]);
  #else
  return Event(this->state->fds[1]);
  #endif
}

void SocketEventHandle::toggle_fake_events(bool enabled) {
  if (enabled)
    setenv("CEREAL_FAKE", "1", true);
  else
    unsetenv("CEREAL_FAKE");
}

void SocketEventHandle::set_fake_prefix(std::string prefix) {
  if (prefix.size() == 0) {
    unsetenv("CEREAL_FAKE_PREFIX");
  } else {
    setenv("CEREAL_FAKE_PREFIX", prefix.c_str(), true);
  }
}

std::string SocketEventHandle::fake_prefix() {
  const char* prefix = std::getenv("CEREAL_FAKE_PREFIX");
  if (prefix == nullptr) {
    return "";
  } else {
    return std::string(prefix);
  }
}

Event::Event(int fd, int write_fd): event_fd(fd), write_fd(write_fd) {}

void Event::set() const {
  throw_if_invalid();

  uint64_t val = 1;
  #ifdef __APPLE__
  size_t count = write(this->write_fd, &val, sizeof(uint64_t));
  #else
  size_t count = write(this->event_fd, &val, sizeof(uint64_t));
  #endif
  assert(count == sizeof(uint64_t));
}

int Event::clear() const {
  throw_if_invalid();

  uint64_t val = 0;
  #ifdef __APPLE__
  // Consume all events from the pipe
  int count = 0;
  while (read(this->event_fd, &val, sizeof(uint64_t)) > 0) {
    count++;
  }
  return count;
  #else
  // read the eventfd to clear it
  read(this->event_fd, &val, sizeof(uint64_t));
  return val;
  #endif
}

void Event::wait(int timeout_sec) const {
  throw_if_invalid();

  int event_count;
  struct pollfd fds = { this->event_fd, POLLIN, 0 };
  struct timespec timeout = { timeout_sec, 0 };;

  sigset_t signals;
  sigfillset(&signals);
  sigdelset(&signals, SIGALRM);
  sigdelset(&signals, SIGINT);
  sigdelset(&signals, SIGTERM);
  sigdelset(&signals, SIGQUIT);

  event_count = ppoll(&fds, 1, timeout_sec < 0 ? nullptr : &timeout, &signals);

  if (event_count == 0) {
    throw std::runtime_error("Event timed out pid: " + std::to_string(getpid()));
  } else if (event_count < 0) {
    throw std::runtime_error("Event poll failed, errno: " + std::to_string(errno) + " pid: " + std::to_string(getpid()));
  }
}

bool Event::peek() const {
  throw_if_invalid();

  int event_count;

  struct pollfd fds = { this->event_fd, POLLIN, 0 };

  // poll with timeout zero to return status immediately
  event_count = poll(&fds, 1, 0);

  return event_count != 0;
}

bool Event::is_valid() const {
  return event_fd != -1;
}

int Event::fd() const {
  return event_fd;
}

int Event::wait_for_one(const std::vector<Event>& events, int timeout_sec) {
  struct pollfd fds[events.size()];
  for (size_t i = 0; i < events.size(); i++) {
    fds[i] = { events[i].fd(), POLLIN, 0 };
  }

  struct timespec timeout = { timeout_sec, 0 };

  sigset_t signals;
  sigfillset(&signals);
  sigdelset(&signals, SIGALRM);
  sigdelset(&signals, SIGINT);
  sigdelset(&signals, SIGTERM);
  sigdelset(&signals, SIGQUIT);

  int event_count = ppoll(fds, events.size(), timeout_sec < 0 ? nullptr : &timeout, &signals);

  if (event_count == 0) {
    throw std::runtime_error("Event timed out pid: " + std::to_string(getpid()));
  } else if (event_count < 0) {
    throw std::runtime_error("Event poll failed, errno: " + std::to_string(errno) + " pid: " + std::to_string(getpid()));
  }

  for (size_t i = 0; i < events.size(); i++) {
    if (fds[i].revents & POLLIN) {
      return i;
    }
  }

  throw std::runtime_error("Event poll failed, no events ready");
}
