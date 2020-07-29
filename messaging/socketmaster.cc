#include <algorithm>
#include <assert.h>
#include <time.h>

#include "messaging.hpp"
#include "services.h"

#ifdef __APPLE__
#define CLOCK_BOOTTIME CLOCK_MONOTONIC
#endif

static inline uint64_t nanos_since_boot() {
  struct timespec t;
  clock_gettime(CLOCK_BOOTTIME, &t);
  return t.tv_sec * 1000000000ULL + t.tv_nsec;
}

static const service *get_service(std::string name) {
  for (const auto &it : services) {
    if (name.compare(it.name) == 0) return &it;
  }
  return nullptr;
}

static inline bool inList(const std::vector<std::string > &list, std::string value) {
  return std::find(list.begin(), list.end(), value) != list.end();
}

class MessageContext {
public:
  MessageContext() { ctx = Context::create(); }
  ~MessageContext() { delete ctx; }
  Context *ctx;
};
MessageContext message_context;


// SubMaster

SubMaster::SubMaster(const std::vector<std::string > &service_list, std::string address,
                     const std::vector<std::string > &ignore_alive) {
  frame = 0;
  poller_ = Poller::create();

  for (auto service_name : service_list) {
    const service *service = get_service(service_name);
    assert(service != nullptr);

    SubSocket *sock = SubSocket::create(message_context.ctx, service_name, address, true);
    assert(sock != 0);
    poller_->registerSocket(sock);

    SubMessage *m = new SubMessage {
      .name = service_name,
      .socket = sock,
      .freq = service->frequency,
      .updated = false,
      .alive = false,
      .valid = false,
      .ignore_alive = inList(service_list, service_name),
      .rcv_time = 0,
      .rcv_frame = 0,
      .logMonoTime = 0,
      .allocated_msg_reader = malloc(sizeof(capnp::FlatArrayMessageReader)),
      .buf = kj::heapArray<capnp::word>(1024)
    };
    messages_[sock] = m;
    services[service_name] = m;
  }
}

int SubMaster::update(int timeout) {
  uint64_t current_time = nanos_since_boot();
  for (auto &kv : messages_) {
    kv.second->updated = false;
  }
  frame++;

  // poll for updates
  auto recvd_socks = poller_->poll(timeout);
  for (auto s : recvd_socks) {
    Message *msg = s->receive(true);
    if (msg == nullptr) continue;

    SubMessage *m = messages_.at(s);
    const size_t size = (msg->getSize() / sizeof(capnp::word)) + 1;
    if (m->buf.size() < size) {
      m->buf = kj::heapArray<capnp::word>(size);
    }
    memcpy(m->buf.begin(), msg->getData(), msg->getSize());
    if (m->msg) {
      delete m->msg;
    }
    m->msg = msg;

    m->updated = true;
    m->rcv_time = current_time;
    m->rcv_frame = frame;
    m->valid = m->event.getValid();
    m->logMonoTime = m->event.getLogMonoTime();

    // TODO: don't do any of this if we're a python SubMaster
    if (m->msg_reader) {
      m->msg_reader->~FlatArrayMessageReader();
    }
    m->msg_reader = new (m->allocated_msg_reader) capnp::FlatArrayMessageReader(kj::ArrayPtr<capnp::word>(m->buf.begin(), size));
    m->event = m->msg_reader->getRoot<cereal::Event>();
  }

  // update alive
  for (auto &kv : messages_) {
    SubMessage *m = kv.second;
    m->alive = m->freq <= (1e-5) || ((current_time - m->rcv_time) * (1e-9)) < (10.0 / m->freq);
  }
  return recvd_socks.size();
}

bool SubMaster::allAlive(const std::vector<std::string > &service_list) {
  for (auto &kv : messages_) {
    SubMessage *m = kv.second;
    if (!m->valid && (service_list.size() == 0 || inList(service_list, m->name))) {
      return false;
    }
  }
  return true;
}

bool SubMaster::allValid(const std::vector<std::string > &service_list) {
  for (auto &kv : messages_) {
    SubMessage *m = kv.second;
    if (!m->valid && (service_list.size() == 0 || inList(service_list, m->name))) {
      return false;
    }
  }
  return true;
}

bool SubMaster::allAliveAndValid(const std::vector<std::string > &service_list) {
  return allAlive(service_list) && allValid(service_list);
}

void SubMaster::drain() {
  while (true) {
    auto polls = poller_->poll(0);
    if (polls.size() == 0)
      break;

    for (auto sock : polls) {
      Message *msg = sock->receive(true);
      delete msg;
    }
  }
}

bool SubMaster::updated(std::string name) {
  return services.at(name)->updated;
}

cereal::Event::Reader &SubMaster::operator[](std::string name) {
  return services.at(name)->event;
}

SubMaster::~SubMaster() {
  delete poller_;
  for (auto &kv : messages_) {
    SubMessage *m = kv.second;
    if (m->msg_reader) {
      m->msg_reader->~FlatArrayMessageReader();
    }
    free(m->allocated_msg_reader);
    delete m->socket;
    delete m;
  }
}

// PubMaster

PubMaster::PubMaster(const std::vector<std::string> &service_list) {
  for (auto name : service_list) {
    PubSocket *sock = PubSocket::create(message_context.ctx, name);
    assert(sock);
    sockets_[name] = sock;
  }
}

PubMaster::~PubMaster() {
  for (auto s : sockets_) {
    delete s.second;
  }
}

int PubMaster::send(std::string name, capnp::MessageBuilder &msg) {
  auto words = capnp::messageToFlatArray(msg);
  auto bytes = words.asBytes();
  return send(name, bytes.begin(), bytes.size());
}

int PubMaster::send(std::string name, capnp::byte *data, size_t size) {
  return send(name, (char *)data, size);
}

int PubMaster::send(std::string name, char *data, size_t size) {
  return sockets_.at(name)->send(data, size);
}
