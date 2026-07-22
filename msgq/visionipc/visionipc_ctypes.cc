#include "msgq/visionipc/visionipc_ctypes.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "msgq/visionipc/visionipc_client.h"
#include "msgq/visionipc/visionipc_server.h"

struct CffiVisionClient {
  VisionIpcClient client;
  VisionIpcBufExtra extra = {};
  CffiVisionClient(const char *name, VisionStreamType stream, bool conflate) : client(name, stream, conflate) {}
};

static VisionStreamType stream_type(int stream) { return static_cast<VisionStreamType>(stream); }

extern "C" size_t vipc_get_endpoint_name(const char *name, int stream, char *output, size_t capacity) {
  const std::string endpoint = get_endpoint_name(name, stream_type(stream));
  if (capacity) memcpy(output, endpoint.data(), std::min(capacity, endpoint.size()));
  return endpoint.size();
}
extern "C" void *vipc_server_create(const char *name) { return new VisionIpcServer(name); }
extern "C" void vipc_server_delete(void *server) { delete static_cast<VisionIpcServer *>(server); }
extern "C" void vipc_server_create_buffers(void *server, int stream, size_t count, size_t width, size_t height) {
  static_cast<VisionIpcServer *>(server)->create_buffers(stream_type(stream), count, width, height);
}
extern "C" void vipc_server_create_buffers_with_sizes(void *server, int stream, size_t count, size_t width, size_t height, size_t size, size_t stride, size_t uv_offset) {
  static_cast<VisionIpcServer *>(server)->create_buffers_with_sizes(stream_type(stream), count, width, height, size, stride, uv_offset);
}
extern "C" int vipc_server_send(void *server, int stream, const unsigned char *data, size_t size, uint32_t frame_id, uint64_t timestamp_sof, uint64_t timestamp_eof) {
  VisionIpcServer *s = static_cast<VisionIpcServer *>(server);
  VisionBuf *buffer = s->get_buffer(stream_type(stream));
  if (buffer->len != size) return -1;
  memcpy(buffer->addr, data, size);
  buffer->set_frame_id(frame_id);
  VisionIpcBufExtra extra = {frame_id, timestamp_sof, timestamp_eof, false};
  s->send(buffer, &extra, false);
  return 0;
}
extern "C" void vipc_server_start_listener(void *server) { static_cast<VisionIpcServer *>(server)->start_listener(); }

extern "C" void *vipc_client_create(const char *name, int stream, int conflate) { return new CffiVisionClient(name, stream_type(stream), conflate); }
extern "C" void vipc_client_delete(void *client) { delete static_cast<CffiVisionClient *>(client); }
extern "C" int vipc_client_connect(void *client, int blocking) { return static_cast<CffiVisionClient *>(client)->client.connect(blocking); }
extern "C" int vipc_client_is_connected(void *client) { return static_cast<CffiVisionClient *>(client)->client.is_connected(); }
extern "C" void *vipc_client_recv(void *client, int timeout_ms) {
  CffiVisionClient *c = static_cast<CffiVisionClient *>(client);
  return c->client.recv(&c->extra, timeout_ms);
}
extern "C" uint64_t vipc_client_available_streams(const char *name, int blocking) {
  uint64_t streams = 0;
  for (const auto stream : VisionIpcClient::getAvailableStreams(name, blocking)) streams |= UINT64_C(1) << stream;
  return streams;
}
extern "C" size_t vipc_client_num_buffers(void *client) { return static_cast<CffiVisionClient *>(client)->client.num_buffers; }
extern "C" void *vipc_client_buffer(void *client, size_t index) { return &static_cast<CffiVisionClient *>(client)->client.buffers[index]; }
extern "C" uint32_t vipc_client_frame_id(void *client) { return static_cast<CffiVisionClient *>(client)->extra.frame_id; }
extern "C" uint64_t vipc_client_timestamp_sof(void *client) { return static_cast<CffiVisionClient *>(client)->extra.timestamp_sof; }
extern "C" uint64_t vipc_client_timestamp_eof(void *client) { return static_cast<CffiVisionClient *>(client)->extra.timestamp_eof; }
extern "C" int vipc_client_valid(void *client) { return static_cast<CffiVisionClient *>(client)->extra.valid; }

extern "C" void *vipc_buffer_data(void *buffer) { return static_cast<VisionBuf *>(buffer)->addr; }
extern "C" size_t vipc_buffer_len(void *buffer) { return static_cast<VisionBuf *>(buffer)->len; }
extern "C" size_t vipc_buffer_width(void *buffer) { return static_cast<VisionBuf *>(buffer)->width; }
extern "C" size_t vipc_buffer_height(void *buffer) { return static_cast<VisionBuf *>(buffer)->height; }
extern "C" size_t vipc_buffer_stride(void *buffer) { return static_cast<VisionBuf *>(buffer)->stride; }
extern "C" size_t vipc_buffer_uv_offset(void *buffer) { return static_cast<VisionBuf *>(buffer)->uv_offset; }
extern "C" size_t vipc_buffer_idx(void *buffer) { return static_cast<VisionBuf *>(buffer)->idx; }
extern "C" int vipc_buffer_fd(void *buffer) { return static_cast<VisionBuf *>(buffer)->fd; }
extern "C" uint64_t vipc_buffer_frame_id(void *buffer) { return static_cast<VisionBuf *>(buffer)->get_frame_id(); }
