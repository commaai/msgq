#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "cereal/messaging/messaging.h"

enum FakeEventPurpose {
  RECV_CALLED,
  RECV_READY
};

bool event_fd_from_environ(std::string& endpoint, FakeEventPurpose purpose, int* fd);
std::string env_var_name_from_purpose(FakeEventPurpose purpose, std::string endpoint);

class FakeEvent {
private:
  int event_fd = -1;

  void throw_if_invalid();
public:
  FakeEvent();
  FakeEvent(int fd);

  // sets the counter to 1
  void set();
  // sets the counter to 0, and returns the previous value
  int clear();
  // waits for event having nonzero counter
  void wait();
  // checks if event has nonzero counter, without blocking
  bool peek();
  bool is_valid();
  int fd();
  static FakeEvent * create(std::string endpoint, FakeEventPurpose purpose);
  static FakeEvent * create_and_register(std::string endpoint, FakeEventPurpose purpose);
  static void invalidate_and_deregister(std::string endpoint, FakeEventPurpose purpose);
  static void toggle_fake_events(bool enabled);
  static int wait_for_one(const std::vector<FakeEvent*>& events);
};

template<typename TSubSocket>
class FakeSubSocket: public TSubSocket {
private:
  FakeEvent * recv_called = nullptr;
  FakeEvent * recv_ready = nullptr;
  bool events_enabled = false;

public:
  FakeSubSocket(): TSubSocket() {}
  ~FakeSubSocket() {
    delete recv_called;
    delete recv_ready;
  }

  int connect(Context *context, std::string endpoint, std::string address, bool conflate=false, bool check_endpoint=true) override {
    this->recv_called = FakeEvent::create(endpoint, FakeEventPurpose::RECV_CALLED);
    this->recv_ready = FakeEvent::create(endpoint, FakeEventPurpose::RECV_READY);
    if (this->recv_called == nullptr || this->recv_ready == nullptr) {
      std::cout << "File descriptor env vars not set for endpoint " << endpoint << ". cross-process locks disabled" << std::endl;
      this->events_enabled = false;
    } else {
      this->events_enabled = true;
    }

    return TSubSocket::connect(context, endpoint, address, conflate, check_endpoint);
  }

  Message *receive(bool non_blocking=false) override {
    if (this->events_enabled) {
      this->recv_called->set();
      this->recv_ready->wait();
      this->recv_ready->clear();
    }

    return TSubSocket::receive(non_blocking);
  }
};

class FakePoller: public Poller {
private:
  std::vector<SubSocket*> sockets;

public:
  void registerSocket(SubSocket *socket) override;
  std::vector<SubSocket*> poll(int timeout) override;
  ~FakePoller(){};
};
