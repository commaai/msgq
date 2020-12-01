#pragma once
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <map>

#include "messaging.hpp"
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

  Context * msg_ctx;
  std::map<VisionStreamType, PubSocket*> sockets;

  void listener(void);
  void init(std::string name, std::vector<VisionStreamType> types, size_t num_buffers, cl_device_id device_id, cl_context ctx);

 public:
  VisionIpcServer(std::string name, std::vector<VisionStreamType> types, size_t num_buffers, cl_device_id device_id, cl_context ctx);
  VisionIpcServer(std::string name, std::vector<VisionStreamType> types, size_t num_buffers=10, bool opencl=true);
  ~VisionIpcServer();

  VisionBuf * get_buffer(VisionStreamType type);
  void send(VisionBuf * buf, bool sync=true);
};
