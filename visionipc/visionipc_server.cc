#include <iostream>
#include <chrono>
#include <cassert>

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "messaging.hpp"
// TODO: rename header files to hpp for consistency
#include "ipc.h"
#include "visionipc_server.h"

VisionIpcServer::VisionIpcServer(std::string name, cl_device_id device_id, cl_context ctx) : name(name), device_id(device_id), ctx(ctx) {

  msg_ctx = Context::create();
}

VisionIpcServer::VisionIpcServer(std::string name, bool opencl) : name(name) {
  // Get openCL context
  if (opencl){
    device_id = cereal_cl_get_device_id(CL_DEVICE_TYPE_CPU);
    ctx = CL_CHECK_ERR(clCreateContext(NULL, 1, &device_id, NULL, NULL, &err));
  }

  msg_ctx = Context::create();
}

void VisionIpcServer::create_buffers(VisionStreamType type, size_t num_buffers, bool rgb, size_t width, size_t height){
  // TODO: assert that this type is not created yet
  // TODO: deal with rgb alignment
  size_t size = rgb ? 3 * width * height : width * height * 3 / 2;

  // Create map + alloc requested buffers
  for (size_t i = 0; i < num_buffers; i++){
    VisionBuf* buf = new VisionBuf();

    *buf = visionbuf_allocate(size);
    buf->idx = i;
    buf->type = type;

    if (device_id) visionbuf_init_cl(buf, device_id, ctx);

    rgb ? visionbuf_init_rgb(buf, width, height) : visionbuf_init_yuv(buf, width, height);

    buffers[type].push_back(buf);
  }

  cur_idx[type] = 0;

  // Create msgq publisher for each of the `name` + type combos
  // TODO: compute port number directly if using zmq, and hide warnings on msgq
  std::string endpoint = "visionipc_" + name + "_" + std::to_string(type);
  sockets[type] = PubSocket::create(msg_ctx, endpoint);
}


void VisionIpcServer::start_listener(){
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

void VisionIpcServer::send(VisionBuf * buf, VIPCBufExtra * extra, bool sync){
  if (sync) visionbuf_sync(buf, VISIONBUF_SYNC_FROM_DEVICE);
  assert(buffers.count(buf->type));
  assert(buf->idx < buffers[buf->type].size());

  // Send over correct msgq socket
  VisionIpcPacket packet = {0};
  packet.idx = buf->idx;
  packet.extra = *extra;

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
