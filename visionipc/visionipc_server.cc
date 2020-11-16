#include <iostream>
#include <chrono>
#include <cassert>

#include "ipc.h"
#include "visionipc_server.h"


VisionIpcServer::VisionIpcServer(std::string name, std::vector<VisionStreamType> types, size_t num_buffers, bool opencl) : name(name) {
  // Create server mutex
  // Create map + alloc requested buffers

  // Start listener thread
  should_exit = false;
  listener_thread = std::thread(&VisionIpcServer::listener, this);


  // Create msgq publisher for each of the `name` + type combos
}

void VisionIpcServer::listener(){
  // Start socket listener for `name`
  // - Wait for connection request of certain type
  // - Send over list of FDs
  std::cout << "Starting listener for: " << name << std::endl;

  std::string path = "/tmp/visionipc_" + name;
  int socket_fd = ipc_bind(path.c_str());
  assert(socket_fd >= 0);

  while (!should_exit){
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  std::cout << "Stopping listener for: " << name << std::endl;
}



VisionBuf * VisionIpcServer::get_buffer(VisionStreamType type){
  // Lock mutex
  // Get next available buffer idx/buffer
  return nullptr;
}

void VisionIpcServer::send(VisionBuf * buf){
  // Sync buffer
  // Send over correct msgq socket
}

VisionIpcServer::~VisionIpcServer(){
  should_exit = true;
  listener_thread.join();
}
