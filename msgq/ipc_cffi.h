#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *msgq_context_create(void);
void msgq_context_delete(void *context);
void *msgq_sub_create(void);
void msgq_sub_delete(void *socket);
int msgq_sub_connect(void *socket, void *context, const char *endpoint, const char *address, int conflate, size_t segment_size);
void msgq_sub_set_timeout(void *socket, int timeout);
void *msgq_sub_receive(void *socket, int non_blocking);
void *msgq_sub_receive_data(void *socket, int non_blocking, const char **data, size_t *size);
size_t msgq_message_size(void *message);
const char *msgq_message_data(void *message);
void msgq_message_delete(void *message);
void *msgq_pub_create(void);
void msgq_pub_delete(void *socket);
int msgq_pub_connect(void *socket, void *context, const char *endpoint, size_t segment_size);
int msgq_pub_send(void *socket, const char *data, size_t size);
int msgq_pub_all_readers_updated(void *socket);
void *msgq_poller_create(void);
void msgq_poller_delete(void *poller);
void msgq_poller_register(void *poller, void *socket);
size_t msgq_poller_poll(void *poller, int timeout, void **sockets, size_t capacity);

void msgq_toggle_fake_events(int enabled);
void msgq_set_fake_prefix(const char *prefix);
size_t msgq_get_fake_prefix(char *prefix, size_t capacity);
void *msgq_event_handle_create(const char *endpoint, const char *identifier, int override);
void msgq_event_handle_delete(void *handle);
int msgq_event_handle_enabled(void *handle);
void msgq_event_handle_set_enabled(void *handle, int enabled);
void *msgq_event_handle_recv_called(void *handle);
void *msgq_event_handle_recv_ready(void *handle);
void msgq_event_delete(void *event);
int msgq_event_set(void *event);
int msgq_event_clear(void *event);
int msgq_event_wait(void *event, int timeout);
int msgq_event_peek(void *event);
int msgq_event_fd(void *event);
int msgq_event_wait_for_one(void **events, size_t count, int timeout);

#ifdef __cplusplus
}
#endif
