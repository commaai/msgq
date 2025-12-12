#include <cstring>
#include <vector>
#include <string>
#include <mutex>
#include <poll.h>
#include <time.h>
#include <cerrno>
#include <stdexcept>
#include <unistd.h>
#include <sys/eventfd.h>
#include <zmq.h>

#include "msgq/ipc.h"
#include "msgq/event.h"

static thread_local std::string last_error;

static std::string get_error_str() {
  return std::string(strerror(errno));
}

extern "C" {

const char* msgq_get_last_error() {
  return last_error.c_str();
}

// Context
void* msgq_context_create() {
  try {
    return (void*)Context::create();
  } catch (const std::exception& e) {
    last_error = e.what();
    return NULL;
  }
}

void msgq_context_destroy(void* c) {
  delete (Context*)c;
}

// SubSocket
void* msgq_subsocket_create() {
  try {
    return (void*)SubSocket::create();
  } catch (const std::exception& e) {
    last_error = e.what();
    return NULL;
  }
}

void msgq_subsocket_destroy(void* s) {
  delete (SubSocket*)s;
}

int msgq_subsocket_connect(void* s, void* c, const char* endpoint, const char* address, bool conflate) {
  try {
    int ret = ((SubSocket*)s)->connect((Context*)c, endpoint, address ? address : "127.0.0.1", conflate);
    if (ret != 0) {
      last_error = get_error_str();
      return -1;
    }
    return 0;
  } catch (const std::exception& e) {
    last_error = e.what();
    return -1;
  }
}

void msgq_subsocket_set_timeout(void* s, int timeout) {
  ((SubSocket*)s)->setTimeout(timeout);
}

void* msgq_subsocket_receive(void* s, bool non_blocking) {
  try {
    return (void*)((SubSocket*)s)->receive(non_blocking);
  } catch (const std::exception& e) {
    last_error = e.what();
    return NULL;
  }
}

void msgq_message_destroy(void* m) {
  if (m) delete (Message*)m;
}

size_t msgq_message_get_size(void* m) {
  if (!m) return 0;
  return ((Message*)m)->getSize();
}

char* msgq_message_get_data(void* m) {
  if (!m) return NULL;
  return ((Message*)m)->getData();
}

// PubSocket
void* msgq_pubsocket_create() {
  try {
    return (void*)PubSocket::create();
  } catch (const std::exception& e) {
    last_error = e.what();
    return NULL;
  }
}

void msgq_pubsocket_destroy(void* s) {
  delete (PubSocket*)s;
}

int msgq_pubsocket_connect(void* s, void* c, const char* endpoint) {
  try {
    int ret = ((PubSocket*)s)->connect((Context*)c, endpoint);
    if (ret != 0) {
      last_error = get_error_str();
      return -1;
    }
    return 0;
  } catch (const std::exception& e) {
    last_error = e.what();
    return -1;
  }
}

int msgq_pubsocket_send(void* s, char* data, size_t size) {
  try {
    return ((PubSocket*)s)->send(data, size);
  } catch (const std::exception& e) {
    last_error = e.what();
    return -1;
  }
}

bool msgq_pubsocket_all_readers_updated(void* s) {
  try {
    return ((PubSocket*)s)->all_readers_updated();
  } catch (const std::exception& e) {
    last_error = e.what();
    return false;
  }
}

// Poller
void* msgq_poller_create() {
  try {
    return (void*)Poller::create();
  } catch (const std::exception& e) {
    last_error = e.what();
    return NULL;
  }
}

void msgq_poller_destroy(void* p) {
  delete (Poller*)p;
}

void msgq_poller_register(void* p, void* s) {
  try {
    ((Poller*)p)->registerSocket((SubSocket*)s);
  } catch (const std::exception& e) {
    last_error = e.what();
  }
}

int msgq_poller_poll(void* p, void** sockets, int max_count, int timeout) {
  try {
    std::vector<SubSocket*> res = ((Poller*)p)->poll(timeout);
    int count = 0;
    for (SubSocket* s : res) {
      if (count < max_count) {
        sockets[count++] = (void*)s;
      }
    }
    return count;
  } catch (const std::exception& e) {
    last_error = e.what();
    return -1;
  }
}

// Event Wrapper to manage ownership
struct CapiEvent {
  int fd;
  bool owned;

  CapiEvent(int f, bool o) : fd(f), owned(o) {}
  ~CapiEvent() {
    if (owned && fd != -1) {
      close(fd);
    }
  }
};

void* msgq_event_create() {
  try {
    int fd = eventfd(0, EFD_NONBLOCK);
    return new CapiEvent(fd, true);
  } catch (const std::exception& e) {
    last_error = e.what();
    return NULL;
  }
}

void* msgq_event_create_from_fd(int fd) {
  try {
    return new CapiEvent(fd, false);
  } catch (const std::exception& e) {
    last_error = e.what();
    return NULL;
  }
}

void msgq_event_destroy(void* e) {
  delete (CapiEvent*)e;
}

void msgq_event_set(void* e) {
  try {
    CapiEvent* ce = (CapiEvent*)e;
    Event evt(ce->fd);
    evt.set();
  } catch (const std::exception& e) {
    last_error = e.what();
  }
}

void msgq_event_clear(void* e) {
  try {
    CapiEvent* ce = (CapiEvent*)e;
    Event evt(ce->fd);
    if (evt.is_valid()) {
      evt.clear();
    }
  } catch (const std::exception& e) {
    last_error = e.what();
  }
}

int msgq_event_wait(void* e, int timeout) {
  CapiEvent* ce = (CapiEvent*)e;
  Event evt(ce->fd);

  if (!evt.is_valid()) {
    if (timeout > 0) {
       usleep(timeout * 1000);
    }
    return 0;
  }

  struct pollfd fds;
  fds.fd = evt.fd();
  fds.events = POLLIN;

  int ret;
  do {
    ret = poll(&fds, 1, timeout);
  } while (ret < 0 && errno == EINTR);

  if (ret < 0) {
     last_error = "Event poll failed: " + std::string(strerror(errno));
     return -1;
  } else if (ret == 0) {
     if (timeout == 0) {
       last_error = "Event timed out";
       return -1;
     }
  }

  if (fds.revents & (POLLERR | POLLHUP | POLLNVAL)) {
     last_error = "Event poll error flags set";
     return -1;
  }

  return 0;
}

bool msgq_event_is_valid(void* e) {
  try {
    CapiEvent* ce = (CapiEvent*)e;
    Event evt(ce->fd);
    return evt.is_valid();
  } catch (const std::exception& e) {
    last_error = e.what();
    return false;
  }
}

int msgq_event_get_fd(void* e) {
  try {
    return ((CapiEvent*)e)->fd;
  } catch (const std::exception& e) {
    last_error = e.what();
    return -1;
  }
}

bool msgq_event_peek(void* e) {
  try {
    CapiEvent* ce = (CapiEvent*)e;
    Event evt(ce->fd);
    return evt.peek();
  } catch (const std::exception& e) {
    last_error = e.what();
    return false;
  }
}

int msgq_event_wait_for_one(void** events, int count, int timeout) {
  std::vector<struct pollfd> fds;
  for (int i=0; i<count; ++i) {
    if (events[i]) {
      CapiEvent* ce = (CapiEvent*)events[i];
      fds.push_back({ce->fd, POLLIN, 0});
    } else {
      fds.push_back({-1, 0, 0});
    }
  }

  int ret = poll(fds.data(), fds.size(), timeout);
  if (ret < 0) {
    last_error = "Event poll failed";
    return -1;
  } else if (ret == 0 && timeout != -1) {
    last_error = "Event timed out";
    return -1;
  }

  for (size_t i=0; i<fds.size(); ++i) {
    if (fds[i].revents & POLLIN) {
      return i;
    }
  }
  return -1;
}


// SocketEventHandle
void* msgq_socket_event_handle_create(const char* endpoint, const char* identifier, bool override) {
  try {
    return new SocketEventHandle(endpoint, identifier, override);
  } catch (const std::exception& e) {
    last_error = e.what();
    return NULL;
  }
}

void msgq_socket_event_handle_destroy(void* h) {
  delete (SocketEventHandle*)h;
}

bool msgq_socket_event_handle_is_enabled(void* h) {
  return ((SocketEventHandle*)h)->is_enabled();
}

void msgq_socket_event_handle_set_enabled(void* h, bool enabled) {
  ((SocketEventHandle*)h)->set_enabled(enabled);
}

void* msgq_socket_event_handle_recv_called(void* h) {
  try {
    Event e = ((SocketEventHandle*)h)->recv_called();
    // recv_called returns Event wrapping an existing FD. It is not owned by the Event.
    // We create a CapiEvent wrapping it. Do we own it?
    // SocketEventHandle owns the FDs (via EventState in shm).
    // So we should NOT close it.
    return new CapiEvent(e.fd(), false);
  } catch (const std::exception& e) {
    last_error = e.what();
    return NULL;
  }
}

void* msgq_socket_event_handle_recv_ready(void* h) {
  try {
    Event e = ((SocketEventHandle*)h)->recv_ready();
    return new CapiEvent(e.fd(), false);
  } catch (const std::exception& e) {
    last_error = e.what();
    return NULL;
  }
}

// Static SocketEventHandle
void msgq_toggle_fake_events(bool enabled) {
  SocketEventHandle::toggle_fake_events(enabled);
}

void msgq_set_fake_prefix(const char* prefix) {
  SocketEventHandle::set_fake_prefix(prefix);
}

const char* msgq_get_fake_prefix() {
  static thread_local std::string ret;
  ret = SocketEventHandle::fake_prefix();
  return ret.c_str();
}

}
