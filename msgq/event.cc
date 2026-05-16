#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "msgq/event.h"

namespace {

size_t event_fifo_counter = 0;

[[noreturn]] void throw_errno(const std::string& msg) {
  throw std::runtime_error(msg + ", errno: " + std::to_string(errno) + " pid: " + std::to_string(getpid()));
}

int open_event_fifo(const char* path) {
  if (path[0] == '\0') return -1;
  int fd = open(path, O_RDWR | O_NONBLOCK);
  if (fd < 0 && errno != ENOENT) throw_errno("Could not open event fifo");
  return fd;
}

}  // namespace

void event_state_shm_mmap(std::string endpoint, std::string identifier, char **shm_mem, std::string *shm_path) {
  const char* op_prefix = std::getenv("OPENPILOT_PREFIX");

  std::string full_path = "/tmp/msgq_";
  if (op_prefix) {
    full_path += std::string(op_prefix) + "/";
  }
  full_path += CEREAL_EVENTS_PREFIX + "/";
  if (!identifier.empty()) {
    full_path += identifier + "/";
  }
  std::filesystem::create_directories(full_path);
  full_path += endpoint;

  int shm_fd = open(full_path.c_str(), O_RDWR | O_CREAT, 0664);
  if (shm_fd < 0) throw_errno("Could not open shared memory file");

  if (ftruncate(shm_fd, sizeof(EventState)) < 0) {
    close(shm_fd);
    throw_errno("Could not truncate shared memory file");
  }

  char *mem = reinterpret_cast<char*>(mmap(NULL, sizeof(EventState), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
  close(shm_fd);
  if (mem == MAP_FAILED) throw_errno("Could not map shared memory file");

  if (shm_mem != nullptr) *shm_mem = mem;
  if (shm_path != nullptr) *shm_path = full_path;
}

SocketEventHandle::SocketEventHandle(std::string endpoint, std::string identifier, bool override) {
  char *mem;
  event_state_shm_mmap(endpoint, identifier, &mem, &this->shm_path);

  this->state = (EventState*)mem;
  this->owns_fifos = override;

  if (override) {
    std::string base = "/tmp/msgq_event_" + std::to_string(getpid()) + "_" + std::to_string(event_fifo_counter++);
    for (size_t i = 0; i < 2; i++) {
      std::string p = base + "." + std::to_string(i);
      if (p.size() >= EVENT_PATH_MAX) {
        throw std::runtime_error("Event path too long: " + p);
      }
      unlink(p.c_str());
      if (mkfifo(p.c_str(), 0664) < 0) throw_errno("Could not create event fifo");
      std::memset(this->state->paths[i], 0, EVENT_PATH_MAX);
      std::memcpy(this->state->paths[i], p.c_str(), p.size() + 1);
    }
    this->state->enabled = false;
  }

  for (size_t i = 0; i < 2; i++) {
    this->fds[i] = open_event_fifo(this->state->paths[i]);
  }
}

SocketEventHandle::~SocketEventHandle() {
  if (this->state == nullptr) return;
  for (int fd : this->fds) {
    if (fd >= 0) close(fd);
  }
  if (this->owns_fifos) {
    unlink(this->state->paths[RECV_CALLED]);
    unlink(this->state->paths[RECV_READY]);
    unlink(this->shm_path.c_str());
  }
  munmap(this->state, sizeof(EventState));
}

bool SocketEventHandle::is_enabled() {
  return this->state->enabled;
}

void SocketEventHandle::set_enabled(bool enabled) {
  this->state->enabled = enabled;
}

Event SocketEventHandle::recv_called() {
  return Event(this->fds[RECV_CALLED]);
}

Event SocketEventHandle::recv_ready() {
  return Event(this->fds[RECV_READY]);
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
  return prefix == nullptr ? "" : std::string(prefix);
}

Event::Event(int fd): event_fd(fd) {}

void Event::set() const {
  throw_if_invalid();

  char val = 1;
  ssize_t count = write(this->event_fd, &val, sizeof(val));
  if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
    throw_errno("Event write failed");
  }
}

int Event::clear() const {
  throw_if_invalid();

  int total = 0;
  char buf[64];
  while (true) {
    ssize_t count = read(this->event_fd, buf, sizeof(buf));
    if (count > 0) {
      total += static_cast<int>(count);
      continue;
    }
    if (count == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
      return total;
    }
    throw_errno("Event read failed");
  }
}

void Event::wait(int timeout_sec) const {
  throw_if_invalid();

  pollfd fds = {this->event_fd, POLLIN, 0};
  int event_count = poll(&fds, 1, timeout_sec < 0 ? -1 : timeout_sec * 1000);

  if (event_count == 0) {
    throw std::runtime_error("Event timed out pid: " + std::to_string(getpid()));
  } else if (event_count < 0) {
    throw_errno("Event poll failed");
  }
}

bool Event::peek() const {
  throw_if_invalid();

  pollfd fds = {this->event_fd, POLLIN, 0};
  return poll(&fds, 1, 0) > 0;
}

bool Event::is_valid() const {
  return event_fd >= 0;
}

int Event::fd() const {
  return event_fd;
}

int Event::wait_for_one(const std::vector<Event>& events, int timeout_sec) {
  pollfd fds[events.size()];
  for (size_t i = 0; i < events.size(); i++) {
    fds[i] = {events[i].fd(), POLLIN, 0};
  }

  int event_count = poll(fds, events.size(), timeout_sec < 0 ? -1 : timeout_sec * 1000);
  if (event_count == 0) {
    throw std::runtime_error("Event timed out pid: " + std::to_string(getpid()));
  } else if (event_count < 0) {
    throw_errno("Event poll failed");
  }

  for (size_t i = 0; i < events.size(); i++) {
    if (fds[i].revents & POLLIN) {
      return static_cast<int>(i);
    }
  }

  throw std::runtime_error("Event poll failed, no events ready");
}
