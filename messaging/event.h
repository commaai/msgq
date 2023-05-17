#pragma once

#include <string>
#include <vector>

enum EventPurpose {
  RECV_CALLED,
  RECV_READY
};

bool event_fd_from_environ(std::string& endpoint, EventPurpose purpose, int* fd);
std::string env_var_name_from_purpose(EventPurpose purpose, std::string endpoint);

class Event {
private:
  int event_fd = -1;

  void throw_if_invalid();
public:
  Event();
  Event(int fd);

  // sets the counter to 1
  void set();
  // sets the counter to 0, and returns the previous value
  int clear();
  // waits for event having nonzero counter
  void wait(int timeout_sec = -1);
  // checks if event has nonzero counter, without blocking
  bool peek();
  bool is_valid();
  int fd();
  static Event * create(std::string endpoint, EventPurpose purpose);
  static Event * create_and_register(std::string endpoint, EventPurpose purpose);
  static void invalidate_and_deregister(std::string endpoint, EventPurpose purpose);
  static void toggle_fake_events(bool enabled);
  static int wait_for_one(const std::vector<Event*>& events, int timeout_sec = -1);
};
