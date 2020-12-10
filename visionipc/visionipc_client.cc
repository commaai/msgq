#include <chrono>
#include <cassert>
#include <iostream>
#include <thread>

#include "ipc.h"
#include "visionipc_client.h"
#include "cl_helpers.h"

VisionIpcClient::VisionIpcClient(std::string name, VisionStreamType type, bool conflate, cl_device_id device_id, cl_context ctx) : name(name), type(type), device_id(device_id), ctx(ctx) {
  init_msgq(conflate);
}

VisionIpcClient::VisionIpcClient(std::string name, VisionStreamType type, bool conflate, bool opencl) :
  name(name), type(type) {
  device_id = nullptr;
  ctx = nullptr;

  if (opencl){
    device_id = cereal_cl_get_device_id(CL_DEVICE_TYPE_CPU);
    ctx = CL_CHECK_ERR(clCreateContext(NULL, 1, &device_id, NULL, NULL, &err));
  }

  init_msgq(conflate);
}

void VisionIpcClient::init_msgq(bool conflate){
  msg_ctx = Context::create();
  std::string endpoint = "visionipc_" + name + "_" + std::to_string(type);
  sock = SubSocket::create(msg_ctx, endpoint, "127.0.0.1", conflate);
  sock->setTimeout(100);
}

void VisionIpcClient::connect(void){
  // TODO: What to do with old buffers?
  assert(!connected);

  // Connect to server socket and ask for all FDs of type
  std::string path = "/tmp/visionipc_" + name;

  int socket_fd = -1;
  while (socket_fd < 0) {
    socket_fd = ipc_connect(path.c_str());

    if (socket_fd < 0) {
      std::cout << "VisionIpcClient connecting" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  // Send stream type to server to request FDs
  int r = ipc_sendrecv_with_fds(true, socket_fd, &type, sizeof(type), nullptr, 0, nullptr);
  assert(r == sizeof(type));

  // Get FDs
  int fds[VISIONIPC_MAX_FDS];
  VisionBuf bufs[VISIONIPC_MAX_FDS];
  r = ipc_sendrecv_with_fds(false, socket_fd, &bufs, sizeof(bufs), fds, VISIONIPC_MAX_FDS, &num_buffers);

  assert(num_buffers > 0);
  assert(r == sizeof(VisionBuf) * num_buffers);

  // Import buffers
  for (size_t i = 0; i < num_buffers; i++){
    buffers[i] = bufs[i];
    buffers[i].fd = fds[i];
    visionbuf_import(&buffers[i]);
    if (buffers[i].rgb) {
      visionbuf_init_rgb(&buffers[i], buffers[i].width, buffers[i].height);
    } else {
      visionbuf_init_yuv(&buffers[i], buffers[i].width, buffers[i].height);
    }

    if (device_id) visionbuf_init_cl(&buffers[i], device_id, ctx);
  }

  connected = true;
}

VisionBuf * VisionIpcClient::recv(VIPCBufExtra * extra){
  // TODO: implement non blocking receive
  Message * r = sock->receive();

  if (r == nullptr){
    return nullptr;
  }

  // Get buffer
  assert(r->getSize() == sizeof(VisionIpcPacket));
  VisionIpcPacket *packet = (VisionIpcPacket*)r->getData();

  assert(packet->idx < num_buffers);
  VisionBuf * buf = &buffers[packet->idx];

  if (buf->server_id != packet->server_id){
    connected = false;
    delete r;
    return nullptr;
  }

  if (extra) {
    *extra = packet->extra;
  }

  // Sync buffer
  visionbuf_sync(buf, VISIONBUF_SYNC_TO_DEVICE);
  delete r;
  return buf;
}



VisionIpcClient::~VisionIpcClient(){
  delete sock;
  delete msg_ctx;
}
