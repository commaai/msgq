#pragma once

// Helpers shared by the nanobind Python modules.

#include <string>

#include <nanobind/nanobind.h>

namespace nb = nanobind;

// msgq accepts endpoint names as either str or bytes
struct Name {
  std::string value;
};

namespace nanobind::detail {
template <>
struct type_caster<Name> {
  NB_TYPE_CASTER(Name, const_name("str | bytes"))

  bool from_python(handle src, uint8_t, cleanup_list *) noexcept {
    Py_ssize_t len;
    if (PyBytes_Check(src.ptr())) {
      char *data;
      if (PyBytes_AsStringAndSize(src.ptr(), &data, &len) < 0) return false;
      value.value.assign(data, len);
      return true;
    }
    if (PyUnicode_Check(src.ptr())) {
      const char *data = PyUnicode_AsUTF8AndSize(src.ptr(), &len);
      if (!data) {
        PyErr_Clear();
        return false;
      }
      value.value.assign(data, len);
      return true;
    }
    return false;
  }
};
}  // namespace nanobind::detail

// A contiguous view of any object supporting the buffer protocol
class Buffer {
 public:
  explicit Buffer(nb::handle obj) {
    if (PyObject_GetBuffer(obj.ptr(), &view, PyBUF_SIMPLE) < 0) throw nb::python_error();
  }
  ~Buffer() { PyBuffer_Release(&view); }
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  char *data() const { return (char *)view.buf; }
  size_t size() const { return view.len; }

 private:
  Py_buffer view;
};
