#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <vector>
#include <string>
#include <set>

#include "msgq/visionipc/visionipc.h"
#include "msgq/visionipc/visionipc_client.h"
#include "msgq/visionipc/visionipc_server.h"
#include "msgq/visionipc/visionbuf.h"

// Global Type Pointers
static PyTypeObject *CLContextType = NULL;
static PyTypeObject *VisionBufType = NULL;
static PyTypeObject *VisionIpcServerType = NULL;
static PyTypeObject *VisionIpcClientType = NULL;

// --- CLContext ---
typedef struct {
  PyObject_HEAD
  cl_device_id device_id;
  cl_context context;
} CLContext;

static void CLContext_dealloc(CLContext *self) {
  if (self->context) {
    clReleaseContext(self->context);
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static int CLContext_init(CLContext *self, PyObject *args, PyObject *kwds) {
  self->device_id = nullptr;
  self->context = nullptr;
  return 0;
}

static PyObject *CLContext_get_device_id(CLContext *self, void *closure) {
  return PyLong_FromUnsignedLongLong((unsigned long long)self->device_id);
}

static int CLContext_set_device_id(CLContext *self, PyObject *value, void *closure) {
  unsigned long long val = PyLong_AsUnsignedLongLong(value);
  if (PyErr_Occurred()) return -1;
  self->device_id = (cl_device_id)val;
  return 0;
}

static PyObject *CLContext_get_context(CLContext *self, void *closure) {
  return PyLong_FromUnsignedLongLong((unsigned long long)self->context);
}

static int CLContext_set_context(CLContext *self, PyObject *value, void *closure) {
  unsigned long long val = PyLong_AsUnsignedLongLong(value);
  if (PyErr_Occurred()) return -1;
  self->context = (cl_context)val;
  return 0;
}

static PyGetSetDef CLContext_getset[] = {
    {"device_id", (getter)CLContext_get_device_id, (setter)CLContext_set_device_id, "OpenCL Device ID", NULL},
    {"context", (getter)CLContext_get_context, (setter)CLContext_set_context, "OpenCL Context", NULL},
    {NULL}
};

static PyType_Slot CLContext_slots[] = {
    {Py_tp_dealloc, (void*)CLContext_dealloc},
    {Py_tp_init, (void*)CLContext_init},
    {Py_tp_getset, CLContext_getset},
    {Py_tp_new, (void*)PyType_GenericNew},
    {0, 0}
};

static PyType_Spec CLContext_spec = {
    "msgq.visionipc._visionipc_module.CLContext",
    sizeof(CLContext),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    CLContext_slots
};


// --- VisionBuf ---
typedef struct {
  PyObject_HEAD
  VisionBuf *buf;
  bool owner;
} PyVisionBuf;

static void VisionBuf_dealloc(PyVisionBuf *self) {
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *VisionBuf_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyVisionBuf *self = (PyVisionBuf *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->buf = NULL;
        self->owner = false;
    }
    return (PyObject *)self;
}

static PyObject *VisionBuf_get_width(PyVisionBuf *self, void *closure) { return PyLong_FromSize_t(self->buf->width); }
static PyObject *VisionBuf_get_height(PyVisionBuf *self, void *closure) { return PyLong_FromSize_t(self->buf->height); }
static PyObject *VisionBuf_get_stride(PyVisionBuf *self, void *closure) { return PyLong_FromSize_t(self->buf->stride); }
static PyObject *VisionBuf_get_uv_offset(PyVisionBuf *self, void *closure) { return PyLong_FromSize_t(self->buf->uv_offset); }
static PyObject *VisionBuf_get_idx(PyVisionBuf *self, void *closure) { return PyLong_FromSize_t(self->buf->idx); }
static PyObject *VisionBuf_get_fd(PyVisionBuf *self, void *closure) { return PyLong_FromLong(self->buf->fd); }
static PyObject *VisionBuf_get_server_id(PyVisionBuf *self, void *closure) { return PyLong_FromUnsignedLongLong(self->buf->server_id); }
static PyObject *VisionBuf_get_addr(PyVisionBuf *self, void *closure) { return PyLong_FromUnsignedLongLong((unsigned long long)self->buf->addr); }
static PyObject *VisionBuf_get_buf_cl(PyVisionBuf *self, void *closure) { return PyLong_FromUnsignedLongLong((unsigned long long)self->buf->buf_cl); }

static int VisionBuf_getbuffer(PyVisionBuf *self, Py_buffer *view, int flags) {
    if (self->buf == NULL || self->buf->addr == NULL) {
        PyErr_SetString(PyExc_ValueError, "VisionBuf is not initialized or invalid");
        return -1;
    }
    return PyBuffer_FillInfo(view, (PyObject*)self, self->buf->addr, self->buf->len, 0, flags);
}

static PyObject *numpy_module = NULL;

static PyObject *VisionBuf_get_data(PyVisionBuf *self, void *closure) {
     if (!self->buf) Py_RETURN_NONE;
     PyObject *view = PyMemoryView_FromObject((PyObject*)self);
     if (!view) return NULL;

     if (!numpy_module) {
         numpy_module = PyImport_ImportModule("numpy");
         if (!numpy_module) {
             Py_DECREF(view);
             return NULL;
         }
     }

     PyObject *dtype_str = PyUnicode_FromString("uint8");
     PyObject *args = PyTuple_Pack(2, view, dtype_str);
     PyObject *ret = PyObject_CallMethod(numpy_module, "frombuffer", "OO", view, dtype_str);

     Py_DECREF(view);
     Py_DECREF(dtype_str);
     Py_DECREF(args);
     return ret;
}

static PyGetSetDef VisionBuf_getset_extended[] = {
   {"width", (getter)VisionBuf_get_width, NULL, "width", NULL},
    {"height", (getter)VisionBuf_get_height, NULL, "height", NULL},
    {"stride", (getter)VisionBuf_get_stride, NULL, "stride", NULL},
    {"uv_offset", (getter)VisionBuf_get_uv_offset, NULL, "uv_offset", NULL},
    {"idx", (getter)VisionBuf_get_idx, NULL, "idx", NULL},
    {"fd", (getter)VisionBuf_get_fd, NULL, "fd", NULL},
    {"server_id", (getter)VisionBuf_get_server_id, NULL, "server_id", NULL},
    {"addr", (getter)VisionBuf_get_addr, NULL, "Address pointer", NULL},
    {"buf_cl", (getter)VisionBuf_get_buf_cl, NULL, "OpenCL Buffer pointer", NULL},
    {"data", (getter)VisionBuf_get_data, NULL, "MemoryView of data", NULL},
    {NULL}
};

static PyType_Slot VisionBuf_slots[] = {
    {Py_tp_dealloc, (void*)VisionBuf_dealloc},
    {Py_tp_new, (void*)VisionBuf_new},
    {Py_tp_getset, VisionBuf_getset_extended},
    {Py_bf_getbuffer, (void*)VisionBuf_getbuffer},
    {0, 0}
};

static PyType_Spec VisionBuf_spec = {
    "msgq.visionipc._visionipc_module.VisionBuf",
    sizeof(PyVisionBuf),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    VisionBuf_slots
};

static PyObject* wrap_VisionBuf(VisionBuf *cbuf) {
   PyVisionBuf *obj = (PyVisionBuf*)PyObject_New(PyVisionBuf, VisionBufType);
   if (obj) {
       obj->buf = cbuf;
       obj->owner = false;
   }
   return (PyObject*)obj;
}


// --- VisionIpcServer ---
typedef struct {
  PyObject_HEAD
  VisionIpcServer *server;
} PyVisionIpcServer;

static void VisionIpcServer_dealloc(PyVisionIpcServer *self) {
  if (self->server) delete self->server;
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static int VisionIpcServer_init(PyVisionIpcServer *self, PyObject *args, PyObject *kwds) {
  char *name;
  if (!PyArg_ParseTuple(args, "s", &name)) return -1;
  self->server = new VisionIpcServer(name);
  return 0;
}

static PyObject *VisionIpcServer_create_buffers(PyVisionIpcServer *self, PyObject *args) {
  int type;
  size_t num_buffers, width, height;
  if (!PyArg_ParseTuple(args, "ikkk", &type, &num_buffers, &width, &height)) return NULL;

  Py_BEGIN_ALLOW_THREADS
  self->server->create_buffers((VisionStreamType)type, num_buffers, width, height);
  Py_END_ALLOW_THREADS
  Py_RETURN_NONE;
}

static PyObject *VisionIpcServer_create_buffers_with_sizes(PyVisionIpcServer *self, PyObject *args) {
  int type;
  size_t num_buffers, width, height, size, stride, uv_offset;
  if (!PyArg_ParseTuple(args, "ikkkkkk", &type, &num_buffers, &width, &height, &size, &stride, &uv_offset)) return NULL;

  Py_BEGIN_ALLOW_THREADS
  self->server->create_buffers_with_sizes((VisionStreamType)type, num_buffers, width, height, size, stride, uv_offset);
  Py_END_ALLOW_THREADS
  Py_RETURN_NONE;
}

static PyObject *VisionIpcServer_send(PyVisionIpcServer *self, PyObject *args, PyObject *kwds) {
  int type;
  Py_buffer data;
  unsigned int frame_id = 0;
  double timestamp_sof = 0.0;
  double timestamp_eof = 0.0;

  static char *kwlist[] = {(char*)"tp", (char*)"data", (char*)"frame_id", (char*)"timestamp_sof", (char*)"timestamp_eof", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "iy*|Idd", kwlist, &type, &data, &frame_id, &timestamp_sof, &timestamp_eof)) return NULL;

  VisionBuf *buf = self->server->get_buffer((VisionStreamType)type);
  if (!buf) {
      PyBuffer_Release(&data);
      PyErr_SetString(PyExc_RuntimeError, "Failed to get buffer");
      return NULL;
  }

  if (data.len != (Py_ssize_t)buf->len) {
      PyBuffer_Release(&data);
      PyErr_SetString(PyExc_ValueError, "Data length mismatch");
      return NULL;
  }

  memcpy(buf->addr, data.buf, data.len);
  PyBuffer_Release(&data);

  buf->set_frame_id(frame_id);

  VisionIpcBufExtra extra;
  extra.frame_id = frame_id;
  extra.timestamp_sof = (unsigned long long)timestamp_sof;
  extra.timestamp_eof = (unsigned long long)timestamp_eof;
  extra.valid = true;

  Py_BEGIN_ALLOW_THREADS
  self->server->send(buf, &extra, false);
  Py_END_ALLOW_THREADS

  Py_RETURN_NONE;
}

static PyObject *VisionIpcServer_start_listener(PyVisionIpcServer *self, PyObject *args) {
  Py_BEGIN_ALLOW_THREADS
  self->server->start_listener();
  Py_END_ALLOW_THREADS
  Py_RETURN_NONE;
}

static PyMethodDef VisionIpcServer_methods[] = {
    {"create_buffers", (PyCFunction)VisionIpcServer_create_buffers, METH_VARARGS, ""},
    {"create_buffers_with_sizes", (PyCFunction)VisionIpcServer_create_buffers_with_sizes, METH_VARARGS, ""},
    {"send", (PyCFunction)VisionIpcServer_send, METH_VARARGS|METH_KEYWORDS, ""},
    {"start_listener", (PyCFunction)VisionIpcServer_start_listener, METH_VARARGS, ""},
    {NULL}
};

static PyType_Slot VisionIpcServer_slots[] = {
    {Py_tp_dealloc, (void*)VisionIpcServer_dealloc},
    {Py_tp_init, (void*)VisionIpcServer_init},
    {Py_tp_methods, VisionIpcServer_methods},
    {Py_tp_new, (void*)PyType_GenericNew},
    {0, 0}
};

static PyType_Spec VisionIpcServer_spec = {
    "msgq.visionipc._visionipc_module.VisionIpcServer",
    sizeof(PyVisionIpcServer),
    0,
    Py_TPFLAGS_DEFAULT,
    VisionIpcServer_slots
};


// --- VisionIpcClient ---
typedef struct {
  PyObject_HEAD
  VisionIpcClient *client;
  VisionIpcBufExtra extra;
} PyVisionIpcClient;

static void VisionIpcClient_dealloc(PyVisionIpcClient *self) {
  if (self->client) delete self->client;
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static int VisionIpcClient_init(PyVisionIpcClient *self, PyObject *args, PyObject *kwds) {
  char *name;
  int type;
  int conflate;
  PyObject *context = Py_None;

  static char *kwlist[] = {(char*)"name", (char*)"stream", (char*)"conflate", (char*)"context", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "sii|O", kwlist, &name, &type, &conflate, &context)) return -1;

  cl_device_id did = nullptr;
  cl_context ctx = nullptr;

  if (context != Py_None) {
      if (PyObject_IsInstance(context, (PyObject*)CLContextType)) {
          CLContext *cl = (CLContext*)context;
          did = cl->device_id;
          ctx = cl->context;
      } else {
          PyErr_SetString(PyExc_TypeError, "context must be CLContext");
          return -1;
      }
  }

  Py_BEGIN_ALLOW_THREADS
  self->client = new VisionIpcClient(name, (VisionStreamType)type, (bool)conflate, did, ctx);
  Py_END_ALLOW_THREADS // cppcheck-suppress unknownMacro
  return 0;
}

static PyObject *VisionIpcClient_recv(PyVisionIpcClient *self, PyObject *args, PyObject *kwds) {
  int timeout_ms = 100;
  static char *kwlist[] = {(char*)"timeout_ms", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &timeout_ms)) return NULL;

  VisionBuf *buf = nullptr;
  Py_BEGIN_ALLOW_THREADS
  buf = self->client->recv(&self->extra, timeout_ms);
  Py_END_ALLOW_THREADS

  if (!buf) Py_RETURN_NONE;
  return wrap_VisionBuf(buf);
}

static PyObject *VisionIpcClient_connect(PyVisionIpcClient *self, PyObject *args) {
  int blocking = 1;
  if (!PyArg_ParseTuple(args, "p", &blocking)) return NULL;
  bool res;
  Py_BEGIN_ALLOW_THREADS
  res = self->client->connect(blocking);
  Py_END_ALLOW_THREADS
  return PyBool_FromLong(res);
}

static PyObject *VisionIpcClient_is_connected(PyVisionIpcClient *self, PyObject *args) {
  return PyBool_FromLong(self->client->is_connected());
}

static PyObject *VisionIpcClient_available_streams(PyObject *type, PyObject *args) {
    char *name;
    int block = 1;
    if (!PyArg_ParseTuple(args, "s|p", &name, &block)) return NULL;

    std::set<VisionStreamType> streams;
    Py_BEGIN_ALLOW_THREADS
    streams = VisionIpcClient::getAvailableStreams(name, block);
    Py_END_ALLOW_THREADS

    PyObject *set = PySet_New(NULL);
    for (auto s : streams) {
        PyObject *val = PyLong_FromLong(s);
        PySet_Add(set, val);
        Py_DECREF(val);
    }
    return set;
}

static PyObject *Client_get_width(PyVisionIpcClient *self, void *closure) { return self->client->num_buffers ? PyLong_FromSize_t(self->client->buffers[0].width) : Py_NewRef(Py_None); }
static PyObject *Client_get_height(PyVisionIpcClient *self, void *closure) { return self->client->num_buffers ? PyLong_FromSize_t(self->client->buffers[0].height) : Py_NewRef(Py_None); }
static PyObject *Client_get_stride(PyVisionIpcClient *self, void *closure) { return self->client->num_buffers ? PyLong_FromSize_t(self->client->buffers[0].stride) : Py_NewRef(Py_None); }
static PyObject *Client_get_uv_offset(PyVisionIpcClient *self, void *closure) { return self->client->num_buffers ? PyLong_FromSize_t(self->client->buffers[0].uv_offset) : Py_NewRef(Py_None); }
static PyObject *Client_get_buffer_len(PyVisionIpcClient *self, void *closure) { return self->client->num_buffers ? PyLong_FromSize_t(self->client->buffers[0].len) : Py_NewRef(Py_None); }
static PyObject *Client_get_num_buffers(PyVisionIpcClient *self, void *closure) { return PyLong_FromLong(self->client->num_buffers); }

static PyObject *Client_get_frame_id(PyVisionIpcClient *self, void *closure) { return PyLong_FromUnsignedLong(self->extra.frame_id); }
static PyObject *Client_get_timestamp_sof(PyVisionIpcClient *self, void *closure) { return PyLong_FromUnsignedLongLong(self->extra.timestamp_sof); }
static PyObject *Client_get_timestamp_eof(PyVisionIpcClient *self, void *closure) { return PyLong_FromUnsignedLongLong(self->extra.timestamp_eof); }
static PyObject *Client_get_valid(PyVisionIpcClient *self, void *closure) { return PyBool_FromLong(self->extra.valid); }


static PyGetSetDef VisionIpcClient_getset[] = {
    {"width", (getter)Client_get_width, NULL, "", NULL},
    {"height", (getter)Client_get_height, NULL, "", NULL},
    {"stride", (getter)Client_get_stride, NULL, "", NULL},
    {"uv_offset", (getter)Client_get_uv_offset, NULL, "", NULL},
    {"buffer_len", (getter)Client_get_buffer_len, NULL, "", NULL},
    {"num_buffers", (getter)Client_get_num_buffers, NULL, "", NULL},
    {"frame_id", (getter)Client_get_frame_id, NULL, "", NULL},
    {"timestamp_sof", (getter)Client_get_timestamp_sof, NULL, "", NULL},
    {"timestamp_eof", (getter)Client_get_timestamp_eof, NULL, "", NULL},
    {"valid", (getter)Client_get_valid, NULL, "", NULL},
    {NULL}
};

static PyMethodDef VisionIpcClient_methods[] = {
    {"recv", (PyCFunction)VisionIpcClient_recv, METH_VARARGS|METH_KEYWORDS, ""},
    {"connect", (PyCFunction)VisionIpcClient_connect, METH_VARARGS, ""},
    {"is_connected", (PyCFunction)VisionIpcClient_is_connected, METH_NOARGS, ""},
    {"available_streams", (PyCFunction)VisionIpcClient_available_streams, METH_VARARGS|METH_STATIC, ""},
    {NULL}
};

static PyType_Slot VisionIpcClient_slots[] = {
    {Py_tp_dealloc, (void*)VisionIpcClient_dealloc},
    {Py_tp_init, (void*)VisionIpcClient_init},
    {Py_tp_methods, VisionIpcClient_methods},
    {Py_tp_getset, VisionIpcClient_getset},
    {Py_tp_new, (void*)PyType_GenericNew},
    {0, 0}
};

static PyType_Spec VisionIpcClient_spec = {
    "msgq.visionipc._visionipc_module.VisionIpcClient",
    sizeof(PyVisionIpcClient),
    0,
    Py_TPFLAGS_DEFAULT,
    VisionIpcClient_slots
};


// --- Module Functions ---
static PyObject *func_get_endpoint_name(PyObject *self, PyObject *args) {
    char *name;
    int stream;
    if (!PyArg_ParseTuple(args, "si", &name, &stream)) return NULL;
    std::string res = get_endpoint_name(name, (VisionStreamType)stream);
    return PyUnicode_FromString(res.c_str());
}

static PyMethodDef module_methods[] = {
    {"get_endpoint_name", (PyCFunction)func_get_endpoint_name, METH_VARARGS, ""},
    {NULL}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "_visionipc_module",
    NULL,
    -1,
    module_methods
};

PyMODINIT_FUNC PyInit__visionipc_module(void) {
    PyObject *m;

    m = PyModule_Create(&module);
    if (m == NULL) return NULL;

    CLContextType = (PyTypeObject*)PyType_FromSpec(&CLContext_spec);
    if (!CLContextType) return NULL;
    Py_INCREF(CLContextType);
    if (PyModule_AddObject(m, "CLContext", (PyObject *)CLContextType) < 0) { Py_DECREF(CLContextType); Py_DECREF(m); return NULL; }

    VisionBufType = (PyTypeObject*)PyType_FromSpec(&VisionBuf_spec);
    if (!VisionBufType) return NULL;
    Py_INCREF(VisionBufType);
    if (PyModule_AddObject(m, "VisionBuf", (PyObject *)VisionBufType) < 0) { Py_DECREF(VisionBufType); Py_DECREF(m); return NULL; }

    VisionIpcServerType = (PyTypeObject*)PyType_FromSpec(&VisionIpcServer_spec);
    if (!VisionIpcServerType) return NULL;
    Py_INCREF(VisionIpcServerType);
    if (PyModule_AddObject(m, "VisionIpcServer", (PyObject *)VisionIpcServerType) < 0) { Py_DECREF(VisionIpcServerType); Py_DECREF(m); return NULL; }

    VisionIpcClientType = (PyTypeObject*)PyType_FromSpec(&VisionIpcClient_spec);
    if (!VisionIpcClientType) return NULL;
    Py_INCREF(VisionIpcClientType);
    if (PyModule_AddObject(m, "VisionIpcClient", (PyObject *)VisionIpcClientType) < 0) { Py_DECREF(VisionIpcClientType); Py_DECREF(m); return NULL; }

    PyModule_AddIntConstant(m, "VISION_STREAM_ROAD", VISION_STREAM_ROAD);
    PyModule_AddIntConstant(m, "VISION_STREAM_DRIVER", VISION_STREAM_DRIVER);
    PyModule_AddIntConstant(m, "VISION_STREAM_WIDE_ROAD", VISION_STREAM_WIDE_ROAD);
    PyModule_AddIntConstant(m, "VISION_STREAM_MAP", VISION_STREAM_MAP);

    const char *enum_code =
        "import enum\n"
        "class VisionStreamType(enum.IntEnum):\n"
        "    VISION_STREAM_ROAD = 0\n"
        "    VISION_STREAM_DRIVER = 1\n"
        "    VISION_STREAM_WIDE_ROAD = 2\n"
        "    VISION_STREAM_MAP = 3\n";

    PyObject *globals = PyDict_New();
    PyObject *res = PyRun_String(enum_code, Py_file_input, globals, globals);
    if (!res) {
        Py_DECREF(globals);
        return NULL;
    }
    Py_DECREF(res);

    PyObject *vst = PyDict_GetItemString(globals, "VisionStreamType");
    if (vst) {
        Py_INCREF(vst);
        PyModule_AddObject(m, "VisionStreamType", vst);
    }
    Py_DECREF(globals);

    return m;
}
