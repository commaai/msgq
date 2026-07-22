#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *msgq_last_error(void);

void *msgq_context_create(void);
void msgq_context_destroy(void *context);

void *msgq_subsocket_create(void);
void msgq_subsocket_destroy(void *socket);
int msgq_subsocket_connect(void *socket, void *context, const char *endpoint, const char *address,
                           int conflate, size_t segment_size);
void msgq_subsocket_set_timeout(void *socket, int timeout);
int msgq_subsocket_receive(void *socket, int non_blocking, void **message);

const void *msgq_message_data(void *message);
size_t msgq_message_size(void *message);
void msgq_message_destroy(void *message);

void *msgq_pubsocket_create(void);
void msgq_pubsocket_destroy(void *socket);
int msgq_pubsocket_connect(void *socket, void *context, const char *endpoint, size_t segment_size);
int msgq_pubsocket_send(void *socket, const char *data, size_t size);
int msgq_pubsocket_all_readers_updated(void *socket);

void *msgq_poller_create(void);
void msgq_poller_destroy(void *poller);
int msgq_poller_register_socket(void *poller, void *socket);
int msgq_poller_poll(void *poller, int timeout);
void *msgq_poller_result(void *poller, size_t index);

int msgq_toggle_fake_events(int enabled);
int msgq_set_fake_prefix(const char *prefix);
const char *msgq_get_fake_prefix(void);

void *msgq_socket_event_handle_create(const char *endpoint, const char *identifier, int override_events);
void msgq_socket_event_handle_destroy(void *handle);
int msgq_socket_event_handle_is_enabled(void *handle);
void msgq_socket_event_handle_set_enabled(void *handle, int enabled);
int msgq_socket_event_handle_recv_called_fd(void *handle);
int msgq_socket_event_handle_recv_ready_fd(void *handle);

int msgq_event_set(int fd);
int msgq_event_clear(int fd);
int msgq_event_wait(int fd, int timeout);
int msgq_event_peek(int fd);
int msgq_event_wait_for_one(const int *fds, size_t count, int timeout);

typedef struct VisionIpcBufExtraC {
  uint32_t frame_id;
  uint64_t timestamp_sof;
  uint64_t timestamp_eof;
  int valid;
} VisionIpcBufExtraC;

const char *visionipc_get_endpoint_name(const char *name, int stream_type);

void *visionipc_server_create(const char *name);
void visionipc_server_destroy(void *server);
int visionipc_server_create_buffers(void *server, int stream_type, size_t num_buffers,
                                    size_t width, size_t height);
int visionipc_server_create_buffers_with_sizes(void *server, int stream_type, size_t num_buffers,
                                               size_t width, size_t height, size_t size,
                                               size_t stride, size_t uv_offset);
int visionipc_server_send(void *server, int stream_type, const unsigned char *data, size_t size,
                          uint32_t frame_id, uint64_t timestamp_sof, uint64_t timestamp_eof);
int visionipc_server_start_listener(void *server);

void *visionipc_client_create(const char *name, int stream_type, int conflate);
void visionipc_client_destroy(void *client);
int visionipc_client_connect(void *client, int blocking);
int visionipc_client_is_connected(void *client);
int visionipc_client_num_buffers(void *client);
void *visionipc_client_buffer(void *client, size_t index);
void *visionipc_client_recv(void *client, VisionIpcBufExtraC *extra, int timeout_ms);
uint32_t visionipc_client_available_streams(const char *name, int blocking);

const void *visionipc_buffer_data(void *buffer);
size_t visionipc_buffer_len(void *buffer);
size_t visionipc_buffer_width(void *buffer);
size_t visionipc_buffer_height(void *buffer);
size_t visionipc_buffer_stride(void *buffer);
size_t visionipc_buffer_uv_offset(void *buffer);
size_t visionipc_buffer_index(void *buffer);
int visionipc_buffer_fd(void *buffer);
uint64_t visionipc_buffer_frame_id(void *buffer);

#ifdef __cplusplus
}
#endif
