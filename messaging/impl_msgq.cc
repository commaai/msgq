#include <cassert>
#include <iostream>

#include "services.h"
#include "impl_msgq.h"

static bool service_exists(std::string path){
  for (const auto& it : services) {
    if (it.name == path) {
      return true;
    }
  }
  return false;
}


MSGQContext::MSGQContext() {
}

MSGQContext::~MSGQContext() {
}

void MSGQMessage::init(size_t sz) {
  size = sz;
  data = new char[size];
}

void MSGQMessage::init(char * d, size_t sz) {
  size = sz;
  data = new char[size];
  memcpy(data, d, size);
}

void MSGQMessage::takeOwnership(char * d, size_t sz) {
  size = sz;
  data = d;
}

void MSGQMessage::close() {
  if (size > 0){
    delete[] data;
  }
  size = 0;
}

MSGQMessage::~MSGQMessage() {
  this->close();
}

int MSGQSubSocket::connect(Context *context, std::string endpoint, std::string address, bool conflate, bool check_endpoint){
  assert(context);
  assert(address == "127.0.0.1");

  if (check_endpoint && !service_exists(std::string(endpoint))){
    std::cout << "Warning, " << std::string(endpoint) << " is not in service list." << std::endl;
  }

  q = new msgq_queue_t;
  int r = msgq_new_queue(q, endpoint.c_str(), DEFAULT_SEGMENT_SIZE);
  if (r != 0){
    return r;
  }

  msgq_init_subscriber(q);

  if (conflate){
    q->read_conflate = true;
  }

  timeout = -1;

  return 0;
}

Message *MSGQSubSocket::receive(bool non_blocking) {
  msgq_msg_t msg;
  int rc = 0;
  if (non_blocking) {
    rc = msgq_msg_recv(&msg, q);
  } else {
    msgq_pollitem_t pollitem = {.q = q};
    if (msgq_poll(&pollitem, 1, timeout) == 1) {
      rc = msgq_msg_recv(&msg, q);
    }
  }

  MSGQMessage *r = nullptr;
  if (rc > 0) {
    r = new MSGQMessage;
    r->takeOwnership(msg.data, msg.size);
  }
  return r;
}

void MSGQSubSocket::setTimeout(int t){
  timeout = t;
}

MSGQSubSocket::~MSGQSubSocket(){
  if (q != NULL){
    msgq_close_queue(q);
    delete q;
  }
}

int MSGQPubSocket::connect(Context *context, std::string endpoint, bool check_endpoint){
  assert(context);

  if (check_endpoint && !service_exists(std::string(endpoint))){
    std::cout << "Warning, " << std::string(endpoint) << " is not in service list." << std::endl;
  }

  q = new msgq_queue_t;
  int r = msgq_new_queue(q, endpoint.c_str(), DEFAULT_SEGMENT_SIZE);
  if (r != 0){
    return r;
  }

  msgq_init_publisher(q);

  return 0;
}

int MSGQPubSocket::sendMessage(Message *message){
  msgq_msg_t msg;
  msg.data = message->getData();
  msg.size = message->getSize();

  return msgq_msg_send(&msg, q);
}

int MSGQPubSocket::send(char *data, size_t size){
  msgq_msg_t msg;
  msg.data = data;
  msg.size = size;

  return msgq_msg_send(&msg, q);
}

bool MSGQPubSocket::all_readers_updated() {
  return msgq_all_readers_updated(q);
}

MSGQPubSocket::~MSGQPubSocket(){
  if (q != NULL){
    msgq_close_queue(q);
    delete q;
  }
}


void MSGQPoller::registerSocket(SubSocket * socket){
  assert(num_polls + 1 < MAX_POLLERS);
  polls[num_polls].q = (msgq_queue_t*)socket->getRawSocket();

  sockets.push_back(socket);
  num_polls++;
}

std::vector<SubSocket*> MSGQPoller::poll(int timeout){
  std::vector<SubSocket*> r;

  msgq_poll(polls, num_polls, timeout);
  for (size_t i = 0; i < num_polls; i++){
    if (polls[i].revents){
      r.push_back(sockets[i]);
    }
  }

  return r;
}
