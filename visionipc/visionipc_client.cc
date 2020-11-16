#include "visionipc_client.h"

VisionIpcClient::VisionIpcClient(std::string name, VisionStreamType type, bool opencl){
  // Connect to server socket and ask for all FDs of type
  // Import buffers
  // Create msgq subscribers
}

VisionBuf * VisionIpcClient::recv(){
  // Blocking receive on msgq socket
  // Sync buffer

  return nullptr;
}
