# distutils: language = c++
# cython: c_string_encoding=ascii, language_level=3

import sys
from libcpp.string cimport string
from libcpp cimport bool
from libc cimport errno


from .messaging cimport Context as cppContext
from .messaging cimport SubSocket as cppSubSocket
from .messaging cimport PubSocket as cppPubSocket
from .messaging cimport Poller as cppPoller
from .messaging cimport Message as cppMessage
from .messaging cimport FakeEvent as cppFakeEvent, FakeEventPurpose as cppFakeEventPurpose


class MessagingError(Exception):
  pass


class MultiplePublishersError(MessagingError):
  pass


def toggle_fake_events(bool enabled):
  cppFakeEvent.toggle_fake_events(enabled)


cdef class FakeEvent:
  cdef cppFakeEvent * event;
  cdef string endpoint;
  cdef cppFakeEventPurpose purpose;

  def __dealloc__(self):
    del self.event
    cppFakeEvent.invalidate_and_deregister(self.endpoint, self.purpose)

  def create_and_register(self, string endpoint, int purpose):
    self.event = cppFakeEvent.create_and_register(endpoint, <cppFakeEventPurpose> purpose)
    self.endpoint = endpoint
    self.purpose = <cppFakeEventPurpose> purpose

  def set(self):
    self.event.set()

  def clear(self):
    return self.event.clear()

  def wait(self):
    self.event.wait()

  def peek(self):
    return self.event.peek()


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

  def send(self, bytes data):
    length = len(data)
    r = self.socket.send(<char*>data, length)

    if r != length:
      if errno.errno == errno.EADDRINUSE:
        raise MultiplePublishersError
      else:
        raise MessagingError

  def all_readers_updated(self):
    return self.socket.all_readers_updated()
