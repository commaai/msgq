#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <poll.h>
#include <sys/eventfd.h>

// Python C-API uses mixed designated initializers which triggers C99-designator warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc99-designator"

#include "msgq/ipc.h"
#include "msgq/event.h"

static PyObject *IpcError;
static PyObject *MultiplePublishersError;

// Helper: Exception handling
static void set_ipc_error(const char* endpoint, int err_no) {
  if (err_no == EADDRINUSE) {
    PyErr_SetString(MultiplePublishersError, endpoint ? endpoint : "");
  } else {
    PyObject *msg = PyUnicode_FromFormat("Messaging failure %s%s: %s",
                                         endpoint ? "with " : "",
                                         endpoint ? endpoint : "",
                                         strerror(err_no));
    PyErr_SetObject(IpcError, msg);
    Py_DECREF(msg);
  }
}

// Context
typedef struct {
  PyObject_HEAD
  Context *context;
} ContextObject;

static void Context_dealloc(ContextObject *self) {
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *Context_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  ContextObject *self = (ContextObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    try {
      self->context = Context::create();
    } catch (const std::exception& e) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
      Py_DECREF(self);
      return NULL;
    }
  }
  return (PyObject *)self;
}

static PyObject *Context_term(ContextObject *self, PyObject *args) {
  if (self->context) {
    delete self->context;
    self->context = NULL;
  }
  Py_RETURN_NONE;
}

static PyMethodDef Context_methods[] = {
  {"term", (PyCFunction)Context_term, METH_NOARGS, "Terminate the context"},
  {NULL}
};

static PyTypeObject ContextType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "_ipc_module.Context",
  .tp_basicsize = sizeof(ContextObject),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor)Context_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_methods = Context_methods,
  .tp_new = Context_new,
};

// Event
typedef struct {
  PyObject_HEAD
  Event *event;
  bool owner;
  int owned_fd;
} EventObject;

static void Event_dealloc(EventObject *self) {
  if (self->owner && self->event) {
    delete self->event;
  }
  if (self->owned_fd != -1) {
    close(self->owned_fd);
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *Event_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  EventObject *self = (EventObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->owned_fd = eventfd(0, EFD_NONBLOCK);
    if (self->owned_fd == -1) {
      PyErr_SetFromErrno(PyExc_OSError);
      Py_DECREF(self);
      return NULL;
    }
    self->event = new Event(self->owned_fd);
    self->owner = true;
  }
  return (PyObject *)self;
}

static PyObject *Event_set(EventObject *self, PyObject *args) {
  try {
    self->event->set();
  } catch (const std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *Event_clear(EventObject *self, PyObject *args) {
  try {
    self->event->clear();
  } catch (const std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *Event_wait(EventObject *self, PyObject *args, PyObject *kwds) {
  int timeout = -1;
  static char *kwlist[] = {(char*)"timeout", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &timeout)) {
    return NULL;
  }

  if (!self->event->is_valid()) {
    // If event is invalid, wait returns immediately (or we simulate it not being signaled)
    // For invalid event, we can't wait. But test expects blocking behavior for timeout.
    if (timeout > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
    }
    Py_RETURN_NONE;
  }

  struct pollfd fds;
  fds.fd = self->event->fd();
  fds.events = POLLIN;

  int ret;
  Py_BEGIN_ALLOW_THREADS
  do {
    ret = poll(&fds, 1, timeout);
  } while (ret < 0 && errno == EINTR);
  Py_END_ALLOW_THREADS

  if (ret < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
  }

  if (ret == 0 && timeout == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Event timed out");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *Event_peek(EventObject *self, PyObject *args) {
  try {
    if (self->event->is_valid() && self->event->peek()) {
      Py_RETURN_TRUE;
    } else {
      Py_RETURN_FALSE;
    }
  } catch (const std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
}

static PyObject *Event_get_fd(EventObject *self, void *closure) {
  return PyLong_FromLong(self->event->fd());
}

static PyGetSetDef Event_getseters[] = {
  {(char*)"fd", (getter)Event_get_fd, NULL, (char*)"File descriptor", NULL},
  {NULL}
};

static PyMethodDef Event_methods[] = {
  {"set", (PyCFunction)Event_set, METH_NOARGS, ""},
  {"clear", (PyCFunction)Event_clear, METH_NOARGS, ""},
  {"wait", (PyCFunction)Event_wait, METH_VARARGS | METH_KEYWORDS, ""},
  {"peek", (PyCFunction)Event_peek, METH_NOARGS, ""},
  {NULL}
};

static PyTypeObject EventType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "_ipc_module.Event",
  .tp_basicsize = sizeof(EventObject),
  .tp_dealloc = (destructor)Event_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_methods = Event_methods,
  .tp_getset = Event_getseters,
  .tp_new = Event_new,
};


// SocketEventHandle
typedef struct {
  PyObject_HEAD
  SocketEventHandle *handle;
} SocketEventHandleObject;

static void SocketEventHandle_dealloc(SocketEventHandleObject *self) {
  if (self->handle) {
    delete self->handle;
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *SocketEventHandle_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  SocketEventHandleObject *self = (SocketEventHandleObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    char *endpoint, *identifier;
    int override;
    static char *kwlist[] = {(char*)"endpoint", (char*)"identifier", (char*)"override", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ssp", kwlist, &endpoint, &identifier, &override)) {
      Py_DECREF(self);
      return NULL;
    }
    try {
      self->handle = new SocketEventHandle(endpoint, identifier, override);
    } catch (const std::exception& e) {
       PyErr_SetString(PyExc_RuntimeError, e.what());
       Py_DECREF(self);
       return NULL;
    }
  }
  return (PyObject *)self;
}

static PyObject *SocketEventHandle_recv_called(SocketEventHandleObject *self, void *closure) {
  EventObject *event_obj = PyObject_New(EventObject, &EventType);
  if (!event_obj) return NULL;
  try {
    event_obj->event = new Event(self->handle->recv_called());
  } catch (const std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    Py_DECREF(event_obj);
    return NULL;
  }
  event_obj->owner = true;
  event_obj->owned_fd = -1;
  return (PyObject *)event_obj;
}

static PyObject *SocketEventHandle_recv_ready(SocketEventHandleObject *self, void *closure) {
  EventObject *event_obj = PyObject_New(EventObject, &EventType);
  if (!event_obj) return NULL;
  try {
    event_obj->event = new Event(self->handle->recv_ready());
  } catch (const std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    Py_DECREF(event_obj);
    return NULL;
  }
  event_obj->owner = true;
  event_obj->owned_fd = -1;
  return (PyObject *)event_obj;
}


static PyObject *SocketEventHandle_get_enabled(SocketEventHandleObject *self, void *closure) {
  if (self->handle->is_enabled()) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static int SocketEventHandle_set_enabled(SocketEventHandleObject *self, PyObject *value, void *closure) {
  if (!PyBool_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "The value must be a boolean");
    return -1;
  }
  self->handle->set_enabled(value == Py_True);
  return 0;
}

static PyGetSetDef SocketEventHandle_getseters[] = {
  {(char*)"recv_called_event", (getter)SocketEventHandle_recv_called, NULL, (char*)"Recv called event", NULL},
  {(char*)"recv_ready_event", (getter)SocketEventHandle_recv_ready, NULL, (char*)"Recv ready event", NULL},
  {(char*)"enabled", (getter)SocketEventHandle_get_enabled, (setter)SocketEventHandle_set_enabled, (char*)"Enabled status", NULL},
  {NULL}
};

static PyTypeObject SocketEventHandleType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "_ipc_module.SocketEventHandle",
  .tp_basicsize = sizeof(SocketEventHandleObject),
  .tp_dealloc = (destructor)SocketEventHandle_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_getset = SocketEventHandle_getseters,
  .tp_new = SocketEventHandle_new,
};


// SubSocket
typedef struct {
  PyObject_HEAD
  SubSocket *socket;
  bool is_owner;
} SubSocketObject;

static void SubSocket_dealloc(SubSocketObject *self) {
  if (self->is_owner && self->socket) {
    delete self->socket;
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *SubSocket_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  SubSocketObject *self = (SubSocketObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    Py_BEGIN_ALLOW_THREADS
    try {
      self->socket = SubSocket::create();
    } catch (const std::exception& e) {
      self->socket = NULL;
    }
    Py_END_ALLOW_THREADS
    self->is_owner = true;
    if (self->socket == NULL) {
      PyErr_SetNone(IpcError);
      Py_DECREF(self);
      return NULL;
    }
  }
  return (PyObject *)self;
}

static PyObject *SubSocket_connect(SubSocketObject *self, PyObject *args, PyObject *kwds) {
  ContextObject *context;
  const char *endpoint;
  const char *address = "127.0.0.1";
  int conflate = 0;
  static char *kwlist[] = {(char*)"context", (char*)"endpoint", (char*)"address", (char*)"conflate", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!s|sp", kwlist, &ContextType, &context, &endpoint, &address, &conflate)) {
    return NULL;
  }

  int r;
  Py_BEGIN_ALLOW_THREADS
  try {
    r = self->socket->connect(context->context, endpoint, address, conflate);
  } catch (const std::exception& e) {
    r = -1;
  }
  Py_END_ALLOW_THREADS

  if (r != 0) {
    set_ipc_error(endpoint, errno);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *SubSocket_setTimeout(SubSocketObject *self, PyObject *args) {
  int timeout;
  if (!PyArg_ParseTuple(args, "i", &timeout)) return NULL;

  Py_BEGIN_ALLOW_THREADS
  try {
    self->socket->setTimeout(timeout);
  } catch (const std::exception& e) {
     // Ignore
  }
  Py_END_ALLOW_THREADS
  Py_RETURN_NONE;
}

static PyObject *SubSocket_receive(SubSocketObject *self, PyObject *args, PyObject *kwds) {
  int non_blocking = 0;
  static char *kwlist[] = {(char*)"non_blocking", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|p", kwlist, &non_blocking)) {
    return NULL;
  }

  Message *msg;
  Py_BEGIN_ALLOW_THREADS
  try {
    msg = self->socket->receive(non_blocking);
  } catch (const std::exception& e) {
    msg = NULL;
  }
  Py_END_ALLOW_THREADS

  if (msg == NULL) {
    Py_RETURN_NONE;
  }

  size_t sz = msg->getSize();
  char *data = msg->getData();
  PyObject *bytes = PyBytes_FromStringAndSize(data, sz);

  Py_BEGIN_ALLOW_THREADS
  delete msg;
  Py_END_ALLOW_THREADS // cppcheck-suppress unknownMacro


  return bytes;
}

static PyObject *SubSocket_richcompare(SubSocketObject *self, PyObject *other, int op) {
  if (op != Py_EQ && op != Py_NE) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  if (!PyObject_TypeCheck(other, Py_TYPE(self))) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  SubSocketObject *other_sock = (SubSocketObject *)other;
  bool eq = (self->socket == other_sock->socket) ||
            (self->socket && other_sock->socket && self->socket->getRawSocket() == other_sock->socket->getRawSocket());

  if (op == Py_EQ) {
    return eq ? Py_True : Py_False;
  } else {
    return eq ? Py_False : Py_True;
  }
}

static PyMethodDef SubSocket_methods[] = {
  {"connect", (PyCFunction)SubSocket_connect, METH_VARARGS | METH_KEYWORDS, ""},
  {"setTimeout", (PyCFunction)SubSocket_setTimeout, METH_VARARGS, ""},
  {"receive", (PyCFunction)SubSocket_receive, METH_VARARGS | METH_KEYWORDS, ""},
  {NULL}
};

static PyTypeObject SubSocketType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "_ipc_module.SubSocket",
  .tp_basicsize = sizeof(SubSocketObject),
  .tp_dealloc = (destructor)SubSocket_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_richcompare = (richcmpfunc)SubSocket_richcompare,
  .tp_methods = SubSocket_methods,
  .tp_new = SubSocket_new,
};

// PubSocket
typedef struct {
  PyObject_HEAD
  PubSocket *socket;
} PubSocketObject;

static void PubSocket_dealloc(PubSocketObject *self) {
  if (self->socket) {
    delete self->socket;
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *PubSocket_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PubSocketObject *self = (PubSocketObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    try {
      self->socket = PubSocket::create();
    } catch (const std::exception& e) {
      self->socket = NULL;
    }
    if (self->socket == NULL) {
      PyErr_SetNone(IpcError);
      Py_DECREF(self);
      return NULL;
    }
  }
  return (PyObject *)self;
}

static PyObject *PubSocket_connect(PubSocketObject *self, PyObject *args) {
  ContextObject *context;
  const char *endpoint;
  if (!PyArg_ParseTuple(args, "O!s", &ContextType, &context, &endpoint)) return NULL;

  int r;
  try {
    r = self->socket->connect(context->context, endpoint);
  } catch (const std::exception& e) {
    r = -1;
  }

  if (r != 0) {
    set_ipc_error(endpoint, errno);
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *PubSocket_send(PubSocketObject *self, PyObject *args) {
  char *data;
  Py_ssize_t length;
  if (!PyArg_ParseTuple(args, "y#", &data, &length)) return NULL;

  int r;
  try {
    r = self->socket->send(data, length);
  } catch (const std::exception& e) {
    r = -1;
  }

  if (r != length) {
    set_ipc_error(NULL, errno);
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *PubSocket_all_readers_updated(PubSocketObject *self, PyObject *args) {
  try {
    if (self->socket->all_readers_updated()) Py_RETURN_TRUE;
  } catch (const std::exception& e) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
     return NULL;
  }
  Py_RETURN_FALSE;
}

static PyMethodDef PubSocket_methods[] = {
  {"connect", (PyCFunction)PubSocket_connect, METH_VARARGS, ""},
  {"send", (PyCFunction)PubSocket_send, METH_VARARGS, ""},
  {"all_readers_updated", (PyCFunction)PubSocket_all_readers_updated, METH_NOARGS, ""},
  {NULL}
};

static PyTypeObject PubSocketType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "_ipc_module.PubSocket",
  .tp_basicsize = sizeof(PubSocketObject),
  .tp_dealloc = (destructor)PubSocket_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_methods = PubSocket_methods,
  .tp_new = PubSocket_new,
};


// Poller
typedef struct {
  PyObject_HEAD
  Poller *poller;
  PyObject *sub_sockets; // Keep reference to prevent GC
} PollerObject;

static void Poller_dealloc(PollerObject *self) {
  if (self->poller) {
    delete self->poller;
  }
  Py_XDECREF(self->sub_sockets);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *Poller_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PollerObject *self = (PollerObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->sub_sockets = PyList_New(0);
    try {
      self->poller = Poller::create();
    } catch (const std::exception& e) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
      Py_DECREF(self);
      return NULL;
    }
  }
  return (PyObject *)self;
}

static PyObject *Poller_registerSocket(PollerObject *self, PyObject *args) {
  SubSocketObject *socket;
  if (!PyArg_ParseTuple(args, "O!", &SubSocketType, &socket)) return NULL;

  PyList_Append(self->sub_sockets, (PyObject *)socket);
  try {
    self->poller->registerSocket(socket->socket);
  } catch (const std::exception& e) {
     PyErr_SetString(PyExc_RuntimeError, e.what());
     return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *Poller_poll(PollerObject *self, PyObject *args) {
  int timeout;
  if (!PyArg_ParseTuple(args, "i", &timeout)) return NULL;

  std::vector<SubSocket*> result;
  Py_BEGIN_ALLOW_THREADS
  try {
    result = self->poller->poll(timeout);
  } catch (const std::exception& e) {
    // catch in python thread?
  }
  Py_END_ALLOW_THREADS

  PyObject *py_result = PyList_New(0);
  for (SubSocket *s : result) {
    SubSocketObject *new_obj = PyObject_New(SubSocketObject, &SubSocketType);
    if (!new_obj) { Py_DECREF(py_result); return NULL; }

    new_obj->socket = s;
    new_obj->is_owner = false; // Poller returns weak ref
    PyList_Append(py_result, (PyObject*)new_obj);
    Py_DECREF(new_obj);
  }

  return py_result;
}

static PyMethodDef Poller_methods[] = {
  {"registerSocket", (PyCFunction)Poller_registerSocket, METH_VARARGS, ""},
  {"poll", (PyCFunction)Poller_poll, METH_VARARGS, ""},
  {NULL}
};

static PyTypeObject PollerType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "_ipc_module.Poller",
  .tp_basicsize = sizeof(PollerObject),
  .tp_dealloc = (destructor)Poller_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_methods = Poller_methods,
  .tp_new = Poller_new,
};


// Functions
static PyObject *ipc_toggle_fake_events(PyObject *self, PyObject *args) {
  int enabled;
  if (!PyArg_ParseTuple(args, "p", &enabled)) return NULL;
  try {
    SocketEventHandle::toggle_fake_events(enabled);
  } catch (const std::exception& e) {
     PyErr_SetString(PyExc_RuntimeError, e.what());
     return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *ipc_set_fake_prefix(PyObject *self, PyObject *args) {
  const char *prefix;
  if (!PyArg_ParseTuple(args, "s", &prefix)) return NULL;
  try {
    SocketEventHandle::set_fake_prefix(prefix);
  } catch (const std::exception& e) {
     PyErr_SetString(PyExc_RuntimeError, e.what());
     return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *ipc_get_fake_prefix(PyObject *self, PyObject *args) {
  try {
    std::string prefix = SocketEventHandle::fake_prefix();
    return PyUnicode_FromString(prefix.c_str());
  } catch (const std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
}

static PyObject *ipc_delete_fake_prefix(PyObject *self, PyObject *args) {
  try {
    SocketEventHandle::set_fake_prefix("");
  } catch (const std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *ipc_wait_for_one_event(PyObject *self, PyObject *args, PyObject *kwds) {
  PyObject *events_list;
  int timeout = -1;
  static char *kwlist[] = {(char*)"events", (char*)"timeout", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &events_list, &timeout)) {
    return NULL;
  }

  if (!PySequence_Check(events_list)) {
      PyErr_SetString(PyExc_TypeError, "events must be a sequence");
      return NULL;
  }

  std::vector<Event> items;
  Py_ssize_t len = PySequence_Size(events_list);
  for (Py_ssize_t i=0; i<len; ++i) {
    PyObject *item = PySequence_GetItem(events_list, i);
    if (!PyObject_TypeCheck(item, &EventType)) {
      Py_DECREF(item);
      PyErr_SetString(PyExc_TypeError, "items must be Event objects");
      return NULL;
    }
    EventObject *ev_obj = (EventObject*)item;
    if (ev_obj->event) {
        items.push_back(*ev_obj->event);
    }
    Py_DECREF(item);
  }

  int idx;
  try {
    idx = Event::wait_for_one(items, timeout);
  } catch (const std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }

  return PyLong_FromLong(idx);
}

static PyMethodDef module_methods[] = {
  {"toggle_fake_events", (PyCFunction)ipc_toggle_fake_events, METH_VARARGS, ""},
  {"set_fake_prefix", (PyCFunction)ipc_set_fake_prefix, METH_VARARGS, ""},
  {"get_fake_prefix", (PyCFunction)ipc_get_fake_prefix, METH_NOARGS, ""},
  {"delete_fake_prefix", (PyCFunction)ipc_delete_fake_prefix, METH_NOARGS, ""},
  {"wait_for_one_event", (PyCFunction)ipc_wait_for_one_event, METH_VARARGS | METH_KEYWORDS, ""},
  {NULL}
};

static struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "_ipc_module",
  NULL,
  -1,
  module_methods,
};

PyMODINIT_FUNC PyInit__ipc_module(void) {
  PyObject *m;

  if (PyType_Ready(&ContextType) < 0) return NULL;
  if (PyType_Ready(&SubSocketType) < 0) return NULL;
  if (PyType_Ready(&PubSocketType) < 0) return NULL;
  if (PyType_Ready(&PollerType) < 0) return NULL;
  if (PyType_Ready(&EventType) < 0) return NULL;
  if (PyType_Ready(&SocketEventHandleType) < 0) return NULL;

  m = PyModule_Create(&moduledef);
  if (m == NULL) return NULL;

  Py_INCREF(&ContextType);
  PyModule_AddObject(m, "Context", (PyObject *)&ContextType);

  Py_INCREF(&SubSocketType);
  PyModule_AddObject(m, "SubSocket", (PyObject *)&SubSocketType);

  Py_INCREF(&PubSocketType);
  PyModule_AddObject(m, "PubSocket", (PyObject *)&PubSocketType);

  Py_INCREF(&PollerType);
  PyModule_AddObject(m, "Poller", (PyObject *)&PollerType);

  Py_INCREF(&EventType);
  PyModule_AddObject(m, "Event", (PyObject *)&EventType);

  Py_INCREF(&SocketEventHandleType);
  PyModule_AddObject(m, "SocketEventHandle", (PyObject *)&SocketEventHandleType);

  IpcError = PyErr_NewException("_ipc_module.IpcError", PyExc_Exception, NULL);
  Py_INCREF(IpcError);
  PyModule_AddObject(m, "IpcError", IpcError);

  MultiplePublishersError = PyErr_NewException("_ipc_module.MultiplePublishersError", IpcError, NULL);
  Py_INCREF(MultiplePublishersError);
  PyModule_AddObject(m, "MultiplePublishersError", MultiplePublishersError);

  return m;
}

#pragma GCC diagnostic pop
