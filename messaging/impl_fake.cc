#include <cassert>
#include <cstdlib>
#include <string>
#include <exception>

#include <sys/eventfd.h>
#include <unistd.h>
#include <poll.h>

#include "cereal/messaging/impl_fake.h"

bool parse_event_fds_using_env(std::string& endpoint, int* recv_called_fd, int* recv_ready_fd) {
  std::string recv_called_name = "CEREAL_FAKE_RECV_CALLED_FD_" + endpoint;
  std::string recv_ready_name = "CEREAL_FAKE_RECV_READY_FD_" + endpoint;

  char* recv_called_env = std::getenv(recv_called_name.c_str());
  char* recv_ready_env = std::getenv(recv_ready_name.c_str());
  if (recv_called_env == nullptr || recv_ready_env == nullptr) {
    return false;
  }

  *recv_called_fd = std::atoi(recv_called_env);
  *recv_ready_fd = std::atoi(recv_ready_env);
  if (*recv_called_fd == 0 || *recv_ready_fd == 0) {
    return false;
  }

  return true;
}

FakeEvent::FakeEvent(): timeout_in_sec(FAKE_EVENT_TIMEOUT) {}

FakeEvent::FakeEvent(int fd, int timeout_in_sec): 
  event_fd(fd), timeout_in_sec(timeout_in_sec) {}

FakeEvent::FakeEvent(int timeout_in_sec): timeout_in_sec(timeout_in_sec) {
  this->event_fd = eventfd(0, EFD_NONBLOCK);
}

void FakeEvent::set() {
  // LOGE("write to event_fd: %d, pid: %d", this->event_fd, getpid());
  uint64_t val = 1;
  size_t count = write(this->event_fd, &val, sizeof(uint64_t));
  assert(count == sizeof(uint64_t));
}

void FakeEvent::clear() {
  // LOGE("read to event_fd: %d, pid: %d", this->event_fd, getpid());
  uint64_t val = 0;
  // read the eventfd to clear it
  uint64_t count = read(this->event_fd, &val, sizeof(uint64_t));
  assert(count == sizeof(uint64_t));
}

void FakeEvent::wait() {
  // LOGE("waiting for event_fd: %d, pid: %d", this->event_fd, getpid());
  int event_count;
  struct pollfd fds = { this->event_fd, POLLIN, 0 };

  do {
    event_count = poll(&fds, 1, this->timeout_in_sec * 1000);
  } while (event_count < 0 && errno == EINTR);

  // LOGE("event_count: %d pid: %d", event_count, getpid());

  if (event_count == 0) {
    throw std::runtime_error("FakeEvent timed out pid: " + std::to_string(getpid()));
  } else if (event_count < 0) {
    throw std::runtime_error("FakeEvent poll failed, errno: " + std::to_string(errno) + " pid: " + std::to_string(getpid()));
  }
}

bool FakeEvent::is_valid() {
  return event_fd != -1;
}

bool FakeEvent::peek() {
  int event_count;
  struct pollfd fds = { this->event_fd, POLLIN, 0 };

  // poll with timeout zero to return status immediatelly
  event_count = poll(&fds, 1, 0);

  return event_count != 0;
}

int FakeEvent::fd() {
  return event_fd;
}
