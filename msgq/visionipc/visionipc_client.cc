#include <chrono>
#include <cassert>
#include <iostream>
#include <thread>
#include <string>
#include <set>

#include <unistd.h>
#include "msgq/visionipc/visionipc.h"
#include "msgq/visionipc/visionipc_client.h"
#include "msgq/visionipc/visionipc_server.h"
#include "msgq/logger/logger.h"

static int connect_to_vipc_server(const std::string &name, bool blocking) {
  const std::string ipc_path = get_ipc_path(name);
  int socket_fd = ipc_connect(ipc_path.c_str());
  while (socket_fd < 0 && blocking) {
    std::cout << "VisionIpcClient connecting" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    socket_fd = ipc_connect(ipc_path.c_str());
  }
  return socket_fd;
}

VisionIpcClient::VisionIpcClient(std::string name, VisionStreamType type, bool conflate, cl_device_id device_id, cl_context ctx) : name(name), type(type), device_id(device_id), ctx(ctx) {
  msg_ctx = Context::create();
  sock = SubSocket::create(msg_ctx, get_endpoint_name(name, type), "127.0.0.1", conflate, false);

  poller = Poller::create();
  poller->registerSocket(sock);
}

// Connect is not thread safe. Do not use the buffers while calling connect
bool VisionIpcClient::connect(bool blocking){
  auto start_total = std::chrono::high_resolution_clock::now();
  connected = false;

  // Cleanup old buffers on reconnect
  auto start_cleanup = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < num_buffers; i++){
    if (buffers[i].free() != 0) {
      LOGE("Failed to free buffer %zu", i);
    }
  }

  num_buffers = 0;
  auto end_cleanup = std::chrono::high_resolution_clock::now();
  auto cleanup_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_cleanup - start_cleanup).count();
  std::cout << "[VisionIpcClient::connect] Cleanup old buffers: " << cleanup_duration << " us" << std::endl;

  auto start_connect = std::chrono::high_resolution_clock::now();
  int socket_fd = connect_to_vipc_server(name, blocking);
  auto end_connect = std::chrono::high_resolution_clock::now();
  auto connect_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_connect - start_connect).count();
  std::cout << "[VisionIpcClient::connect] Connect to VIPC server: " << connect_duration << " us" << std::endl;
  if (socket_fd < 0) {
    return false;
  }
  // Send stream type to server to request FDs
  auto start_send_type = std::chrono::high_resolution_clock::now();
  int r = ipc_sendrecv_with_fds(true, socket_fd, &type, sizeof(type), nullptr, 0, nullptr);
  assert(r == sizeof(type));
  auto end_send_type = std::chrono::high_resolution_clock::now();
  auto send_type_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_send_type - start_send_type).count();
  std::cout << "[VisionIpcClient::connect] Send stream type to server: " << send_type_duration << " us" << std::endl;

  // Get FDs
  auto start_get_fds = std::chrono::high_resolution_clock::now();
  int fds[VISIONIPC_MAX_FDS];
  VisionBuf bufs[VISIONIPC_MAX_FDS];
  r = ipc_sendrecv_with_fds(false, socket_fd, &bufs, sizeof(bufs), fds, VISIONIPC_MAX_FDS, &num_buffers);
  auto end_get_fds = std::chrono::high_resolution_clock::now();
  auto get_fds_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_get_fds - start_get_fds).count();
  std::cout << "[VisionIpcClient::connect] Get FDs from server: " << get_fds_duration << " us" << std::endl;
  if (r < 0) {
    // only expected error is server shutting down
    assert(errno == ECONNRESET);
    close(socket_fd);
    return false;
  }

  assert(num_buffers >= 0);
  assert(r == sizeof(VisionBuf) * num_buffers);

  (void)device_id;
  (void)ctx;

  // Import buffers
  auto start_import = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < num_buffers; i++){
    buffers[i] = bufs[i];
    buffers[i].fd = fds[i];
    buffers[i].import();
    buffers[i].init_yuv(buffers[i].width, buffers[i].height, buffers[i].stride, buffers[i].uv_offset);

    if (device_id) buffers[i].init_cl(device_id, ctx);
  }
  auto end_import = std::chrono::high_resolution_clock::now();
  auto import_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_import - start_import).count();
  std::cout << "[VisionIpcClient::connect] Import buffers: " << import_duration << " us" << std::endl;

  auto start_finalize = std::chrono::high_resolution_clock::now();
  close(socket_fd);
  connected = true;
  auto end_finalize = std::chrono::high_resolution_clock::now();
  auto finalize_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_finalize - start_finalize).count();
  std::cout << "[VisionIpcClient::connect] Finalize (close socket, set connected): " << finalize_duration << " us" << std::endl;

  auto end_total = std::chrono::high_resolution_clock::now();
  auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_total - start_total).count();
  std::cout << "[VisionIpcClient::connect] Total time: " << total_duration << " us" << std::endl;

  return true;
}

VisionBuf * VisionIpcClient::recv(VisionIpcBufExtra * extra, const int timeout_ms){
  auto p = poller->poll(timeout_ms);

  if (!p.size()){
    return nullptr;
  }

  Message * r = sock->receive(true);
  if (r == nullptr){
    return nullptr;
  }

  // Get buffer
  assert(r->getSize() == sizeof(VisionIpcPacket));
  VisionIpcPacket *packet = (VisionIpcPacket*)r->getData();

  // Check if packet index is out of bounds, indicating server has changed
  if (packet->idx >= num_buffers) {
    connected = false;
    delete r;
    return nullptr;
  }

  VisionBuf * buf = &buffers[packet->idx];

  if (buf->server_id != packet->server_id){
    connected = false;
    delete r;
    return nullptr;
  }

  if (extra) {
    *extra = packet->extra;
  }

  if (buf->sync(VISIONBUF_SYNC_TO_DEVICE) != 0) {
    LOGE("Failed to sync buffer");
  }

  delete r;
  return buf;
}

std::set<VisionStreamType> VisionIpcClient::getAvailableStreams(const std::string &name, bool blocking) {
  int socket_fd = connect_to_vipc_server(name, blocking);
  if (socket_fd < 0) {
    return {};
  }
  // Send VISION_STREAM_MAX to server to request available streams
  int request = VISION_STREAM_MAX;
  int r = ipc_sendrecv_with_fds(true, socket_fd, &request, sizeof(request), nullptr, 0, nullptr);
  assert(r == sizeof(request));

  VisionStreamType available_streams[VISION_STREAM_MAX] = {};
  r = ipc_sendrecv_with_fds(false, socket_fd, &available_streams, sizeof(available_streams), nullptr, 0, nullptr);
  if (r < 0) {
    // only expected error is server shutting down
    assert(errno == ECONNRESET);
    close(socket_fd);
    return {};
  }

  assert(r % sizeof(VisionStreamType) == 0);
  close(socket_fd);
  return std::set<VisionStreamType>(available_streams, available_streams + r / sizeof(VisionStreamType));
}

VisionIpcClient::~VisionIpcClient(){
  for (size_t i = 0; i < num_buffers; i++){
    if (buffers[i].free() != 0) {
      LOGE("Failed to free buffer %zu", i);
    }
  }

  delete sock;
  delete poller;
  delete msg_ctx;
}
