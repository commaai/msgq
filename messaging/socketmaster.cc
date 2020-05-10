#include <assert.h>

#include "messaging.hpp"
#include "services.h"

SubMaster::SubMaster(std::vector<const char *> services, bool needPoller,
                     const char *address, bool conflate, Context *context) : ctx(context), ownContext(false), frame(0) {
  if (ctx == NULL) {
    ctx = Context::create();
    ownContext = true;
  }
  poller = needPoller ? Poller::create() : nullptr;

  for (auto name : services) {
    createSocket(name, address, conflate);
  }
}

SubMaster::~SubMaster() {
  if (poller) delete poller;

  for (auto s : sockets) {
    delete s.second;
    delete s.first;
  }
  if (ownContext) delete ctx;
}

void SubMaster::createSocket(const char *endpoint, const char *address, bool conflate) {
  for (const auto &it : services) {
    if (strcmp(it.name, endpoint) == 0) {
      SubSocket *socket = SubSocket::create(ctx, endpoint, address ? address : "127.0.0.1", conflate);
      assert(socket != NULL);
      if (poller) {
        poller->registerSocket(socket);
      }
      sockets[socket] = new SubMessage(it.name, it.frequency, it.decimation);
      return;
    }
  }
  assert(0);
}

SubMessage *SubMaster::receive(bool non_blocking) {
  assert(sockets.size() == 1);
  if (++frame == UINT32_MAX) frame = 0;

  Message *msg = sockets.begin()->first->receive(non_blocking);
  if (msg) {
    SubMessage *m = sockets.begin()->second;
    m->update(frame, msg, false);
    return m;
  }
  return NULL;
}

std::vector<SubMessage *> SubMaster::poll_(int timeout, bool checkValid, bool alive) {
  assert(poller != NULL);
  if (++frame == UINT32_MAX) frame = 0;

  for (const auto &kv : sockets) {
    kv.second->updated = false;
  }

  time_t cur_time = time(NULL);
  std::vector<SubMessage *> messages;

  for (auto sock : poller->poll(timeout)) {
    Message *msg = sock->receive(true);
    if (msg) {
      SubMessage *m = sockets[sock];
      m->update(frame, msg, checkValid);
      bool is_alive = (cur_time - m->recv_time) < (10.0 / m->freq);
      if ((!checkValid || m->valid) && (!alive || is_alive)) {
        messages.push_back(m);
      }
    }
  }
  return messages;
}

SubMessage::SubMessage(const char *name, int frequency, int decimation) {
  this->name = name;
  this->freq = frequency;
  this->decimation = decimation;

  updated = valid = false;
  recv_time = 0;
  recv_frame = 0;

  msg_reader = NULL;
  msg = NULL;
}

SubMessage::~SubMessage() {
  if (msg_reader) delete msg_reader;
  if (msg) delete msg;
}

void SubMessage::update(int frame, Message *message, bool checkValid) {
  updated = true;
  recv_frame = frame;
  recv_time = time(NULL);
  if (msg_reader) {
    delete msg_reader;
    msg_reader = NULL;
  }
  if (msg) delete msg;

  msg = message;
  if (checkValid) {
    valid = getEvent().getValid();
  }
}

cereal::Event::Reader &SubMessage::getEvent() {
  if (msg_reader == NULL) {
    const size_t size = (msg->getSize() / sizeof(capnp::word)) + 1;
    if (size > buf.size()) {
      buf = kj::heapArray<capnp::word>(size);
    }
    memcpy(buf.begin(), msg->getData(), msg->getSize());
    msg_reader = new capnp::FlatArrayMessageReader(kj::ArrayPtr<capnp::word>(buf.begin(), size));
    event = msg_reader->getRoot<cereal::Event>();
  }
  return event;
}

PubMaster::PubMaster(std::vector<const char *> services, Context *context) : ctx(context), ownContext(false) {
  if (ctx == NULL) {
    ctx = Context::create();
    ownContext = true;
  }
  for (auto name : services) {
    PubSocket *sock = PubSocket::create(ctx, name);
    assert(sock != NULL);
    sockets[name] = sock;
  }
}

PubMaster::~PubMaster() {
  for (auto s : sockets) {
    delete s.second;
  }
  if (ownContext) delete ctx;
}

int PubMaster::send(const char *name, char *data, size_t size) {
  PubSocket *sock = sockets[name];
  assert(sock != NULL);
  return sock->send(data, size);
}

int PubMaster::send(const char *name, capnp::MessageBuilder &msg) {
  auto words = capnp::messageToFlatArray(msg);
  auto bytes = words.asBytes();
  return send(name, (char *)bytes.begin(), bytes.size());
}
