# distutils: language = c++
# cython: c_string_encoding=ascii, language_level=3

import sys
import capnp

from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp cimport bool
from libc cimport errno


from messaging cimport Context as cppContext
from messaging cimport SubSocket as cppSubSocket
from messaging cimport PubSocket as cppPubSocket
from messaging cimport Poller as cppPoller
from messaging cimport Message as cppMessage
from messaging cimport SubMaster as cppSubMaster
from messaging cimport PubMaster as cppPubMaster


class MessagingError(Exception):
  pass


class MultiplePublishersError(MessagingError):
  pass


cdef class Context:
  cdef cppContext * context

  def __cinit__(self):
    self.context = cppContext.create()

  def term(self):
    del self.context
    self.context = NULL

  def __dealloc__(self):
    pass
    # Deleting the context will hang if sockets are still active
    # TODO: Figure out a way to make sure the context is closed last
    # del self.context


cdef class Poller:
  cdef cppPoller * poller
  cdef list sub_sockets

  def __cinit__(self):
    self.sub_sockets = []
    self.poller = cppPoller.create()

  def __dealloc__(self):
    del self.poller

  def registerSocket(self, SubSocket socket):
    self.sub_sockets.append(socket)
    self.poller.registerSocket(socket.socket)

  def poll(self, timeout):
    sockets = []
    cdef int t = timeout

    with nogil:
        result = self.poller.poll(t)

    for s in result:
        socket = SubSocket()
        socket.setPtr(s)
        sockets.append(socket)

    return sockets

cdef class SubSocket:
  cdef cppSubSocket * socket
  cdef bool is_owner

  def __cinit__(self):
    self.socket = cppSubSocket.create()
    self.is_owner = True

    if self.socket == NULL:
      raise MessagingError

  def __dealloc__(self):
    if self.is_owner:
      del self.socket

  cdef setPtr(self, cppSubSocket * ptr):
    if self.is_owner:
      del self.socket

    self.is_owner = False
    self.socket = ptr

  def connect(self, Context context, string endpoint, string address=b"127.0.0.1", bool conflate=False):
    r = self.socket.connect(context.context, endpoint, address, conflate)

    if r != 0:
      if errno.errno == errno.EADDRINUSE:
        raise MultiplePublishersError
      else:
        raise MessagingError

  def setTimeout(self, int timeout):
    self.socket.setTimeout(timeout)

  def receive(self, bool non_blocking=False):
    msg = self.socket.receive(non_blocking)

    if msg == NULL:
      # If a blocking read returns no message check errno if SIGINT was caught in the C++ code
      if errno.errno == errno.EINTR:
        print("SIGINT received, exiting")
        sys.exit(1)

      return None
    else:
      sz = msg.getSize()
      m = msg.getData()[:sz]
      del msg

      return m


cdef class PubSocket:
  cdef cppPubSocket * socket

  def __cinit__(self):
    self.socket = cppPubSocket.create()
    if self.socket == NULL:
      raise MessagingError

  def __dealloc__(self):
    del self.socket

  def connect(self, Context context, string endpoint):
    r = self.socket.connect(context.context, endpoint)

    if r != 0:
      if errno.errno == errno.EADDRINUSE:
        raise MultiplePublishersError
      else:
        raise MessagingError

  def send(self, string data):
    length = len(data)
    r = self.socket.send(<char*>data.c_str(), length)

    if r != length:
      if errno.errno == errno.EADDRINUSE:
        raise MultiplePublishersError
      else:
        raise MessagingError

# TODO: is there a better way to do this?
class CallbackDict:
  def __init__(self, callback):
    self.callback = callback

  def __getitem__(self, x):
    return self.callback(x)

cdef class SubMaster:
  cdef cppSubMaster * sm

  # TODO: cinit or init?
  def __init__(self, services, ignore_alive=None, string addr=b"127.0.0.1"):
    self.updated = CallbackDict(self._updated_callback)
    self.valid = CallbackDict(self._valid_callback)
    self.logMonoTime = CallbackDict(self._logmonotime_callback)

    cdef vector[const char *] service_list, ignore
    service_list = services
    ignore = [] if ignore_alive is None else ignore_alive
    self.sm = new cppSubMaster(service_list, addr, ignore)

  def __dealloc__(self):
    del self.sm

  def __getitem__(self, s):
    # only convert bytes to capnp if read
    return None

  @property
  def frame(self):
    return self.sm.frame

  # TODO: make updated, logMonoTime, and valid a map in C++ class
  @property
  def updated(self):
    return self.sm.updated

  @property
  def valid(self):
    return self.sm.valid

  @property
  def logMonoTime(self):
    return self.sm.logMonoTime

  def update(self, int timeout=1000):
    self.sm.update(timeout)
    #msgs = []
    #for sock in self.poller.poll(timeout):
    #  msgs.append(recv_one_or_none(sock))
    #self.update_msgs(sec_since_boot(), msgs)

  # this should stay in python for fake submaster
  def update_msgs(self, cur_time, msgs):
    self.frame += 1
    self.updated = dict.fromkeys(self.updated, False)
    for msg in msgs:
      if msg is None:
        continue

      s = msg.which()
      self.updated[s] = True
      self.rcv_time[s] = cur_time
      self.rcv_frame[s] = self.frame
      self.data[s] = getattr(msg, s)
      self.logMonoTime[s] = msg.logMonoTime
      self.valid[s] = msg.valid

    for s in self.data:
      # arbitrary small number to avoid float comparison. If freq is 0, we can skip the check
      if self.freq[s] > 1e-5:
        # alive if delay is within 10x the expected frequency
        self.alive[s] = (cur_time - self.rcv_time[s]) < (10. / self.freq[s])
      else:
        self.alive[s] = True

  def all_alive(self, service_list=None):
    if service_list is None:
      service_list = self.alive.keys()
    return all(self.alive[s] for s in service_list if s not in self.ignore_alive)

  def all_valid(self, service_list=None):
    if service_list is None:
      service_list = self.valid.keys()
    return all(self.valid[s] for s in service_list)

  def all_alive_and_valid(self, service_list=None):
    if service_list is None:
      service_list = self.alive.keys()
    cdef bool all_alive = True
    cdef bool all_valid = True
    return all_alive and all_valid

cdef class PubMaster:
  cdef cppPubMaster * pm

  def __cinit__(self, services):
    cdef vector[const char *] service_list = services
    self.pm = new cppPubMaster(service_list)

  def __dealloc__(self):
    del self.pm

  def send(self, service, data):
    if not isinstance(data, bytes):
      data = data.to_bytes()
    self.send_bytes(service, data)

  def send_bytes(self, const char *service, string data):
    self.pm.send(service, <char*>data.c_str(), len(data))

