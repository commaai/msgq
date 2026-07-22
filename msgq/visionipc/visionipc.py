import ctypes
from enum import IntEnum
from typing import Optional

from msgq._ffi import bind


Handle = ctypes.c_void_p


class _VisionIpcBufExtra(ctypes.Structure):
  _fields_ = [
    ("frame_id", ctypes.c_uint32),
    ("timestamp_sof", ctypes.c_uint64),
    ("timestamp_eof", ctypes.c_uint64),
    ("valid", ctypes.c_int),
  ]


get_endpoint_name_native = bind("visionipc_get_endpoint_name", [ctypes.c_char_p, ctypes.c_int], ctypes.c_char_p)
server_create = bind("visionipc_server_create", [ctypes.c_char_p], Handle)
server_destroy = bind("visionipc_server_destroy", [Handle])
server_create_buffers = bind("visionipc_server_create_buffers", [Handle, ctypes.c_int, ctypes.c_size_t, ctypes.c_size_t, ctypes.c_size_t])
server_create_buffers_with_sizes = bind("visionipc_server_create_buffers_with_sizes", [
  Handle, ctypes.c_int, ctypes.c_size_t, ctypes.c_size_t, ctypes.c_size_t,
  ctypes.c_size_t, ctypes.c_size_t, ctypes.c_size_t,
])
server_send = bind("visionipc_server_send", [Handle, ctypes.c_int, ctypes.c_char_p, ctypes.c_size_t, ctypes.c_uint32, ctypes.c_uint64, ctypes.c_uint64])
server_start_listener = bind("visionipc_server_start_listener", [Handle])
client_create = bind("visionipc_client_create", [ctypes.c_char_p, ctypes.c_int, ctypes.c_int], Handle)
client_destroy = bind("visionipc_client_destroy", [Handle])
client_connect = bind("visionipc_client_connect", [Handle, ctypes.c_int], ctypes.c_int)
client_is_connected = bind("visionipc_client_is_connected", [Handle], ctypes.c_int)
client_num_buffers = bind("visionipc_client_num_buffers", [Handle], ctypes.c_int)
client_buffer = bind("visionipc_client_buffer", [Handle, ctypes.c_size_t], Handle)
client_recv = bind("visionipc_client_recv", [Handle, ctypes.POINTER(_VisionIpcBufExtra), ctypes.c_int], Handle)
client_available_streams = bind("visionipc_client_available_streams", [ctypes.c_char_p, ctypes.c_int], ctypes.c_uint32)
buffer_data = bind("visionipc_buffer_data", [Handle], Handle)
buffer_len = bind("visionipc_buffer_len", [Handle], ctypes.c_size_t)
buffer_width = bind("visionipc_buffer_width", [Handle], ctypes.c_size_t)
buffer_height = bind("visionipc_buffer_height", [Handle], ctypes.c_size_t)
buffer_stride = bind("visionipc_buffer_stride", [Handle], ctypes.c_size_t)
buffer_uv_offset = bind("visionipc_buffer_uv_offset", [Handle], ctypes.c_size_t)
buffer_index = bind("visionipc_buffer_index", [Handle], ctypes.c_size_t)
buffer_fd = bind("visionipc_buffer_fd", [Handle], ctypes.c_int)
buffer_frame_id = bind("visionipc_buffer_frame_id", [Handle], ctypes.c_uint64)


def _as_bytes(value: str | bytes, name: str) -> bytes:
  if isinstance(value, str):
    return value.encode("ascii")
  if isinstance(value, bytes):
    return value
  raise TypeError(f"{name} must be str or bytes")


def _stream_value(stream: "VisionStreamType | int") -> int:
  return int(stream)


class VisionStreamType(IntEnum):
  VISION_STREAM_ROAD = 0
  VISION_STREAM_DRIVER = 1
  VISION_STREAM_WIDE_ROAD = 2
  VISION_STREAM_MAP = 3


def get_endpoint_name(name: str | bytes, stream: VisionStreamType | int) -> str:
  result = get_endpoint_name_native(_as_bytes(name, "name"), _stream_value(stream))
  return result.decode("utf-8")


class VisionBuf:
  def __init__(self, pointer: int, owner: "VisionIpcClient"):
    self._ptr = pointer
    self._owner = owner

  @property
  def data(self) -> memoryview:
    size = int(buffer_len(self._ptr))
    address = buffer_data(self._ptr)
    array = (ctypes.c_ubyte * size).from_address(address)
    return memoryview(array).cast("B")

  @property
  def width(self) -> int:
    return int(buffer_width(self._ptr))

  @property
  def height(self) -> int:
    return int(buffer_height(self._ptr))

  @property
  def stride(self) -> int:
    return int(buffer_stride(self._ptr))

  @property
  def uv_offset(self) -> int:
    return int(buffer_uv_offset(self._ptr))

  @property
  def idx(self) -> int:
    return int(buffer_index(self._ptr))

  @property
  def fd(self) -> int:
    return int(buffer_fd(self._ptr))

  @property
  def frame_id(self) -> int:
    return int(buffer_frame_id(self._ptr))


class VisionIpcServer:
  def __init__(self, name: str | bytes):
    self._ptr = server_create(_as_bytes(name, "name"))

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None:
      server_destroy(pointer)
      self._ptr = None

  def create_buffers(self, stream: VisionStreamType | int, num_buffers: int, width: int, height: int) -> None:
    server_create_buffers(self._ptr, _stream_value(stream), num_buffers, width, height)

  def create_buffers_with_sizes(self, stream: VisionStreamType | int, num_buffers: int, width: int,
                                height: int, size: int, stride: int, uv_offset: int) -> None:
    server_create_buffers_with_sizes(
      self._ptr, _stream_value(stream), num_buffers, width, height, size, stride, uv_offset,
    )

  def send(self, stream: VisionStreamType | int, data: bytes | bytearray | memoryview, frame_id: int = 0,
           timestamp_sof: int = 0, timestamp_eof: int = 0) -> None:
    payload = bytes(data)
    server_send(
      self._ptr, _stream_value(stream), payload, len(payload), frame_id, timestamp_sof, timestamp_eof,
    )

  def start_listener(self) -> None:
    server_start_listener(self._ptr)


class VisionIpcClient:
  def __init__(self, name: str | bytes, stream: VisionStreamType | int, conflate: bool):
    self._ptr = client_create(_as_bytes(name, "name"), _stream_value(stream), conflate)
    self._extra = _VisionIpcBufExtra()

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None:
      client_destroy(pointer)
      self._ptr = None

  def _first_buffer(self) -> Optional[VisionBuf]:
    pointer = client_buffer(self._ptr, 0)
    return None if pointer is None else VisionBuf(pointer, self)

  @property
  def width(self) -> Optional[int]:
    buffer = self._first_buffer()
    return None if buffer is None else buffer.width

  @property
  def height(self) -> Optional[int]:
    buffer = self._first_buffer()
    return None if buffer is None else buffer.height

  @property
  def stride(self) -> Optional[int]:
    buffer = self._first_buffer()
    return None if buffer is None else buffer.stride

  @property
  def uv_offset(self) -> Optional[int]:
    buffer = self._first_buffer()
    return None if buffer is None else buffer.uv_offset

  @property
  def buffer_len(self) -> Optional[int]:
    buffer = self._first_buffer()
    return None if buffer is None else self._get_buffer_len(buffer)

  @staticmethod
  def _get_buffer_len(buffer: VisionBuf) -> int:
    return int(buffer_len(buffer._ptr))

  @property
  def num_buffers(self) -> int:
    return int(client_num_buffers(self._ptr))

  @property
  def frame_id(self) -> int:
    return int(self._extra.frame_id)

  @property
  def timestamp_sof(self) -> int:
    return int(self._extra.timestamp_sof)

  @property
  def timestamp_eof(self) -> int:
    return int(self._extra.timestamp_eof)

  @property
  def valid(self) -> bool:
    return bool(self._extra.valid)

  def recv(self, timeout_ms: int = 100) -> Optional[VisionBuf]:
    pointer = client_recv(self._ptr, ctypes.byref(self._extra), timeout_ms)
    if pointer is None:
      return None
    return VisionBuf(pointer, self)

  def connect(self, blocking: bool) -> bool:
    return bool(client_connect(self._ptr, blocking))

  def is_connected(self) -> bool:
    return bool(client_is_connected(self._ptr))

  @staticmethod
  def available_streams(name: str | bytes, block: bool) -> set[int]:
    mask = client_available_streams(_as_bytes(name, "name"), block)
    return {stream.value for stream in VisionStreamType if mask & (1 << stream.value)}
