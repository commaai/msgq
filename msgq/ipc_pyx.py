import ctypes
import errno
import os
import time
from typing import Optional

from ._ffi import lib, native_error_message


_void_p = ctypes.c_void_p
_size_t = ctypes.c_size_t

lib.msgq_context_create.argtypes = []
lib.msgq_context_create.restype = _void_p
lib.msgq_context_destroy.argtypes = [_void_p]
lib.msgq_context_destroy.restype = None

lib.msgq_subsocket_create.argtypes = []
lib.msgq_subsocket_create.restype = _void_p
lib.msgq_subsocket_destroy.argtypes = [_void_p]
lib.msgq_subsocket_destroy.restype = None
lib.msgq_subsocket_connect.argtypes = [_void_p, _void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, _size_t]
lib.msgq_subsocket_connect.restype = ctypes.c_int
lib.msgq_subsocket_set_timeout.argtypes = [_void_p, ctypes.c_int]
lib.msgq_subsocket_set_timeout.restype = None
lib.msgq_subsocket_receive.argtypes = [_void_p, ctypes.c_int, ctypes.POINTER(_void_p)]
lib.msgq_subsocket_receive.restype = ctypes.c_int

lib.msgq_message_data.argtypes = [_void_p]
lib.msgq_message_data.restype = _void_p
lib.msgq_message_size.argtypes = [_void_p]
lib.msgq_message_size.restype = _size_t
lib.msgq_message_destroy.argtypes = [_void_p]
lib.msgq_message_destroy.restype = None

lib.msgq_pubsocket_create.argtypes = []
lib.msgq_pubsocket_create.restype = _void_p
lib.msgq_pubsocket_destroy.argtypes = [_void_p]
lib.msgq_pubsocket_destroy.restype = None
lib.msgq_pubsocket_connect.argtypes = [_void_p, _void_p, ctypes.c_char_p, _size_t]
lib.msgq_pubsocket_connect.restype = ctypes.c_int
lib.msgq_pubsocket_send.argtypes = [_void_p, ctypes.c_char_p, _size_t]
lib.msgq_pubsocket_send.restype = ctypes.c_int
lib.msgq_pubsocket_all_readers_updated.argtypes = [_void_p]
lib.msgq_pubsocket_all_readers_updated.restype = ctypes.c_int

lib.msgq_poller_create.argtypes = []
lib.msgq_poller_create.restype = _void_p
lib.msgq_poller_destroy.argtypes = [_void_p]
lib.msgq_poller_destroy.restype = None
lib.msgq_poller_register_socket.argtypes = [_void_p, _void_p]
lib.msgq_poller_register_socket.restype = ctypes.c_int
lib.msgq_poller_poll.argtypes = [_void_p, ctypes.c_int]
lib.msgq_poller_poll.restype = ctypes.c_int
lib.msgq_poller_result.argtypes = [_void_p, _size_t]
lib.msgq_poller_result.restype = _void_p

lib.msgq_toggle_fake_events.argtypes = [ctypes.c_int]
lib.msgq_toggle_fake_events.restype = ctypes.c_int
lib.msgq_set_fake_prefix.argtypes = [ctypes.c_char_p]
lib.msgq_set_fake_prefix.restype = ctypes.c_int
lib.msgq_get_fake_prefix.argtypes = []
lib.msgq_get_fake_prefix.restype = ctypes.c_char_p

lib.msgq_socket_event_handle_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
lib.msgq_socket_event_handle_create.restype = _void_p
lib.msgq_socket_event_handle_destroy.argtypes = [_void_p]
lib.msgq_socket_event_handle_destroy.restype = None
lib.msgq_socket_event_handle_is_enabled.argtypes = [_void_p]
lib.msgq_socket_event_handle_is_enabled.restype = ctypes.c_int
lib.msgq_socket_event_handle_set_enabled.argtypes = [_void_p, ctypes.c_int]
lib.msgq_socket_event_handle_set_enabled.restype = None
lib.msgq_socket_event_handle_recv_called_fd.argtypes = [_void_p]
lib.msgq_socket_event_handle_recv_called_fd.restype = ctypes.c_int
lib.msgq_socket_event_handle_recv_ready_fd.argtypes = [_void_p]
lib.msgq_socket_event_handle_recv_ready_fd.restype = ctypes.c_int

lib.msgq_event_set.argtypes = [ctypes.c_int]
lib.msgq_event_set.restype = ctypes.c_int
lib.msgq_event_clear.argtypes = [ctypes.c_int]
lib.msgq_event_clear.restype = ctypes.c_int
lib.msgq_event_wait.argtypes = [ctypes.c_int, ctypes.c_int]
lib.msgq_event_wait.restype = ctypes.c_int
lib.msgq_event_peek.argtypes = [ctypes.c_int]
lib.msgq_event_peek.restype = ctypes.c_int
lib.msgq_event_wait_for_one.argtypes = [ctypes.POINTER(ctypes.c_int), _size_t, ctypes.c_int]
lib.msgq_event_wait_for_one.restype = ctypes.c_int


def _as_bytes(value: str | bytes, name: str) -> bytes:
  if isinstance(value, str):
    return value.encode("ascii")
  if isinstance(value, bytes):
    return value
  raise TypeError(f"{name} must be str or bytes")


def _require_pointer(pointer: Optional[int], object_name: str) -> int:
  if pointer is None:
    raise RuntimeError(f"{object_name} is closed")
  return pointer


def _raise_native() -> None:
  raise RuntimeError(native_error_message())


class IpcError(Exception):
  def __init__(self, endpoint: str | bytes | None = None, error_number: Optional[int] = None):
    error_number = ctypes.get_errno() if error_number is None else error_number
    endpoint_bytes = _as_bytes(endpoint, "endpoint") if endpoint is not None else None
    suffix = f"with {endpoint_bytes.decode('utf-8')}" if endpoint_bytes else ""
    detail = native_error_message() if lib.msgq_last_error() else os.strerror(error_number)
    super().__init__(f"Messaging failure {suffix}: {detail}")


class MultiplePublishersError(IpcError):
  pass


def _raise_ipc(endpoint: str | bytes | None = None) -> None:
  error_number = ctypes.get_errno()
  error_type = MultiplePublishersError if error_number == errno.EADDRINUSE else IpcError
  raise error_type(endpoint, error_number)


def toggle_fake_events(enabled: bool) -> None:
  if lib.msgq_toggle_fake_events(enabled) != 0:
    _raise_native()


def set_fake_prefix(prefix: str | bytes) -> None:
  if lib.msgq_set_fake_prefix(_as_bytes(prefix, "prefix")) != 0:
    _raise_native()


def get_fake_prefix() -> bytes:
  prefix = lib.msgq_get_fake_prefix()
  if prefix is None:
    _raise_native()
  return prefix


def delete_fake_prefix() -> None:
  set_fake_prefix(b"")


class Event:
  def __init__(self, fd: int = -1, owner: "SocketEventHandle | None" = None):
    self._fd = fd
    self._owner = owner

  def set(self) -> None:
    if lib.msgq_event_set(self._fd) != 0:
      _raise_native()

  def clear(self) -> int:
    result = lib.msgq_event_clear(self._fd)
    if result < 0:
      _raise_native()
    return result

  def wait(self, timeout: int = -1) -> None:
    if lib.msgq_event_wait(self._fd, timeout) != 0:
      _raise_native()

  def peek(self) -> bool:
    result = lib.msgq_event_peek(self._fd)
    if result < 0:
      _raise_native()
    return bool(result)

  @property
  def fd(self) -> int:
    return self._fd

  @property
  def ptr(self) -> int:
    return self._fd


def wait_for_one_event(events: list[Event], timeout: int = -1) -> int:
  fds = (ctypes.c_int * len(events))(*(event.fd for event in events))
  result = lib.msgq_event_wait_for_one(fds, len(events), timeout)
  if result < 0:
    _raise_native()
  return result


class SocketEventHandle:
  def __init__(self, endpoint: str | bytes, identifier: str | bytes, override: bool):
    self._ptr = lib.msgq_socket_event_handle_create(
      _as_bytes(endpoint, "endpoint"), _as_bytes(identifier, "identifier"), override,
    )
    if self._ptr is None:
      _raise_native()

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None:
      lib.msgq_socket_event_handle_destroy(pointer)
      self._ptr = None

  @property
  def enabled(self) -> bool:
    return bool(lib.msgq_socket_event_handle_is_enabled(_require_pointer(self._ptr, "SocketEventHandle")))

  @enabled.setter
  def enabled(self, value: bool) -> None:
    lib.msgq_socket_event_handle_set_enabled(_require_pointer(self._ptr, "SocketEventHandle"), value)

  @property
  def recv_called_event(self) -> Event:
    fd = lib.msgq_socket_event_handle_recv_called_fd(_require_pointer(self._ptr, "SocketEventHandle"))
    return Event(fd, self)

  @property
  def recv_ready_event(self) -> Event:
    fd = lib.msgq_socket_event_handle_recv_ready_fd(_require_pointer(self._ptr, "SocketEventHandle"))
    return Event(fd, self)


class Context:
  def __init__(self) -> None:
    self._ptr = lib.msgq_context_create()
    if self._ptr is None:
      _raise_native()

  def term(self) -> None:
    if self._ptr is not None:
      lib.msgq_context_destroy(self._ptr)
      self._ptr = None


class SubSocket:
  def __init__(self, _pointer: Optional[int] = None, _owner: bool = True, _owner_ref: object = None):
    self._ptr = lib.msgq_subsocket_create() if _pointer is None else _pointer
    self._owner = _owner
    self._owner_ref = _owner_ref
    if self._ptr is None:
      _raise_native()

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None and getattr(self, "_owner", False):
      lib.msgq_subsocket_destroy(pointer)
      self._ptr = None

  def connect(self, context: Context, endpoint: str | bytes, address: str | bytes = b"127.0.0.1",
              conflate: bool = False, segment_size: int = 0) -> None:
    context_pointer = _require_pointer(context._ptr, "Context")
    result = lib.msgq_subsocket_connect(
      _require_pointer(self._ptr, "SubSocket"), context_pointer, _as_bytes(endpoint, "endpoint"),
      _as_bytes(address, "address"), conflate, segment_size,
    )
    if result != 0:
      _raise_ipc(endpoint)

  def setTimeout(self, timeout: int) -> None:
    lib.msgq_subsocket_set_timeout(_require_pointer(self._ptr, "SubSocket"), timeout)

  def receive(self, non_blocking: bool = False) -> Optional[bytes]:
    message = _void_p()
    result = lib.msgq_subsocket_receive(
      _require_pointer(self._ptr, "SubSocket"), non_blocking, ctypes.byref(message),
    )
    if result < 0:
      _raise_native()
    if result == 0:
      return None

    try:
      size = lib.msgq_message_size(message)
      return ctypes.string_at(lib.msgq_message_data(message), size)
    finally:
      lib.msgq_message_destroy(message)


class PubSocket:
  def __init__(self) -> None:
    self._ptr = lib.msgq_pubsocket_create()
    if self._ptr is None:
      _raise_native()

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None:
      lib.msgq_pubsocket_destroy(pointer)
      self._ptr = None

  def connect(self, context: Context, endpoint: str | bytes, segment_size: int = 0) -> None:
    result = lib.msgq_pubsocket_connect(
      _require_pointer(self._ptr, "PubSocket"), _require_pointer(context._ptr, "Context"),
      _as_bytes(endpoint, "endpoint"), segment_size,
    )
    if result != 0:
      _raise_ipc(endpoint)

  def send(self, data: bytes) -> None:
    if not isinstance(data, bytes):
      raise TypeError("data must be bytes")
    result = lib.msgq_pubsocket_send(_require_pointer(self._ptr, "PubSocket"), data, len(data))
    if result != len(data):
      _raise_ipc()

  def all_readers_updated(self) -> bool:
    return bool(lib.msgq_pubsocket_all_readers_updated(_require_pointer(self._ptr, "PubSocket")))

  def wait_for_readers(self, timeout: float = 1.0, interval: float = 0.001) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
      if self.all_readers_updated():
        return
      time.sleep(interval)
    raise TimeoutError("subscriber did not connect")


class Poller:
  def __init__(self) -> None:
    self._ptr = lib.msgq_poller_create()
    self._sub_sockets: list[SubSocket] = []
    if self._ptr is None:
      _raise_native()

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None:
      lib.msgq_poller_destroy(pointer)
      self._ptr = None

  def registerSocket(self, socket: SubSocket) -> None:
    result = lib.msgq_poller_register_socket(
      _require_pointer(self._ptr, "Poller"), _require_pointer(socket._ptr, "SubSocket"),
    )
    if result != 0:
      _raise_native()
    self._sub_sockets.append(socket)

  def poll(self, timeout: int) -> list[SubSocket]:
    count = lib.msgq_poller_poll(_require_pointer(self._ptr, "Poller"), timeout)
    if count < 0:
      _raise_native()
    return [
      SubSocket(lib.msgq_poller_result(self._ptr, index), _owner=False, _owner_ref=self)
      for index in range(count)
    ]
