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

from posix.time cimport clock_gettime, timespec, CLOCK_MONOTONIC_RAW, clockid_t
IF UNAME_SYSNAME == "Darwin":
  # Darwin doesn't have a CLOCK_BOOTTIME
  CLOCK_BOOTTIME = CLOCK_MONOTONIC_RAW
ELSE:
  from posix.time cimport CLOCK_BOOTTIME

cdef double sec_since_boot():
  cdef timespec ts
  cdef double current

  clock_gettime(CLOCK_BOOTTIME, &ts)
  current = ts.tv_sec + (ts.tv_nsec / 1000000000.)
  return current


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
    vector[string] ignore_alive
    vector[string] services_cpp

    cppmap[string, bool] _alive
    cppmap[string, bool] _valid
    cppmap[string, float] _freq
    cppmap[string, float] _rcv_time

  cdef readonly:
    int frame
    dict updated
    #dict alive
    #dict valid
    #dict rcv_time
    dict rcv_frame
    #dict freq
    dict logMonoTime
    dict sock
    dict data
    list services

  def __init__(self, services, vector[string] ignore_alive=[], string addr=b"127.0.0.1"):
    self.services = services
    self.services_cpp = services
    self.ignore_alive = ignore_alive

    self.frame = -1

    self.updated = {s: False for s in services}
    #self.rcv_time = {s: 0. for s in services}
    self.rcv_frame = {s: 0 for s in services}
    #self.alive = {s: False for s in services}
    self.sock = {}
    #self.freq = {}
    self.data = {}
    self.logMonoTime = {}
    #self.valid = {}

    self.poller = cppPoller.create()
    for s in self.services_cpp:
      py_str = s.decode('utf8')

      self._alive[s] = False
      self._valid[s] = True
      self._freq[s] = service_list[py_str].frequency
      self._rcv_time[s] = 0.

      sock = SubSocket()
      sock.connect(context, s, addr, True)
      self.poller.registerSocket(sock.socket)
      self.sock[py_str] = sock

      data = log.Event.new_message()
      try:
        data.init(py_str)
      except capnp.lib.capnp.KjException:
        data.init(py_str, 0) # lists
      self.data[py_str] = getattr(data, py_str)
      self.logMonoTime[py_str] = 0

  def __dealloc__(self):
    del self.poller

  def __getitem__(self, s):
    return self.data[s]

  @property
  def alive(self):
    return {s: <bool>self._alive[s] for s in self.services}

  @property
  def valid(self):
    return {s: <bool>self._valid[s] for s in self.services}

  @property
  def rcv_time(self):
    return {s: self._rcv_time[s] for s in self.services}

  cpdef update(self, int timeout=1000):
    msgs = self._update(timeout)
    self.update_msgs(sec_since_boot(), msgs)

  cdef _update(self, int timeout):
    with nogil:
      result = self.poller.poll(timeout)

    msgs = []
    for s in result:
      msg = s.receive(True)
      if msg != NULL:
        m = log.Event.from_bytes(msg.getData()[:msg.getSize()])
        msgs.append(m)
        del msg
    return msgs

  cdef update_msgs(self, float cur_time, list msgs):
    self.frame += 1
    for k in self.services:
      self.updated[k] = False

    cdef int i = 0
    for i in range(len(msgs)):
      msg = msgs[i]
      s = msg.which()
      self.updated[s] = True
      self.rcv_frame[s] = self.frame
      self.data[s] = getattr(msg, s)
      self.logMonoTime[s] = msg.logMonoTime

      self._valid[s.encode('utf8')] = msg.valid
      self._rcv_time[s.encode('utf8')] = cur_time

    for s in self.services_cpp:
      # arbitrary small number to avoid float comparison. If freq is 0, we can skip the check
      if self._freq[s] > 1e-5:
        # alive if delay is within 10x the expected frequency
        self._alive[s] = (cur_time - self._rcv_time[s]) < (10. / self._freq[s])
      else:
        self._alive[s] = True

  cdef _check_valid_alive(self, bool check_valid, bool check_alive, vector[string] service_list=[]):
    if service_list.size() == 0:
      service_list = self.services_cpp
    for s in service_list:
      if (check_alive and not self._alive[s]) or (check_valid and not self._valid[s]):
        return False
    return True

  cpdef all_alive(self, vector[string] service_list=[]):
    return self._check_valid_alive(False, True, service_list)

  cpdef all_valid(self, vector[string] service_list=[]):
    return self._check_valid_alive(True, False, service_list)

  cpdef all_alive_and_valid(self, vector[string] service_list=[]):
    return self._check_valid_alive(True, True, service_list)

