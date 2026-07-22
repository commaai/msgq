import errno
import os
import time
from pathlib import Path

from cffi import FFI


ffi = FFI()
declarations = Path(__file__).with_name("ipc_cffi.h").read_text().splitlines()
ffi.cdef("\n".join(line for line in declarations if not line.startswith("#") and line not in ('extern "C" {', '}')))
lib = ffi.dlopen(str(Path(__file__).with_name("libipc_cffi.so")))


def _bytes(value):
  return value.encode() if isinstance(value, str) else value


class IpcError(Exception):
  def __init__(self, endpoint=None):
    suffix = f"with {endpoint.decode('utf-8')}" if endpoint else ""
    super().__init__(f"Messaging failure {suffix}: {os.strerror(ffi.errno)}")


class MultiplePublishersError(IpcError):
  pass


def _check(result, endpoint=None):
  if result != 0:
    if ffi.errno == errno.EADDRINUSE:
      raise MultiplePublishersError(endpoint)
    raise IpcError(endpoint)


def toggle_fake_events(enabled): lib.msgq_toggle_fake_events(enabled)
def set_fake_prefix(prefix): lib.msgq_set_fake_prefix(_bytes(prefix))
def get_fake_prefix():
  size = lib.msgq_get_fake_prefix(ffi.NULL, 0)
  result = ffi.new("char[]", size)
  lib.msgq_get_fake_prefix(result, size)
  return bytes(ffi.buffer(result, size))
def delete_fake_prefix(): set_fake_prefix(b"")


class Event:
  def __init__(self, ptr): self._ptr = ffi.gc(ptr, lib.msgq_event_delete)
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
  def ptr(self): return int(ffi.cast("uintptr_t", self._ptr))


def wait_for_one_event(events, timeout=-1):
  items = ffi.new("void *[]", [event._ptr for event in events])
  result = lib.msgq_event_wait_for_one(items, len(events), timeout)
  if result < 0:
    raise RuntimeError("event wait failed")
  return result


class SocketEventHandle:
  def __init__(self, endpoint, identifier, override):
    self._ptr = ffi.gc(lib.msgq_event_handle_create(_bytes(endpoint), _bytes(identifier), override), lib.msgq_event_handle_delete)
  @property
  def enabled(self): return bool(lib.msgq_event_handle_enabled(self._ptr))
  @enabled.setter
  def enabled(self, value): lib.msgq_event_handle_set_enabled(self._ptr, value)
  @property
  def recv_called_event(self): return Event(lib.msgq_event_handle_recv_called(self._ptr))
  @property
  def recv_ready_event(self): return Event(lib.msgq_event_handle_recv_ready(self._ptr))


class Context:
  def __init__(self): self._ptr = ffi.gc(lib.msgq_context_create(), lib.msgq_context_delete)
  def term(self):
    if self._ptr != ffi.NULL:
      ffi.release(self._ptr)
      self._ptr = ffi.NULL


class SubSocket:
  def __init__(self, ptr=None, owner=True):
    raw = lib.msgq_sub_create() if ptr is None else ptr
    self._ptr = ffi.gc(raw, lib.msgq_sub_delete) if owner else raw
  def connect(self, context, endpoint, address=b"127.0.0.1", conflate=False, segment_size=0):
    encoded_endpoint = _bytes(endpoint)
    _check(lib.msgq_sub_connect(self._ptr, context._ptr, encoded_endpoint, _bytes(address), conflate, segment_size), encoded_endpoint)
  def setTimeout(self, timeout): lib.msgq_sub_set_timeout(self._ptr, timeout)
  def receive(self, non_blocking=False):
    message = lib.msgq_sub_receive(self._ptr, non_blocking)
    if message == ffi.NULL:
      return None
    try:
      return bytes(ffi.buffer(lib.msgq_message_data(message), lib.msgq_message_size(message)))
    finally:
      lib.msgq_message_delete(message)


class PubSocket:
  def __init__(self): self._ptr = ffi.gc(lib.msgq_pub_create(), lib.msgq_pub_delete)
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
    self._ptr = ffi.gc(lib.msgq_poller_create(), lib.msgq_poller_delete)
    self._sockets = {}
  def registerSocket(self, socket):
    self._sockets[int(ffi.cast("uintptr_t", socket._ptr))] = socket
    lib.msgq_poller_register(self._ptr, socket._ptr)
  def poll(self, timeout):
    result = ffi.new("void *[]", max(1, len(self._sockets)))
    count = lib.msgq_poller_poll(self._ptr, int(timeout), result, len(self._sockets))
    return [self._sockets[int(ffi.cast("uintptr_t", result[i]))] for i in range(count)]
