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

  void init(std::string name, VisionStreamType type, bool conflate, cl_device_id device_id, cl_context ctx);

public:
  int num_buffers;
  VisionBuf buffers[VISIONIPC_MAX_FDS];
  VisionIpcClient(std::string name, VisionStreamType type, bool conflate, cl_device_id device_id, cl_context ctx);
  VisionIpcClient(std::string name, VisionStreamType type, bool conflate=false, bool opencl=true);
  ~VisionIpcClient();
  VisionBuf * recv(VIPCBufExtra * extra=nullptr);
};
