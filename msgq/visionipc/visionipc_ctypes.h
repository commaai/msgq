#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t vipc_get_endpoint_name(const char *name, int stream, char *output, size_t capacity);
void *vipc_server_create(const char *name);
void vipc_server_delete(void *server);
void vipc_server_create_buffers(void *server, int stream, size_t count, size_t width, size_t height);
void vipc_server_create_buffers_with_sizes(void *server, int stream, size_t count, size_t width, size_t height, size_t size, size_t stride, size_t uv_offset);
int vipc_server_send(void *server, int stream, const unsigned char *data, size_t size, uint32_t frame_id, uint64_t timestamp_sof, uint64_t timestamp_eof);
void vipc_server_start_listener(void *server);

void *vipc_client_create(const char *name, int stream, int conflate);
void vipc_client_delete(void *client);
int vipc_client_connect(void *client, int blocking);
int vipc_client_is_connected(void *client);
void *vipc_client_recv(void *client, int timeout_ms);
uint64_t vipc_client_available_streams(const char *name, int blocking);
size_t vipc_client_num_buffers(void *client);
void *vipc_client_buffer(void *client, size_t index);
uint32_t vipc_client_frame_id(void *client);
uint64_t vipc_client_timestamp_sof(void *client);
uint64_t vipc_client_timestamp_eof(void *client);
int vipc_client_valid(void *client);

void *vipc_buffer_data(void *buffer);
size_t vipc_buffer_len(void *buffer);
size_t vipc_buffer_width(void *buffer);
size_t vipc_buffer_height(void *buffer);
size_t vipc_buffer_stride(void *buffer);
size_t vipc_buffer_uv_offset(void *buffer);
size_t vipc_buffer_idx(void *buffer);
int vipc_buffer_fd(void *buffer);
uint64_t vipc_buffer_frame_id(void *buffer);

#ifdef __cplusplus
}
#endif
