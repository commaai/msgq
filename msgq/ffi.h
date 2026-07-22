#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
class Context;
class Message;
class Poller;
class PubSocket;
class SocketEventHandle;
class SubSocket;
class VisionBuf;
class VisionIpcClient;
class VisionIpcServer;

using MsgqContext = Context;
using MsgqMessage = Message;
using MsgqPubSocket = PubSocket;
using MsgqSocketEventHandle = SocketEventHandle;
using MsgqSubSocket = SubSocket;
using VisionIpcBuffer = VisionBuf;
using VisionIpcClientHandle = VisionIpcClient;
using VisionIpcServerHandle = VisionIpcServer;
#else
typedef struct MsgqContext MsgqContext;
typedef struct MsgqMessage MsgqMessage;
typedef struct MsgqPubSocket MsgqPubSocket;
typedef struct MsgqSocketEventHandle MsgqSocketEventHandle;
typedef struct MsgqSubSocket MsgqSubSocket;
typedef struct VisionIpcBuffer VisionIpcBuffer;
typedef struct VisionIpcClientHandle VisionIpcClientHandle;
typedef struct VisionIpcServerHandle VisionIpcServerHandle;
#endif

typedef struct MsgqPoller MsgqPoller;

#ifdef __cplusplus
extern "C" {
#endif

const char *msgq_last_error(void);

MsgqContext *msgq_context_create(void);
void msgq_context_destroy(MsgqContext *context);

MsgqSubSocket *msgq_subsocket_create(void);
void msgq_subsocket_destroy(MsgqSubSocket *socket);
int msgq_subsocket_connect(MsgqSubSocket *socket, MsgqContext *context, const char *endpoint,
                           const char *address, int conflate, size_t segment_size);
void msgq_subsocket_set_timeout(MsgqSubSocket *socket, int timeout);
MsgqMessage *msgq_subsocket_receive(MsgqSubSocket *socket, int non_blocking);

const void *msgq_message_data(MsgqMessage *message);
size_t msgq_message_size(MsgqMessage *message);
void msgq_message_destroy(MsgqMessage *message);

MsgqPubSocket *msgq_pubsocket_create(void);
void msgq_pubsocket_destroy(MsgqPubSocket *socket);
int msgq_pubsocket_connect(MsgqPubSocket *socket, MsgqContext *context, const char *endpoint,
                           size_t segment_size);
int msgq_pubsocket_send(MsgqPubSocket *socket, const char *data, size_t size);
int msgq_pubsocket_all_readers_updated(MsgqPubSocket *socket);

MsgqPoller *msgq_poller_create(void);
void msgq_poller_destroy(MsgqPoller *poller);
void msgq_poller_register_socket(MsgqPoller *poller, MsgqSubSocket *socket);
size_t msgq_poller_poll(MsgqPoller *poller, int timeout);
MsgqSubSocket *msgq_poller_result(MsgqPoller *poller, size_t index);

void msgq_toggle_fake_events(int enabled);
void msgq_set_fake_prefix(const char *prefix);
const char *msgq_get_fake_prefix(void);

MsgqSocketEventHandle *msgq_socket_event_handle_create(const char *endpoint,
                                                       const char *identifier,
                                                       int override_events);
void msgq_socket_event_handle_destroy(MsgqSocketEventHandle *handle);
int msgq_socket_event_handle_is_enabled(MsgqSocketEventHandle *handle);
void msgq_socket_event_handle_set_enabled(MsgqSocketEventHandle *handle, int enabled);
int msgq_socket_event_handle_recv_called_fd(MsgqSocketEventHandle *handle);
int msgq_socket_event_handle_recv_ready_fd(MsgqSocketEventHandle *handle);

void msgq_event_set(int fd);
int msgq_event_clear(int fd);
void msgq_event_wait(int fd, int timeout);
int msgq_event_peek(int fd);
int msgq_event_wait_for_one(const int *fds, size_t count, int timeout);

typedef struct VisionIpcBufExtraC {
  uint32_t frame_id;
  uint64_t timestamp_sof;
  uint64_t timestamp_eof;
  int valid;
} VisionIpcBufExtraC;

const char *visionipc_get_endpoint_name(const char *name, int stream_type);

VisionIpcServerHandle *visionipc_server_create(const char *name);
void visionipc_server_destroy(VisionIpcServerHandle *server);
void visionipc_server_create_buffers(VisionIpcServerHandle *server, int stream_type,
                                     size_t num_buffers, size_t width, size_t height);
void visionipc_server_create_buffers_with_sizes(VisionIpcServerHandle *server, int stream_type,
                                                size_t num_buffers, size_t width, size_t height,
                                                size_t size, size_t stride, size_t uv_offset);
void visionipc_server_send(VisionIpcServerHandle *server, int stream_type,
                           const unsigned char *data, size_t size, uint32_t frame_id,
                           uint64_t timestamp_sof, uint64_t timestamp_eof);
void visionipc_server_start_listener(VisionIpcServerHandle *server);

VisionIpcClientHandle *visionipc_client_create(const char *name, int stream_type, int conflate);
void visionipc_client_destroy(VisionIpcClientHandle *client);
int visionipc_client_connect(VisionIpcClientHandle *client, int blocking);
int visionipc_client_is_connected(VisionIpcClientHandle *client);
int visionipc_client_num_buffers(VisionIpcClientHandle *client);
VisionIpcBuffer *visionipc_client_buffer(VisionIpcClientHandle *client, size_t index);
VisionIpcBuffer *visionipc_client_recv(VisionIpcClientHandle *client, VisionIpcBufExtraC *extra,
                                      int timeout_ms);
uint32_t visionipc_client_available_streams(const char *name, int blocking);

const void *visionipc_buffer_data(VisionIpcBuffer *buffer);
size_t visionipc_buffer_len(VisionIpcBuffer *buffer);
size_t visionipc_buffer_width(VisionIpcBuffer *buffer);
size_t visionipc_buffer_height(VisionIpcBuffer *buffer);
size_t visionipc_buffer_stride(VisionIpcBuffer *buffer);
size_t visionipc_buffer_uv_offset(VisionIpcBuffer *buffer);
size_t visionipc_buffer_index(VisionIpcBuffer *buffer);
int visionipc_buffer_fd(VisionIpcBuffer *buffer);
uint64_t visionipc_buffer_frame_id(VisionIpcBuffer *buffer);

#ifdef __cplusplus
}
#endif
