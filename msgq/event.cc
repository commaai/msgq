#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "msgq/event.h"

namespace {

size_t event_socket_count = 0;

bool is_would_block() {
  return errno == EAGAIN || errno == EWOULDBLOCK;
}

void throw_if_errno(std::string prefix) {
  throw std::runtime_error(prefix + ", errno: " + std::to_string(errno) + " pid: " + std::to_string(getpid()));
}

std::string event_state_path(std::string endpoint, std::string identifier) {
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
  return full_path + endpoint;
}

void set_event_path(char *destination, const std::string& path) {
  if (path.size() >= EVENT_PATH_MAX) {
    throw std::runtime_error("Event path is too long: " + path);
  }

  std::memset(destination, 0, EVENT_PATH_MAX);
  std::memcpy(destination, path.c_str(), path.size() + 1);
}

std::string event_path(const EventState *state, EventPurpose purpose) {
  return std::string(state->paths[purpose]);
}

sockaddr_un event_sockaddr(const std::string& path) {
  sockaddr_un addr = {};
  addr.sun_family = AF_UNIX;
  if (path.size() >= sizeof(addr.sun_path)) {
    throw std::runtime_error("Event socket path is too long: " + path);
  }
  std::memcpy(addr.sun_path, path.c_str(), path.size() + 1);
  return addr;
}

int create_event_socket() {
  int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (fd < 0) {
    throw_if_errno("Could not create event socket");
  }

  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    close(fd);
    throw_if_errno("Could not make event socket non-blocking");
  }
  return fd;
}

bool connect_event_socket(int fd, const std::string& path) {
  sockaddr_un addr = event_sockaddr(path);
  while (true) {
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
      return true;
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno == ENOENT || errno == ECONNREFUSED) {
      return false;
    }
    throw_if_errno("Could not connect event socket");
  }
}

int poll_events(pollfd *fds, nfds_t nfds, int timeout_sec) {
  if (nfds == 0) {
    throw std::runtime_error("Event poll failed, no events provided");
  }

  int timeout_ms = timeout_sec < 0 ? -1 : timeout_sec * 1000;
  while (true) {
    int ret = poll(fds, nfds, timeout_ms);
    if (ret >= 0) {
      return ret;
    }
    if (errno != EINTR) {
      throw_if_errno("Event poll failed");
    }
  }
}

}  // namespace

struct EventFileDescriptor {
  int fd = -1;
  bool receives = false;
  bool close_on_destroy = false;
  bool connected = false;
  std::string path;

  EventFileDescriptor(int fd, bool receives, bool close_on_destroy) :
      fd(fd), receives(receives), close_on_destroy(close_on_destroy) {}

  EventFileDescriptor(std::string path, bool receives) :
      fd(create_event_socket()), receives(receives), close_on_destroy(true), path(std::move(path)) {
    if (receives) {
      sockaddr_un addr = event_sockaddr(this->path);
      unlink(this->path.c_str());
      if (bind(this->fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(this->fd);
        this->fd = -1;
        throw_if_errno("Could not bind event socket");
      }
    }
    this->connected = connect_event_socket(this->fd, this->path);
  }

  ~EventFileDescriptor() {
    if (close_on_destroy && fd >= 0) {
      close(fd);
    }
  }
};

void event_state_shm_mmap(std::string endpoint, std::string identifier, char **shm_mem, std::string *shm_path) {
  std::string full_path = event_state_path(endpoint, identifier);

  int shm_fd = open(full_path.c_str(), O_RDWR | O_CREAT, 0664);
  if (shm_fd < 0) {
    throw_if_errno("Could not open shared memory file");
  }

  int rc = ftruncate(shm_fd, sizeof(EventState));
  if (rc < 0) {
    close(shm_fd);
    throw_if_errno("Could not truncate shared memory file");
  }

  char *mem = reinterpret_cast<char*>(mmap(NULL, sizeof(EventState), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
  close(shm_fd);
  if (mem == MAP_FAILED) {
    throw_if_errno("Could not map shared memory file");
  }

  if (shm_mem != nullptr)
    *shm_mem = mem;
  if (shm_path != nullptr)
    *shm_path = full_path;
}

SocketEventHandle::SocketEventHandle(std::string endpoint, std::string identifier, bool override) {
  char *mem;
  event_state_shm_mmap(endpoint, identifier, &mem, &this->shm_path);

  this->state = reinterpret_cast<EventState*>(mem);
  this->owns_state = override;

  if (override) {
    std::string socket_path = "/tmp/msgq_event_" + std::to_string(getpid()) + "_" + std::to_string(event_socket_count++);
    for (size_t i = 0; i < 2; i++) {
      set_event_path(this->state->paths[i], socket_path + "." + std::to_string(i));
    }
    this->state->enabled = false;
  }

  this->recv_called_event = Event(event_path(this->state, EventPurpose::RECV_CALLED), override);
  this->recv_ready_event = Event(event_path(this->state, EventPurpose::RECV_READY), false);
}

SocketEventHandle::~SocketEventHandle() {
  if (this->state != nullptr) {
    if (this->owns_state) {
      unlink(event_path(this->state, EventPurpose::RECV_CALLED).c_str());
      unlink(event_path(this->state, EventPurpose::RECV_READY).c_str());
      unlink(this->shm_path.c_str());
    }
    munmap(this->state, sizeof(EventState));
  }
}

bool SocketEventHandle::is_enabled() {
  return this->state->enabled;
}

void SocketEventHandle::set_enabled(bool enabled) {
  this->state->enabled = enabled;
}

Event SocketEventHandle::recv_called() {
  return this->recv_called_event;
}

Event SocketEventHandle::recv_ready() {
  return this->recv_ready_event;
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

Event::Event(int fd) {
  if (fd >= 0) {
    this->fd_ = std::make_shared<EventFileDescriptor>(fd, true, false);
  }
}

Event::Event(std::string path, bool receives) {
  if (path.empty()) {
    return;
  }
  this->fd_ = std::make_shared<EventFileDescriptor>(std::move(path), receives);
}

void Event::set() const {
  throw_if_invalid();

  char val = 1;
  while (true) {
    if (!this->fd_->connected) {
      this->fd_->connected = connect_event_socket(this->fd_->fd, this->fd_->path);
      if (!this->fd_->connected) {
        return;
      }
    }

    ssize_t count = send(this->fd_->fd, &val, sizeof(val), 0);
    if (count == sizeof(val)) {
      return;
    }
    if (count < 0 && errno == EINTR) {
      continue;
    }
    if (count < 0 && this->fd_->connected && (errno == ENOENT || errno == ECONNREFUSED)) {
      this->fd_->connected = false;
      continue;
    }
    if (count < 0 && is_would_block()) {
      return;
    }
    throw_if_errno("Event write failed");
  }
}

int Event::clear() const {
  throw_if_invalid();
  if (!this->fd_->receives) {
    throw std::runtime_error("Event does not receive.");
  }

  int total = 0;
  char buf[128];
  while (true) {
    ssize_t count = recv(this->fd_->fd, buf, sizeof(buf), 0);
    if (count > 0) {
      total += static_cast<int>(count);
      continue;
    }
    if (count == 0) {
      return total;
    }
    if (errno == EINTR) {
      continue;
    }
    if (is_would_block()) {
      return total;
    }
    throw_if_errno("Event read failed");
  }
}

void Event::wait(int timeout_sec) const {
  throw_if_invalid();
  if (!this->fd_->receives) {
    throw std::runtime_error("Event does not receive.");
  }

  pollfd fds = {this->fd_->fd, POLLIN, 0};
  int event_count = poll_events(&fds, 1, timeout_sec);

  if (event_count == 0) {
    throw std::runtime_error("Event timed out pid: " + std::to_string(getpid()));
  }
  if ((fds.revents & POLLIN) == 0) {
    throw std::runtime_error("Event poll failed, event not readable pid: " + std::to_string(getpid()));
  }
}

bool Event::peek() const {
  throw_if_invalid();
  if (!this->fd_->receives) {
    return false;
  }

  pollfd poll_fd = {this->fd_->fd, POLLIN, 0};
  int event_count = poll(&poll_fd, 1, 0);
  if (event_count < 0) {
    throw_if_errno("Event poll failed");
  }

  return event_count > 0 && (poll_fd.revents & POLLIN) != 0;
}

bool Event::is_valid() const {
  return this->fd_ && this->fd_->fd >= 0;
}

int Event::fd() const {
  return this->fd_ ? this->fd_->fd : -1;
}

int Event::wait_for_one(const std::vector<Event>& events, int timeout_sec) {
  pollfd fds[events.size()];
  for (size_t i = 0; i < events.size(); i++) {
    events[i].throw_if_invalid();
    if (!events[i].fd_->receives) {
      throw std::runtime_error("Event does not receive.");
    }
    fds[i] = {events[i].fd(), POLLIN, 0};
  }

  int event_count = poll_events(fds, events.size(), timeout_sec);
  if (event_count == 0) {
    throw std::runtime_error("Event timed out pid: " + std::to_string(getpid()));
  }

  for (size_t i = 0; i < events.size(); i++) {
    if ((fds[i].revents & POLLIN) != 0) {
      return static_cast<int>(i);
    }
  }

  throw std::runtime_error("Event poll failed, no events ready");
}
