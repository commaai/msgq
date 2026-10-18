#pragma once

// Small helpers shared by the limited API (abi3) Python modules. Include after Python.h.

#include <climits>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <exception>
#include <string>

inline PyObject *alloc_object(PyTypeObject *type) {
  return ((allocfunc)PyType_GetSlot(type, Py_tp_alloc))(type, 0);
}

inline void free_object(PyObject *obj) {
  PyTypeObject *type = Py_TYPE(obj);
  ((freefunc)PyType_GetSlot(type, Py_tp_free))(obj);
  Py_DECREF(type);
}

// Adds a heap type named after the last component of its fully qualified name
inline bool add_type(PyObject *module, const char *qualname, int size, PyType_Slot *slots, PyTypeObject **out,
                     unsigned int flags = Py_TPFLAGS_BASETYPE) {
  PyType_Spec spec = {qualname, size, 0, Py_TPFLAGS_DEFAULT | flags, slots};
  PyObject *type = PyType_FromSpec(&spec);
  if (!type) return false;
  *out = (PyTypeObject *)type;
  const char *name = strrchr(qualname, '.');
  return PyModule_AddObjectRef(module, name ? name + 1 : qualname, type) == 0;
}

// Parses METH_FASTCALL | METH_KEYWORDS arguments into out, which must be zeroed and hold one slot per kwlist entry
inline bool parse_args(const char *fname, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames,
                       const char *const *kwlist, Py_ssize_t required, PyObject **out) {
  Py_ssize_t n = 0;
  while (kwlist[n]) n++;
  if (nargs > n) {
    PyErr_Format(PyExc_TypeError, "%s() takes at most %zd positional arguments (%zd given)", fname, n, nargs);
    return false;
  }
  for (Py_ssize_t i = 0; i < nargs; i++) out[i] = args[i];

  Py_ssize_t nkw = kwnames ? PyTuple_Size(kwnames) : 0;
  for (Py_ssize_t i = 0; i < nkw; i++) {
    PyObject *key = PyTuple_GetItem(kwnames, i);
    Py_ssize_t j = 0;
    while (j < n && PyUnicode_CompareWithASCIIString(key, kwlist[j]) != 0) j++;
    if (j == n) {
      PyErr_Format(PyExc_TypeError, "%s() got an unexpected keyword argument '%U'", fname, key);
      return false;
    }
    if (out[j]) {
      PyErr_Format(PyExc_TypeError, "%s() got multiple values for argument '%s'", fname, kwlist[j]);
      return false;
    }
    out[j] = args[nargs + i];
  }

  for (Py_ssize_t i = 0; i < required; i++) {
    if (!out[i]) {
      PyErr_Format(PyExc_TypeError, "%s() missing required argument '%s'", fname, kwlist[i]);
      return false;
    }
  }
  return true;
}

inline bool to_string(PyObject *obj, std::string &out) {
  const char *data;
  Py_ssize_t len;
  if (PyBytes_Check(obj)) {
    char *buf;
    if (PyBytes_AsStringAndSize(obj, &buf, &len) < 0) return false;
    data = buf;
  } else if (PyUnicode_Check(obj)) {
    if (!(data = PyUnicode_AsUTF8AndSize(obj, &len))) return false;
  } else if (PyByteArray_Check(obj)) {
    data = PyByteArray_AsString(obj);
    len = PyByteArray_Size(obj);
  } else {
    PyErr_SetString(PyExc_TypeError, "expected str or bytes");
    return false;
  }
  out.assign(data, len);
  return true;
}

inline bool to_bool(PyObject *obj, bool &out) {
  int r = PyObject_IsTrue(obj);
  if (r < 0) return false;
  out = r;
  return true;
}

inline bool to_int(PyObject *obj, int &out) {
  long v = PyLong_AsLong(obj);
  if (v == -1 && PyErr_Occurred()) return false;
  if (v < INT_MIN || v > INT_MAX) {
    PyErr_SetString(PyExc_OverflowError, "value too large to convert to int");
    return false;
  }
  out = (int)v;
  return true;
}

inline bool to_size(PyObject *obj, size_t &out) {
  size_t v = PyLong_AsSize_t(obj);
  if (v == (size_t)-1 && PyErr_Occurred()) return false;
  out = v;
  return true;
}

inline bool to_uint64(PyObject *obj, uint64_t &out) {
  unsigned long long v = PyLong_AsUnsignedLongLong(obj);
  if (v == (unsigned long long)-1 && PyErr_Occurred()) return false;
  out = v;
  return true;
}

inline bool to_double(PyObject *obj, double &out) {
  double v = PyFloat_AsDouble(obj);
  if (v == -1.0 && PyErr_Occurred()) return false;
  out = v;
  return true;
}

inline double monotonic() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Calls f, translating C++ exceptions into RuntimeError
template <typename F>
PyObject *call_cpp(F f) {
  try {
    return f();
  } catch (const std::exception &e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return nullptr;
  }
}

// Calls f with the GIL released
template <typename F>
void release_gil(F f) {
  PyThreadState *state = PyEval_SaveThread();
  f();
  PyEval_RestoreThread(state);
}

// Calls f with the GIL released, translating C++ exceptions into RuntimeError
template <typename F>
bool without_gil(F f) {
  std::string error;
  bool ok = true;
  release_gil([&] {
    try {
      f();
    } catch (const std::exception &e) {
      error = e.what();
      ok = false;
    }
  });
  if (!ok) PyErr_SetString(PyExc_RuntimeError, error.c_str());
  return ok;
}
