#pragma once
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <map>

#include "visionipc.h"
#include "visionbuf.h"

class VisionIpcServer {
 private:
  std::atomic<bool> should_exit;
  std::string name;
  std::thread listener_thread;

  std::map<VisionStreamType, std::atomic<size_t> > cur_idx;
  std::map<VisionStreamType, std::vector<VisionBuf*> > buffers;
  std::map<VisionStreamType, std::map<VisionBuf*, size_t> > idxs;

 public:
  VisionIpcServer(std::string name, std::vector<VisionStreamType> types, size_t num_buffers=10, bool opencl=true);
  ~VisionIpcServer();

  void listener(void);
  VisionBuf * get_buffer(VisionStreamType type);
  void send(VisionBuf * buf, bool sync=true);
};
