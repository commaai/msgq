#include "msgq/ffi.h"

#include <memory>
#include <string>
#include <vector>

#include "msgq/event.h"
#include "msgq/ffi_internal.h"
#include "msgq/ipc.h"

using msgq_ffi::translate_exceptions;

namespace msgq_ffi {
thread_local char last_error[512] = {};
thread_local std::string string_result;
}  // namespace msgq_ffi

struct MsgqPoller {
  MsgqPoller() : poller(Poller::create()) {}

  std::unique_ptr<Poller> poller;
  std::vector<SubSocket *> ready;
};

extern "C" {

const char *msgq_last_error(void) {
  return msgq_ffi::last_error;
}

MsgqContext *msgq_context_create(void) {
  return translate_exceptions<MsgqContext *>(nullptr, []() { return Context::create(); });
}

void msgq_context_destroy(MsgqContext *context) {
  translate_exceptions([&]() { delete context; });
}

MsgqSubSocket *msgq_subsocket_create(void) {
  return translate_exceptions<MsgqSubSocket *>(nullptr, []() { return SubSocket::create(); });
}

void msgq_subsocket_destroy(MsgqSubSocket *socket) {
  translate_exceptions([&]() { delete socket; });
}

int msgq_subsocket_connect(MsgqSubSocket *socket, MsgqContext *context, const char *endpoint,
                           const char *address, int conflate, size_t segment_size) {
  return translate_exceptions(-1, [&]() {
    return socket->connect(context, endpoint, address, conflate != 0, true, segment_size);
  });
}

void msgq_subsocket_set_timeout(MsgqSubSocket *socket, int timeout) {
  translate_exceptions([&]() { socket->setTimeout(timeout); });
}

MsgqMessage *msgq_subsocket_receive(MsgqSubSocket *socket, int non_blocking) {
  return translate_exceptions<MsgqMessage *>(nullptr, [&]() {
    return socket->receive(non_blocking != 0);
  });
}

const void *msgq_message_data(MsgqMessage *message) {
  return translate_exceptions<const void *>(nullptr, [&]() { return message->getData(); });
}

size_t msgq_message_size(MsgqMessage *message) {
  return translate_exceptions<size_t>(0, [&]() { return message->getSize(); });
}

void msgq_message_destroy(MsgqMessage *message) {
  translate_exceptions([&]() { delete message; });
}

MsgqPubSocket *msgq_pubsocket_create(void) {
  return translate_exceptions<MsgqPubSocket *>(nullptr, []() { return PubSocket::create(); });
}

void msgq_pubsocket_destroy(MsgqPubSocket *socket) {
  translate_exceptions([&]() { delete socket; });
}

int msgq_pubsocket_connect(MsgqPubSocket *socket, MsgqContext *context, const char *endpoint,
                           size_t segment_size) {
  return translate_exceptions(-1, [&]() {
    return socket->connect(context, endpoint, true, segment_size);
  });
}

int msgq_pubsocket_send(MsgqPubSocket *socket, const char *data, size_t size) {
  return translate_exceptions(-1, [&]() {
    return socket->send(const_cast<char *>(data), size);
  });
}

int msgq_pubsocket_all_readers_updated(MsgqPubSocket *socket) {
  return translate_exceptions(false, [&]() { return socket->all_readers_updated(); });
}

MsgqPoller *msgq_poller_create(void) {
  return translate_exceptions<MsgqPoller *>(nullptr, []() { return new MsgqPoller; });
}

void msgq_poller_destroy(MsgqPoller *poller) {
  translate_exceptions([&]() { delete poller; });
}

void msgq_poller_register_socket(MsgqPoller *poller, MsgqSubSocket *socket) {
  translate_exceptions([&]() { poller->poller->registerSocket(socket); });
}

size_t msgq_poller_poll(MsgqPoller *poller, int timeout) {
  return translate_exceptions<size_t>(0, [&]() {
    poller->ready = poller->poller->poll(timeout);
    return poller->ready.size();
  });
}

MsgqSubSocket *msgq_poller_result(MsgqPoller *poller, size_t index) {
  return translate_exceptions<MsgqSubSocket *>(nullptr, [&]() { return poller->ready.at(index); });
}

void msgq_toggle_fake_events(int enabled) {
  translate_exceptions([&]() { SocketEventHandle::toggle_fake_events(enabled != 0); });
}

void msgq_set_fake_prefix(const char *prefix) {
  translate_exceptions([&]() { SocketEventHandle::set_fake_prefix(prefix); });
}

const char *msgq_get_fake_prefix(void) {
  return translate_exceptions<const char *>(nullptr, []() {
    msgq_ffi::string_result = SocketEventHandle::fake_prefix();
    return msgq_ffi::string_result.c_str();
  });
}

MsgqSocketEventHandle *msgq_socket_event_handle_create(const char *endpoint,
                                                       const char *identifier,
                                                       int override_events) {
  return translate_exceptions<MsgqSocketEventHandle *>(nullptr, [&]() {
    return new SocketEventHandle(endpoint, identifier, override_events != 0);
  });
}

void msgq_socket_event_handle_destroy(MsgqSocketEventHandle *handle) {
  translate_exceptions([&]() { delete handle; });
}

int msgq_socket_event_handle_is_enabled(MsgqSocketEventHandle *handle) {
  return translate_exceptions(false, [&]() { return handle->is_enabled(); });
}

void msgq_socket_event_handle_set_enabled(MsgqSocketEventHandle *handle, int enabled) {
  translate_exceptions([&]() { handle->set_enabled(enabled != 0); });
}

int msgq_socket_event_handle_recv_called_fd(MsgqSocketEventHandle *handle) {
  return translate_exceptions(-1, [&]() { return handle->recv_called().fd(); });
}

int msgq_socket_event_handle_recv_ready_fd(MsgqSocketEventHandle *handle) {
  return translate_exceptions(-1, [&]() { return handle->recv_ready().fd(); });
}

void msgq_event_set(int fd) {
  translate_exceptions([&]() { Event(fd).set(); });
}

int msgq_event_clear(int fd) {
  return translate_exceptions(-1, [&]() { return Event(fd).clear(); });
}

void msgq_event_wait(int fd, int timeout) {
  translate_exceptions([&]() { Event(fd).wait(timeout); });
}

int msgq_event_peek(int fd) {
  return translate_exceptions(false, [&]() { return Event(fd).peek(); });
}

int msgq_event_wait_for_one(const int *fds, size_t count, int timeout) {
  return translate_exceptions(-1, [&]() {
    std::vector<Event> events;
    events.reserve(count);
    for (size_t i = 0; i < count; ++i) events.emplace_back(fds[i]);
    return Event::wait_for_one(events, timeout);
  });
}

}  // extern "C"
