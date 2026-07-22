#include "msgq/ffi.h"

#include <cstring>
#include <stdexcept>
#include <string>

#include "msgq/ffi_internal.h"
#include "msgq/visionipc/visionbuf.h"
#include "msgq/visionipc/visionipc.h"
#include "msgq/visionipc/visionipc_client.h"
#include "msgq/visionipc/visionipc_server.h"

using msgq_ffi::translate_exceptions;

namespace {
thread_local std::string string_result;

VisionStreamType stream_type(int value) {
  return static_cast<VisionStreamType>(value);
}
}  // namespace

extern "C" {

const char *visionipc_get_endpoint_name(const char *name, int stream) {
  return translate_exceptions<const char *>(nullptr, [&]() {
    string_result = get_endpoint_name(name, stream_type(stream));
    return string_result.c_str();
  });
}

VisionIpcServerHandle *visionipc_server_create(const char *name) {
  return translate_exceptions<VisionIpcServerHandle *>(nullptr, [&]() {
    return new VisionIpcServer(name);
  });
}

void visionipc_server_destroy(VisionIpcServerHandle *server) {
  translate_exceptions([&]() { delete server; });
}

void visionipc_server_create_buffers(VisionIpcServerHandle *server, int stream,
                                     size_t num_buffers, size_t width, size_t height) {
  translate_exceptions([&]() {
    server->create_buffers(stream_type(stream), num_buffers, width, height);
  });
}

void visionipc_server_create_buffers_with_sizes(VisionIpcServerHandle *server, int stream,
                                                size_t num_buffers, size_t width, size_t height,
                                                size_t size, size_t stride, size_t uv_offset) {
  translate_exceptions([&]() {
    server->create_buffers_with_sizes(stream_type(stream), num_buffers, width, height,
                                      size, stride, uv_offset);
  });
}

void visionipc_server_send(VisionIpcServerHandle *server, int stream,
                           const unsigned char *data, size_t size, uint32_t frame_id,
                           uint64_t timestamp_sof, uint64_t timestamp_eof) {
  translate_exceptions([&]() {
    VisionBuf *buffer = server->get_buffer(stream_type(stream));
    if (buffer->len != size) throw std::invalid_argument("VisionIPC buffer size mismatch");

    std::memcpy(buffer->addr, data, size);
    buffer->set_frame_id(frame_id);
    VisionIpcBufExtra extra = {frame_id, timestamp_sof, timestamp_eof, false};
    server->send(buffer, &extra, false);
  });
}

void visionipc_server_start_listener(VisionIpcServerHandle *server) {
  translate_exceptions([&]() { server->start_listener(); });
}

VisionIpcClientHandle *visionipc_client_create(const char *name, int stream, int conflate) {
  return translate_exceptions<VisionIpcClientHandle *>(nullptr, [&]() {
    return new VisionIpcClient(name, stream_type(stream), conflate != 0);
  });
}

void visionipc_client_destroy(VisionIpcClientHandle *client) {
  translate_exceptions([&]() { delete client; });
}

int visionipc_client_connect(VisionIpcClientHandle *client, int blocking) {
  return translate_exceptions(false, [&]() { return client->connect(blocking != 0); });
}

int visionipc_client_is_connected(VisionIpcClientHandle *client) {
  return translate_exceptions(false, [&]() { return client->is_connected(); });
}

int visionipc_client_num_buffers(VisionIpcClientHandle *client) {
  return translate_exceptions(-1, [&]() { return client->num_buffers; });
}

VisionIpcBuffer *visionipc_client_buffer(VisionIpcClientHandle *client, size_t index) {
  return translate_exceptions<VisionIpcBuffer *>(nullptr, [&]() {
    return index < static_cast<size_t>(client->num_buffers) ? &client->buffers[index] : nullptr;
  });
}

VisionIpcBuffer *visionipc_client_recv(VisionIpcClientHandle *client, VisionIpcBufExtraC *extra,
                                      int timeout_ms) {
  return translate_exceptions<VisionIpcBuffer *>(nullptr, [&]() {
    VisionIpcBufExtra native_extra = {};
    VisionBuf *buffer = client->recv(&native_extra, timeout_ms);
    if (buffer != nullptr) {
      *extra = {native_extra.frame_id, native_extra.timestamp_sof, native_extra.timestamp_eof,
                native_extra.valid};
    }
    return buffer;
  });
}

uint32_t visionipc_client_available_streams(const char *name, int blocking) {
  return translate_exceptions<uint32_t>(0, [&]() {
    uint32_t result = 0;
    for (VisionStreamType stream : VisionIpcClient::getAvailableStreams(name, blocking != 0)) {
      result |= 1U << static_cast<unsigned int>(stream);
    }
    return result;
  });
}

const void *visionipc_buffer_data(VisionIpcBuffer *buffer) {
  return translate_exceptions<const void *>(nullptr, [&]() { return buffer->addr; });
}

size_t visionipc_buffer_len(VisionIpcBuffer *buffer) {
  return translate_exceptions<size_t>(0, [&]() { return buffer->len; });
}

size_t visionipc_buffer_width(VisionIpcBuffer *buffer) {
  return translate_exceptions<size_t>(0, [&]() { return buffer->width; });
}

size_t visionipc_buffer_height(VisionIpcBuffer *buffer) {
  return translate_exceptions<size_t>(0, [&]() { return buffer->height; });
}

size_t visionipc_buffer_stride(VisionIpcBuffer *buffer) {
  return translate_exceptions<size_t>(0, [&]() { return buffer->stride; });
}

size_t visionipc_buffer_uv_offset(VisionIpcBuffer *buffer) {
  return translate_exceptions<size_t>(0, [&]() { return buffer->uv_offset; });
}

size_t visionipc_buffer_index(VisionIpcBuffer *buffer) {
  return translate_exceptions<size_t>(0, [&]() { return buffer->idx; });
}

int visionipc_buffer_fd(VisionIpcBuffer *buffer) {
  return translate_exceptions(-1, [&]() { return buffer->fd; });
}

uint64_t visionipc_buffer_frame_id(VisionIpcBuffer *buffer) {
  return translate_exceptions<uint64_t>(0, [&]() { return buffer->get_frame_id(); });
}

}  // extern "C"
