#include <iostream>
#include <chrono>
#include <cassert>

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ipc.h"
#include "cl_helpers.h"
#include "visionipc_server.h"

// TODO: Create constructor that accepts CL context if we want to reuse existing one

VisionIpcServer::VisionIpcServer(std::string name, std::vector<VisionStreamType> types, size_t num_buffers, bool opencl) : name(name) {
  // Get openCL context
  int err;
  cl_device_id device_id = cl_get_device_id(CL_DEVICE_TYPE_CPU);
  cl_context ctx = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
  assert(err == 0);

  for (auto type : types) {
    // TODO: get length from stream type?
    size_t buf_len = 1024;

    // Create map + alloc requested buffers
    for (size_t i = 0; i < num_buffers; i++){
      VisionBuf* buf = new VisionBuf();

      *buf = visionbuf_allocate(buf_len);
      buf->idx = i;
      buf->type = type;

      if (opencl) visionbuf_init_cl(buf, device_id, ctx);

      buffers[type].push_back(buf);
    }

    cur_idx[type] = 0;
  }

  // Start listener thread
  should_exit = false;
  listener_thread = std::thread(&VisionIpcServer::listener, this);

  // Create msgq publisher for each of the `name` + type combos
}

void VisionIpcServer::listener(){
  std::cout << "Starting listener for: " << name << std::endl;

  std::string path = "/tmp/visionipc_" + name;
  int sock = ipc_bind(path.c_str());
  assert(sock >= 0);

  while (!should_exit){
    // Wait for incoming connection
    struct pollfd polls[1] = {{0}};
    polls[0].fd = sock;
    polls[0].events = POLLIN;

    int ret = poll(polls, 1, 100);
    if (ret < 0) {
      if (errno == EINTR || errno == EAGAIN) continue;
      std::cout << "poll failed, stopping listener" << std::endl;
      break;
    }

    if (should_exit) break;
    if (!polls[0].revents) {
      continue;
    }

    // Handle incoming request
    int fd = accept(sock, NULL, NULL);
    assert(fd >= 0);

    std::cout << "Connection accepted" << std::endl;

    // Get stream type from client
    // Send back fds belonging to type

    close(fd);
  }

  std::cout << "Stopping listener for: " << name << std::endl;
  close(sock);
}



VisionBuf * VisionIpcServer::get_buffer(VisionStreamType type){
  assert(buffers.count(type));
  return buffers[type][cur_idx[type]++ % buffers[type].size()];
}

void VisionIpcServer::send(VisionBuf * buf, bool sync){
  if (sync) visionbuf_sync(buf, VISIONBUF_SYNC_FROM_DEVICE);

  std::cout << "Sending buffer idx: " << buf->idx;
  std::cout << " type " << buf->type << std::endl;
  // Send over correct msgq socket
}

VisionIpcServer::~VisionIpcServer(){
  should_exit = true;
  listener_thread.join();

  for( auto const& [type, buf] : buffers ) {
    for (VisionBuf* b : buf){
      delete b;
    }
  }

}
