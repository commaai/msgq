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
    dict alive
    dict valid
    dict updated
    dict logMonoTime

  def __init__(self, vector[string] services, vector[string] ignore_alive=[], string addr=b"127.0.0.1"):
    self.services = services
    self.sm = new cppSubMaster(services, addr, ignore_alive)

    # setup dicts to preserve the current python submaster API
    self.data, self.alive, self.valid, self.updated, self.logMonoTime = {}, {}, {}, {}, {}
    self.update_msgs()

  def __dealloc__(self):
    del self.sm

  def __getitem__(self, s):
    if self.data[s] is None:
      msg = self.sm.services[s.encode('utf8')].msg
      dat = log.Event.from_bytes(msg.getData()[:msg.getSize()])
      self.data[s] = getattr(dat, s)
    return self.data[s]

  @property
  def frame(self):
    return self.sm.frame

  cpdef update(self, int timeout=1000):
    self.data = {s: None for s in self.services}
    self.sm.update(timeout)
    self.update_msgs()

  cpdef update_msgs(self):
    cdef cppMessage * msg
    for s in self.services:
      s_str = s.decode('utf8')

      # TODO: do this in the cpp constructor
      if self.sm.services[s].msg is NULL:
        m = log.Event.new_message()
        try:
          m.init(s)
        except:
          m.init(s, 1)
        dat = getattr(m, s_str)
        self.data[s_str] = dat
      else:
        msg = self.sm.services[s].msg
        dat = log.Event.from_bytes(msg.getData()[:msg.getSize()])
        self.data[s_str] = getattr(dat, s_str)

      self.alive[s_str] = self.sm.services[s].alive
      self.valid[s_str] = self.sm.services[s].valid
      self.updated[s_str] = self.sm.services[s].updated
      self.logMonoTime[s_str] = self.sm.services[s].logMonoTime

  cpdef all_alive(self, vector[string] service_list=[]):
    return self.sm.allAlive(service_list)

  cpdef all_valid(self, vector[string] service_list=[]):
    return self.sm.allValid(service_list)

  cpdef all_alive_and_valid(self, vector[string] service_list=[]):
    return self.sm.allAliveAndValid(service_list)


cdef class PubMaster:
  cdef cppPubMaster * pm

  def __cinit__(self, vector[string] services):
    self.pm = new cppPubMaster(services)

  def __dealloc__(self):
    del self.pm

  cpdef send(self, service, data):
    if not isinstance(data, bytes):
      data = data.to_bytes()
    self.send_bytes(service, data)

  cpdef send_bytes(self, string service, string data):
    self.pm.send(service, <char*>data.c_str(), len(data))
