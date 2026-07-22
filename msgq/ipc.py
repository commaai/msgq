import ctypes
import errno
import os
import time
from typing import Optional

from ._ffi import bind, msgq_last_error


Handle = ctypes.c_void_p

context_create = bind("msgq_context_create", [], Handle)
context_destroy = bind("msgq_context_destroy", [Handle])
subsocket_create = bind("msgq_subsocket_create", [], Handle)
subsocket_destroy = bind("msgq_subsocket_destroy", [Handle])
subsocket_connect = bind("msgq_subsocket_connect", [Handle, Handle, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_size_t], ctypes.c_int)
subsocket_set_timeout = bind("msgq_subsocket_set_timeout", [Handle, ctypes.c_int])
subsocket_receive = bind("msgq_subsocket_receive", [Handle, ctypes.c_int], Handle)
message_data = bind("msgq_message_data", [Handle], Handle)
message_size = bind("msgq_message_size", [Handle], ctypes.c_size_t)
message_destroy = bind("msgq_message_destroy", [Handle])
pubsocket_create = bind("msgq_pubsocket_create", [], Handle)
pubsocket_destroy = bind("msgq_pubsocket_destroy", [Handle])
pubsocket_connect = bind("msgq_pubsocket_connect", [Handle, Handle, ctypes.c_char_p, ctypes.c_size_t], ctypes.c_int)
pubsocket_send = bind("msgq_pubsocket_send", [Handle, ctypes.c_char_p, ctypes.c_size_t], ctypes.c_int)
pubsocket_all_readers_updated = bind("msgq_pubsocket_all_readers_updated", [Handle], ctypes.c_int)
poller_create = bind("msgq_poller_create", [], Handle)
poller_destroy = bind("msgq_poller_destroy", [Handle])
poller_register_socket = bind("msgq_poller_register_socket", [Handle, Handle])
poller_poll = bind("msgq_poller_poll", [Handle, ctypes.c_int], ctypes.c_size_t)
poller_result = bind("msgq_poller_result", [Handle, ctypes.c_size_t], Handle)
toggle_fake_events_native = bind("msgq_toggle_fake_events", [ctypes.c_int])
set_fake_prefix_native = bind("msgq_set_fake_prefix", [ctypes.c_char_p])
get_fake_prefix_native = bind("msgq_get_fake_prefix", [], ctypes.c_char_p)
socket_event_handle_create = bind("msgq_socket_event_handle_create", [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int], Handle)
socket_event_handle_destroy = bind("msgq_socket_event_handle_destroy", [Handle])
socket_event_handle_is_enabled = bind("msgq_socket_event_handle_is_enabled", [Handle], ctypes.c_int)
socket_event_handle_set_enabled = bind("msgq_socket_event_handle_set_enabled", [Handle, ctypes.c_int])
socket_event_handle_recv_called_fd = bind("msgq_socket_event_handle_recv_called_fd", [Handle], ctypes.c_int)
socket_event_handle_recv_ready_fd = bind("msgq_socket_event_handle_recv_ready_fd", [Handle], ctypes.c_int)
event_set = bind("msgq_event_set", [ctypes.c_int])
event_clear = bind("msgq_event_clear", [ctypes.c_int], ctypes.c_int)
event_wait = bind("msgq_event_wait", [ctypes.c_int, ctypes.c_int])
event_peek = bind("msgq_event_peek", [ctypes.c_int], ctypes.c_int)
event_wait_for_one = bind("msgq_event_wait_for_one", [ctypes.POINTER(ctypes.c_int), ctypes.c_size_t, ctypes.c_int], ctypes.c_int)


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


class IpcError(Exception):
  def __init__(self, endpoint: str | bytes | None = None, error_number: Optional[int] = None):
    error_number = ctypes.get_errno() if error_number is None else error_number
    endpoint_bytes = _as_bytes(endpoint, "endpoint") if endpoint is not None else None
    suffix = f"with {endpoint_bytes.decode('utf-8')}" if endpoint_bytes else ""
    error = msgq_last_error()
    detail = error.decode("utf-8", errors="replace") if error else os.strerror(error_number)
    super().__init__(f"Messaging failure {suffix}: {detail}")


class MultiplePublishersError(IpcError):
  pass


def _raise_ipc(endpoint: str | bytes | None = None) -> None:
  error_number = ctypes.get_errno()
  error_type = MultiplePublishersError if error_number == errno.EADDRINUSE else IpcError
  raise error_type(endpoint, error_number)


def toggle_fake_events(enabled: bool) -> None:
  toggle_fake_events_native(enabled)


def set_fake_prefix(prefix: str | bytes) -> None:
  set_fake_prefix_native(_as_bytes(prefix, "prefix"))


def get_fake_prefix() -> bytes:
  return get_fake_prefix_native()


def delete_fake_prefix() -> None:
  set_fake_prefix(b"")


class Event:
  def __init__(self, fd: int = -1, owner: "SocketEventHandle | None" = None):
    self._fd = fd
    self._owner = owner

  def set(self) -> None:
    event_set(self._fd)

  def clear(self) -> int:
    return event_clear(self._fd)

  def wait(self, timeout: int = -1) -> None:
    event_wait(self._fd, timeout)

  def peek(self) -> bool:
    return bool(event_peek(self._fd))

  @property
  def fd(self) -> int:
    return self._fd

  @property
  def ptr(self) -> int:
    return self._fd


def wait_for_one_event(events: list[Event], timeout: int = -1) -> int:
  fds = (ctypes.c_int * len(events))(*(event.fd for event in events))
  return event_wait_for_one(fds, len(events), timeout)


class SocketEventHandle:
  def __init__(self, endpoint: str | bytes, identifier: str | bytes, override: bool):
    self._ptr = socket_event_handle_create(
      _as_bytes(endpoint, "endpoint"), _as_bytes(identifier, "identifier"), override,
    )

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None:
      socket_event_handle_destroy(pointer)
      self._ptr = None

  @property
  def enabled(self) -> bool:
    return bool(socket_event_handle_is_enabled(_require_pointer(self._ptr, "SocketEventHandle")))

  @enabled.setter
  def enabled(self, value: bool) -> None:
    socket_event_handle_set_enabled(_require_pointer(self._ptr, "SocketEventHandle"), value)

  @property
  def recv_called_event(self) -> Event:
    fd = socket_event_handle_recv_called_fd(_require_pointer(self._ptr, "SocketEventHandle"))
    return Event(fd, self)

  @property
  def recv_ready_event(self) -> Event:
    fd = socket_event_handle_recv_ready_fd(_require_pointer(self._ptr, "SocketEventHandle"))
    return Event(fd, self)


class Context:
  def __init__(self) -> None:
    self._ptr = context_create()

  def term(self) -> None:
    if self._ptr is not None:
      context_destroy(self._ptr)
      self._ptr = None


class SubSocket:
  def __init__(self, _pointer: Optional[int] = None, _owner: bool = True, _owner_ref: object = None):
    self._ptr = subsocket_create() if _pointer is None else _pointer
    self._owner = _owner
    self._owner_ref = _owner_ref

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None and getattr(self, "_owner", False):
      subsocket_destroy(pointer)
      self._ptr = None

  def connect(self, context: Context, endpoint: str | bytes, address: str | bytes = b"127.0.0.1",
              conflate: bool = False, segment_size: int = 0) -> None:
    context_pointer = _require_pointer(context._ptr, "Context")
    result = subsocket_connect(
      _require_pointer(self._ptr, "SubSocket"), context_pointer, _as_bytes(endpoint, "endpoint"),
      _as_bytes(address, "address"), conflate, segment_size,
    )
    if result != 0:
      _raise_ipc(endpoint)

  def setTimeout(self, timeout: int) -> None:
    subsocket_set_timeout(_require_pointer(self._ptr, "SubSocket"), timeout)

  def receive(self, non_blocking: bool = False) -> Optional[bytes]:
    message = subsocket_receive(_require_pointer(self._ptr, "SubSocket"), non_blocking)
    if message is None:
      return None

    try:
      size = message_size(message)
      return ctypes.string_at(message_data(message), size)
    finally:
      message_destroy(message)


class PubSocket:
  def __init__(self) -> None:
    self._ptr = pubsocket_create()

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None:
      pubsocket_destroy(pointer)
      self._ptr = None

  def connect(self, context: Context, endpoint: str | bytes, segment_size: int = 0) -> None:
    result = pubsocket_connect(
      _require_pointer(self._ptr, "PubSocket"), _require_pointer(context._ptr, "Context"),
      _as_bytes(endpoint, "endpoint"), segment_size,
    )
    if result != 0:
      _raise_ipc(endpoint)

  def send(self, data: bytes) -> None:
    if not isinstance(data, bytes):
      raise TypeError("data must be bytes")
    result = pubsocket_send(_require_pointer(self._ptr, "PubSocket"), data, len(data))
    if result != len(data):
      _raise_ipc()

  def all_readers_updated(self) -> bool:
    return bool(pubsocket_all_readers_updated(_require_pointer(self._ptr, "PubSocket")))

  def wait_for_readers(self, timeout: float = 1.0, interval: float = 0.001) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
      if self.all_readers_updated():
        return
      time.sleep(interval)
    raise TimeoutError("subscriber did not connect")


class Poller:
  def __init__(self) -> None:
    self._ptr = poller_create()
    self._sub_sockets: list[SubSocket] = []

  def __del__(self) -> None:
    pointer = getattr(self, "_ptr", None)
    if pointer is not None:
      poller_destroy(pointer)
      self._ptr = None

  def registerSocket(self, socket: SubSocket) -> None:
    poller_register_socket(
      _require_pointer(self._ptr, "Poller"), _require_pointer(socket._ptr, "SubSocket"),
    )
    self._sub_sockets.append(socket)

  def poll(self, timeout: int) -> list[SubSocket]:
    count = poller_poll(_require_pointer(self._ptr, "Poller"), timeout)
    return [
      SubSocket(poller_result(self._ptr, index), _owner=False, _owner_ref=self)
      for index in range(count)
    ]
