#include <iostream>
#include <chrono>
#include <cassert>

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "messaging.hpp"
// TODO: rename header files to hpp for consistency
#include "ipc.h"
#include "cl_helpers.h"
#include "visionipc_server.h"

// TODO: Create constructor that accepts CL context if we want to reuse existing one

VisionIpcServer::VisionIpcServer(std::string name, std::vector<VisionStreamType> types, size_t num_buffers, bool opencl) : name(name) {
  assert(num_buffers <= VISIONIPC_MAX_FDS);

  // Get openCL context
  int err;
  cl_device_id device_id = cl_get_device_id(CL_DEVICE_TYPE_CPU);
  cl_context ctx = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
  assert(err == 0);

  msg_ctx = Context::create();

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

    // Create msgq publisher for each of the `name` + type combos
    // TODO: compute port number directly if using zmq, and hide warnings on msgq
    std::string endpoint = "visionipc_" + name + "_" + std::to_string(type);
    sockets[type] = PubSocket::create(msg_ctx, endpoint);
  }

  // Start listener thread
  should_exit = false;
  listener_thread = std::thread(&VisionIpcServer::listener, this);

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

    VisionStreamType type = VisionStreamType::VISION_STREAM_MAX;
    int r = ipc_sendrecv_with_fds(false, fd, &type, sizeof(type), nullptr, 0, nullptr);
    assert(r == sizeof(type));
    assert(buffers.count(type));

    int fds[VISIONIPC_MAX_FDS];
    int num_fds = buffers[type].size();
    VisionBuf bufs[VISIONIPC_MAX_FDS];

    for (int i = 0; i < num_fds; i++){
      fds[i] = buffers[type][i]->fd;
      bufs[i] = *buffers[type][i];

      // Remove some private openCL/ion metadata
      bufs[i].buf_cl = 0;
      bufs[i].copy_q = 0;
      bufs[i].handle = 0;
    }

    r = ipc_sendrecv_with_fds(true, fd, &bufs, sizeof(VisionBuf) * num_fds, fds, num_fds, nullptr);

    close(fd);
  }

  std::cout << "Stopping listener for: " << name << std::endl;
  close(sock);
}



VisionBuf * VisionIpcServer::get_buffer(VisionStreamType type){
  // Do we want to keep track if the buffer has been sent out yet and warn user?
  assert(buffers.count(type));
  auto b = buffers[type];
  return b[cur_idx[type]++ % b.size()];
}

void VisionIpcServer::send(VisionBuf * buf, bool sync){
  if (sync) visionbuf_sync(buf, VISIONBUF_SYNC_FROM_DEVICE);
  assert(buffers.count(buf->type));
  assert(buf->idx < buffers[buf->type].size());

  std::cout << "Sending buffer idx: " << buf->idx;
  std::cout << " type " << buf->type << std::endl;

  // Send over correct msgq socket
  VisionIpcPacket packet = {0};
  packet.idx = buf->idx;
  // TODO: fill in other metadata
  sockets[buf->type]->send((char*)&packet, sizeof(packet));
}

VisionIpcServer::~VisionIpcServer(){
  should_exit = true;
  listener_thread.join();

  // VisionBuf cleanup
  for( auto const& [type, buf] : buffers ) {
    for (VisionBuf* b : buf){
      delete b;
    }
  }

  // Messaging cleanup
  for( auto const& [type, sock] : sockets ) {
    delete sock;
  }
  delete msg_ctx;
}
