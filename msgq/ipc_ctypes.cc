#include "msgq/ipc_ctypes.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "msgq/event.h"
#include "msgq/ipc.h"

extern "C" void *msgq_context_create() { return Context::create(); }
extern "C" void msgq_context_delete(void *p) { delete static_cast<Context *>(p); }
extern "C" void *msgq_sub_create() { return SubSocket::create(); }
extern "C" void msgq_sub_delete(void *p) { delete static_cast<SubSocket *>(p); }
extern "C" int msgq_sub_connect(void *s, void *c, const char *e, const char *a, int cf, size_t z) {
  return static_cast<SubSocket *>(s)->connect(static_cast<Context *>(c), e, a, cf, true, z);
}
extern "C" void msgq_sub_set_timeout(void *s, int t) { static_cast<SubSocket *>(s)->setTimeout(t); }
extern "C" void *msgq_sub_receive(void *s, int nb) { return static_cast<SubSocket *>(s)->receive(nb); }
extern "C" size_t msgq_message_size(void *m) { return static_cast<Message *>(m)->getSize(); }
extern "C" const char *msgq_message_data(void *m) { return static_cast<Message *>(m)->getData(); }
extern "C" void msgq_message_delete(void *m) { delete static_cast<Message *>(m); }
extern "C" void *msgq_pub_create() { return PubSocket::create(); }
extern "C" void msgq_pub_delete(void *p) { delete static_cast<PubSocket *>(p); }
extern "C" int msgq_pub_connect(void *s, void *c, const char *e, size_t z) {
  return static_cast<PubSocket *>(s)->connect(static_cast<Context *>(c), e, true, z);
}
extern "C" int msgq_pub_send(void *s, const char *d, size_t z) { return static_cast<PubSocket *>(s)->send(const_cast<char *>(d), z); }
extern "C" int msgq_pub_all_readers_updated(void *s) { return static_cast<PubSocket *>(s)->all_readers_updated(); }
extern "C" void *msgq_poller_create() { return Poller::create(); }
extern "C" void msgq_poller_delete(void *p) { delete static_cast<Poller *>(p); }
extern "C" void msgq_poller_register(void *p, void *s) { static_cast<Poller *>(p)->registerSocket(static_cast<SubSocket *>(s)); }
extern "C" size_t msgq_poller_poll(void *p, int t, void **out, size_t capacity) {
  const auto sockets = static_cast<Poller *>(p)->poll(t);
  const size_t count = std::min(capacity, sockets.size());
  std::copy_n(sockets.begin(), count, out);
  return count;
}

extern "C" void msgq_toggle_fake_events(int e) { SocketEventHandle::toggle_fake_events(e); }
extern "C" void msgq_set_fake_prefix(const char *p) { SocketEventHandle::set_fake_prefix(p); }
extern "C" size_t msgq_get_fake_prefix(char *out, size_t capacity) {
  const std::string prefix = SocketEventHandle::fake_prefix();
  if (capacity) memcpy(out, prefix.data(), std::min(capacity, prefix.size()));
  return prefix.size();
}
extern "C" void *msgq_event_handle_create(const char *e, const char *i, int o) { return new SocketEventHandle(e, i, o); }
extern "C" void msgq_event_handle_delete(void *h) { delete static_cast<SocketEventHandle *>(h); }
extern "C" int msgq_event_handle_enabled(void *h) { return static_cast<SocketEventHandle *>(h)->is_enabled(); }
extern "C" void msgq_event_handle_set_enabled(void *h, int e) { static_cast<SocketEventHandle *>(h)->set_enabled(e); }
extern "C" void *msgq_event_handle_recv_called(void *h) { return new Event(static_cast<SocketEventHandle *>(h)->recv_called()); }
extern "C" void *msgq_event_handle_recv_ready(void *h) { return new Event(static_cast<SocketEventHandle *>(h)->recv_ready()); }
extern "C" void msgq_event_delete(void *e) { delete static_cast<Event *>(e); }
extern "C" int msgq_event_set(void *e) { try { static_cast<Event *>(e)->set(); return 0; } catch (...) { return -1; } }
extern "C" int msgq_event_clear(void *e) { try { return static_cast<Event *>(e)->clear(); } catch (...) { return -1; } }
extern "C" int msgq_event_wait(void *e, int t) { try { static_cast<Event *>(e)->wait(t); return 0; } catch (...) { return -1; } }
extern "C" int msgq_event_peek(void *e) { try { return static_cast<Event *>(e)->peek(); } catch (...) { return -1; } }
extern "C" int msgq_event_fd(void *e) { return static_cast<Event *>(e)->fd(); }
extern "C" int msgq_event_wait_for_one(void **items, size_t count, int timeout) {
  std::vector<Event> events;
  events.reserve(count);
  for (size_t i = 0; i < count; ++i) events.push_back(*static_cast<Event *>(items[i]));
  try { return Event::wait_for_one(events, timeout); } catch (...) { return -1; }
}
