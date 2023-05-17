
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include <sys/eventfd.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>

#include "cereal/messaging/event.h"

bool event_fd_from_environ(std::string& endpoint, EventPurpose purpose, int* fd) {
  std::string env_var_name = env_var_name_from_purpose(purpose, endpoint);
  char* env_var = std::getenv(env_var_name.c_str());
  if (env_var == nullptr) {
    return false;
  }

  *fd = std::atoi(env_var);
  if (*fd == 0) {
    return false;
  }

  return true;
}

std::string env_var_name_from_purpose(EventPurpose purpose, std::string endpoint) {
  switch (purpose) {
    case EventPurpose::RECV_CALLED:
      return "CEREAL_FAKE_RECV_CALLED_FD_" + endpoint;
    case EventPurpose::RECV_READY:
      return "CEREAL_FAKE_RECV_READY_FD_" +  endpoint;
    default:
      throw std::runtime_error("Invalid EventPurpose");
  }
}

Event::Event(int fd): event_fd(fd) {}

Event::Event() {
  this->event_fd = eventfd(0, EFD_NONBLOCK);
}

void Event::throw_if_invalid() {
  if (!this->is_valid()) {
    throw std::runtime_error("Event does not have valid file descriptor.");
  }
}

void Event::set() {
  throw_if_invalid();

  uint64_t val = 1;
  size_t count = write(this->event_fd, &val, sizeof(uint64_t));
  assert(count == sizeof(uint64_t));
}

int Event::clear() {
  throw_if_invalid();

  uint64_t val = 0;
  // read the eventfd to clear it
  read(this->event_fd, &val, sizeof(uint64_t));

  return val;
}

void Event::wait(int timeout_sec) {
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

bool Event::peek() {
  throw_if_invalid();

  int event_count;
  struct pollfd fds = { this->event_fd, POLLIN, 0 };

  // poll with timeout zero to return status immediately
  event_count = poll(&fds, 1, 0);

  return event_count != 0;
}

bool Event::is_valid() {
  return event_fd != -1;
}

int Event::fd() {
  return event_fd;
}

Event * Event::create(std::string endpoint, EventPurpose purpose) {
  int fd;

  if (!event_fd_from_environ(endpoint, purpose, &fd)) {
    return nullptr;
  }

  return new Event(fd);
}

Event * Event::create_and_register(std::string endpoint, EventPurpose purpose) {
  Event * event = new Event();
  std::string env_var_name = env_var_name_from_purpose(purpose, endpoint);
  setenv(env_var_name.c_str(), std::to_string(event->fd()).c_str(), true);

  return event;
}

void Event::invalidate_and_deregister(std::string endpoint, EventPurpose purpose) {
  int fd;
  if (!event_fd_from_environ(endpoint, purpose, &fd)) {
    return;
  }

  close(fd);
  std::string env_var_name = env_var_name_from_purpose(purpose, endpoint);
  unsetenv(env_var_name.c_str());
}

void Event::toggle_fake_events(bool enabled) {
  if (enabled)
    setenv("CEREAL_FAKE", "1", true);
  else
    unsetenv("CEREAL_FAKE");
}

int Event::wait_for_one(const std::vector<Event*>& events, int timeout_sec) {
  struct pollfd fds[events.size()];
  for (size_t i = 0; i < events.size(); i++) {
    fds[i] = { events[i]->fd(), POLLIN, 0 };
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