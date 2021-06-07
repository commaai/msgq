#include <iostream>
#include <string>
#include <cassert>
#include <csignal>
#include <map>

typedef void (*sighandler_t)(int sig);

#include "services.h"

#include "impl_msgq.h"
#include "impl_zmq.h"

void sigpipe_handler(int sig) {
  assert(sig == SIGPIPE);
  std::cout << "SIGPIPE received" << std::endl;
}

static std::vector<std::string> get_services() {
  std::vector<std::string> name_list;

  for (const auto& it : services) {
    std::string name = it.name;
    if (name == "plusFrame" || name == "uiLayoutState") continue;
    name_list.push_back(name);
  }

  return name_list;
}

int main(int argc, char** argv) {
  signal(SIGPIPE, (sighandler_t)sigpipe_handler);

  Poller *poller;
  Context *pub_context;
  Context *sub_context;

  bool unbridge = argc > 1;
  if (unbridge) {  // republishes zmq debugging messages as msgq
    poller = new ZMQPoller();
    pub_context = new MSGQContext();
    sub_context = new ZMQContext();
  } else {
    poller = new MSGQPoller();
    pub_context = new ZMQContext();
    sub_context = new MSGQContext();
  }

  std::map<SubSocket*, PubSocket*> sub2pub;
  for (auto endpoint: get_services()) {
    PubSocket * pub_sock;
    SubSocket * sub_sock;
    if (unbridge) {
      if (endpoint.rfind("test") != 0) {  // only republish debugging messages to avoid MultiplePublishersError
        continue;
      }
      pub_sock = new MSGQPubSocket();
      sub_sock = new ZMQSubSocket();
    } else {
      pub_sock = new ZMQPubSocket();
      sub_sock = new MSGQSubSocket();
    }
    pub_sock->connect(pub_context, endpoint);
    poller->registerSocket(sub_sock);

    sub_sock->connect(sub_context, endpoint, unbridge ? argv[1] : "127.0.0.1", false);
    sub2pub[sub_sock] = pub_sock;
  }


  while (true) {
    for (auto sub_sock : poller->poll(100)) {
      Message * msg = sub_sock->receive();
      if (msg == NULL) continue;
      sub2pub[sub_sock]->sendMessage(msg);
      delete msg;
    }
  }
  return 0;
}
