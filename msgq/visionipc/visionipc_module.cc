// Python bindings for visionipc
#include <cstdint>
#include <cstring>
#include <optional>
#include <set>
#include <string>

#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>

#include "msgq/visionipc/visionbuf.h"
#include "msgq/visionipc/visionipc.h"
#include "msgq/visionipc/visionipc_client.h"
#include "msgq/visionipc/visionipc_server.h"
#include "msgq/nb_helpers.h"

using namespace nb::literals;

struct PyVisionIpcClient {
  VisionIpcClient client;
  VisionIpcBufExtra extra = {};

  PyVisionIpcClient(const std::string &name, VisionStreamType stream, bool conflate) : client(name, stream, conflate) {}

  // properties of the first buffer, or None before connecting
  std::optional<size_t> buffer_prop(size_t VisionBuf::*field) const {
    if (!client.num_buffers) return std::nullopt;
    return client.buffers[0].*field;
  }
};

NB_MODULE(visionipc_pyx, m) {
  m.def("get_endpoint_name", [](Name name, VisionStreamType stream) { return get_endpoint_name(name.value, stream); }, "name"_a, "stream"_a);

  // VisionBufs are only created by VisionIpcClient.recv()
  nb::class_<VisionBuf>(m, "VisionBuf")
    .def_prop_ro("data", [](VisionBuf &self) {
      return nb::steal(PyMemoryView_FromMemory((char *)self.addr, self.len, PyBUF_WRITE));
    })
    .def_ro("width", &VisionBuf::width)
    .def_ro("height", &VisionBuf::height)
    .def_ro("stride", &VisionBuf::stride)
    .def_ro("uv_offset", &VisionBuf::uv_offset)
    .def_ro("idx", &VisionBuf::idx)
    .def_ro("fd", &VisionBuf::fd)
    .def_prop_ro("frame_id", &VisionBuf::get_frame_id);

  nb::class_<VisionIpcServer>(m, "VisionIpcServer")
    .def("__init__", [](VisionIpcServer *self, Name name) { new (self) VisionIpcServer(name.value); }, "name"_a)
    .def("create_buffers", &VisionIpcServer::create_buffers, "tp"_a, "num_buffers"_a, "width"_a, "height"_a)
    .def("create_buffers_with_sizes", &VisionIpcServer::create_buffers_with_sizes,
         "tp"_a, "num_buffers"_a, "width"_a, "height"_a, "size"_a, "stride"_a, "uv_offset"_a)
    .def("send", [](VisionIpcServer &self, VisionStreamType tp, nb::handle data, uint32_t frame_id, uint64_t timestamp_sof, uint64_t timestamp_eof) {
      Buffer src(data);
      VisionBuf *buf = self.get_buffer(tp);
      if (src.size() != buf->len) {
        throw nb::value_error(("data is " + std::to_string(src.size()) + " bytes, but the buffer is " + std::to_string(buf->len) + " bytes").c_str());
      }
      memcpy(buf->addr, src.data(), src.size());
      buf->set_frame_id(frame_id);

      VisionIpcBufExtra extra = {frame_id, timestamp_sof, timestamp_eof, false};
      self.send(buf, &extra, false);
    }, "tp"_a, "data"_a, "frame_id"_a = 0, "timestamp_sof"_a = 0, "timestamp_eof"_a = 0)
    .def("start_listener", &VisionIpcServer::start_listener);

  nb::class_<PyVisionIpcClient>(m, "VisionIpcClient")
    .def("__init__", [](PyVisionIpcClient *self, Name name, VisionStreamType stream, bool conflate) {
      new (self) PyVisionIpcClient(name.value, stream, conflate);
    }, "name"_a, "stream"_a, "conflate"_a)
    .def_prop_ro("width", [](const PyVisionIpcClient &self) { return self.buffer_prop(&VisionBuf::width); })
    .def_prop_ro("height", [](const PyVisionIpcClient &self) { return self.buffer_prop(&VisionBuf::height); })
    .def_prop_ro("stride", [](const PyVisionIpcClient &self) { return self.buffer_prop(&VisionBuf::stride); })
    .def_prop_ro("uv_offset", [](const PyVisionIpcClient &self) { return self.buffer_prop(&VisionBuf::uv_offset); })
    .def_prop_ro("buffer_len", [](const PyVisionIpcClient &self) { return self.buffer_prop(&VisionBuf::len); })
    .def_prop_ro("num_buffers", [](const PyVisionIpcClient &self) { return self.client.num_buffers; })
    .def_prop_ro("frame_id", [](const PyVisionIpcClient &self) { return self.extra.frame_id; })
    .def_prop_ro("timestamp_sof", [](const PyVisionIpcClient &self) { return self.extra.timestamp_sof; })
    .def_prop_ro("timestamp_eof", [](const PyVisionIpcClient &self) { return self.extra.timestamp_eof; })
    .def_prop_ro("valid", [](const PyVisionIpcClient &self) { return self.extra.valid; })
    // the returned VisionBuf keeps the client alive
    .def("recv", [](PyVisionIpcClient &self, int timeout_ms) { return self.client.recv(&self.extra, timeout_ms); },
         "timeout_ms"_a = 100, nb::rv_policy::reference_internal, nb::call_guard<nb::gil_scoped_release>())
    .def("connect", [](PyVisionIpcClient &self, bool blocking) { return self.client.connect(blocking); },
         "blocking"_a, nb::call_guard<nb::gil_scoped_release>())
    .def("is_connected", [](PyVisionIpcClient &self) { return self.client.is_connected(); })
    .def_static("available_streams", [](Name name, bool block) { return VisionIpcClient::getAvailableStreams(name.value, block); },
                "name"_a, "block"_a, nb::call_guard<nb::gil_scoped_release>());
}
