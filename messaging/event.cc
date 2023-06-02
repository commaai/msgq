#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <filesystem>
#include <algorithm>

#ifndef __APPLE__
#include <sys/eventfd.h>
#endif
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>

#include "cereal/messaging/event.h"

void event_state_shm_mmap(std::string endpoint, std::string identifier, char **shm_mem, std::string *shm_path) {
  const char* op_prefix = std::getenv("OPENPILOT_PREFIX");

  std::string full_path = "/";
  if (op_prefix) {
    full_path += std::string(op_prefix) + "__";
  }
  full_path += CEREAL_EVENTS_PREFIX + "__";
  if (identifier.size() > 0) {
    full_path += identifier + "__";
  }
  full_path += endpoint;

  int shm_fd = shm_open(full_path.c_str(), O_RDWR | O_CREAT, 0664);
  if (shm_fd < 0) {
    throw std::runtime_error("Could not open shared memory file: " + std::string(strerror(errno)));
  }

  // subsequent calls to ftruncate on non-empty shm fails on macos
  struct stat st;
  if (fstat(shm_fd, &st) != -1 && st.st_size == 0) {
    int rc = ftruncate(shm_fd, sizeof(EventState));
    if (rc < 0) {
      close(shm_fd);
      throw std::runtime_error("Could not truncate shared memory file: " + std::string(strerror(errno)));
    }
  }

  char * mem = (char*)mmap(NULL, sizeof(EventState), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  close(shm_fd);
  if (mem == nullptr) {
    throw std::runtime_error("Could not map shared memory file: " + std::string(strerror(errno)));
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
    char called_path[0x80];
    char ready_path[0x80];
    snprintf(called_path, sizeof(called_path)-1, "/tmp/.recv_called_%s_%s", endpoint.c_str(), identifier.c_str());
    snprintf(ready_path, sizeof(ready_path)-1, "/tmp/.recv_ready_%s_%s", endpoint.c_str(), identifier.c_str());

    unlink(called_path);
    unlink(ready_path);
    if (mkfifo(called_path, 0666) < 0 || mkfifo(ready_path, 0666) < 0)
      throw std::runtime_error("Could not create named pipes: " + std::string(strerror(errno)));

    this->fifo_called_path = called_path;
    this->fifo_ready_path = ready_path;
    this->state->fds[0] = open(called_path, O_RDWR | O_NONBLOCK);
    this->state->fds[1] = open(ready_path, O_RDWR | O_NONBLOCK);
#else
    this->state->fds[0] = eventfd(0, EFD_NONBLOCK);
    this->state->fds[1] = eventfd(0, EFD_NONBLOCK);
#endif
  }
}

SocketEventHandle::~SocketEventHandle() {
  close(this->state->fds[0]);
  close(this->state->fds[1]);
  munmap(this->state, sizeof(EventState));
  shm_unlink(this->shm_path.c_str());
#if __APPLE__
  unlink(fifo_called_path.c_str());
  unlink(fifo_ready_path.c_str());
#endif
}

bool SocketEventHandle::is_enabled() {
  return this->state->enabled;
}

void SocketEventHandle::set_enabled(bool enabled) {
  this->state->enabled = enabled;
}

Event SocketEventHandle::recv_called() {
  return Event(this->state->fds[0]);
}

Event SocketEventHandle::recv_ready() {
  return Event(this->state->fds[1]);
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

Event::Event(int fd): event_fd(fd) {}

void Event::set() const {
  throw_if_invalid();

  uint64_t val = 1;
  size_t count = write(this->event_fd, &val, sizeof(uint64_t));
  assert(count == sizeof(uint64_t));
}

int Event::clear() const {
  throw_if_invalid();

  uint64_t val = 0;
  // read the eventfd to clear it
#ifdef __APPLE__
  while (read(this->event_fd, &val, sizeof(uint64_t)) > 0) {}
#else
  read(this->event_fd, &val, sizeof(uint64_t));
#endif

  return val;
}

void Event::wait(int timeout_sec) const {
  throw_if_invalid();

  int event_count;
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(this->event_fd, &fds);

  struct timespec timeout = { timeout_sec, 0 };

  sigset_t signals;
  sigfillset(&signals);
  sigdelset(&signals, SIGALRM);
  sigdelset(&signals, SIGINT);
  sigdelset(&signals, SIGTERM);
  sigdelset(&signals, SIGQUIT);

  event_count = pselect(this->event_fd + 1, &fds, nullptr, nullptr, timeout_sec < 0 ? nullptr : &timeout, &signals);

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
  fd_set fds;
  int max_fd = -1;
  FD_ZERO(&fds);
  for (size_t i = 0; i < events.size(); i++) {
    FD_SET(events[i].fd(), &fds);
    max_fd = std::max(max_fd, events[i].fd());
  }

  struct timespec timeout = { timeout_sec, 0 };

  sigset_t signals;
  sigfillset(&signals);
  sigdelset(&signals, SIGALRM);
  sigdelset(&signals, SIGINT);
  sigdelset(&signals, SIGTERM);
  sigdelset(&signals, SIGQUIT);

  int event_count = pselect(max_fd + 1, &fds, nullptr, nullptr, timeout_sec < 0 ? nullptr : &timeout, &signals);

  if (event_count == 0) {
    throw std::runtime_error("Event timed out pid: " + std::to_string(getpid()));
  } else if (event_count < 0) {
    throw std::runtime_error("Event poll failed, errno: " + std::to_string(errno) + " pid: " + std::to_string(getpid()));
  }

  for (size_t i = 0; i < events.size(); i++) {
    if (FD_ISSET(events[i].fd(), &fds)) {
      return i;
    }
  }

  throw std::runtime_error("Event poll failed, no events ready");
}
