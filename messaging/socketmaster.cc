#include <assert.h>
#include <time.h>
#include "messaging.hpp"
#include "services.h"

static inline uint64_t nanos_since_boot() {
  struct timespec t;
  clock_gettime(CLOCK_BOOTTIME, &t);
  return t.tv_sec * 1000000000ULL + t.tv_nsec;
}

static const service *get_service(const char *name) {
  for (const auto &it : services) {
    if (strcmp(it.name, name) == 0) return &it;
  }
  return nullptr;
}

static inline bool inList(const std::initializer_list<const char *> &list, const char *value) {
  for (auto &v : list) {
    if (strcmp(value, v) == 0) return true;
  }
  return false;
}

class MessageContext {
public:
  MessageContext() { ctx_ = Context::create(); }
  ~MessageContext() { delete ctx_; }
  Context *ctx_;
};
MessageContext ctx;

SubMessage::SubMessage(const char *name, const char *address, int timeout, bool conflate) : name(name) {
  buf = kj::heapArray<capnp::word>(1024);
  allocated_msg_reader = malloc(sizeof(capnp::FlatArrayMessageReader));

  socket = SubSocket::create(ctx.ctx_, name, address ? address : "127.0.0.1", conflate);
  assert(socket != nullptr);
  if (timeout > 0) {
    socket->setTimeout(timeout);
  }
}

std::optional<cereal::Event::Reader> SubMessage::receive(bool non_blocking) {
  Message *msg = socket->receive(non_blocking);
  if (msg == nullptr) return std::nullopt;

  const size_t size = (msg->getSize() + sizeof(capnp::word) - 1) / sizeof(capnp::word);
  if (buf.size() < size) {
    buf = kj::heapArray<capnp::word>(size);
  }
  memcpy(buf.begin(), msg->getData(), msg->getSize());

  if (msg_reader) {
    msg_reader->~FlatArrayMessageReader();
  }
  msg_reader = new (allocated_msg_reader) capnp::FlatArrayMessageReader(buf.slice(0, size));
  event = msg_reader->getRoot<cereal::Event>();

  return std::make_optional(event);
}


void SubMessage::drain() {
  while (receive(true)) { /* empty */ }
}

SubMessage::~SubMessage() {
  if (msg_reader) {
    msg_reader->~FlatArrayMessageReader();
  }
  free(allocated_msg_reader);
  delete socket;
}

SubMaster::SubMaster(const std::initializer_list<const char *> &service_list, const char *address,
                     const std::initializer_list<const char *> &ignore_alive) {
  poller_ = Poller::create();
  for (auto name : service_list) {
    const service *serv = get_service(name);
    assert(serv != nullptr);
    SubMessage *m = new SubMessage(name, address, 0, true);
    m->freq = serv->frequency;
    m->ignore_alive = inList(ignore_alive, name);

    poller_->registerSocket(m->socket);
    services_[name] = messages_[m->socket] = m;
  }
}

int SubMaster::update(int timeout) {
  if (++frame == UINT64_MAX) frame = 1;
  for (auto &kv : messages_) kv.second->updated = false;

  int updated = 0;
  auto sockets = poller_->poll(timeout);
  uint64_t current_time = nanos_since_boot();
  for (auto s : sockets) {
    SubMessage *m = messages_.at(s);
    if (!m->receive(true)) continue;

    m->updated = true;
    m->rcv_time = current_time;
    m->rcv_frame = frame;
    m->valid = m->event.getValid();

    ++updated;
  }

  for (auto &kv : messages_) {
    SubMessage *m = kv.second;
    m->alive = (m->freq <= (1e-5) || ((current_time - m->rcv_time) * (1e-9)) < (10.0 / m->freq));
  }
  return updated;
}

bool SubMaster::all_(const std::initializer_list<const char *> &service_list, bool valid, bool alive) {
  for (auto &kv : messages_) {
    SubMessage *m = kv.second;
    if (service_list.size() > 0 && !inList(service_list, m->name.c_str())) continue;

    if ((valid && !m->valid) || (alive && !(m->alive && !m->ignore_alive))) {
      return false;
    }
  }
  return true;
}

void SubMaster::drain() {
  while (true) {
    auto polls = poller_->poll(0);
    if (polls.size() == 0) break;

    for (auto sock : polls) {
      delete sock->receive(true);
    }
  }
}

bool SubMaster::updated(const char *name) const {
  return services_.at(name)->updated;
}

uint64_t SubMaster::rcv_frame(const char *name) const {
  return services_.at(name)->rcv_frame;
}

cereal::Event::Reader &SubMaster::operator[](const char *name) {
  return services_.at(name)->event;
};

SubMaster::~SubMaster() {
  delete poller_;
  for (auto &kv : messages_) delete kv.second;
}

PubMaster::PubMaster(const std::initializer_list<const char *> &service_list) {
  for (auto name : service_list) {
    assert(get_service(name) != nullptr);
    PubSocket *socket = PubSocket::create(ctx.ctx_, name);
    assert(socket);
    sockets_[name] = socket;
  }
}

int PubMaster::send(const char *name, MessageBuilder &msg) {
  auto bytes = msg.toBytes();
  return send(name, bytes.begin(), bytes.size());
}

PubMaster::~PubMaster() {
  for (auto s : sockets_) delete s.second;
}
