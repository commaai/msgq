#include "messaging.hpp"
#include "impl_zmq.hpp"
#include "impl_msgq.hpp"

Context * Context::create(){
  Context * c;
  if (std::getenv("MSGQ")){
    c = new MSGQContext();
  } else {
    c = new ZMQContext();
  }
  return c;
}

SubSocket * SubSocket::create(){
  SubSocket * s;
  if (std::getenv("MSGQ")){
    s = new MSGQSubSocket();
  } else {
    s = new ZMQSubSocket();
  }
  return s;
}

SubSocket * SubSocket::create(Context * context, std::string endpoint){
  SubSocket *s = SubSocket::create();
  s->connect(context, endpoint, "127.0.0.1");

  return s;
}

SubSocket * SubSocket::create(Context * context, std::string endpoint, std::string address){
  SubSocket *s = SubSocket::create();
  s->connect(context, endpoint, address);

  return s;
}

SubSocket * SubSocket::create(Context * context, std::string endpoint, std::string address, bool conflate){
  SubSocket *s = SubSocket::create();
  s->connect(context, endpoint, address, conflate);

  return s;
}

PubSocket * PubSocket::create(){
  PubSocket * s;
  if (std::getenv("MSGQ")){
    s = new MSGQPubSocket();
  } else {
    s = new ZMQPubSocket();
  }
  return s;
}

PubSocket * PubSocket::create(Context * context, std::string endpoint){
  PubSocket *s = PubSocket::create();
  s->connect(context, endpoint);
  return s;
}

Poller * Poller::create(){
  Poller * p;
  if (std::getenv("MSGQ")){
    p = new MSGQPoller();
  } else {
    p = new ZMQPoller();
  }
  return p;
}

Poller * Poller::create(std::vector<SubSocket*> sockets){
  Poller * p = Poller::create();

  for (auto s : sockets){
    p->registerSocket(s);
  }
  return p;
}

extern "C" Context * messaging_context_create() {
  return Context::create();
}

extern "C" SubSocket * messaging_subsocket_create(Context* context, const char* endpoint) {
  return SubSocket::create(context, std::string(endpoint));
}

extern "C" PubSocket * messaging_pubsocket_create(Context* context, const char* endpoint) {
  return PubSocket::create(context, std::string(endpoint));
}

extern "C" Poller * messaging_poller_create(SubSocket** sockets, int size) {
  std::vector<SubSocket*> socketsVec(sockets, sockets + size);
  return Poller::create(socketsVec);
}
