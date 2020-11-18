#pragma once
#include <vector>
#include <string>
#include <unistd.h>

#include "messaging.hpp"
#include "visionipc.h"
#include "visionbuf.h"

class VisionIpcClient {
 private:
  Context * msg_ctx;
  SubSocket * sock;

  int num_buffers;
  VisionBuf buffers[VISIONIPC_MAX_FDS];

 public:
  VisionIpcClient(std::string name, VisionStreamType type, bool opencl=true);
  ~VisionIpcClient();
  VisionBuf * recv();
};
