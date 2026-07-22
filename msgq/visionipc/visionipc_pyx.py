import ctypes
from enum import IntEnum
from typing import Optional

from msgq._ffi import lib, native_error_message


_void_p = ctypes.c_void_p
_size_t = ctypes.c_size_t


class _VisionIpcBufExtra(ctypes.Structure):
  _fields_ = [
    ("frame_id", ctypes.c_uint32),
    ("timestamp_sof", ctypes.c_uint64),
    ("timestamp_eof", ctypes.c_uint64),
    ("valid", ctypes.c_int),
  ]


lib.visionipc_get_endpoint_name.argtypes = [ctypes.c_char_p, ctypes.c_int]
lib.visionipc_get_endpoint_name.restype = ctypes.c_char_p

lib.visionipc_server_create.argtypes = [ctypes.c_char_p]
lib.visionipc_server_create.restype = _void_p
lib.visionipc_server_destroy.argtypes = [_void_p]
lib.visionipc_server_destroy.restype = None
lib.visionipc_server_create_buffers.argtypes = [_void_p, ctypes.c_int, _size_t, _size_t, _size_t]
lib.visionipc_server_create_buffers.restype = ctypes.c_int
lib.visionipc_server_create_buffers_with_sizes.argtypes = [
  _void_p, ctypes.c_int, _size_t, _size_t, _size_t, _size_t, _size_t, _size_t,
]
lib.visionipc_server_create_buffers_with_sizes.restype = ctypes.c_int
lib.visionipc_server_send.argtypes = [
  _void_p, ctypes.c_int, ctypes.c_char_p, _size_t, ctypes.c_uint32, ctypes.c_uint64, ctypes.c_uint64,
]
lib.visionipc_server_send.restype = ctypes.c_int
lib.visionipc_server_start_listener.argtypes = [_void_p]
lib.visionipc_server_start_listener.restype = ctypes.c_int

lib.visionipc_client_create.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
lib.visionipc_client_create.restype = _void_p
lib.visionipc_client_destroy.argtypes = [_void_p]
lib.visionipc_client_destroy.restype = None
lib.visionipc_client_connect.argtypes = [_void_p, ctypes.c_int]
lib.visionipc_client_connect.restype = ctypes.c_int
lib.visionipc_client_is_connected.argtypes = [_void_p]
lib.visionipc_client_is_connected.restype = ctypes.c_int
lib.visionipc_client_num_buffers.argtypes = [_void_p]
lib.visionipc_client_num_buffers.restype = ctypes.c_int
lib.visionipc_client_buffer.argtypes = [_void_p, _size_t]
lib.visionipc_client_buffer.restype = _void_p
lib.visionipc_client_recv.argtypes = [_void_p, ctypes.POINTER(_VisionIpcBufExtra), ctypes.c_int]
lib.visionipc_client_recv.restype = _void_p
lib.visionipc_client_available_streams.argtypes = [ctypes.c_char_p, ctypes.c_int]
lib.visionipc_client_available_streams.restype = ctypes.c_uint32

lib.visionipc_buffer_data.argtypes = [_void_p]
lib.visionipc_buffer_data.restype = _void_p
lib.visionipc_buffer_len.argtypes = [_void_p]
lib.visionipc_buffer_len.restype = _size_t
lib.visionipc_buffer_width.argtypes = [_void_p]
lib.visionipc_buffer_width.restype = _size_t
lib.visionipc_buffer_height.argtypes = [_void_p]
lib.visionipc_buffer_height.restype = _size_t
lib.visionipc_buffer_stride.argtypes = [_void_p]
lib.visionipc_buffer_stride.restype = _size_t
lib.visionipc_buffer_uv_offset.argtypes = [_void_p]
lib.visionipc_buffer_uv_offset.restype = _size_t
lib.visionipc_buffer_index.argtypes = [_void_p]
lib.visionipc_buffer_index.restype = _size_t
lib.visionipc_buffer_fd.argtypes = [_void_p]
lib.visionipc_buffer_fd.restype = ctypes.c_int
lib.visionipc_buffer_frame_id.argtypes = [_void_p]
lib.visionipc_buffer_frame_id.restype = ctypes.c_uint64


def _as_bytes(value: str | bytes, name: str) -> bytes:
  if isinstance(value, str):
    return value.encode("ascii")
  if isinstance(value, bytes):
    return value
  raise TypeError(f"{name} must be str or bytes")


def _stream_value(stream: "VisionStreamType | int") -> int:
  return int(stream)


def _raise_native() -> None:
  raise RuntimeError(native_error_message())


class VisionStreamType(IntEnum):
  VISION_STREAM_ROAD = 0
  VISION_STREAM_DRIVER = 1
  VISION_STREAM_WIDE_ROAD = 2
  VISION_STREAM_MAP = 3


def get_endpoint_name(name: str | bytes, stream: VisionStreamType | int) -> str:
  result = lib.visionipc_get_endpoint_name(_as_bytes(name, "name"), _stream_value(stream))
  if result is None:
    _raise_native()
  return result.decode("utf-8")


class VisionBuf:
  def __init__(self, pointer: int, owner: "VisionIpcClient"):
    self._ptr = pointer
    self._owner = owner

  @property
  def data(self) -> memoryview:
    size = int(lib.visionipc_buffer_len(self._ptr))
    address = lib.visionipc_buffer_data(self._ptr)
    array = (ctypes.c_ubyte * size).from_address(address)
    return memoryview(array).cast("B")

  @property
  def width(self) -> int:
    return int(lib.visionipc_buffer_width(self._ptr))

  @property
  def height(self) -> int:
    return int(lib.visionipc_buffer_height(self._ptr))

  @property
  def stride(self) -> int:
    return int(lib.visionipc_buffer_stride(self._ptr))

  @property
  def uv_offset(self) -> int:
    return int(lib.visionipc_buffer_uv_offset(self._ptr))

  @property
  def idx(self) -> int:
    return int(lib.visionipc_buffer_index(self._ptr))

  @property
  def fd(self) -> int:
    return int(lib.visionipc_buffer_fd(self._ptr))

  @property
  def frame_id(self) -> int:
    return int(lib.visionipc_buffer_frame_id(self._ptr))


class VisionIpcServer:
  def __init__(self, name: str | bytes):
    self._ptr = lib.visionipc_server_create(_as_bytes(name, "name"))
    if self._ptr is None:
      _raise_native()

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None:
      lib.visionipc_server_destroy(pointer)
      self._ptr = None

  def create_buffers(self, stream: VisionStreamType | int, num_buffers: int, width: int, height: int) -> None:
    result = lib.visionipc_server_create_buffers(self._ptr, _stream_value(stream), num_buffers, width, height)
    if result != 0:
      _raise_native()

  def create_buffers_with_sizes(self, stream: VisionStreamType | int, num_buffers: int, width: int,
                                height: int, size: int, stride: int, uv_offset: int) -> None:
    result = lib.visionipc_server_create_buffers_with_sizes(
      self._ptr, _stream_value(stream), num_buffers, width, height, size, stride, uv_offset,
    )
    if result != 0:
      _raise_native()

  def send(self, stream: VisionStreamType | int, data: bytes | bytearray | memoryview, frame_id: int = 0,
           timestamp_sof: int = 0, timestamp_eof: int = 0) -> None:
    payload = bytes(data)
    result = lib.visionipc_server_send(
      self._ptr, _stream_value(stream), payload, len(payload), frame_id, timestamp_sof, timestamp_eof,
    )
    if result != 0:
      _raise_native()

  def start_listener(self) -> None:
    if lib.visionipc_server_start_listener(self._ptr) != 0:
      _raise_native()


class VisionIpcClient:
  def __init__(self, name: str | bytes, stream: VisionStreamType | int, conflate: bool):
    self._ptr = lib.visionipc_client_create(_as_bytes(name, "name"), _stream_value(stream), conflate)
    self._extra = _VisionIpcBufExtra()
    if self._ptr is None:
      _raise_native()

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None:
      lib.visionipc_client_destroy(pointer)
      self._ptr = None

  def _first_buffer(self) -> Optional[VisionBuf]:
    pointer = lib.visionipc_client_buffer(self._ptr, 0)
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
    return int(lib.visionipc_buffer_len(buffer._ptr))

  @property
  def num_buffers(self) -> int:
    return int(lib.visionipc_client_num_buffers(self._ptr))

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
    pointer = lib.visionipc_client_recv(self._ptr, ctypes.byref(self._extra), timeout_ms)
    if pointer is None:
      if lib.msgq_last_error():
        _raise_native()
      return None
    return VisionBuf(pointer, self)

  def connect(self, blocking: bool) -> bool:
    result = lib.visionipc_client_connect(self._ptr, blocking)
    if result < 0:
      _raise_native()
    return bool(result)

  def is_connected(self) -> bool:
    return bool(lib.visionipc_client_is_connected(self._ptr))

  @staticmethod
  def available_streams(name: str | bytes, block: bool) -> set[int]:
    mask = lib.visionipc_client_available_streams(_as_bytes(name, "name"), block)
    if lib.msgq_last_error():
      _raise_native()
    return {stream.value for stream in VisionStreamType if mask & (1 << stream.value)}
