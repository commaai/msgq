#pragma once
#include <vector>
#include <string>

#include "visionipc.h"
#include "visionbuf.h"

class VisionIpcClient {
 private:

 public:
  VisionIpcClient(std::string name, VisionStreamType type, bool opencl=true);
  ~VisionIpcClient();
  VisionBuf * recv();
};
