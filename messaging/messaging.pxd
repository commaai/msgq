# distutils: language = c++
#cython: language_level=3

from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp cimport bool


cdef extern from "cereal/messaging/impl_fake.h":
  cdef cppclass FakeEventPurpose:
    pass 

  cdef cppclass FakeEvent:
    @staticmethod
    FakeEvent * create_and_register(string, FakeEventPurpose)
    @staticmethod
    void invalidate_and_deregister(string, FakeEventPurpose)
    @staticmethod
    void toggle_fake_events(bool)
    void set()
    int clear()
    void wait()
    bool peek()


cdef extern from "cereal/messaging/messaging.h":
  cdef cppclass Context:
    @staticmethod
    Context * create()

  cdef cppclass Message:
    void init(size_t)
    void init(char *, size_t)
    void close()
    size_t getSize()
    char *getData()

  cdef cppclass SubSocket:
    @staticmethod
    SubSocket * create()
    int connect(Context *, string, string, bool)
    Message * receive(bool)
    void setTimeout(int)

  cdef cppclass PubSocket:
    @staticmethod
    PubSocket * create()
    int connect(Context *, string)
    int sendMessage(Message *)
    int send(char *, size_t)
    bool all_readers_updated()

  cdef cppclass Poller:
    @staticmethod
    Poller * create()
    void registerSocket(SubSocket *)
    vector[SubSocket*] poll(int) nogil
