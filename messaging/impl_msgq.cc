#include <cassert>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <cerrno>
#include <mutex>

#include "impl_msgq.hpp"

volatile sig_atomic_t msgq_do_exit = 0;

void (*old_sigint_handler)(int);
void (*old_sigterm_handler)(int);

void sig_handler(int signal) {
  assert(signal == SIGINT || signal == SIGTERM);
  msgq_do_exit = 1;
  if (signal == SIGINT && old_sigint_handler) {
    old_sigint_handler(signal);
  }
  if (signal == SIGTERM && old_sigterm_handler) {
    old_sigterm_handler(signal);
  }
}

std::once_flag flag;

void init_sig_handler() {
  std::call_once(flag, []() {
    old_sigint_handler = old_sigterm_handler = nullptr;
    struct sigaction old;
    if (sigaction(SIGINT, NULL, &old) == 0) {
      old_sigint_handler = old.sa_handler;
    }
    if (sigaction(SIGTERM, NULL, &old) == 0) {
      old_sigterm_handler = old.sa_handler;
    }

    struct sigaction sa = {};
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
  });
}

static size_t get_size(std::string endpoint){
  size_t sz = DEFAULT_SEGMENT_SIZE;

#if !defined(QCOM) && !defined(QCOM2)
  if (endpoint == "frame" || endpoint == "frontFrame" || endpoint == "wideFrame"){
    sz *= 10;
  }
#endif

  return sz;
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

int MSGQSubSocket::connect(Context *context, std::string endpoint, std::string address, bool conflate){
  assert(context);
  assert(address == "127.0.0.1");

  init_sig_handler();

  q = new msgq_queue_t;
  int r = msgq_new_queue(q, endpoint.c_str(), get_size(endpoint));
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


Message * MSGQSubSocket::receive(bool non_blocking){
  msgq_do_exit = 0;

  msgq_msg_t msg;

  MSGQMessage *r = NULL;

  int rc = msgq_msg_recv(&msg, q);

  // Hack to implement blocking read with a poller. Don't use this
  while (!non_blocking && rc == 0 && msgq_do_exit == 0){
    msgq_pollitem_t items[1];
    items[0].q = q;

    int t = (timeout != -1) ? timeout : 100;

    int n = msgq_poll(items, 1, t);
    rc = msgq_msg_recv(&msg, q);

    // The poll indicated a message was ready, but the receive failed. Try again
    if (n == 1 && rc == 0){
      continue;
    }

    if (timeout != -1){
      break;
    }
  }

  errno = msgq_do_exit ? EINTR : 0;

  if (rc > 0){
    if (msgq_do_exit){
      msgq_msg_close(&msg); // Free unused message on exit
    } else {
      r = new MSGQMessage;
      r->takeOwnership(msg.data, msg.size);
    }
  }

  return (Message*)r;
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

int MSGQPubSocket::connect(Context *context, std::string endpoint){
  assert(context);

  q = new msgq_queue_t;
  int r = msgq_new_queue(q, endpoint.c_str(), get_size(endpoint));
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
