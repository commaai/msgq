# distutils: language = c++
# cython: c_string_encoding=ascii, language_level=3

import sys
import capnp
from cereal import log

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


cdef class SubMaster:
  cdef:
    cppSubMaster * sm

  cdef public:
    vector[string] services
    dict data

  def __init__(self, vector[string] services, vector[string] ignore_alive=[], string addr=b"127.0.0.1"):
    self.services = services
    self.data = {s: None for s in self.services}
    self.sm = new cppSubMaster(services, addr, ignore_alive)

  def __dealloc__(self):
    del self.sm

  def __getitem__(self, s):
    s = s.encode('utf8')
    if self.data[s] is None:
      msg = self.sm.getMessage(s)
      dat = log.Event.from_bytes(msg.getData()[:msg.getSize()])
      self.data[s] = getattr(dat, s.decode('utf8'))
    return self.data[s]

  @property
  def frame(self):
    return self.sm.frame

  @property
  def updated(self):
    return {s: self.sm.updated(s) for s in self.services}

  @property
  def logMonoTime(self):
    return {s: self.sm.logMonoTime(s) for s in self.services}

  cpdef update(self, int timeout=1000):
    self.data = {s: None for s in self.services}
    self.sm.update(timeout)

  cpdef all_alive(self, vector[string] service_list=[]):
    return True

  cpdef all_valid(self, vector[string] service_list=[]):
    return True

  cpdef all_alive_and_valid(self, vector[string] service_list=[]):
    return True


cdef class PubMaster:
  cdef cppPubMaster * pm

  def __cinit__(self, services):
    cdef vector[string] service_list = services
    self.pm = new cppPubMaster(service_list)

  def __dealloc__(self):
    del self.pm

  cpdef send(self, service, data):
    if not isinstance(data, bytes):
      data = data.to_bytes()
    self.send_bytes(service, data)

  cpdef send_bytes(self, string service, string data):
    self.pm.send(service, <char*>data.c_str(), len(data))
