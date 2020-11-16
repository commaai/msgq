#include "visionipc_server.h"


VisionIpcServer::VisionIpcServer(std::string name, std::vector<VisionStreamType> types, size_t num_buffers, bool opencl){
  // Create server mutex
  // Create map + alloc requested buffers

  // Start socket listener for `name`
  // - Wait for connection request of certain type
  // - Send over list of FDs

  // Create msgq publisher for each of the `name` + type combos
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
