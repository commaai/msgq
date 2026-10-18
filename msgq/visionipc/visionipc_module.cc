// Python bindings for visionipc, written against the CPython limited API (abi3).
#define PY_SSIZE_T_CLEAN
#define Py_LIMITED_API 0x030B0000
#include <Python.h>

#include <cstring>
#include <set>
#include <string>

#include "msgq/visionipc/visionbuf.h"
#include "msgq/visionipc/visionipc.h"
#include "msgq/visionipc/visionipc_client.h"
#include "msgq/visionipc/visionipc_server.h"

#include "msgq/python_helpers.h"

static PyTypeObject *VisionBufType, *VisionIpcServerType, *VisionIpcClientType;

static bool to_stream_type(PyObject *obj, VisionStreamType &out) {
  unsigned long v = PyLong_AsUnsignedLong(obj);
  if (v == (unsigned long)-1 && PyErr_Occurred()) return false;
  if (v > UINT32_MAX) {
    PyErr_SetString(PyExc_OverflowError, "stream type must fit in 32 bits");
    return false;
  }
  out = (VisionStreamType)v;
  return true;
}

static PyObject *py_get_endpoint_name(PyObject *, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"name", "stream", nullptr};
  PyObject *o[2] = {};
  std::string name;
  VisionStreamType stream;
  if (!parse_args("get_endpoint_name", args, nargs, kwnames, kwlist, 2, o) || !to_string(o[0], name) || !to_stream_type(o[1], stream)) {
    return nullptr;
  }
  std::string endpoint = get_endpoint_name(name, stream);
  return PyUnicode_FromStringAndSize(endpoint.data(), endpoint.size());
}

// *** VisionBuf ***

struct VisionBufObject {
  PyObject_HEAD
  VisionBuf *buf;
  PyObject *owner;  // the VisionIpcClient that owns buf
};

static void VisionBuf_dealloc(PyObject *obj) {
  Py_XDECREF(((VisionBufObject *)obj)->owner);
  free_object(obj);
}

static PyObject *VisionBuf_get_data(VisionBufObject *self, void *) {
  return PyMemoryView_FromMemory((char *)self->buf->addr, self->buf->len, PyBUF_WRITE);
}
static PyObject *VisionBuf_get_width(VisionBufObject *self, void *) { return PyLong_FromSize_t(self->buf->width); }
static PyObject *VisionBuf_get_height(VisionBufObject *self, void *) { return PyLong_FromSize_t(self->buf->height); }
static PyObject *VisionBuf_get_stride(VisionBufObject *self, void *) { return PyLong_FromSize_t(self->buf->stride); }
static PyObject *VisionBuf_get_uv_offset(VisionBufObject *self, void *) { return PyLong_FromSize_t(self->buf->uv_offset); }
static PyObject *VisionBuf_get_idx(VisionBufObject *self, void *) { return PyLong_FromSize_t(self->buf->idx); }
static PyObject *VisionBuf_get_fd(VisionBufObject *self, void *) { return PyLong_FromLong(self->buf->fd); }
static PyObject *VisionBuf_get_frame_id(VisionBufObject *self, void *) { return PyLong_FromUnsignedLongLong(self->buf->get_frame_id()); }

static PyGetSetDef VisionBuf_getset[] = {
  {"data", (getter)VisionBuf_get_data, nullptr, nullptr, nullptr},
  {"width", (getter)VisionBuf_get_width, nullptr, nullptr, nullptr},
  {"height", (getter)VisionBuf_get_height, nullptr, nullptr, nullptr},
  {"stride", (getter)VisionBuf_get_stride, nullptr, nullptr, nullptr},
  {"uv_offset", (getter)VisionBuf_get_uv_offset, nullptr, nullptr, nullptr},
  {"idx", (getter)VisionBuf_get_idx, nullptr, nullptr, nullptr},
  {"fd", (getter)VisionBuf_get_fd, nullptr, nullptr, nullptr},
  {"frame_id", (getter)VisionBuf_get_frame_id, nullptr, nullptr, nullptr},
  {},
};

static PyType_Slot VisionBuf_slots[] = {
  {Py_tp_dealloc, (void *)VisionBuf_dealloc},
  {Py_tp_getset, VisionBuf_getset},
  {},
};

// *** VisionIpcServer ***

struct VisionIpcServerObject {
  PyObject_HEAD
  VisionIpcServer *server;
};

static PyObject *VisionIpcServer_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  static const char *kwlist[] = {"name", nullptr};
  PyObject *name_obj;
  std::string name;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:VisionIpcServer", (char **)kwlist, &name_obj) || !to_string(name_obj, name)) return nullptr;
  auto self = (VisionIpcServerObject *)alloc_object(type);
  if (self) self->server = new VisionIpcServer(name);
  return (PyObject *)self;
}

static void VisionIpcServer_dealloc(PyObject *obj) {
  auto self = (VisionIpcServerObject *)obj;
  if (self->server) {
    // joins the listener thread
    Py_BEGIN_ALLOW_THREADS
    delete self->server;
    Py_END_ALLOW_THREADS
  }
  free_object(obj);
}

static PyObject *VisionIpcServer_create_buffers(VisionIpcServerObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"tp", "num_buffers", "width", "height", nullptr};
  PyObject *o[4] = {};
  VisionStreamType tp;
  size_t num_buffers, width, height;
  if (!parse_args("create_buffers", args, nargs, kwnames, kwlist, 4, o) || !to_stream_type(o[0], tp) || !to_size(o[1], num_buffers) ||
      !to_size(o[2], width) || !to_size(o[3], height)) {
    return nullptr;
  }
  self->server->create_buffers(tp, num_buffers, width, height);
  Py_RETURN_NONE;
}

static PyObject *VisionIpcServer_create_buffers_with_sizes(VisionIpcServerObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"tp", "num_buffers", "width", "height", "size", "stride", "uv_offset", nullptr};
  PyObject *o[7] = {};
  VisionStreamType tp;
  size_t num_buffers, width, height, size, stride, uv_offset;
  if (!parse_args("create_buffers_with_sizes", args, nargs, kwnames, kwlist, 7, o) || !to_stream_type(o[0], tp) ||
      !to_size(o[1], num_buffers) || !to_size(o[2], width) || !to_size(o[3], height) || !to_size(o[4], size) ||
      !to_size(o[5], stride) || !to_size(o[6], uv_offset)) {
    return nullptr;
  }
  self->server->create_buffers_with_sizes(tp, num_buffers, width, height, size, stride, uv_offset);
  Py_RETURN_NONE;
}

static PyObject *VisionIpcServer_send(VisionIpcServerObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"tp", "data", "frame_id", "timestamp_sof", "timestamp_eof", nullptr};
  PyObject *o[5] = {};
  VisionStreamType tp;
  VisionIpcBufExtra extra = {};
  if (!parse_args("send", args, nargs, kwnames, kwlist, 2, o) || !to_stream_type(o[0], tp) || (o[2] && !to_stream_type(o[2], extra.frame_id)) ||
      (o[3] && !to_uint64(o[3], extra.timestamp_sof)) || (o[4] && !to_uint64(o[4], extra.timestamp_eof))) {
    return nullptr;
  }

  Py_buffer data;
  if (PyObject_GetBuffer(o[1], &data, PyBUF_SIMPLE) < 0) return nullptr;
  VisionBuf *buf = self->server->get_buffer(tp);
  if ((size_t)data.len != buf->len) {
    PyErr_Format(PyExc_ValueError, "data is %zd bytes, but the buffer is %zu bytes", data.len, buf->len);
    PyBuffer_Release(&data);
    return nullptr;
  }
  memcpy(buf->addr, data.buf, data.len);
  PyBuffer_Release(&data);

  buf->set_frame_id(extra.frame_id);
  self->server->send(buf, &extra, false);
  Py_RETURN_NONE;
}

static PyObject *VisionIpcServer_start_listener(VisionIpcServerObject *self, PyObject *) {
  self->server->start_listener();
  Py_RETURN_NONE;
}

static PyMethodDef VisionIpcServer_methods[] = {
  {"create_buffers", (PyCFunction)(void (*)(void))VisionIpcServer_create_buffers, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {"create_buffers_with_sizes", (PyCFunction)(void (*)(void))VisionIpcServer_create_buffers_with_sizes, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {"send", (PyCFunction)(void (*)(void))VisionIpcServer_send, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {"start_listener", (PyCFunction)VisionIpcServer_start_listener, METH_NOARGS, nullptr},
  {},
};

static PyType_Slot VisionIpcServer_slots[] = {
  {Py_tp_new, (void *)VisionIpcServer_new},
  {Py_tp_dealloc, (void *)VisionIpcServer_dealloc},
  {Py_tp_methods, VisionIpcServer_methods},
  {},
};

// *** VisionIpcClient ***

struct VisionIpcClientObject {
  PyObject_HEAD
  VisionIpcClient *client;
  VisionIpcBufExtra extra;
};

static PyObject *VisionIpcClient_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  static const char *kwlist[] = {"name", "stream", "conflate", nullptr};
  PyObject *o[3];
  std::string name;
  VisionStreamType stream;
  bool conflate;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OOO:VisionIpcClient", (char **)kwlist, &o[0], &o[1], &o[2]) ||
      !to_string(o[0], name) || !to_stream_type(o[1], stream) || !to_bool(o[2], conflate)) {
    return nullptr;
  }
  auto self = (VisionIpcClientObject *)alloc_object(type);
  if (self) self->client = new VisionIpcClient(name, stream, conflate);
  return (PyObject *)self;
}

static void VisionIpcClient_dealloc(PyObject *obj) {
  delete ((VisionIpcClientObject *)obj)->client;
  free_object(obj);
}

// properties of the first buffer, or None before connecting
static PyObject *buffer_prop(VisionIpcClientObject *self, size_t VisionBuf::*field) {
  if (!self->client->num_buffers) Py_RETURN_NONE;
  return PyLong_FromSize_t(self->client->buffers[0].*field);
}
static PyObject *VisionIpcClient_get_width(VisionIpcClientObject *self, void *) { return buffer_prop(self, &VisionBuf::width); }
static PyObject *VisionIpcClient_get_height(VisionIpcClientObject *self, void *) { return buffer_prop(self, &VisionBuf::height); }
static PyObject *VisionIpcClient_get_stride(VisionIpcClientObject *self, void *) { return buffer_prop(self, &VisionBuf::stride); }
static PyObject *VisionIpcClient_get_uv_offset(VisionIpcClientObject *self, void *) { return buffer_prop(self, &VisionBuf::uv_offset); }
static PyObject *VisionIpcClient_get_buffer_len(VisionIpcClientObject *self, void *) { return buffer_prop(self, &VisionBuf::len); }
static PyObject *VisionIpcClient_get_num_buffers(VisionIpcClientObject *self, void *) { return PyLong_FromLong(self->client->num_buffers); }
static PyObject *VisionIpcClient_get_frame_id(VisionIpcClientObject *self, void *) { return PyLong_FromUnsignedLong(self->extra.frame_id); }
static PyObject *VisionIpcClient_get_timestamp_sof(VisionIpcClientObject *self, void *) { return PyLong_FromUnsignedLongLong(self->extra.timestamp_sof); }
static PyObject *VisionIpcClient_get_timestamp_eof(VisionIpcClientObject *self, void *) { return PyLong_FromUnsignedLongLong(self->extra.timestamp_eof); }
static PyObject *VisionIpcClient_get_valid(VisionIpcClientObject *self, void *) { return PyBool_FromLong(self->extra.valid); }

static PyObject *VisionIpcClient_recv(VisionIpcClientObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"timeout_ms", nullptr};
  PyObject *o[1] = {};
  int timeout_ms = 100;
  if (!parse_args("recv", args, nargs, kwnames, kwlist, 0, o) || (o[0] && !to_int(o[0], timeout_ms))) return nullptr;

  VisionBuf *buf;
  Py_BEGIN_ALLOW_THREADS
  buf = self->client->recv(&self->extra, timeout_ms);
  Py_END_ALLOW_THREADS
  if (!buf) Py_RETURN_NONE;

  auto ret = (VisionBufObject *)alloc_object(VisionBufType);
  if (!ret) return nullptr;
  ret->buf = buf;
  ret->owner = Py_NewRef((PyObject *)self);
  return (PyObject *)ret;
}

static PyObject *VisionIpcClient_connect(VisionIpcClientObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"blocking", nullptr};
  PyObject *o[1] = {};
  bool blocking, ret;
  if (!parse_args("connect", args, nargs, kwnames, kwlist, 1, o) || !to_bool(o[0], blocking)) return nullptr;
  Py_BEGIN_ALLOW_THREADS
  ret = self->client->connect(blocking);
  Py_END_ALLOW_THREADS
  return PyBool_FromLong(ret);
}

static PyObject *VisionIpcClient_is_connected(VisionIpcClientObject *self, PyObject *) {
  return PyBool_FromLong(self->client->is_connected());
}

static PyObject *VisionIpcClient_available_streams(PyObject *, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  static const char *const kwlist[] = {"name", "block", nullptr};
  PyObject *o[2] = {};
  std::string name;
  bool block;
  if (!parse_args("available_streams", args, nargs, kwnames, kwlist, 2, o) || !to_string(o[0], name) || !to_bool(o[1], block)) return nullptr;

  std::set<VisionStreamType> streams;
  Py_BEGIN_ALLOW_THREADS
  streams = VisionIpcClient::getAvailableStreams(name, block);
  Py_END_ALLOW_THREADS

  PyObject *ret = PySet_New(nullptr);
  for (VisionStreamType stream : streams) {
    PyObject *item = PyLong_FromUnsignedLong(stream);
    if (!item || PySet_Add(ret, item) < 0) {
      Py_XDECREF(item);
      Py_DECREF(ret);
      return nullptr;
    }
    Py_DECREF(item);
  }
  return ret;
}

static PyGetSetDef VisionIpcClient_getset[] = {
  {"width", (getter)VisionIpcClient_get_width, nullptr, nullptr, nullptr},
  {"height", (getter)VisionIpcClient_get_height, nullptr, nullptr, nullptr},
  {"stride", (getter)VisionIpcClient_get_stride, nullptr, nullptr, nullptr},
  {"uv_offset", (getter)VisionIpcClient_get_uv_offset, nullptr, nullptr, nullptr},
  {"buffer_len", (getter)VisionIpcClient_get_buffer_len, nullptr, nullptr, nullptr},
  {"num_buffers", (getter)VisionIpcClient_get_num_buffers, nullptr, nullptr, nullptr},
  {"frame_id", (getter)VisionIpcClient_get_frame_id, nullptr, nullptr, nullptr},
  {"timestamp_sof", (getter)VisionIpcClient_get_timestamp_sof, nullptr, nullptr, nullptr},
  {"timestamp_eof", (getter)VisionIpcClient_get_timestamp_eof, nullptr, nullptr, nullptr},
  {"valid", (getter)VisionIpcClient_get_valid, nullptr, nullptr, nullptr},
  {},
};

static PyMethodDef VisionIpcClient_methods[] = {
  {"recv", (PyCFunction)(void (*)(void))VisionIpcClient_recv, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {"connect", (PyCFunction)(void (*)(void))VisionIpcClient_connect, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {"is_connected", (PyCFunction)VisionIpcClient_is_connected, METH_NOARGS, nullptr},
  {"available_streams", (PyCFunction)(void (*)(void))VisionIpcClient_available_streams, METH_FASTCALL | METH_KEYWORDS | METH_STATIC, nullptr},
  {},
};

static PyType_Slot VisionIpcClient_slots[] = {
  {Py_tp_new, (void *)VisionIpcClient_new},
  {Py_tp_dealloc, (void *)VisionIpcClient_dealloc},
  {Py_tp_methods, VisionIpcClient_methods},
  {Py_tp_getset, VisionIpcClient_getset},
  {},
};

static PyMethodDef module_methods[] = {
  {"get_endpoint_name", (PyCFunction)(void (*)(void))py_get_endpoint_name, METH_FASTCALL | METH_KEYWORDS, nullptr},
  {},
};

static PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "msgq.visionipc.visionipc_pyx", nullptr, -1, module_methods};

PyMODINIT_FUNC PyInit_visionipc_pyx(void) {
  PyObject *m = PyModule_Create(&module_def);
  if (!m) return nullptr;
  if (!add_type(m, "msgq.visionipc.visionipc_pyx.VisionBuf", sizeof(VisionBufObject), VisionBuf_slots, &VisionBufType,
                Py_TPFLAGS_DISALLOW_INSTANTIATION) ||
      !add_type(m, "msgq.visionipc.visionipc_pyx.VisionIpcServer", sizeof(VisionIpcServerObject), VisionIpcServer_slots, &VisionIpcServerType) ||
      !add_type(m, "msgq.visionipc.visionipc_pyx.VisionIpcClient", sizeof(VisionIpcClientObject), VisionIpcClient_slots, &VisionIpcClientType)) {
    Py_DECREF(m);
    return nullptr;
  }
  return m;
}
