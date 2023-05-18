#pragma once

#include <string>
#include <vector>

#define MAX_ENDPOINT_LEN 64

enum EventPurpose {
  RECV_CALLED,
  RECV_READY
};

struct EventState {
  char endpoint[MAX_ENDPOINT_LEN];
  int fds[2];
  bool enabled;
};

class Event {
private:
  int event_fd = -1;

  inline void throw_if_invalid() const {
    if (!this->is_valid()) {
      throw std::runtime_error("Event does not have valid file descriptor.");
    }
  }
public:
  Event(int fd = -1);

  void set() const;
  int clear() const;
  void wait(int timeout_sec = -1) const;
  bool peek() const;
  bool is_valid() const;
  int fd() const;

  static int wait_for_one(const std::vector<Event>& events, int timeout_sec = -1);
};

class EventManager {
private:
  std::string shm_path;
  EventState* state;
public:
  EventManager(std::string endpoint, std::string identifier);
  ~EventManager();

  bool is_enabled();
  void set_enabled(bool enabled);
  Event recv_called();
  Event recv_ready();

  static void toggle_fake_events(bool enabled);
};
