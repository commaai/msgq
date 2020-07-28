#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <capnp/serialize.h>
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

struct SubMessage {
  std::string name;
  SubSocket *socket;
  int freq;
  bool updated, alive, valid, ignore_alive;
  uint64_t rcv_time, rcv_frame, logMonoTime;
  void *allocated_msg_reader;
  capnp::FlatArrayMessageReader *msg_reader;
  kj::Array<capnp::word> buf;
  Message * msg;
  cereal::Event::Reader event;
};

class SubMaster {
public:
  SubMaster(const std::vector<std::string> &service_list, std::string address = "127.0.0.1",
            const std::vector<std::string> &ignore_alive = {});
  ~SubMaster();

  int update(int timeout = 1000);
  void drain();

  cereal::Event::Reader &operator[](std::string);
  Message * getMessage(std::string name);

  uint64_t frame;
  std::map<std::string, SubMessage *> services;

  bool allAlive(const std::vector<std::string> &service_list = {});
  bool allValid(const std::vector<std::string> &service_list = {});
  bool allAliveAndValid(const std::vector<std::string> &service_list = {});
  bool updated(std::string);
  uint64_t logMonoTime(std::string);

private:
  Poller *poller_;
  std::map<SubSocket *, SubMessage *> messages_;
};

class PubMaster {
public:
  PubMaster(const std::vector<std::string> &service_list);
  int send(std::string name, capnp::MessageBuilder &msg);
  int send(std::string name, capnp::byte *data, size_t size);
  int send(std::string name, char *data, size_t size);
  ~PubMaster();

private:
  std::map<std::string, PubSocket *> sockets_;
};
