import ctypes
import weakref
from enum import IntEnum
from pathlib import Path


lib = ctypes.CDLL(str(Path(__file__).with_name("libvisionipc_ctypes.so")))
c_void_p = ctypes.c_void_p
c_size_t = ctypes.c_size_t
c_int = ctypes.c_int
c_char_p = ctypes.c_char_p
c_uint64 = ctypes.c_uint64

lib.vipc_get_endpoint_name.argtypes = (c_char_p, c_int, c_void_p, c_size_t)
lib.vipc_get_endpoint_name.restype = c_size_t
lib.vipc_server_create.argtypes = (c_char_p,)
lib.vipc_server_create.restype = c_void_p
lib.vipc_server_delete.argtypes = (c_void_p,)
lib.vipc_server_create_buffers.argtypes = (c_void_p, c_int, c_size_t, c_size_t, c_size_t)
lib.vipc_server_create_buffers_with_sizes.argtypes = (c_void_p, c_int, c_size_t, c_size_t, c_size_t, c_size_t, c_size_t, c_size_t)
lib.vipc_server_send.argtypes = (c_void_p, c_int, c_void_p, c_size_t, ctypes.c_uint32, c_uint64, c_uint64)
lib.vipc_server_start_listener.argtypes = (c_void_p,)
lib.vipc_client_create.argtypes = (c_char_p, c_int, c_int)
lib.vipc_client_create.restype = c_void_p
lib.vipc_client_delete.argtypes = (c_void_p,)
lib.vipc_client_connect.argtypes = (c_void_p, c_int)
lib.vipc_client_is_connected.argtypes = (c_void_p,)
lib.vipc_client_recv.argtypes = (c_void_p, c_int)
lib.vipc_client_recv.restype = c_void_p
lib.vipc_client_available_streams.argtypes = (c_char_p, c_int)
lib.vipc_client_available_streams.restype = c_uint64
lib.vipc_client_num_buffers.argtypes = (c_void_p,)
lib.vipc_client_num_buffers.restype = c_size_t
lib.vipc_client_buffer.argtypes = (c_void_p, c_size_t)
lib.vipc_client_buffer.restype = c_void_p
for name in ("data",):
  getattr(lib, f"vipc_buffer_{name}").argtypes = (c_void_p,)
  getattr(lib, f"vipc_buffer_{name}").restype = c_void_p
for name in ("len", "width", "height", "stride", "uv_offset", "idx"):
  getattr(lib, f"vipc_buffer_{name}").argtypes = (c_void_p,)
  getattr(lib, f"vipc_buffer_{name}").restype = c_size_t
lib.vipc_buffer_fd.argtypes = (c_void_p,)
lib.vipc_buffer_frame_id.argtypes = (c_void_p,)
lib.vipc_buffer_frame_id.restype = c_uint64
for name in ("frame_id", "timestamp_sof", "timestamp_eof", "valid"):
  getattr(lib, f"vipc_client_{name}").argtypes = (c_void_p,)


def _bytes(value):
  return value.encode() if isinstance(value, str) else value


class VisionStreamType(IntEnum):
  VISION_STREAM_ROAD = 0
  VISION_STREAM_DRIVER = 1
  VISION_STREAM_WIDE_ROAD = 2
  VISION_STREAM_MAP = 3


def get_endpoint_name(name, stream):
  encoded_name = _bytes(name)
  size = lib.vipc_get_endpoint_name(encoded_name, stream, None, 0)
  output = ctypes.create_string_buffer(size)
  lib.vipc_get_endpoint_name(encoded_name, stream, output, size)
  return output.raw.decode()


class VisionBuf:
  def __init__(self, buffer): self._buffer = buffer
  @property
  def data(self):
    size = lib.vipc_buffer_len(self._buffer)
    return memoryview((ctypes.c_ubyte * size).from_address(lib.vipc_buffer_data(self._buffer)))
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
    self._server = lib.vipc_server_create(_bytes(name))
    self._finalizer = weakref.finalize(self, lib.vipc_server_delete, self._server)
  def create_buffers(self, stream, num_buffers, width, height):
    lib.vipc_server_create_buffers(self._server, stream, num_buffers, width, height)
  def create_buffers_with_sizes(self, stream, num_buffers, width, height, size, stride, uv_offset):
    lib.vipc_server_create_buffers_with_sizes(self._server, stream, num_buffers, width, height, size, stride, uv_offset)
  def send(self, stream, data, frame_id=0, timestamp_sof=0, timestamp_eof=0):
    if isinstance(data, bytes):
      source = ctypes.c_char_p(data)
    else:
      source = (ctypes.c_ubyte * len(data)).from_buffer(data)
    if lib.vipc_server_send(self._server, stream, source, len(data), frame_id, timestamp_sof, timestamp_eof) != 0:
      raise AssertionError("buffer size mismatch")
  def start_listener(self): lib.vipc_server_start_listener(self._server)


class VisionIpcClient:
  def __init__(self, name, stream, conflate):
    self._client = lib.vipc_client_create(_bytes(name), stream, conflate)
    self._finalizer = weakref.finalize(self, lib.vipc_client_delete, self._client)
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
    return None if not buffer else VisionBuf(buffer)
  def connect(self, blocking): return bool(lib.vipc_client_connect(self._client, blocking))
  def is_connected(self): return bool(lib.vipc_client_is_connected(self._client))
  @staticmethod
  def available_streams(name, block):
    mask = lib.vipc_client_available_streams(_bytes(name), block)
    return {stream.value for stream in VisionStreamType if mask & (1 << stream.value)}
