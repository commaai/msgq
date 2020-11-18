#include <chrono>
#include <cassert>
#include <iostream>
#include <thread>

#include "ipc.h"
#include "visionipc_client.h"

VisionIpcClient::VisionIpcClient(std::string name, VisionStreamType type, bool opencl){
  // Connect to server socket and ask for all FDs of type
  std::string path = "/tmp/visionipc_" + name;

  int socket_fd = -1;
  while (socket_fd < 0) {
    std::cout << "Connecting to server" << std::endl;
    socket_fd = ipc_connect(path.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Import buffers
  // Create msgq subscribers
}

VisionBuf * VisionIpcClient::recv(){
  // Blocking receive on msgq socket
  // Sync buffer

  return nullptr;
}


VisionIpcClient::~VisionIpcClient(){
}
