#pragma once
#include <cstddef>
#include <vector>
#include <string>
#include <capnp/serialize.h>
#include <map>
#include "../gen/cpp/log.capnp.h"

#define MSG_MULTIPLE_PUBLISHERS 100

class Context {
public:
  virtual void * getRawContext() = 0;
  static Context * create();
  virtual ~Context(){};
};

class Message {
public:
  virtual void init(size_t size) = 0;
  virtual void init(char * data, size_t size) = 0;
  virtual void close() = 0;
  virtual size_t getSize() = 0;
  virtual char * getData() = 0;
  virtual ~Message(){};
};


class SubSocket {
public:
  virtual int connect(Context *context, std::string endpoint, std::string address, bool conflate=false) = 0;
  virtual void setTimeout(int timeout) = 0;
  virtual Message *receive(bool non_blocking=false) = 0;
  virtual void * getRawSocket() = 0;
  static SubSocket * create();
  static SubSocket * create(Context * context, std::string endpoint);
  static SubSocket * create(Context * context, std::string endpoint, std::string address);
  static SubSocket * create(Context * context, std::string endpoint, std::string address, bool conflate);
  virtual ~SubSocket(){};
};

class PubSocket {
public:
  virtual int connect(Context *context, std::string endpoint) = 0;
  virtual int sendMessage(Message *message) = 0;
  virtual int send(char *data, size_t size) = 0;
  static PubSocket * create();
  static PubSocket * create(Context * context, std::string endpoint);
  virtual ~PubSocket(){};
};

class Poller {
public:
  virtual void registerSocket(SubSocket *socket) = 0;
  virtual std::vector<SubSocket*> poll(int timeout) = 0;
  static Poller * create();
  static Poller * create(std::vector<SubSocket*> sockets);
  virtual ~Poller(){};
};

class SubMessage {
 public:
  SubMessage(const char *name, int frequency, int decimation);
  ~SubMessage();
  cereal::Event::Reader& getEvent();

  inline std::string getName() { return name; }
  inline char *getData() { return msg->getData(); }
  inline size_t getSize() { return msg->getSize(); }

 private:
  void update(int frame, Message *msg, bool checkValid);
  std::string name;
  int freq;
  int decimation;

  time_t recv_time;
  int recv_frame;
  bool valid;
  bool updated;
  Message *msg;

  capnp::word *msg_data;
  size_t msg_data_size;

  capnp::FlatArrayMessageReader *msg_reader;
  cereal::Event::Reader event;
  friend class SubMaster;
};

class SubMaster {
 public:
  SubMaster(Context *context = NULL);
  SubMaster(std::vector<const char *> services, const char *address = NULL, bool conflate = false, Context *context = NULL);
  ~SubMaster();
  SubMessage *createSocket(const char *endpoint, const char *address = NULL, bool conflate = false);
  inline std::vector<SubMessage *> poll(int timeout, bool checkValid = true) { return poll_(timeout, checkValid, false); }
  inline std::vector<SubMessage *> pollAlive(int timeout) { return poll_(timeout, true, true); }
  inline SubMessage *pollOne(int timeout, bool alive = false) {
    auto msgs = poll_(timeout, true, alive);
    return msgs.size() > 0 ? msgs[0] : NULL;
  }

 private:
  std::vector<SubMessage *> poll_(int timeout, bool checkValid, bool alive);
  bool readSocket(SubSocket *sock);
  uint32_t frame;
  bool ownContext;
  Context *ctx;
  Poller *poller;
  std::map<SubSocket *, SubMessage *> sockets;
};

class PubMaster {
 public:
  PubMaster(std::vector<const char *> services, Context *context = NULL);
  ~PubMaster();
  int send(const char *name, char *data, size_t size);
  int send(const char *name, capnp::MessageBuilder &msg);

 private:
  bool ownContext;
  Context *ctx;
  std::map<std::string, PubSocket *> sockets;
};
