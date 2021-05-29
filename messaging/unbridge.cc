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


int main(void){
  signal(SIGPIPE, (sighandler_t)sigpipe_handler);

  auto endpoints = get_services();

  std::map<SubSocket*, PubSocket*> sub2pub;

  Context *zmq_context = new ZMQContext();
  Context *msgq_context = new MSGQContext();
  Poller *poller = new ZMQPoller();

  for (auto endpoint: endpoints){
    SubSocket * zmq_sock = new ZMQSubSocket();
    zmq_sock->connect(zmq_context, endpoint, "127.0.0.1", false);  // TODO: need to change this to the IP of the laptop?
    poller->registerSocket(zmq_sock);

    PubSocket * msgq_sock = new MSGQPubSocket();
    // TODO: we only want to republish testJoystick here, which should be the ONLY zmq service we see
    // TODO: don't want a multipub error, so see if we can define two pub socks (in openpilot and here), only send on one (openpilot), and be fine.
    // TODO: else we'll just have to give unbridge a service list of what services we want it to republish to openpilot
    msgq_sock->connect(msgq_context, endpoint);

    sub2pub[zmq_sock] = msgq_sock;
  }


  while (true){
    for (auto sub_sock : poller->poll(100)){
      Message * msg = sub_sock->receive();
      if (msg == NULL) continue;
      sub2pub[sub_sock]->sendMessage(msg);
      delete msg;
    }
  }
  return 0;
}
