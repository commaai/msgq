#include <iostream>
#include <string>
#include <cassert>
#include <csignal>
#include <map>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include <yaml-cpp/yaml.h>
#pragma GCC diagnostic pop

#include "impl_msgq.hpp"
#include "impl_zmq.hpp"

void sigpipe_handler(int sig) {
  assert(sig == SIGPIPE);
  std::cout << "SIGPIPE received" << std::endl;
}

static std::vector<std::string> get_services() {
  char * base_dir_ptr = std::getenv("BASEDIR");

  if (base_dir_ptr == NULL){
    base_dir_ptr = std::getenv("PYTHONPATH");
  }

  assert(base_dir_ptr);
  std::string base_dir = base_dir_ptr;
  std::string service_list_path = base_dir + "/cereal/service_list.yaml";
  YAML::Node service_list = YAML::LoadFile(service_list_path);

  std::vector<std::string> name_list;

  for (const auto& it : service_list) {
    auto name = it.first.as<std::string>();
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
  Poller *poller = new MSGQPoller();

  for (auto endpoint: endpoints){
    SubSocket * msgq_sock = new MSGQSubSocket();
    msgq_sock->connect(msgq_context, endpoint, "127.0.0.1", false);
    poller->registerSocket(msgq_sock);

    PubSocket * zmq_sock = new ZMQPubSocket();
    zmq_sock->connect(zmq_context, endpoint);

    sub2pub[msgq_sock] = zmq_sock;
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
