import ctypes
import errno
import os
import time
import weakref
from pathlib import Path


lib = ctypes.CDLL(str(Path(__file__).with_name("libipc_ctypes.so")), use_errno=True)
c_void_p = ctypes.c_void_p
c_size_t = ctypes.c_size_t
c_int = ctypes.c_int
c_char_p = ctypes.c_char_p

lib.msgq_context_create.restype = c_void_p
lib.msgq_context_delete.argtypes = (c_void_p,)
lib.msgq_sub_create.restype = c_void_p
lib.msgq_sub_delete.argtypes = (c_void_p,)
lib.msgq_sub_connect.argtypes = (c_void_p, c_void_p, c_char_p, c_char_p, c_int, c_size_t)
lib.msgq_sub_set_timeout.argtypes = (c_void_p, c_int)
lib.msgq_sub_receive.argtypes = (c_void_p, c_int)
lib.msgq_sub_receive.restype = c_void_p
lib.msgq_message_size.argtypes = (c_void_p,)
lib.msgq_message_size.restype = c_size_t
lib.msgq_message_data.argtypes = (c_void_p,)
lib.msgq_message_data.restype = c_void_p
lib.msgq_message_delete.argtypes = (c_void_p,)
lib.msgq_pub_create.restype = c_void_p
lib.msgq_pub_delete.argtypes = (c_void_p,)
lib.msgq_pub_connect.argtypes = (c_void_p, c_void_p, c_char_p, c_size_t)
lib.msgq_pub_send.argtypes = (c_void_p, c_char_p, c_size_t)
lib.msgq_pub_all_readers_updated.argtypes = (c_void_p,)
lib.msgq_poller_create.restype = c_void_p
lib.msgq_poller_delete.argtypes = (c_void_p,)
lib.msgq_poller_register.argtypes = (c_void_p, c_void_p)
lib.msgq_poller_poll.argtypes = (c_void_p, c_int, ctypes.POINTER(c_void_p), c_size_t)
lib.msgq_poller_poll.restype = c_size_t
lib.msgq_toggle_fake_events.argtypes = (c_int,)
lib.msgq_set_fake_prefix.argtypes = (c_char_p,)
lib.msgq_get_fake_prefix.argtypes = (c_void_p, c_size_t)
lib.msgq_get_fake_prefix.restype = c_size_t
lib.msgq_event_handle_create.argtypes = (c_char_p, c_char_p, c_int)
lib.msgq_event_handle_create.restype = c_void_p
lib.msgq_event_handle_delete.argtypes = (c_void_p,)
lib.msgq_event_handle_enabled.argtypes = (c_void_p,)
lib.msgq_event_handle_set_enabled.argtypes = (c_void_p, c_int)
lib.msgq_event_handle_recv_called.argtypes = (c_void_p,)
lib.msgq_event_handle_recv_called.restype = c_void_p
lib.msgq_event_handle_recv_ready.argtypes = (c_void_p,)
lib.msgq_event_handle_recv_ready.restype = c_void_p
lib.msgq_event_delete.argtypes = (c_void_p,)
for name in ("set", "clear", "peek", "fd"):
  getattr(lib, f"msgq_event_{name}").argtypes = (c_void_p,)
lib.msgq_event_wait.argtypes = (c_void_p, c_int)
lib.msgq_event_wait_for_one.argtypes = (ctypes.POINTER(c_void_p), c_size_t, c_int)


def _bytes(value):
  return value.encode() if isinstance(value, str) else value


class IpcError(Exception):
  def __init__(self, endpoint=None):
    suffix = f"with {endpoint.decode('utf-8')}" if endpoint else ""
    super().__init__(f"Messaging failure {suffix}: {os.strerror(ctypes.get_errno())}")


class MultiplePublishersError(IpcError):
  pass


def _check(result, endpoint=None):
  if result != 0:
    if ctypes.get_errno() == errno.EADDRINUSE:
      raise MultiplePublishersError(endpoint)
    raise IpcError(endpoint)


def toggle_fake_events(enabled): lib.msgq_toggle_fake_events(enabled)
def set_fake_prefix(prefix): lib.msgq_set_fake_prefix(_bytes(prefix))
def get_fake_prefix():
  size = lib.msgq_get_fake_prefix(None, 0)
  result = ctypes.create_string_buffer(size)
  lib.msgq_get_fake_prefix(result, size)
  return result.raw
def delete_fake_prefix(): set_fake_prefix(b"")


class Event:
  def __init__(self, ptr):
    self._ptr = ptr
    self._finalizer = weakref.finalize(self, lib.msgq_event_delete, ptr)
  def set(self):
    if lib.msgq_event_set(self._ptr) < 0:
      raise RuntimeError("event operation failed")
  def clear(self): return lib.msgq_event_clear(self._ptr)
  def wait(self, timeout=-1):
    if lib.msgq_event_wait(self._ptr, timeout) < 0:
      raise RuntimeError("event timed out")
  def peek(self): return bool(lib.msgq_event_peek(self._ptr))
  @property
  def fd(self): return lib.msgq_event_fd(self._ptr)
  @property
  def ptr(self): return self._ptr


def wait_for_one_event(events, timeout=-1):
  items = (c_void_p * len(events))(*(event._ptr for event in events))
  result = lib.msgq_event_wait_for_one(items, len(events), timeout)
  if result < 0:
    raise RuntimeError("event wait failed")
  return result


class SocketEventHandle:
  def __init__(self, endpoint, identifier, override):
    self._ptr = lib.msgq_event_handle_create(_bytes(endpoint), _bytes(identifier), override)
    self._finalizer = weakref.finalize(self, lib.msgq_event_handle_delete, self._ptr)
  @property
  def enabled(self): return bool(lib.msgq_event_handle_enabled(self._ptr))
  @enabled.setter
  def enabled(self, value): lib.msgq_event_handle_set_enabled(self._ptr, value)
  @property
  def recv_called_event(self): return Event(lib.msgq_event_handle_recv_called(self._ptr))
  @property
  def recv_ready_event(self): return Event(lib.msgq_event_handle_recv_ready(self._ptr))


class Context:
  def __init__(self):
    self._ptr = lib.msgq_context_create()
    self._finalizer = weakref.finalize(self, lib.msgq_context_delete, self._ptr)
  def term(self):
    if self._finalizer.alive:
      self._finalizer()
      self._ptr = None


class SubSocket:
  def __init__(self):
    self._ptr = lib.msgq_sub_create()
    self._finalizer = weakref.finalize(self, lib.msgq_sub_delete, self._ptr)
  def connect(self, context, endpoint, address=b"127.0.0.1", conflate=False, segment_size=0):
    encoded_endpoint = _bytes(endpoint)
    _check(lib.msgq_sub_connect(self._ptr, context._ptr, encoded_endpoint, _bytes(address), conflate, segment_size), encoded_endpoint)
  def setTimeout(self, timeout): lib.msgq_sub_set_timeout(self._ptr, timeout)
  def receive(self, non_blocking=False):
    message = lib.msgq_sub_receive(self._ptr, non_blocking)
    if not message:
      return None
    try:
      return ctypes.string_at(lib.msgq_message_data(message), lib.msgq_message_size(message))
    finally:
      lib.msgq_message_delete(message)


class PubSocket:
  def __init__(self):
    self._ptr = lib.msgq_pub_create()
    self._finalizer = weakref.finalize(self, lib.msgq_pub_delete, self._ptr)
  def connect(self, context, endpoint, segment_size=0):
    encoded_endpoint = _bytes(endpoint)
    _check(lib.msgq_pub_connect(self._ptr, context._ptr, encoded_endpoint, segment_size), encoded_endpoint)
  def send(self, data):
    result = lib.msgq_pub_send(self._ptr, data, len(data))
    if result != len(data):
      _check(result)
  def all_readers_updated(self): return bool(lib.msgq_pub_all_readers_updated(self._ptr))
  def wait_for_readers(self, timeout=1.0, interval=0.001):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
      if self.all_readers_updated():
        return
      time.sleep(interval)
    raise TimeoutError("subscriber did not connect")


class Poller:
  def __init__(self):
    self._ptr = lib.msgq_poller_create()
    self._finalizer = weakref.finalize(self, lib.msgq_poller_delete, self._ptr)
    self._sockets = {}
  def registerSocket(self, socket):
    self._sockets[socket._ptr] = socket
    lib.msgq_poller_register(self._ptr, socket._ptr)
  def poll(self, timeout):
    result = (c_void_p * max(1, len(self._sockets)))()
    count = lib.msgq_poller_poll(self._ptr, int(timeout), result, len(self._sockets))
    return [self._sockets[result[i]] for i in range(count)]
