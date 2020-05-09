#include <assert.h>
#include "messaging.hpp"
#include "services.h"

SubMaster::SubMaster(Context *context) : ctx(context), poller(nullptr), ownContext(false), frame(0) {
  if (ctx == NULL) {
    ctx = Context::create();
    ownContext = true;
  }
  poller = Poller::create();
}

SubMaster::SubMaster(std::vector<const char *> services, const char *address, bool conflate, Context *context) : SubMaster(context) {
  for (auto name : services) {
    createSocket(name, address, conflate);
  }
}

SubMaster::~SubMaster() {
  delete poller;
  for (auto s : sockets) {
    delete s.second;
    delete s.first;
  }
  if (ownContext) delete ctx;
}

SubMessage *SubMaster::createSocket(const char *endpoint, const char *address, bool conflate) {
  SubMessage *msg = NULL;
  for (const auto &it : services) {
    if (strcmp(it.name, endpoint) == 0) {
      SubSocket *socket = SubSocket::create(ctx, endpoint, address ? address : "127.0.0.1", conflate);
      assert(socket != NULL);
      poller->registerSocket(socket);
      msg = new SubMessage(it.name, it.frequency, it.decimation);
      sockets[socket] = msg;
      break;
    }
  }
  assert(msg != NULL);
  return msg;
}

std::vector<SubMessage *> SubMaster::poll_(int timeout, bool checkValid, bool alive) {
  if (++frame == UINT32_MAX) {
    frame = 0;
  }
  for (const auto &kv : sockets) {
    SubMessage *sd = kv.second;
    sd->updated = false;
  }

  while (true) {
    auto polls = poller->poll(timeout);
    if (polls.size() == 0) {
      break;
    }
    for (auto sock : polls) {
      Message *msg = sock->receive(true);
      if (msg) {
        sockets[sock]->update(frame, msg, checkValid);
      }
    }
  }

  std::vector<SubMessage *> messages;
  time_t cur_time = time(NULL);
  for (const auto &kv : sockets) {
    SubMessage *msg = kv.second;
    bool is_alive = (cur_time - msg->recv_time) < (10.0 / msg->freq);
    if (msg->updated && (!checkValid ||  msg->valid) && (!alive || is_alive)) {
      messages.push_back(msg);
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
  msg_data = NULL;
  msg_data_size = 0;
  msg = NULL;
}

SubMessage::~SubMessage() {
  if (msg_reader) delete msg_reader;
  if (msg_data) free(msg_data);
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
  if (msg) {
    delete msg;
  }
  msg = message;
  if (checkValid) {
    valid = getEvent().getValid();
  }
}

cereal::Event::Reader &SubMessage::getEvent() {
  if (msg_reader == NULL) {
    const size_t size = (msg->getSize() / sizeof(capnp::word)) + 1;
    if (msg_data_size < size) {
      msg_data = (capnp::word *)realloc(msg_data, size * sizeof(capnp::word));
      msg_data_size = size;
    }
    memcpy(msg_data, msg->getData(), msg->getSize());

    msg_reader = new capnp::FlatArrayMessageReader(kj::ArrayPtr<capnp::word>(msg_data, size));
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
