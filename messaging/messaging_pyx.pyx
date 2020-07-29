# distutils: language = c++
# cython: c_string_encoding=ascii, language_level=3

import sys
from libcpp.map cimport map as cppmap
from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp cimport bool
from libc cimport errno

from messaging cimport Context as cppContext
from messaging cimport SubSocket as cppSubSocket
from messaging cimport PubSocket as cppPubSocket
from messaging cimport Poller as cppPoller
from messaging cimport Message as cppMessage

import capnp
from cereal import log
from cereal.services import service_list

try:
  from common.realtime import sec_since_boot
except ImportError:
  import time
  sec_since_boot = time.time
  print("Warning, using python time.time() instead of faster sec_since_boot")

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


context = Context()

cdef class SubMaster:
  cdef:
    cppPoller * poller
    vector[string] services
    vector[string] ignore_alive

  cdef readonly:
    int frame
    dict updated
    dict alive
    dict valid
    dict rcv_time
    dict rcv_frame
    dict freq
    dict logMonoTime
    dict sock
    dict data

  def __init__(self, services, vector[string] ignore_alive=[], string addr=b"127.0.0.1"):
    self.services = services
    self.ignore_alive = ignore_alive

    self.poller = cppPoller.create()

    self.frame = -1

    self.updated = {s: False for s in services}
    self.rcv_time = {s: 0. for s in services}
    self.rcv_frame = {s: 0 for s in services}
    self.alive = {s: False for s in services}
    self.sock = {}
    self.freq = {}
    self.data = {}
    self.logMonoTime = {}
    self.valid = {}

    for s in services:
      sock = SubSocket()
      sock.connect(context, s, addr, True)
      self.poller.registerSocket(sock.socket)
      self.sock[s] = sock
      self.freq[s] = service_list[s].frequency

      data = log.Event.new_message()
      try:
        data.init(s)
      except capnp.lib.capnp.KjException:
        data.init(s, 0) # lists
      self.data[s] = getattr(data, s)
      self.logMonoTime[s] = 0
      self.valid[s] = True

  def __dealloc__(self):
    del self.poller

  def __getitem__(self, s):
    return self.data[s]

  def update(self, int timeout=1000):
    cdef vector[string] msgs = self._update(timeout)
    self.update_msgs(sec_since_boot(), msgs)

  cpdef _update(self, int timeout):
    cdef vector[string] msgs
    with nogil:
      result = self.poller.poll(timeout)
      for s in result:
        msg = s.receive(True)
        if msg != NULL:
          msgs.push_back(string(msg.getData(), msg.getSize()))
    return msgs

  cpdef update_msgs(self, float cur_time, vector[string] msgs):
    self.frame += 1
    self.updated = dict.fromkeys(self.updated, False)
    for msg in msgs:
      msg = log.Event.from_bytes(msg)

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

  cpdef all_alive(self, service_list=None):
    return True

  cpdef all_valid(self, service_list=None):
    return True

  cpdef all_alive_and_valid(self, service_list=None):
    return self.all_alive(service_list=service_list) and self.all_valid(service_list=service_list)

