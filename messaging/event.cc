#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <exception>

#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>

#include "cereal/messaging/event.h"

EventManager::EventManager(std::string endpoint, std::string identifier) {
  const char* op_prefix = std::getenv("OPENPILOT_PREFIX");

  std::string full_path = "/dev/shm/";
  if (op_prefix) {
    full_path += std::string(op_prefix) + "/";
  }
  full_path += identifier + "/" + endpoint;
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

  this->state = (EventState*)mem;
  memcpy(this->state->endpoint, endpoint.c_str(), endpoint.size() > MAX_ENDPOINT_LEN ? MAX_ENDPOINT_LEN : endpoint.size());
  this->state->enabled = true;
  this->state->fds[0] = eventfd(0, EFD_NONBLOCK);
  this->state->fds[1] = eventfd(0, EFD_NONBLOCK);
  this->shm_path = full_path;
}

EventManager::~EventManager() {
  close(this->state->fds[0]);
  close(this->state->fds[1]);
  munmap(this->state, sizeof(EventState));
  unlink(this->shm_path.c_str());
}

bool EventManager::is_enabled() {
  return this->state->enabled;
}

void EventManager::set_enabled(bool enabled) {
  this->state->enabled = enabled;
}

Event EventManager::recv_called() {
  return Event(this->state->fds[0]);
}

Event EventManager::recv_ready() {
  return Event(this->state->fds[1]);
}

void EventManager::toggle_fake_events(bool enabled) {
  if (enabled)
    setenv("CEREAL_FAKE", "1", true);
  else
    unsetenv("CEREAL_FAKE");
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
  read(this->event_fd, &val, sizeof(uint64_t));

  return val;
}

void Event::wait(int timeout_sec) const {
  throw_if_invalid();

  int event_count;
  struct pollfd fds = { this->event_fd, POLLIN, 0 };

  struct timespec timeout = { timeout_sec, 0 };

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
