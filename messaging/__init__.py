# must be built with scons
from .messaging_pyx import Context, Poller, SubSocket, PubSocket, SubMaster, PubMaster  # pylint: disable=no-name-in-module, import-error
from .messaging_pyx import MultiplePublishersError, MessagingError  # pylint: disable=no-name-in-module, import-error
import capnp

from cereal import log
from cereal.services import service_list

assert MultiplePublishersError
assert MessagingError

# sec_since_boot is faster, but allow to run standalone too
try:
  from common.realtime import sec_since_boot
except ImportError:
  import time
  sec_since_boot = time.time
  print("Warning, using python time.time() instead of faster sec_since_boot")

context = Context()

def new_message(service=None, size=None):
  dat = log.Event.new_message()
  dat.logMonoTime = int(sec_since_boot() * 1e9)
  dat.valid = True
  if service is not None:
    if size is None:
      dat.init(service)
    else:
      dat.init(service, size)
  return dat

def pub_sock(endpoint):
  sock = PubSocket()
  sock.connect(context, endpoint)
  return sock

def sub_sock(endpoint, poller=None, addr="127.0.0.1", conflate=False, timeout=None):
  sock = SubSocket()
  addr = addr.encode('utf8')
  sock.connect(context, endpoint, addr, conflate)

  if timeout is not None:
    sock.setTimeout(timeout)

  if poller is not None:
    poller.registerSocket(sock)
  return sock


def drain_sock_raw(sock, wait_for_one=False):
  """Receive all message currently available on the queue"""
  ret = []
  while 1:
    if wait_for_one and len(ret) == 0:
      dat = sock.receive()
    else:
      dat = sock.receive(non_blocking=True)

    if dat is None:
      break

    ret.append(dat)

  return ret

def drain_sock(sock, wait_for_one=False):
  """Receive all message currently available on the queue"""
  ret = []
  while 1:
    if wait_for_one and len(ret) == 0:
      dat = sock.receive()
    else:
      dat = sock.receive(non_blocking=True)

    if dat is None:  # Timeout hit
      break

    dat = log.Event.from_bytes(dat)
    ret.append(dat)

  return ret


# TODO: print when we drop packets?
def recv_sock(sock, wait=False):
  """Same as drain sock, but only returns latest message. Consider using conflate instead."""
  dat = None

  while 1:
    if wait and dat is None:
      rcv = sock.receive()
    else:
      rcv = sock.receive(non_blocking=True)

    if rcv is None:  # Timeout hit
      break

    dat = rcv

  if dat is not None:
    dat = log.Event.from_bytes(dat)

  return dat

def recv_one(sock):
  dat = sock.receive()
  if dat is not None:
    dat = log.Event.from_bytes(dat)
  return dat

def recv_one_or_none(sock):
  dat = sock.receive(non_blocking=True)
  if dat is not None:
    dat = log.Event.from_bytes(dat)
  return dat

def recv_one_retry(sock):
  """Keep receiving until we get a message"""
  while True:
    dat = sock.receive()
    if dat is not None:
      return log.Event.from_bytes(dat)

# TODO: This does not belong in messaging
def get_one_can(logcan):
  while True:
    can = recv_one_retry(logcan)
    if len(can.can) > 0:
      return can
