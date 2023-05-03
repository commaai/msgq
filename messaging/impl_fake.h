#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "cereal/messaging/messaging.h"

#define FAKE_EVENT_TIMEOUT_SEC 30

enum FakeEventPurpose {
  RECV_CALLED,
  RECV_READY,
  POLL_CALLED,
  POLL_READY
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
  // checks whether file descriptor is valid
  bool is_valid();
  // underlying file descriptor
  int fd();
  static FakeEvent * create(std::string endpoint, FakeEventPurpose purpose);
  static FakeEvent * create_and_register(std::string endpoint, FakeEventPurpose purpose);
  static void toggle_fake_events(bool enabled);
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

template<typename TPubSocket>
class FakePubSocket: public TPubSocket {
private:
  FakeEvent * recv_called = nullptr;
  FakeEvent * recv_ready = nullptr;
  bool events_enabled = false;

public:
  FakePubSocket(): TPubSocket() {}
  ~FakePubSocket() {
    delete recv_called;
    delete recv_ready;
  }

  int connect(Context *context, std::string endpoint, bool check_endpoint=true) override {
    this->recv_called = FakeEvent::create(endpoint, FakeEventPurpose::RECV_CALLED);
    this->recv_ready = FakeEvent::create(endpoint, FakeEventPurpose::RECV_READY);
    if (this->recv_called == nullptr || this->recv_ready == nullptr) {
      std::cout << "File descriptor env vars not set for endpoint " << endpoint << ". cross-process locks disabled" << std::endl;
      this->events_enabled = false;
    } else {
      this->events_enabled = true;
    }

    return TPubSocket::connect(context, endpoint, check_endpoint);
  }

  int sendMessage(Message* message) override {
    return this->send(message->getData(), message->getSize());
  }

  int send(char *data, size_t size) override {
    if (this->events_enabled) {
      this->recv_called->wait();
      this->recv_called->clear();
    }

    int result = TPubSocket::send(data, size);

    if (this->events_enabled) {
      this->recv_ready->set();
    }

    return result;
  }
};

template<typename TPoller>
class FakePoller: public TPoller {
private:
  FakeEvent * poll_called = nullptr;
  FakeEvent * poll_ready = nullptr;
  bool events_enabled = false;
  bool events_setup = false;

public:
  FakePoller(): TPoller() {}
  ~FakePoller() {
    delete poll_called;
  }

  void registerSocket(SubSocket *socket) override {
    if (!this->events_setup) {
      this->poll_called = FakeEvent::create("", FakeEventPurpose::POLL_CALLED);
      this->poll_ready = FakeEvent::create("", FakeEventPurpose::POLL_READY);
      if (this->poll_called == nullptr || this->poll_ready == nullptr) {
        std::cout << "File descriptor env vars not set. poller cross-process locks disabled" << std::endl;
        this->events_enabled = false;
      } else {
        this->events_enabled = true;
      }
      this->events_setup = true;
    }

    return TPoller::registerSocket(socket);
  }

  std::vector<SubSocket*> poll(int timeout) override {
    if (this->events_enabled) {
      this->poll_called->set();
      this->poll_ready->wait();
      this->poll_ready->clear();
    }

    return TPoller::poll(timeout);
  }
};
