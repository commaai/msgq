#pragma once
#include <vector>
#include <string>

#include "visionipc.h"
#include "visionbuf.h"

class VisionIpcServer {
 private:

 public:
  VisionIpcServer(std::string name, std::vector<VisionStreamType> types, size_t num_buffers=10, bool opencl=true);
  VisionBuf * get_buffer(VisionStreamType type);
  void send(VisionBuf * buf);
};
