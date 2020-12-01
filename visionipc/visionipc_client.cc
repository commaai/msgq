#include <chrono>
#include <cassert>
#include <iostream>
#include <thread>

#include "ipc.h"
#include "visionipc_client.h"
#include "cl_helpers.h"

VisionIpcClient::VisionIpcClient(std::string name, VisionStreamType type, cl_device_id device_id, cl_context ctx){
  init(name, type, device_id, ctx);
}

VisionIpcClient::VisionIpcClient(std::string name, VisionStreamType type, bool opencl){

  cl_device_id device_id = nullptr;
  cl_context ctx = nullptr;

  if (opencl){
    device_id = cl_get_device_id(CL_DEVICE_TYPE_CPU);
    ctx = CL_CHECK_ERR(clCreateContext(NULL, 1, &device_id, NULL, NULL, &err));
  }

  init(name, type, device_id, ctx);
}

void VisionIpcClient::init(std::string name, VisionStreamType type, cl_device_id device_id, cl_context ctx){
  // Connect to server socket and ask for all FDs of type
  std::string path = "/tmp/visionipc_" + name;

  int socket_fd = -1;
  while (socket_fd < 0) {
    std::cout << "Connecting to server" << std::endl;
    socket_fd = ipc_connect(path.c_str());

    if (socket_fd < 0) {
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
  std::cout << "Got fds! " << num_buffers << std::endl;

  // Import buffers
  for (size_t i = 0; i < num_buffers; i++){
    buffers[i] = bufs[i];
    buffers[i].fd = fds[i];
    visionbuf_import(&buffers[i]);

    if (device_id) visionbuf_init_cl(buffers, device_id, ctx);
  }

  // Create msgq subscriber
  msg_ctx = Context::create();
  std::string endpoint = "visionipc_" + name + "_" + std::to_string(type);
  sock = SubSocket::create(msg_ctx, endpoint);
}

VisionBuf * VisionIpcClient::recv(){
  // TODO: implement non blocking receive
  Message * r = sock->receive();

  if (r != nullptr){
    // Get buffer
    assert(r->getSize() == sizeof(VisionIpcPacket));
    VisionIpcPacket *packet = (VisionIpcPacket*)r->getData();

    assert(packet->idx < num_buffers);
    VisionBuf * buf = &buffers[packet->idx];

    // Sync buffer
    visionbuf_sync(buf, VISIONBUF_SYNC_TO_DEVICE);
    delete r;
    return buf;
  } else {
    return nullptr;
  }
}


VisionIpcClient::~VisionIpcClient(){
  delete sock;
  delete msg_ctx;
}
