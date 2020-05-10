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

class MessageContext {
 public:
  MessageContext() { ctx_ = Context::create(); }
  ~MessageContext() { delete ctx_; }
  inline Context *operator&() { return ctx_; }
  inline Context *getContext() { return ctx_; }

 private:
  Context *ctx_;
};

class SubMessage {
 public:
  SubMessage(Context *ctx, const char *name, const char *address = nullptr, bool conflate = false, bool ignore_alive = false);
  inline int getFreq() const { return freq_; }
  inline bool isAlive() const { return alive_; }
  inline bool isValid() const { return valid_; }
  inline uint64_t getFrame() const { return rcv_frame_; }
  inline uint64_t getLogMonoTime() const { return logMonoTime_; }
  inline cereal::Event::Reader *receive(bool non_blocking = false) { return receive_(non_blocking, 0, rcv_frame_++) ? &event_ : nullptr; }
  ~SubMessage();

 private:
  bool receive_(bool non_blocking, uint64_t current_time, uint64_t current_frame);
  std::string name_;
  Context *ctx_ = nullptr;
  SubSocket *socket_ = nullptr;
  int freq_ = 0;
  bool updated_ = false, alive_ = false, valid_ = false, ignore_alive_;
  uint64_t rcv_time_ = 0, logMonoTime_ = 0, rcv_frame_ = 0;
  void *allocated_msg_reader_ = nullptr;
  capnp::FlatArrayMessageReader *msg_reader_ = nullptr;
  kj::Array<capnp::word> buf_;
  cereal::Event::Reader event_;
  friend class SubMaster;
};

class SubMaster {
 public:
  SubMaster(Context *ctx, std::initializer_list<const char *> service_list, std::initializer_list<const char *> ignore_alive = {}, const char *address = nullptr);
  int update(int timeout = 1000);
  inline std::vector<cereal::Event::Reader> allAlive() { return all_(nullptr, false, true); }
  inline std::vector<cereal::Event::Reader> allValid() { return all_(nullptr, true, false); }
  inline std::vector<cereal::Event::Reader> allAliveAndValid() { return all_(nullptr, true, true); }
  inline std::vector<cereal::Event::Reader> allAlive(std::initializer_list<const char *> service_list) { return all_(&service_list, false, true); }
  inline std::vector<cereal::Event::Reader> allValid(std::initializer_list<const char *> service_list) { return all_(&service_list, true, false); }
  inline std::vector<cereal::Event::Reader> allAliveAndValid(std::initializer_list<const char *> service_list) { return all_(&service_list, true, true); }
  inline uint64_t getFrame() const { return frame_; }
  inline cereal::Event::Reader operator[](const char *name) { return services_[name]->event_; }
  ~SubMaster();

 private:
  std::vector<cereal::Event::Reader> all_(std::initializer_list<const char *> *service_list, bool valid, bool alive);
  Context *ctx_ = nullptr;
  Poller *poller_ = nullptr;
  uint64_t frame_ = 0;
  std::map<SubSocket *, SubMessage *> messages_;
  std::map<std::string, SubMessage *> services_;
};

class PubMessage {
 public:
  PubMessage(Context *ctx, const char *name);
  int send(capnp::MessageBuilder &msg);
  inline int send(char *data, size_t size) { return socket_->send(data, size); }
  ~PubMessage();

 private:
  Context *ctx_ = nullptr;
  PubSocket *socket_ = nullptr;
  kj::Array<kj::byte> buf_;
};

class PubMaster {
 public:
  PubMaster(Context *ctx, std::initializer_list<const char *> service_list){
    if (ctx == nullptr) ctx = ctx_ = Context::create();
    for (auto name : service_list) {
      messages_[name] = new PubMessage(ctx, name);
    }
  }
  inline int send(const char *name, char *data, size_t size) { return messages_[name]->send(data, size); }
  inline int send(const char *name, capnp::MessageBuilder &msg) { return messages_[name]->send(msg); }
  ~PubMaster() {
    for (auto m : messages_) delete m.second;
    delete ctx_;
  }

 private:
  Context *ctx_ = nullptr;
  std::map<std::string, PubMessage *> messages_;
};

