#include "msgq/ffi.h"

#include <cerrno>
#include <cstring>
#include <exception>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <vector>

#include "msgq/event.h"
#include "msgq/ipc.h"
#include "msgq/visionipc/visionbuf.h"
#include "msgq/visionipc/visionipc.h"
#include "msgq/visionipc/visionipc_client.h"
#include "msgq/visionipc/visionipc_server.h"

namespace {

thread_local std::string ffi_error;
thread_local std::string ffi_string_result;

void clear_error() {
  ffi_error.clear();
}

void set_error(const char *message, int error_number = EIO) {
  ffi_error = message;
  errno = error_number;
}

void set_current_exception() {
  try {
    throw;
  } catch (const std::bad_alloc &error) {
    set_error(error.what(), ENOMEM);
  } catch (const std::exception &error) {
    set_error(error.what());
  } catch (...) {
    set_error("unknown native exception");
  }
}

bool valid_stream_type(int stream_type) {
  return stream_type >= VISION_STREAM_ROAD && stream_type < VISION_STREAM_MAX;
}

struct PollerHandle {
  Poller *poller = nullptr;
  std::vector<SubSocket *> ready;
};

struct VisionServerHandle {
  VisionIpcServer *server = nullptr;
  std::map<VisionStreamType, size_t> buffer_sizes;
  bool listener_started = false;
};

}  // namespace

extern "C" {

const char *msgq_last_error(void) {
  return ffi_error.c_str();
}

void *msgq_context_create(void) {
  clear_error();
  try {
    return Context::create();
  } catch (...) {
    set_current_exception();
    return nullptr;
  }
}

void msgq_context_destroy(void *context) {
  delete static_cast<Context *>(context);
}

void *msgq_subsocket_create(void) {
  clear_error();
  try {
    return SubSocket::create();
  } catch (...) {
    set_current_exception();
    return nullptr;
  }
}

void msgq_subsocket_destroy(void *socket) {
  delete static_cast<SubSocket *>(socket);
}

int msgq_subsocket_connect(void *socket, void *context, const char *endpoint, const char *address,
                           int conflate, size_t segment_size) {
  clear_error();
  if (socket == nullptr || context == nullptr || endpoint == nullptr || address == nullptr) {
    set_error("invalid subsocket connection argument", EINVAL);
    return -1;
  }
  try {
    return static_cast<SubSocket *>(socket)->connect(static_cast<Context *>(context), endpoint, address,
                                                     conflate != 0, true, segment_size);
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

void msgq_subsocket_set_timeout(void *socket, int timeout) {
  static_cast<SubSocket *>(socket)->setTimeout(timeout);
}

int msgq_subsocket_receive(void *socket, int non_blocking, void **message) {
  clear_error();
  if (socket == nullptr || message == nullptr) {
    set_error("invalid subsocket receive argument", EINVAL);
    return -1;
  }
  try {
    *message = static_cast<SubSocket *>(socket)->receive(non_blocking != 0);
    return *message == nullptr ? 0 : 1;
  } catch (...) {
    *message = nullptr;
    set_current_exception();
    return -1;
  }
}

const void *msgq_message_data(void *message) {
  return static_cast<Message *>(message)->getData();
}

size_t msgq_message_size(void *message) {
  return static_cast<Message *>(message)->getSize();
}

void msgq_message_destroy(void *message) {
  delete static_cast<Message *>(message);
}

void *msgq_pubsocket_create(void) {
  clear_error();
  try {
    return PubSocket::create();
  } catch (...) {
    set_current_exception();
    return nullptr;
  }
}

void msgq_pubsocket_destroy(void *socket) {
  delete static_cast<PubSocket *>(socket);
}

int msgq_pubsocket_connect(void *socket, void *context, const char *endpoint, size_t segment_size) {
  clear_error();
  if (socket == nullptr || context == nullptr || endpoint == nullptr) {
    set_error("invalid pubsocket connection argument", EINVAL);
    return -1;
  }
  try {
    return static_cast<PubSocket *>(socket)->connect(static_cast<Context *>(context), endpoint, true, segment_size);
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int msgq_pubsocket_send(void *socket, const char *data, size_t size) {
  clear_error();
  if (socket == nullptr || (data == nullptr && size != 0)) {
    set_error("invalid pubsocket send argument", EINVAL);
    return -1;
  }
  try {
    return static_cast<PubSocket *>(socket)->send(const_cast<char *>(data), size);
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int msgq_pubsocket_all_readers_updated(void *socket) {
  return static_cast<PubSocket *>(socket)->all_readers_updated() ? 1 : 0;
}

void *msgq_poller_create(void) {
  clear_error();
  try {
    std::unique_ptr<PollerHandle> handle(new PollerHandle);
    handle->poller = Poller::create();
    return handle.release();
  } catch (...) {
    set_current_exception();
    return nullptr;
  }
}

void msgq_poller_destroy(void *poller) {
  PollerHandle *handle = static_cast<PollerHandle *>(poller);
  if (handle != nullptr) {
    delete handle->poller;
    delete handle;
  }
}

int msgq_poller_register_socket(void *poller, void *socket) {
  clear_error();
  if (poller == nullptr || socket == nullptr) {
    set_error("invalid poller registration argument", EINVAL);
    return -1;
  }
  try {
    static_cast<PollerHandle *>(poller)->poller->registerSocket(static_cast<SubSocket *>(socket));
    return 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int msgq_poller_poll(void *poller, int timeout) {
  clear_error();
  if (poller == nullptr) {
    set_error("invalid poller", EINVAL);
    return -1;
  }
  try {
    PollerHandle *handle = static_cast<PollerHandle *>(poller);
    handle->ready = handle->poller->poll(timeout);
    return static_cast<int>(handle->ready.size());
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

void *msgq_poller_result(void *poller, size_t index) {
  PollerHandle *handle = static_cast<PollerHandle *>(poller);
  if (handle == nullptr || index >= handle->ready.size()) return nullptr;
  return handle->ready[index];
}

int msgq_toggle_fake_events(int enabled) {
  clear_error();
  try {
    SocketEventHandle::toggle_fake_events(enabled != 0);
    return 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int msgq_set_fake_prefix(const char *prefix) {
  clear_error();
  if (prefix == nullptr) {
    set_error("invalid fake prefix", EINVAL);
    return -1;
  }
  try {
    SocketEventHandle::set_fake_prefix(prefix);
    return 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

const char *msgq_get_fake_prefix(void) {
  clear_error();
  try {
    ffi_string_result = SocketEventHandle::fake_prefix();
    return ffi_string_result.c_str();
  } catch (...) {
    set_current_exception();
    return nullptr;
  }
}

void *msgq_socket_event_handle_create(const char *endpoint, const char *identifier, int override_events) {
  clear_error();
  if (endpoint == nullptr || identifier == nullptr) {
    set_error("invalid socket event handle argument", EINVAL);
    return nullptr;
  }
  try {
    return new SocketEventHandle(endpoint, identifier, override_events != 0);
  } catch (...) {
    set_current_exception();
    return nullptr;
  }
}

void msgq_socket_event_handle_destroy(void *handle) {
  delete static_cast<SocketEventHandle *>(handle);
}

int msgq_socket_event_handle_is_enabled(void *handle) {
  return static_cast<SocketEventHandle *>(handle)->is_enabled() ? 1 : 0;
}

void msgq_socket_event_handle_set_enabled(void *handle, int enabled) {
  static_cast<SocketEventHandle *>(handle)->set_enabled(enabled != 0);
}

int msgq_socket_event_handle_recv_called_fd(void *handle) {
  return static_cast<SocketEventHandle *>(handle)->recv_called().fd();
}

int msgq_socket_event_handle_recv_ready_fd(void *handle) {
  return static_cast<SocketEventHandle *>(handle)->recv_ready().fd();
}

int msgq_event_set(int fd) {
  clear_error();
  try {
    Event(fd).set();
    return 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int msgq_event_clear(int fd) {
  clear_error();
  try {
    return Event(fd).clear();
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int msgq_event_wait(int fd, int timeout) {
  clear_error();
  try {
    Event(fd).wait(timeout);
    return 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int msgq_event_peek(int fd) {
  clear_error();
  try {
    return Event(fd).peek() ? 1 : 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int msgq_event_wait_for_one(const int *fds, size_t count, int timeout) {
  clear_error();
  if (fds == nullptr && count != 0) {
    set_error("invalid event list", EINVAL);
    return -1;
  }
  try {
    std::vector<Event> events;
    events.reserve(count);
    for (size_t i = 0; i < count; ++i) events.emplace_back(fds[i]);
    return Event::wait_for_one(events, timeout);
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

const char *visionipc_get_endpoint_name(const char *name, int stream_type) {
  clear_error();
  if (name == nullptr || !valid_stream_type(stream_type)) {
    set_error("invalid VisionIPC endpoint argument", EINVAL);
    return nullptr;
  }
  try {
    ffi_string_result = get_endpoint_name(name, static_cast<VisionStreamType>(stream_type));
    return ffi_string_result.c_str();
  } catch (...) {
    set_current_exception();
    return nullptr;
  }
}

void *visionipc_server_create(const char *name) {
  clear_error();
  if (name == nullptr) {
    set_error("invalid VisionIPC server name", EINVAL);
    return nullptr;
  }
  try {
    std::unique_ptr<VisionServerHandle> handle(new VisionServerHandle);
    handle->server = new VisionIpcServer(name);
    return handle.release();
  } catch (...) {
    set_current_exception();
    return nullptr;
  }
}

void visionipc_server_destroy(void *server) {
  VisionServerHandle *handle = static_cast<VisionServerHandle *>(server);
  if (handle != nullptr) {
    delete handle->server;
    delete handle;
  }
}

int visionipc_server_create_buffers(void *server, int stream_type, size_t num_buffers,
                                    size_t width, size_t height) {
  clear_error();
  if (server == nullptr || !valid_stream_type(stream_type) || num_buffers == 0 || num_buffers >= VISIONIPC_MAX_FDS) {
    set_error("invalid VisionIPC buffer arguments", EINVAL);
    return -1;
  }
  try {
    VisionServerHandle *handle = static_cast<VisionServerHandle *>(server);
    VisionStreamType type = static_cast<VisionStreamType>(stream_type);
    if (handle->buffer_sizes.count(type) != 0) {
      set_error("VisionIPC buffers already exist for this stream", EALREADY);
      return -1;
    }
    handle->server->create_buffers(type, num_buffers, width, height);
    handle->buffer_sizes[type] = width * height * 3 / 2;
    return 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int visionipc_server_create_buffers_with_sizes(void *server, int stream_type, size_t num_buffers,
                                               size_t width, size_t height, size_t size,
                                               size_t stride, size_t uv_offset) {
  clear_error();
  if (server == nullptr || !valid_stream_type(stream_type) || num_buffers == 0 || num_buffers >= VISIONIPC_MAX_FDS) {
    set_error("invalid VisionIPC buffer arguments", EINVAL);
    return -1;
  }
  try {
    VisionServerHandle *handle = static_cast<VisionServerHandle *>(server);
    VisionStreamType type = static_cast<VisionStreamType>(stream_type);
    if (handle->buffer_sizes.count(type) != 0) {
      set_error("VisionIPC buffers already exist for this stream", EALREADY);
      return -1;
    }
    handle->server->create_buffers_with_sizes(type, num_buffers, width, height, size, stride, uv_offset);
    handle->buffer_sizes[type] = size;
    return 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int visionipc_server_send(void *server, int stream_type, const unsigned char *data, size_t size,
                          uint32_t frame_id, uint64_t timestamp_sof, uint64_t timestamp_eof) {
  clear_error();
  if (server == nullptr || !valid_stream_type(stream_type) || (data == nullptr && size != 0)) {
    set_error("invalid VisionIPC send arguments", EINVAL);
    return -1;
  }
  try {
    VisionServerHandle *handle = static_cast<VisionServerHandle *>(server);
    VisionStreamType type = static_cast<VisionStreamType>(stream_type);
    auto expected = handle->buffer_sizes.find(type);
    if (expected == handle->buffer_sizes.end() || expected->second != size) {
      set_error("VisionIPC data size does not match the configured buffer", EINVAL);
      return -1;
    }

    VisionBuf *buffer = handle->server->get_buffer(type);
    std::memcpy(buffer->addr, data, size);
    buffer->set_frame_id(frame_id);
    VisionIpcBufExtra extra = {frame_id, timestamp_sof, timestamp_eof, false};
    handle->server->send(buffer, &extra, false);
    return 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int visionipc_server_start_listener(void *server) {
  clear_error();
  if (server == nullptr) {
    set_error("invalid VisionIPC server", EINVAL);
    return -1;
  }
  try {
    VisionServerHandle *handle = static_cast<VisionServerHandle *>(server);
    if (handle->listener_started) {
      set_error("VisionIPC listener is already running", EALREADY);
      return -1;
    }
    handle->server->start_listener();
    handle->listener_started = true;
    return 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

void *visionipc_client_create(const char *name, int stream_type, int conflate) {
  clear_error();
  if (name == nullptr || !valid_stream_type(stream_type)) {
    set_error("invalid VisionIPC client arguments", EINVAL);
    return nullptr;
  }
  try {
    return new VisionIpcClient(name, static_cast<VisionStreamType>(stream_type), conflate != 0);
  } catch (...) {
    set_current_exception();
    return nullptr;
  }
}

void visionipc_client_destroy(void *client) {
  delete static_cast<VisionIpcClient *>(client);
}

int visionipc_client_connect(void *client, int blocking) {
  clear_error();
  if (client == nullptr) {
    set_error("invalid VisionIPC client", EINVAL);
    return -1;
  }
  try {
    return static_cast<VisionIpcClient *>(client)->connect(blocking != 0) ? 1 : 0;
  } catch (...) {
    set_current_exception();
    return -1;
  }
}

int visionipc_client_is_connected(void *client) {
  return static_cast<VisionIpcClient *>(client)->is_connected() ? 1 : 0;
}

int visionipc_client_num_buffers(void *client) {
  return static_cast<VisionIpcClient *>(client)->num_buffers;
}

void *visionipc_client_buffer(void *client, size_t index) {
  VisionIpcClient *native_client = static_cast<VisionIpcClient *>(client);
  if (native_client == nullptr || index >= static_cast<size_t>(native_client->num_buffers)) return nullptr;
  return &native_client->buffers[index];
}

void *visionipc_client_recv(void *client, VisionIpcBufExtraC *extra, int timeout_ms) {
  clear_error();
  if (client == nullptr || extra == nullptr) {
    set_error("invalid VisionIPC receive arguments", EINVAL);
    return nullptr;
  }
  try {
    VisionIpcBufExtra native_extra = {};
    VisionBuf *buffer = static_cast<VisionIpcClient *>(client)->recv(&native_extra, timeout_ms);
    if (buffer != nullptr) {
      extra->frame_id = native_extra.frame_id;
      extra->timestamp_sof = native_extra.timestamp_sof;
      extra->timestamp_eof = native_extra.timestamp_eof;
      extra->valid = native_extra.valid ? 1 : 0;
    }
    return buffer;
  } catch (...) {
    set_current_exception();
    return nullptr;
  }
}

uint32_t visionipc_client_available_streams(const char *name, int blocking) {
  clear_error();
  if (name == nullptr) {
    set_error("invalid VisionIPC server name", EINVAL);
    return 0;
  }
  try {
    uint32_t streams = 0;
    for (VisionStreamType stream : VisionIpcClient::getAvailableStreams(name, blocking != 0)) {
      streams |= 1U << static_cast<unsigned int>(stream);
    }
    return streams;
  } catch (...) {
    set_current_exception();
    return 0;
  }
}

const void *visionipc_buffer_data(void *buffer) {
  return static_cast<VisionBuf *>(buffer)->addr;
}

size_t visionipc_buffer_len(void *buffer) {
  return static_cast<VisionBuf *>(buffer)->len;
}

size_t visionipc_buffer_width(void *buffer) {
  return static_cast<VisionBuf *>(buffer)->width;
}

size_t visionipc_buffer_height(void *buffer) {
  return static_cast<VisionBuf *>(buffer)->height;
}

size_t visionipc_buffer_stride(void *buffer) {
  return static_cast<VisionBuf *>(buffer)->stride;
}

size_t visionipc_buffer_uv_offset(void *buffer) {
  return static_cast<VisionBuf *>(buffer)->uv_offset;
}

size_t visionipc_buffer_index(void *buffer) {
  return static_cast<VisionBuf *>(buffer)->idx;
}

int visionipc_buffer_fd(void *buffer) {
  return static_cast<VisionBuf *>(buffer)->fd;
}

uint64_t visionipc_buffer_frame_id(void *buffer) {
  return static_cast<VisionBuf *>(buffer)->get_frame_id();
}

}  // extern "C"
