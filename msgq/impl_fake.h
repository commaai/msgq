#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "msgq/ipc.h"
#include "msgq/event.h"

template<typename TSubSocket>
class FakeSubSocket: public TSubSocket {
private:
  Event *recv_called = nullptr;
  Event *recv_ready = nullptr;
  EventState *state = nullptr;

  void ensure_events_open() {
    if (recv_called == nullptr || !recv_called->is_valid()) {
      delete recv_called;
      recv_called = new Event(std::string(state->paths[EventPurpose::RECV_CALLED]), false);
    }
    if (recv_ready == nullptr || !recv_ready->is_valid()) {
      delete recv_ready;
      recv_ready = new Event(std::string(state->paths[EventPurpose::RECV_READY]), true);
    }
  }

public:
  FakeSubSocket(): TSubSocket() {}
  ~FakeSubSocket() {
    delete recv_called;
    delete recv_ready;
    if (state != nullptr) {
      munmap(state, sizeof(EventState));
    }
  }

  int connect(Context *context, std::string endpoint, std::string address, bool conflate=false, bool check_endpoint=true, size_t segment_size=0) override {
    const char* cereal_prefix = std::getenv("CEREAL_FAKE_PREFIX");

    char* mem;
    std::string identifier = cereal_prefix != nullptr ?  std::string(cereal_prefix) : "";
    event_state_shm_mmap(endpoint, identifier, &mem, nullptr);

    this->state = (EventState*)mem;
    this->recv_called = new Event(std::string(state->paths[EventPurpose::RECV_CALLED]), false);
    this->recv_ready = new Event(std::string(state->paths[EventPurpose::RECV_READY]), true);

    return TSubSocket::connect(context, endpoint, address, conflate, check_endpoint, segment_size);
  }

  Message *receive(bool non_blocking=false) override {
    if (this->state->enabled) {
      ensure_events_open();
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
  ~FakePoller() {}
};
