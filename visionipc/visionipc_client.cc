#include <cassert>
#include "ipc.h"
#include "visionipc_client.h"

VisionIpcClient::VisionIpcClient(std::string name, VisionStreamType type, bool opencl){
  // Connect to server socket and ask for all FDs of type
  std::string path = "/tmp/visionipc_" + name;

  // TODO: try again until succeeds
  int socket_fd = ipc_connect(path.c_str());
  assert(socket_fd >= 0);


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
