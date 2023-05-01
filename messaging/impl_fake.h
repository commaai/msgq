#include "cereal/messaging/messaging.h"
#include "cereal/logger/logger.h"

#define FAKE_EVENT_TIMEOUT 30

bool parse_event_fds_using_env(std::string& endpoint, int* recv_called_fd, int* recv_ready_fd);

class FakeEvent {
private:
  int event_fd = -1;
  int timeout_in_sec = 0;
public:
  FakeEvent();
  FakeEvent(int fd, int timeout_in_sec);
  FakeEvent(int timeout_in_sec);

  void set();
  void clear();
  void wait();
  bool peek();
  bool is_valid();
  int fd();
};

template<typename TSubSocket>
class FakeSubSocket: public TSubSocket {
private:
  FakeEvent recv_called;
  FakeEvent recv_ready;
  bool events_enabled = false;

public:
  FakeSubSocket(): TSubSocket() {}

  int connect(Context *context, std::string endpoint, std::string address, bool conflate=false, bool check_endpoint=true) override {
    int recv_called_fd, recv_ready_fd;
    if (parse_event_fds_using_env(endpoint, &recv_called_fd, &recv_ready_fd)) {
      this->recv_called = FakeEvent(recv_called_fd, FAKE_EVENT_TIMEOUT);
      this->recv_ready = FakeEvent(recv_ready_fd, FAKE_EVENT_TIMEOUT);
      this->events_enabled = true;
    } else {
      LOGW("File descriptor env vars not set. cross-process locks disabled");
      this->events_enabled = false;
    }

    return TSubSocket::connect(context, endpoint, address, conflate, check_endpoint);
  }

  Message *receive(bool non_blocking=false) override {
    if (this->events_enabled) {
      this->recv_called.set();
      this->recv_ready.wait();
      this->recv_ready.clear();
    }

    return TSubSocket::receive(non_blocking);
  }
};

template<typename TPubSocket>
class FakePubSocket: public TPubSocket {
private:
  FakeEvent recv_called;
  FakeEvent recv_ready;
  bool events_enabled = false;

public:
  FakePubSocket(): TPubSocket() {}

  int connect(Context *context, std::string endpoint, bool check_endpoint=true) override {
    int recv_called_fd, recv_ready_fd;
    if (parse_event_fds_using_env(endpoint, &recv_called_fd, &recv_ready_fd)) {
      this->recv_called = FakeEvent(recv_called_fd, FAKE_EVENT_TIMEOUT);
      this->recv_ready = FakeEvent(recv_ready_fd, FAKE_EVENT_TIMEOUT);
      this->events_enabled = true;
    } else {
      LOGW("File descriptor env vars not set. cross-process locks disabled");
      this->events_enabled = false;
    }

    return TPubSocket::connect(context, endpoint, check_endpoint);
  }

  int sendMessage(Message* message) override {
    return this->send(message->getData(), message->getSize());
  }

  int send(char *data, size_t size) override {
    if (this->events_enabled) {
      this->recv_called.wait();
      this->recv_called.clear();
    }

    int result = TPubSocket::send(data, size);

    if (this->events_enabled) {
      this->recv_ready.set();
    }

    return result;
  }
};