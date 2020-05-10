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
static const service *get_service(const char *name) {
  for (const auto &it : services) {
    if (strcmp(it.name, name) == 0) return &it;
  }
  return nullptr;
}
static inline bool inList(std::initializer_list<const char *> &list, const char *value) {
  for (auto &v : list){
    if (strcmp(v, value) == 0) return true;
  }
  return false;
}

SubMessage::SubMessage(Context *ctx, const char *name, const char *address, bool conflate, bool ignore_alive)
    : name_(name), ignore_alive_(ignore_alive), buf_(kj::heapArray<capnp::word>(1024)) {
  if (ctx == nullptr) ctx = ctx_ = Context::create();
  const service *serv = get_service(name);
  assert(serv != nullptr);
  freq_ = serv->frequency;
  socket_ = SubSocket::create(ctx, name, address ? address : "127.0.0.1", conflate);
  assert(socket_ != nullptr);
  allocated_msg_reader_ = malloc(sizeof(capnp::FlatArrayMessageReader));
}

SubMessage::~SubMessage() {
  if (msg_reader_) {
    msg_reader_->~FlatArrayMessageReader();
  }
  free(allocated_msg_reader_);
  delete socket_;
  delete ctx_;
}

bool SubMessage::receive_(bool non_blocking, uint64_t current_time, uint64_t current_frame) {
  Message *msg = socket_->receive(non_blocking);
  if (msg == nullptr) return false;

  const size_t size = (msg->getSize() / sizeof(capnp::word)) + 1;
  if (buf_.size() < size) {
    buf_ = kj::heapArray<capnp::word>(size);
  }
  memcpy(buf_.begin(), msg->getData(), msg->getSize());
  delete msg;

  // construct msg_reader_ at the pre-allocated buf.
  if (msg_reader_) {
    msg_reader_->~FlatArrayMessageReader();
  }
  msg_reader_ = new (allocated_msg_reader_) capnp::FlatArrayMessageReader(kj::ArrayPtr<capnp::word>(buf_.begin(), size));
  event_ = msg_reader_->getRoot<cereal::Event>();

  updated_ = true;
  rcv_time_ = current_time ? current_time : nanos_since_boot();
  rcv_frame_ = current_frame;
  valid_ = event_.getValid();
  logMonoTime_ = event_.getLogMonoTime();
  return true;
}

SubMaster::SubMaster(Context *ctx, std::initializer_list<const char *> service_list, std::initializer_list<const char *> ignore_alive, const char *address) {
  if (ctx == nullptr) ctx = ctx_ = Context::create();
  poller_ = Poller::create();
  for (auto name : service_list) {
    SubMessage *m = new SubMessage(ctx, name, address, false, inList(ignore_alive, name));
    poller_->registerSocket(m->socket_);
    messages_[m->socket_] = m;
    services_[name] = m;
  }
}

SubMaster::~SubMaster() {
  delete poller_;
  for (auto &kv : messages_) delete kv.second;
  delete ctx_;
}

int SubMaster::update(int timeout) {
  if (++frame_ == UINT64_MAX) frame_ = 1;
  for (auto &kv : messages_) kv.second->updated_ = false;

  int updated = 0;
  uint64_t current_time = nanos_since_boot();
  for (auto sock : poller_->poll(timeout)) {
    updated += (int)(messages_[sock]->receive_(true, current_time, frame_));
  }

  for (auto &kv : messages_) {
    SubMessage *m = kv.second;
    m->alive_ = (m->freq_ <= (1e-5) || ((current_time - m->rcv_time_) * (1e-9)) < (10.0 / m->freq_));
  }
  return updated;
}

std::vector<cereal::Event::Reader> SubMaster::all_(std::initializer_list<const char *> *service_list, bool valid, bool alive) {
  std::vector<cereal::Event::Reader> result;
  for (auto &kv : messages_) {
    SubMessage *m = kv.second;
    if (!service_list || inList(*service_list, m->name_.c_str())) {
      if ((!valid || m->valid_) && (!alive || (m->alive_ && !m->ignore_alive_))) {
        result.push_back(m->event_);
      }
    }
  }
  return result;
}

PubMessage::PubMessage(Context *ctx, const char *name) : buf_(kj::heapArray<kj::byte>(4096)) {
  if (ctx == nullptr) ctx = ctx_ = Context::create();
  assert(get_service(name) != nullptr);
  socket_ = PubSocket::create(ctx, name);
  assert(socket_ != nullptr);
}

PubMessage::~PubMessage() {
  delete socket_;
  delete ctx_;
}

int PubMessage::send(capnp::MessageBuilder &msg) {
  size_t byte_size = capnp::computeSerializedSizeInWords(msg) * sizeof(capnp::word);
  if (buf_.size() < byte_size) {
    buf_ = kj::heapArray<kj::byte>(byte_size);
  }
  kj::ArrayOutputStream output(buf_);
  capnp::writeMessage(output, msg);
  auto bytes = output.getArray();
  return send((char *)bytes.begin(), bytes.size());
}
