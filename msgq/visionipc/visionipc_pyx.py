from enum import IntEnum

from msgq.visionipc._visionipc_cffi_api import ffi, lib  # ty: ignore[unresolved-import]


def _bytes(value):
  return value.encode() if isinstance(value, str) else value


class VisionStreamType(IntEnum):
  VISION_STREAM_ROAD = 0
  VISION_STREAM_DRIVER = 1
  VISION_STREAM_WIDE_ROAD = 2
  VISION_STREAM_MAP = 3


def get_endpoint_name(name, stream):
  size = lib.vipc_get_endpoint_name(_bytes(name), stream, ffi.NULL, 0)
  output = ffi.new("char[]", size)
  lib.vipc_get_endpoint_name(_bytes(name), stream, output, size)
  return bytes(ffi.buffer(output, size)).decode()


class VisionBuf:
  def __init__(self, buffer):
    self._buffer = buffer

  @property
  def data(self):
    return memoryview(ffi.buffer(lib.vipc_buffer_data(self._buffer), lib.vipc_buffer_len(self._buffer)))

  @property
  def width(self): return lib.vipc_buffer_width(self._buffer)
  @property
  def height(self): return lib.vipc_buffer_height(self._buffer)
  @property
  def stride(self): return lib.vipc_buffer_stride(self._buffer)
  @property
  def uv_offset(self): return lib.vipc_buffer_uv_offset(self._buffer)
  @property
  def idx(self): return lib.vipc_buffer_idx(self._buffer)
  @property
  def fd(self): return lib.vipc_buffer_fd(self._buffer)
  @property
  def frame_id(self): return lib.vipc_buffer_frame_id(self._buffer)


class VisionIpcServer:
  def __init__(self, name):
    self._server = ffi.gc(lib.vipc_server_create(_bytes(name)), lib.vipc_server_delete)

  def create_buffers(self, stream, num_buffers, width, height):
    lib.vipc_server_create_buffers(self._server, stream, num_buffers, width, height)

  def create_buffers_with_sizes(self, stream, num_buffers, width, height, size, stride, uv_offset):
    lib.vipc_server_create_buffers_with_sizes(self._server, stream, num_buffers, width, height, size, stride, uv_offset)

  def send(self, stream, data, frame_id=0, timestamp_sof=0, timestamp_eof=0):
    view = ffi.from_buffer("const unsigned char[]", data)
    if lib.vipc_server_send(self._server, stream, view, len(data), frame_id, timestamp_sof, timestamp_eof) != 0:
      raise AssertionError("buffer size mismatch")

  def start_listener(self):
    lib.vipc_server_start_listener(self._server)


class VisionIpcClient:
  def __init__(self, name, stream, conflate):
    self._client = ffi.gc(lib.vipc_client_create(_bytes(name), stream, conflate), lib.vipc_client_delete)

  def _first_buffer_value(self, function):
    return function(lib.vipc_client_buffer(self._client, 0)) if self.num_buffers else None

  @property
  def width(self): return self._first_buffer_value(lib.vipc_buffer_width)
  @property
  def height(self): return self._first_buffer_value(lib.vipc_buffer_height)
  @property
  def stride(self): return self._first_buffer_value(lib.vipc_buffer_stride)
  @property
  def uv_offset(self): return self._first_buffer_value(lib.vipc_buffer_uv_offset)
  @property
  def buffer_len(self): return self._first_buffer_value(lib.vipc_buffer_len)
  @property
  def num_buffers(self): return lib.vipc_client_num_buffers(self._client)
  @property
  def frame_id(self): return lib.vipc_client_frame_id(self._client)
  @property
  def timestamp_sof(self): return lib.vipc_client_timestamp_sof(self._client)
  @property
  def timestamp_eof(self): return lib.vipc_client_timestamp_eof(self._client)
  @property
  def valid(self): return bool(lib.vipc_client_valid(self._client))

  def recv(self, timeout_ms=100):
    buffer = lib.vipc_client_recv(self._client, timeout_ms)
    return None if buffer == ffi.NULL else VisionBuf(buffer)

  def connect(self, blocking): return bool(lib.vipc_client_connect(self._client, blocking))
  def is_connected(self): return bool(lib.vipc_client_is_connected(self._client))

  @staticmethod
  def available_streams(name, block):
    mask = lib.vipc_client_available_streams(_bytes(name), block)
    return {stream.value for stream in VisionStreamType if mask & (1 << stream.value)}
