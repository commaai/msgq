#include <cassert>
#include <cstdlib>
#include <string>
#include <exception>

#include <sys/eventfd.h>
#include <unistd.h>
#include <poll.h>

#include "cereal/messaging/impl_fake.h"

bool event_fd_from_environ(std::string& endpoint, FakeEventPurpose purpose, int* fd) {
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

std::string env_var_name_from_purpose(FakeEventPurpose purpose, std::string endpoint) {
  switch (purpose) {
    case FakeEventPurpose::RECV_CALLED:
      return "CEREAL_FAKE_RECV_CALLED_FD_" + endpoint;
    case FakeEventPurpose::RECV_READY:
      return "CEREAL_FAKE_RECV_READY_FD_" + endpoint;
    default:
      throw std::runtime_error("Invalid FakeEventPurpose");
  }
}

FakeEvent::FakeEvent(int fd): event_fd(fd) {}

FakeEvent::FakeEvent() {
  this->event_fd = eventfd(0, EFD_NONBLOCK);
}

void FakeEvent::throw_if_invalid() {
  if (!this->is_valid()) {
    throw std::runtime_error("FakeEvent does not have valid file descriptor.");
  }
}

void FakeEvent::set() {
  throw_if_invalid();

  uint64_t val = 1;
  size_t count = write(this->event_fd, &val, sizeof(uint64_t));
  assert(count == sizeof(uint64_t));
}

void FakeEvent::clear() {
  throw_if_invalid();

  uint64_t val = 0;
  // read the eventfd to clear it
  uint64_t count = read(this->event_fd, &val, sizeof(uint64_t));
  assert(count == sizeof(uint64_t));
}

void FakeEvent::wait() {
  throw_if_invalid();

  int event_count;
  struct pollfd fds = { this->event_fd, POLLIN, 0 };

  do {
    event_count = poll(&fds, 1, FAKE_EVENT_TIMEOUT_SEC * 1000);
  } while (event_count < 0 && errno == EINTR);

  if (event_count == 0) {
    throw std::runtime_error("FakeEvent timed out pid: " + std::to_string(getpid()));
  } else if (event_count < 0) {
    throw std::runtime_error("FakeEvent poll failed, errno: " + std::to_string(errno) + " pid: " + std::to_string(getpid()));
  }
}

bool FakeEvent::peek() {
  throw_if_invalid();

  int event_count;
  struct pollfd fds = { this->event_fd, POLLIN, 0 };

  // poll with timeout zero to return status immediately
  event_count = poll(&fds, 1, 0);

  return event_count != 0;
}

bool FakeEvent::is_valid() {
  return event_fd != -1;
}

int FakeEvent::fd() {
  return event_fd;
}

FakeEvent * FakeEvent::create(std::string endpoint, FakeEventPurpose purpose) {
  int fd;

  if (!event_fd_from_environ(endpoint, purpose, &fd)) {
    return nullptr;
  }

  return new FakeEvent(fd);
}

FakeEvent * FakeEvent::create_and_register(std::string endpoint, FakeEventPurpose purpose) {
  FakeEvent * event = new FakeEvent();
  std::string env_var_name = env_var_name_from_purpose(purpose, endpoint);
  setenv(env_var_name.c_str(), std::to_string(event->fd()).c_str(), true);

  return event;
}

void FakeEvent::toggle_fake_events(bool enabled) {
  if (enabled)
    setenv("CEREAL_FAKE", "1", true);
  else
    unsetenv("CEREAL_FAKE");
}
