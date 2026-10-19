// Python bindings for msgq
#include <cerrno>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "msgq/ipc.h"
#include "msgq/event.h"
#include "msgq/nb_helpers.h"

using namespace nb::literals;

struct IpcError : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct MultiplePublishersError : IpcError {
  using IpcError::IpcError;
};

[[noreturn]] static void throw_ipc_error(const std::string *endpoint = nullptr) {
  int err = errno;
  std::string msg = "Messaging failure " + (endpoint ? "with " + *endpoint : "") + ": " + strerror(err);
  if (err == EADDRINUSE) throw MultiplePublishersError(msg);
  throw IpcError(msg);
}

// Keeps the registered Python SubSockets alive, and lets poll() return them
struct PyPoller {
  Poller poller;
  std::vector<nb::object> sockets;
};

NB_MODULE(ipc_pyx, m) {
  auto ipc_error = nb::exception<IpcError>(m, "IpcError");
  nb::exception<MultiplePublishersError>(m, "MultiplePublishersError", ipc_error);

  nb::class_<Event>(m, "Event")
    .def(nb::init<>())
    .def("set", &Event::set)
    .def("clear", &Event::clear)
    .def("wait", &Event::wait, "timeout"_a = -1, nb::call_guard<nb::gil_scoped_release>())
    .def("peek", &Event::peek)
    .def_prop_ro("fd", &Event::fd)
    .def_prop_ro("ptr", [](Event &self) { return (uintptr_t)&self; });

  nb::class_<SocketEventHandle>(m, "SocketEventHandle")
    .def("__init__", [](SocketEventHandle *self, Name endpoint, Name identifier, bool override) {
      new (self) SocketEventHandle(endpoint.value, identifier.value, override);
    }, "endpoint"_a, "identifier"_a, "override"_a)
    .def_prop_rw("enabled", &SocketEventHandle::is_enabled, &SocketEventHandle::set_enabled)
    .def_prop_ro("recv_called_event", &SocketEventHandle::recv_called)
    .def_prop_ro("recv_ready_event", &SocketEventHandle::recv_ready);

  m.def("toggle_fake_events", &SocketEventHandle::toggle_fake_events, "enabled"_a);
  m.def("set_fake_prefix", [](Name prefix) { SocketEventHandle::set_fake_prefix(prefix.value); }, "prefix"_a);
  m.def("get_fake_prefix", [] {
    std::string prefix = SocketEventHandle::fake_prefix();
    return nb::bytes(prefix.data(), prefix.size());
  });
  m.def("delete_fake_prefix", [] { SocketEventHandle::set_fake_prefix(""); });
  m.def("wait_for_one_event", &Event::wait_for_one, "events"_a, "timeout"_a = -1, nb::call_guard<nb::gil_scoped_release>());

  nb::class_<SubSocket>(m, "SubSocket")
    .def(nb::init<>())
    .def("connect", [](SubSocket &self, Name endpoint, bool conflate, size_t segment_size) {
      int r;
      {
        nb::gil_scoped_release release;
        r = self.connect(endpoint.value, conflate, segment_size);
      }
      if (r != 0) throw_ipc_error(&endpoint.value);
    }, "endpoint"_a, "conflate"_a = false, "segment_size"_a = 0)
    .def("setTimeout", &SubSocket::setTimeout, "timeout"_a, nb::call_guard<nb::gil_scoped_release>())
    .def("receive", [](SubSocket &self, bool non_blocking) -> nb::object {
      Message *msg;
      if (non_blocking) {
        // a non-blocking receive is too fast to be worth releasing the GIL
        msg = self.receive(true);
      } else {
        nb::gil_scoped_release release;
        msg = self.receive(false);
      }
      if (!msg) return nb::none();
      nb::bytes ret(msg->getData(), msg->getSize());
      delete msg;
      return ret;
    }, "non_blocking"_a = false);

  nb::class_<PubSocket>(m, "PubSocket")
    .def(nb::init<>())
    .def("connect", [](PubSocket &self, Name endpoint, size_t segment_size) {
      if (self.connect(endpoint.value, segment_size) != 0) throw_ipc_error(&endpoint.value);
    }, "endpoint"_a, "segment_size"_a = 0)
    .def("send", [](PubSocket &self, nb::handle data) {
      int r;
      size_t len;
      if (nb::isinstance<nb::bytes>(data)) {
        auto bytes = nb::borrow<nb::bytes>(data);
        len = bytes.size();
        r = self.send((char *)bytes.c_str(), len);
      } else {
        Buffer buf(data);
        len = buf.size();
        r = self.send(buf.data(), len);
      }
      if (r != (int)len) throw_ipc_error();
    }, "data"_a)
    .def("all_readers_updated", &PubSocket::all_readers_updated)
    .def("wait_for_readers", [](PubSocket &self, double timeout, double interval) {
      auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double>(timeout);
      while (std::chrono::steady_clock::now() < deadline) {
        if (self.all_readers_updated()) return;
        {
          nb::gil_scoped_release release;
          std::this_thread::sleep_for(std::chrono::duration<double>(interval));
        }
        if (PyErr_CheckSignals() < 0) throw nb::python_error();
      }
      PyErr_SetString(PyExc_TimeoutError, "subscriber did not connect");
      throw nb::python_error();
    }, "timeout"_a = 1.0, "interval"_a = 0.001);

  nb::class_<PyPoller>(m, "Poller")
    .def(nb::init<>())
    .def("registerSocket", [](PyPoller &self, nb::object socket) {
      self.poller.registerSocket(nb::cast<SubSocket *>(socket));
      self.sockets.push_back(socket);
    }, "socket"_a)
    .def("poll", [](PyPoller &self, int timeout) {
      std::vector<SubSocket *> ready;
      {
        nb::gil_scoped_release release;
        ready = self.poller.poll(timeout);
      }
      nb::list ret;
      for (SubSocket *s : ready) {
        for (const nb::object &socket : self.sockets) {
          if (nb::inst_ptr<SubSocket>(socket) == s) {
            ret.append(socket);
            break;
          }
        }
      }
      return ret;
    }, "timeout"_a);
}
