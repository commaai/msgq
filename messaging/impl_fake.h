#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "cereal/messaging/messaging.h"
#include "cereal/messaging/event.h"

template<typename TSubSocket>
class FakeSubSocket: public TSubSocket {
private:
  Event * recv_called = nullptr;
  Event * recv_ready = nullptr;
  bool events_enabled = false;

public:
  FakeSubSocket(): TSubSocket() {}
  ~FakeSubSocket() {
    delete recv_called;
    delete recv_ready;
  }

  int connect(Context *context, std::string endpoint, std::string address, bool conflate=false, bool check_endpoint=true) override {
    this->recv_called = Event::create(endpoint, EventPurpose::RECV_CALLED);
    this->recv_ready = Event::create(endpoint, EventPurpose::RECV_READY);
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
