# must be built with scons
import os
import ctypes
import fcntl
from typing import Optional, List


def get_libpath():
  path = os.path.dirname(os.path.abspath(__file__))
  parent = os.path.dirname(path)
  libname = "libmsgq_c.so"
  paths = [
    os.path.join(path, libname),
    os.path.join(parent, libname),
    os.path.join(path, "..", libname),
  ]
  for p in paths:
    if os.path.exists(p):
      return p
  return libname

try:
  _lib: Optional[ctypes.CDLL] = ctypes.CDLL(get_libpath())
except OSError:
  print("Warning: libmsgq not found")
  _lib = None

if _lib is None:
    pass

def ensure_lib():
  if _lib is None:
    raise IpcError("libmsgq not loaded")

# Type alias for CDLL to avoid Optional issues if we ensure it
LibType = ctypes.CDLL


class IpcError(RuntimeError):
  pass

class MultiplePublishersError(IpcError):
  pass

def check_error(ret):
  assert _lib
  if ret != 0:
    err_ptr = _lib.msgq_get_last_error()
    err_val = ctypes.cast(err_ptr, ctypes.c_char_p).value if err_ptr else None
    err_msg = err_val.decode('utf-8') if err_val else "Unknown error"
    if "Address already in use" in err_msg:
      raise MultiplePublishersError(err_msg)
    raise IpcError(err_msg)

if _lib:
  _lib.msgq_context_create.restype = ctypes.c_void_p
  _lib.msgq_context_destroy.argtypes = [ctypes.c_void_p]

  _lib.msgq_subsocket_create.restype = ctypes.c_void_p
  _lib.msgq_subsocket_destroy.argtypes = [ctypes.c_void_p]
  _lib.msgq_subsocket_connect.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_bool]
  _lib.msgq_subsocket_connect.restype = ctypes.c_int
  _lib.msgq_subsocket_set_timeout.argtypes = [ctypes.c_void_p, ctypes.c_int]
  _lib.msgq_subsocket_receive.restype = ctypes.c_void_p
  _lib.msgq_subsocket_receive.argtypes = [ctypes.c_void_p, ctypes.c_bool]

  _lib.msgq_message_destroy.argtypes = [ctypes.c_void_p]
  _lib.msgq_message_get_data.restype = ctypes.c_void_p
  _lib.msgq_message_get_data.argtypes = [ctypes.c_void_p]
  _lib.msgq_message_get_size.restype = ctypes.c_size_t
  _lib.msgq_message_get_size.argtypes = [ctypes.c_void_p]

  _lib.msgq_pubsocket_create.restype = ctypes.c_void_p
  _lib.msgq_pubsocket_destroy.argtypes = [ctypes.c_void_p]
  _lib.msgq_pubsocket_connect.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_char_p]
  _lib.msgq_pubsocket_connect.restype = ctypes.c_int
  _lib.msgq_pubsocket_send.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
  _lib.msgq_pubsocket_send.restype = ctypes.c_int
  _lib.msgq_pubsocket_all_readers_updated.argtypes = [ctypes.c_void_p]
  _lib.msgq_pubsocket_all_readers_updated.restype = ctypes.c_bool

  _lib.msgq_poller_create.restype = ctypes.c_void_p
  _lib.msgq_poller_destroy.argtypes = [ctypes.c_void_p]
  _lib.msgq_poller_register.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
  _lib.msgq_poller_poll.restype = ctypes.c_int
  _lib.msgq_poller_poll.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p), ctypes.c_int, ctypes.c_int]

  _lib.msgq_event_create.restype = ctypes.c_void_p
  _lib.msgq_event_create_from_fd.restype = ctypes.c_void_p
  _lib.msgq_event_create_from_fd.argtypes = [ctypes.c_int]
  _lib.msgq_event_destroy.argtypes = [ctypes.c_void_p]
  _lib.msgq_event_set.argtypes = [ctypes.c_void_p]
  _lib.msgq_event_clear.argtypes = [ctypes.c_void_p]
  _lib.msgq_event_wait.argtypes = [ctypes.c_void_p, ctypes.c_int]
  _lib.msgq_event_wait.restype = ctypes.c_int
  _lib.msgq_event_peek.argtypes = [ctypes.c_void_p]
  _lib.msgq_event_peek.restype = ctypes.c_bool
  _lib.msgq_event_is_valid.argtypes = [ctypes.c_void_p]
  _lib.msgq_event_is_valid.restype = ctypes.c_bool
  _lib.msgq_event_get_fd.argtypes = [ctypes.c_void_p]
  _lib.msgq_event_get_fd.restype = ctypes.c_int
  _lib.msgq_event_wait_for_one.restype = ctypes.c_int
  _lib.msgq_event_wait_for_one.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_int, ctypes.c_int]

  _lib.msgq_socket_event_handle_create.restype = ctypes.c_void_p
  _lib.msgq_socket_event_handle_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_bool]
  _lib.msgq_socket_event_handle_destroy.argtypes = [ctypes.c_void_p]
  _lib.msgq_socket_event_handle_is_enabled.argtypes = [ctypes.c_void_p]
  _lib.msgq_socket_event_handle_is_enabled.restype = ctypes.c_bool
  _lib.msgq_socket_event_handle_set_enabled.argtypes = [ctypes.c_void_p, ctypes.c_bool]
  _lib.msgq_socket_event_handle_recv_called.restype = ctypes.c_void_p
  _lib.msgq_socket_event_handle_recv_called.argtypes = [ctypes.c_void_p]
  _lib.msgq_socket_event_handle_recv_ready.restype = ctypes.c_void_p
  _lib.msgq_socket_event_handle_recv_ready.argtypes = [ctypes.c_void_p]

  _lib.msgq_toggle_fake_events.argtypes = [ctypes.c_bool]
  _lib.msgq_set_fake_prefix.argtypes = [ctypes.c_char_p]
  _lib.msgq_get_fake_prefix.restype = ctypes.c_char_p
  _lib.msgq_get_last_error.restype = ctypes.c_char_p

class Context:
  def __init__(self):
    ensure_lib()
    assert _lib
    self.ptr = _lib.msgq_context_create()
    if not self.ptr:
      raise IpcError("Failed to create context")

  def __del__(self):
    if self.ptr and _lib:
      _lib.msgq_context_destroy(self.ptr)

  def term(self):
    if self.ptr and _lib:
      _lib.msgq_context_destroy(self.ptr)
      self.ptr = None


class SubSocket:
  def __init__(self, ptr=None):
    ensure_lib()
    assert _lib
    if ptr:
      self.ptr = ptr
      self.owned = False
    else:
      self.ptr = _lib.msgq_subsocket_create()
      self.owned = True
      if not self.ptr:
        raise IpcError("Failed to create SubSocket")

  def __del__(self):
    self.close()

  def close(self):
    if self.ptr and self.owned and _lib:
      _lib.msgq_subsocket_destroy(self.ptr)
    self.ptr = None

  def connect(self, context: Context, endpoint: str, address: str = "127.0.0.1", conflate: bool = False):
    ensure_lib()
    assert _lib
    ret = _lib.msgq_subsocket_connect(self.ptr, context.ptr, endpoint.encode('utf-8'), address.encode('utf-8'), conflate)
    check_error(ret)
    self.context = context

  def setTimeout(self, timeout: int):
    assert _lib
    _lib.msgq_subsocket_set_timeout(self.ptr, timeout)

  def receive(self, non_blocking: bool = False) -> Optional[bytes]:
    assert _lib
    msg_ptr = _lib.msgq_subsocket_receive(self.ptr, non_blocking)
    if not msg_ptr:
      return None
    try:
      sz = _lib.msgq_message_get_size(msg_ptr)
      data = _lib.msgq_message_get_data(msg_ptr)
      return ctypes.string_at(data, sz)
    finally:
      _lib.msgq_message_destroy(msg_ptr)

  def __eq__(self, other):
    if not isinstance(other, SubSocket):
      return False
    return self.ptr == other.ptr

class PubSocket:
  def __init__(self):
    ensure_lib()
    assert _lib
    self.ptr = _lib.msgq_pubsocket_create()
    self.lock_file = None
    if not self.ptr:
      raise IpcError("Failed to create PubSocket")

  def __del__(self):
    self.close()

  def connect(self, context: Context, endpoint: str):
    prefix = os.getenv("OPENPILOT_PREFIX")
    base = "/dev/shm"
    if prefix:
      base = os.path.join(base, prefix)
      try:
        os.makedirs(base, exist_ok=True)
      except OSError:
        pass

    path = os.path.join(base, endpoint + ".lock")
    try:
      self.lock_file = open(path, 'w')
      fcntl.flock(self.lock_file, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
      raise MultiplePublishersError("Address already in use")
    except OSError:
      pass

    ensure_lib()
    assert _lib
    ret = _lib.msgq_pubsocket_connect(self.ptr, context.ptr, endpoint.encode('utf-8'))
    check_error(ret)
    self.context = context

  def send(self, data: bytes):
    assert _lib
    ret = _lib.msgq_pubsocket_send(self.ptr, data, len(data))
    if ret < 0:
      check_error(ret)

  def all_readers_updated(self) -> bool:
    assert _lib
    return bool(_lib.msgq_pubsocket_all_readers_updated(self.ptr))

  def close(self):
    if self.lock_file:
      try:
        fcntl.flock(self.lock_file, fcntl.LOCK_UN)
        self.lock_file.close()
      except (IOError, OSError):
        pass
      self.lock_file = None
    if self.ptr and _lib:
      _lib.msgq_pubsocket_destroy(self.ptr)
      self.ptr = None


class Poller:
  def __init__(self):
    ensure_lib()
    assert _lib
    self.ptr = _lib.msgq_poller_create()
    self.sockets = []

  def __del__(self):
    if self.ptr and _lib:
      _lib.msgq_poller_destroy(self.ptr)

  def registerSocket(self, socket: SubSocket):
    assert _lib
    _lib.msgq_poller_register(self.ptr, socket.ptr)
    self.sockets.append(socket)

  def poll(self, timeout: int) -> List[SubSocket]:
    assert _lib
    max_count = len(self.sockets)
    if max_count == 0:
      return []

    # Alloc array for results
    results = (ctypes.c_void_p * max_count)()
    count = _lib.msgq_poller_poll(self.ptr, results, max_count, timeout)
    if count < 0:
      return []

    ret = []
    for i in range(count):
      ptr = results[i]
      # Find corresponding Python object
      found = None
      for s in self.sockets:
        if s.ptr == ptr:
          found = s
          break
      if found:
        ret.append(found)
      else:
        pass
    return ret

class Event:
  def __init__(self, fd: Optional[int] = None, ptr=None):
    ensure_lib()
    assert _lib
    if ptr:
      self.ptr = ptr
      self.owned = True
    elif fd is not None:
      self.ptr = _lib.msgq_event_create_from_fd(fd)
      self.owned = True
    else:
      self.ptr = _lib.msgq_event_create()
      self.owned = True

    if not self.ptr:
      raise IpcError("Failed to create Event")

  def __del__(self):
    if self.ptr and self.owned and _lib:
      _lib.msgq_event_destroy(self.ptr)

  def set(self):
    assert _lib
    _lib.msgq_event_set(self.ptr)

  def clear(self):
    assert _lib
    _lib.msgq_event_clear(self.ptr)

  def wait(self, timeout: Optional[int] = None):
    assert _lib
    t = -1
    if timeout is not None:
      t = int(timeout)
    ret = _lib.msgq_event_wait(self.ptr, t)
    check_error(ret)

  @property
  def fd(self):
    assert _lib
    return _lib.msgq_event_get_fd(self.ptr)

  def peek(self):
    assert _lib
    return _lib.msgq_event_peek(self.ptr)

  @staticmethod
  def wait_for_one(events: List['Event'], timeout: Optional[int] = None) -> int:
    assert _lib
    t = timeout if timeout is not None else -1
    c_evts = (ctypes.c_void_p * len(events))()
    for i, e in enumerate(events):
      c_evts[i] = e.ptr
    return int(_lib.msgq_event_wait_for_one(c_evts, len(events), t))

class SocketEventHandle:
  def __init__(self, endpoint: str, identifier: str, override: bool = True):
    ensure_lib()
    assert _lib
    self.ptr = _lib.msgq_socket_event_handle_create(endpoint.encode('utf-8'), identifier.encode('utf-8'), override)
    if not self.ptr:
      raise IpcError("Failed to create SocketEventHandle")

  def __del__(self):
    if self.ptr and _lib:
      _lib.msgq_socket_event_handle_destroy(self.ptr)

  @property
  def enabled(self):
    assert _lib
    return _lib.msgq_socket_event_handle_is_enabled(self.ptr)

  @enabled.setter
  def enabled(self, val):
    assert _lib
    _lib.msgq_socket_event_handle_set_enabled(self.ptr, bool(val))

  @property
  def recv_called_event(self) -> 'Event':
    assert _lib
    ptr = _lib.msgq_socket_event_handle_recv_called(self.ptr)
    return Event(ptr=ptr)

  @property
  def recv_ready_event(self) -> 'Event':
    assert _lib
    ptr = _lib.msgq_socket_event_handle_recv_ready(self.ptr)
    return Event(ptr=ptr)


def toggle_fake_events(enabled: bool):
  assert _lib
  _lib.msgq_toggle_fake_events(enabled)

def set_fake_prefix(prefix: str):
  assert _lib
  _lib.msgq_set_fake_prefix(prefix.encode('utf-8'))

def get_fake_prefix() -> str:
  assert _lib
  ptr = _lib.msgq_get_fake_prefix()
  val = ctypes.cast(ptr, ctypes.c_char_p).value if ptr else None
  return val.decode('utf-8') if val else ""

def delete_fake_prefix():
  set_fake_prefix("")

def wait_for_one_event(events: List[Event], timeout: Optional[int] = None) -> int:
  return Event.wait_for_one(events, timeout)



if _lib:
  context: Optional[Context] = Context()
else:
  context = None


NO_TRAVERSAL_LIMIT = 2**64-1

def fake_event_handle(endpoint: str, identifier: Optional[str] = None, override: bool = True, enable: bool = False) -> SocketEventHandle:
  identifier = identifier or get_fake_prefix()
  handle = SocketEventHandle(endpoint, identifier, override)
  if override:
    handle.enabled = enable
  return handle

def pub_sock(endpoint: str) -> PubSocket:
  assert context
  sock = PubSocket()
  sock.connect(context, endpoint)
  return sock

def sub_sock(endpoint: str, poller: Optional[Poller] = None, addr: str = "127.0.0.1",
             conflate: bool = False, timeout: Optional[int] = None) -> SubSocket:
  assert context
  sock = SubSocket()
  sock.connect(context, endpoint, addr, conflate)

  if timeout is not None:
    sock.setTimeout(timeout)

  if poller is not None:
    poller.registerSocket(sock)
  return sock

def drain_sock_raw(sock: SubSocket, wait_for_one: bool = False) -> List[bytes]:
  ret: List[bytes] = []
  while 1:
    if wait_for_one and len(ret) == 0:
      dat = sock.receive()
    else:
      dat = sock.receive(non_blocking=True)

    if dat is None:
      break
    ret.append(dat)
  return ret
