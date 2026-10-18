// Python bindings for msgq, written against the CPython limited API (abi3).
#define PY_SSIZE_T_CLEAN
#define Py_LIMITED_API 0x030B0000
#include <Python.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <exception>
#include <string>
#include <vector>

#include "msgq/ipc.h"
#include "msgq/event.h"

#include "msgq/python_helpers.h"

static PyObject *IpcError;
static PyObject *MultiplePublishersError;
static PyTypeObject *SubSocketType, *PubSocketType, *PollerType, *EventType, *SocketEventHandleType;

static PyObject *raise_ipc_error(const std::string *endpoint) {
  int err = errno;
  std::string suffix = endpoint ? "with " + *endpoint : "";
  PyErr_Format(err == EADDRINUSE ? MultiplePublishersError : IpcError, "Messaging failure %s: %s", suffix.c_str(), strerror(err));
  return nullptr;
}

// *** SubSocket ***

struct SubSocketObject {
  PyObject_HEAD
  SubSocket *socket;
};

static PyObject *SubSocket_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  if (!PyArg_ParseTuple(args, ":SubSocket")) return nullptr;
  auto self = (SubSocketObject *)alloc_object(type);
  if (!self) return nullptr;
  self->socket = new SubSocket();
  return (PyObject *)self;
}

static void SubSocket_dealloc(PyObject *obj) {
  auto self = (SubSocketObject *)obj;
  if (self->socket) {
    release_gil([&] { delete self->socket; });
  }
  free_object(obj);
}

static PyObject *SubSocket_connect(SubSocketObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"endpoint", "conflate", "segment_size", nullptr};
  PyObject *o[3] = {};
  std::string endpoint;
  bool conflate = false;
  size_t segment_size = 0;
  if (!parse_args("connect", args, nargs, kwnames, kwlist, 1, o) || !to_string(o[0], endpoint) ||
      (o[1] && !to_bool(o[1], conflate)) || (o[2] && !to_size(o[2], segment_size))) {
    return nullptr;
  }

  int r;
  release_gil([&] { r = self->socket->connect(endpoint, conflate, segment_size); });
  if (r != 0) return raise_ipc_error(&endpoint);
  Py_RETURN_NONE;
}

static PyObject *SubSocket_setTimeout(SubSocketObject *self, PyObject *arg) {
  int timeout;
  if (!to_int(arg, timeout)) return nullptr;
  release_gil([&] { self->socket->setTimeout(timeout); });
  Py_RETURN_NONE;
}

static PyObject *SubSocket_receive(SubSocketObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"non_blocking", nullptr};
  PyObject *o[1] = {};
  bool non_blocking = false;
  if (!parse_args("receive", args, nargs, kwnames, kwlist, 0, o) || (o[0] && !to_bool(o[0], non_blocking))) return nullptr;

  Message *msg;
  if (non_blocking) {
    // a non-blocking receive is too fast to be worth releasing the GIL
    msg = self->socket->receive(true);
  } else {
    release_gil([&] { msg = self->socket->receive(false); });
  }
  if (!msg) Py_RETURN_NONE;
  PyObject *ret = PyBytes_FromStringAndSize(msg->getData(), msg->getSize());
  delete msg;
  return ret;
}

static PyMethodDef SubSocket_methods[] = {
  {"connect", (PyCFunction)(void (*)(void))SubSocket_connect, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {"setTimeout", (PyCFunction)SubSocket_setTimeout, METH_O, nullptr},
  {"receive", (PyCFunction)(void (*)(void))SubSocket_receive, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {},
};

static PyType_Slot SubSocket_slots[] = {
  {Py_tp_new, (void *)SubSocket_new},
  {Py_tp_dealloc, (void *)SubSocket_dealloc},
  {Py_tp_methods, SubSocket_methods},
  {},
};

// *** PubSocket ***

struct PubSocketObject {
  PyObject_HEAD
  PubSocket *socket;
};

static PyObject *PubSocket_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  if (!PyArg_ParseTuple(args, ":PubSocket")) return nullptr;
  auto self = (PubSocketObject *)alloc_object(type);
  if (!self) return nullptr;
  self->socket = new PubSocket();
  return (PyObject *)self;
}

static void PubSocket_dealloc(PyObject *obj) {
  delete ((PubSocketObject *)obj)->socket;
  free_object(obj);
}

static PyObject *PubSocket_connect(PubSocketObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"endpoint", "segment_size", nullptr};
  PyObject *o[2] = {};
  std::string endpoint;
  size_t segment_size = 0;
  if (!parse_args("connect", args, nargs, kwnames, kwlist, 1, o) || !to_string(o[0], endpoint) ||
      (o[1] && !to_size(o[1], segment_size))) {
    return nullptr;
  }

  if (self->socket->connect(endpoint, segment_size) != 0) return raise_ipc_error(&endpoint);
  Py_RETURN_NONE;
}

static PyObject *PubSocket_send(PubSocketObject *self, PyObject *data) {
  char *buf;
  Py_ssize_t len;
  int r;
  if (PyBytes_Check(data)) {
    if (PyBytes_AsStringAndSize(data, &buf, &len) < 0) return nullptr;
    r = self->socket->send(buf, len);
  } else {
    Py_buffer view;
    if (PyObject_GetBuffer(data, &view, PyBUF_SIMPLE) < 0) return nullptr;
    len = view.len;
    r = self->socket->send((char *)view.buf, len);
    PyBuffer_Release(&view);
  }
  if (r != len) return raise_ipc_error(nullptr);
  Py_RETURN_NONE;
}

static PyObject *PubSocket_all_readers_updated(PubSocketObject *self, PyObject *) {
  return PyBool_FromLong(self->socket->all_readers_updated());
}

static PyObject *PubSocket_wait_for_readers(PubSocketObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"timeout", "interval", nullptr};
  PyObject *o[2] = {};
  double timeout = 1.0, interval = 0.001;
  if (!parse_args("wait_for_readers", args, nargs, kwnames, kwlist, 0, o) || (o[0] && !to_double(o[0], timeout)) ||
      (o[1] && !to_double(o[1], interval))) {
    return nullptr;
  }

  double deadline = monotonic() + timeout;
  while (monotonic() < deadline) {
    if (self->socket->all_readers_updated()) Py_RETURN_NONE;
    struct timespec ts = {(time_t)interval, (long)((interval - (time_t)interval) * 1e9)};
    release_gil([&] { nanosleep(&ts, nullptr); });
    if (PyErr_CheckSignals() < 0) return nullptr;
  }
  PyErr_SetString(PyExc_TimeoutError, "subscriber did not connect");
  return nullptr;
}

static PyMethodDef PubSocket_methods[] = {
  {"connect", (PyCFunction)(void (*)(void))PubSocket_connect, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {"send", (PyCFunction)PubSocket_send, METH_O, nullptr},
  {"all_readers_updated", (PyCFunction)PubSocket_all_readers_updated, METH_NOARGS, nullptr},
  {"wait_for_readers", (PyCFunction)(void (*)(void))PubSocket_wait_for_readers, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {},
};

static PyType_Slot PubSocket_slots[] = {
  {Py_tp_new, (void *)PubSocket_new},
  {Py_tp_dealloc, (void *)PubSocket_dealloc},
  {Py_tp_methods, PubSocket_methods},
  {},
};

// *** Poller ***

struct PollerObject {
  PyObject_HEAD
  Poller *poller;
  PyObject *sockets;  // list of registered SubSockets, keeps them alive and maps results back to them
};

static PyObject *Poller_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  if (!PyArg_ParseTuple(args, ":Poller")) return nullptr;
  auto self = (PollerObject *)alloc_object(type);
  if (!self) return nullptr;
  self->sockets = PyList_New(0);
  if (!self->sockets) {
    Py_DECREF(self);
    return nullptr;
  }
  self->poller = new Poller();
  return (PyObject *)self;
}

static void Poller_dealloc(PyObject *obj) {
  auto self = (PollerObject *)obj;
  delete self->poller;
  Py_XDECREF(self->sockets);
  free_object(obj);
}

static PyObject *Poller_registerSocket(PollerObject *self, PyObject *socket) {
  if (!PyObject_TypeCheck(socket, SubSocketType)) return PyErr_Format(PyExc_TypeError, "socket must be a SubSocket");
  if (PyList_Append(self->sockets, socket) < 0) return nullptr;
  self->poller->registerSocket(((SubSocketObject *)socket)->socket);
  Py_RETURN_NONE;
}

static PyObject *Poller_poll(PollerObject *self, PyObject *arg) {
  int timeout;
  if (!to_int(arg, timeout)) return nullptr;
  std::vector<SubSocket *> ready;
  release_gil([&] { ready = self->poller->poll(timeout); });

  PyObject *ret = PyList_New(0);
  Py_ssize_t n = PyList_Size(self->sockets);
  for (SubSocket *s : ready) {
    for (Py_ssize_t i = 0; ret && i < n; i++) {
      PyObject *socket = PyList_GetItem(self->sockets, i);
      if (((SubSocketObject *)socket)->socket == s) {
        if (PyList_Append(ret, socket) < 0) Py_CLEAR(ret);
        break;
      }
    }
  }
  return ret;
}

static PyMethodDef Poller_methods[] = {
  {"registerSocket", (PyCFunction)Poller_registerSocket, METH_O, nullptr},
  {"poll", (PyCFunction)Poller_poll, METH_O, nullptr},
  {},
};

static PyType_Slot Poller_slots[] = {
  {Py_tp_new, (void *)Poller_new},
  {Py_tp_dealloc, (void *)Poller_dealloc},
  {Py_tp_methods, Poller_methods},
  {},
};

// *** Event ***

struct EventObject {
  PyObject_HEAD
  Event event;
};

static PyObject *new_event(Event event) {
  auto self = (EventObject *)alloc_object(EventType);
  if (self) self->event = event;
  return (PyObject *)self;
}

static PyObject *Event_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  if (!PyArg_ParseTuple(args, ":Event")) return nullptr;
  return new_event(Event());
}

static PyObject *Event_set(EventObject *self, PyObject *) {
  return call_cpp([&] { self->event.set(); Py_RETURN_NONE; });
}

static PyObject *Event_clear(EventObject *self, PyObject *) {
  return call_cpp([&] { return PyLong_FromLong(self->event.clear()); });
}

static PyObject *Event_wait(EventObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"timeout", nullptr};
  PyObject *o[1] = {};
  int timeout = -1;
  if (!parse_args("wait", args, nargs, kwnames, kwlist, 0, o) || (o[0] && !to_int(o[0], timeout))) return nullptr;
  bool ok = without_gil([&] { self->event.wait(timeout); });
  if (!ok) return nullptr;
  Py_RETURN_NONE;
}

static PyObject *Event_peek(EventObject *self, PyObject *) {
  return call_cpp([&] { return PyBool_FromLong(self->event.peek()); });
}

static PyObject *Event_get_fd(EventObject *self, void *) { return PyLong_FromLong(self->event.fd()); }
static PyObject *Event_get_ptr(EventObject *self, void *) { return PyLong_FromVoidPtr(&self->event); }

static PyMethodDef Event_methods[] = {
  {"set", (PyCFunction)Event_set, METH_NOARGS, nullptr},
  {"clear", (PyCFunction)Event_clear, METH_NOARGS, nullptr},
  {"wait", (PyCFunction)(void (*)(void))Event_wait, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {"peek", (PyCFunction)Event_peek, METH_NOARGS, nullptr},
  {},
};

static PyGetSetDef Event_getset[] = {
  {"fd", (getter)Event_get_fd, nullptr, nullptr, nullptr},
  {"ptr", (getter)Event_get_ptr, nullptr, nullptr, nullptr},
  {},
};

static PyType_Slot Event_slots[] = {
  {Py_tp_new, (void *)Event_new},
  {Py_tp_dealloc, (void *)free_object},
  {Py_tp_methods, Event_methods},
  {Py_tp_getset, Event_getset},
  {},
};

// *** SocketEventHandle ***

struct SocketEventHandleObject {
  PyObject_HEAD
  SocketEventHandle *handle;
};

static PyObject *SocketEventHandle_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  static const char *kwlist[] = {"endpoint", "identifier", "override", nullptr};
  PyObject *o[3];
  std::string endpoint, identifier;
  bool override;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OOO:SocketEventHandle", (char **)kwlist, &o[0], &o[1], &o[2]) ||
      !to_string(o[0], endpoint) || !to_string(o[1], identifier) || !to_bool(o[2], override)) {
    return nullptr;
  }
  auto self = (SocketEventHandleObject *)alloc_object(type);
  if (!self) return nullptr;
  try {
    self->handle = new SocketEventHandle(endpoint, identifier, override);
  } catch (const std::exception &e) {
    Py_DECREF(self);
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return nullptr;
  }
  return (PyObject *)self;
}

static void SocketEventHandle_dealloc(PyObject *obj) {
  delete ((SocketEventHandleObject *)obj)->handle;
  free_object(obj);
}

static PyObject *SocketEventHandle_get_enabled(SocketEventHandleObject *self, void *) {
  return PyBool_FromLong(self->handle->is_enabled());
}

static int SocketEventHandle_set_enabled(SocketEventHandleObject *self, PyObject *value, void *) {
  bool enabled;
  if (!value) {
    PyErr_SetString(PyExc_AttributeError, "cannot delete enabled");
    return -1;
  }
  if (!to_bool(value, enabled)) return -1;
  self->handle->set_enabled(enabled);
  return 0;
}

static PyObject *SocketEventHandle_get_recv_called(SocketEventHandleObject *self, void *) { return new_event(self->handle->recv_called()); }
static PyObject *SocketEventHandle_get_recv_ready(SocketEventHandleObject *self, void *) { return new_event(self->handle->recv_ready()); }

static PyGetSetDef SocketEventHandle_getset[] = {
  {"enabled", (getter)SocketEventHandle_get_enabled, (setter)SocketEventHandle_set_enabled, nullptr, nullptr},
  {"recv_called_event", (getter)SocketEventHandle_get_recv_called, nullptr, nullptr, nullptr},
  {"recv_ready_event", (getter)SocketEventHandle_get_recv_ready, nullptr, nullptr, nullptr},
  {},
};

static PyType_Slot SocketEventHandle_slots[] = {
  {Py_tp_new, (void *)SocketEventHandle_new},
  {Py_tp_dealloc, (void *)SocketEventHandle_dealloc},
  {Py_tp_getset, SocketEventHandle_getset},
  {},
};

// *** module functions ***

static PyObject *toggle_fake_events(PyObject *, PyObject *arg) {
  bool enabled;
  if (!to_bool(arg, enabled)) return nullptr;
  SocketEventHandle::toggle_fake_events(enabled);
  Py_RETURN_NONE;
}

static PyObject *set_fake_prefix(PyObject *, PyObject *arg) {
  std::string prefix;
  if (!to_string(arg, prefix)) return nullptr;
  SocketEventHandle::set_fake_prefix(prefix);
  Py_RETURN_NONE;
}

static PyObject *get_fake_prefix(PyObject *, PyObject *) {
  std::string prefix = SocketEventHandle::fake_prefix();
  return PyBytes_FromStringAndSize(prefix.data(), prefix.size());
}

static PyObject *delete_fake_prefix(PyObject *, PyObject *) {
  SocketEventHandle::set_fake_prefix("");
  Py_RETURN_NONE;
}

static PyObject *wait_for_one_event(PyObject *, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"events", "timeout", nullptr};
  PyObject *o[2] = {};
  int timeout = -1;
  if (!parse_args("wait_for_one_event", args, nargs, kwnames, kwlist, 1, o) || (o[1] && !to_int(o[1], timeout))) return nullptr;

  std::vector<Event> events;
  PyObject *iter = PyObject_GetIter(o[0]);
  if (!iter) return nullptr;
  while (PyObject *item = PyIter_Next(iter)) {
    bool ok = PyObject_TypeCheck(item, EventType);
    if (ok) events.push_back(((EventObject *)item)->event);
    Py_DECREF(item);
    if (!ok) {
      PyErr_SetString(PyExc_TypeError, "events must be Event objects");
      break;
    }
  }
  Py_DECREF(iter);
  if (PyErr_Occurred()) return nullptr;

  int ret;
  bool ok = without_gil([&] { ret = Event::wait_for_one(events, timeout); });
  if (!ok) return nullptr;
  return PyLong_FromLong(ret);
}

static PyMethodDef module_methods[] = {
  {"toggle_fake_events", (PyCFunction)toggle_fake_events, METH_O, nullptr},
  {"set_fake_prefix", (PyCFunction)set_fake_prefix, METH_O, nullptr},
  {"get_fake_prefix", (PyCFunction)get_fake_prefix, METH_NOARGS, nullptr},
  {"delete_fake_prefix", (PyCFunction)delete_fake_prefix, METH_NOARGS, nullptr},
  {"wait_for_one_event", (PyCFunction)(void (*)(void))wait_for_one_event, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {},
};

static PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "msgq.ipc_pyx", nullptr, -1, module_methods};

PyMODINIT_FUNC PyInit_ipc_pyx(void) {
  PyObject *m = PyModule_Create(&module_def);
  if (!m) return nullptr;

  IpcError = PyErr_NewException("msgq.ipc_pyx.IpcError", nullptr, nullptr);
  MultiplePublishersError = PyErr_NewException("msgq.ipc_pyx.MultiplePublishersError", IpcError, nullptr);
  if (!IpcError || !MultiplePublishersError || PyModule_AddObjectRef(m, "IpcError", IpcError) < 0 ||
      PyModule_AddObjectRef(m, "MultiplePublishersError", MultiplePublishersError) < 0) {
    Py_DECREF(m);
    return nullptr;
  }

  if (!add_type(m, "msgq.ipc_pyx.SubSocket", sizeof(SubSocketObject), SubSocket_slots, &SubSocketType) ||
      !add_type(m, "msgq.ipc_pyx.PubSocket", sizeof(PubSocketObject), PubSocket_slots, &PubSocketType) ||
      !add_type(m, "msgq.ipc_pyx.Poller", sizeof(PollerObject), Poller_slots, &PollerType) ||
      !add_type(m, "msgq.ipc_pyx.Event", sizeof(EventObject), Event_slots, &EventType) ||
      !add_type(m, "msgq.ipc_pyx.SocketEventHandle", sizeof(SocketEventHandleObject), SocketEventHandle_slots, &SocketEventHandleType)) {
    Py_DECREF(m);
    return nullptr;
  }
  return m;
}
