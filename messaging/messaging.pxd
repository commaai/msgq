# distutils: language = c++
#cython: language_level=3

from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp cimport bool
from libc.stdint cimport uint64_t


cdef extern from "messaging.hpp":
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

  cdef cppclass Poller:
    @staticmethod
    Poller * create()
    void registerSocket(SubSocket *)
    vector[SubSocket*] poll(int) nogil

  cdef cppclass SubMaster:
    SubMaster(vector[string], string, vector[string])
    int update(int)

    bool allAlive(vector[string])
    bool allValid(vector[string])
    bool allAliveAndValid(vector[string])

    Message * getMessage(string)
    bool updated(string)
    uint64_t logMonoTime(string)
    uint64_t frame

  cdef cppclass PubMaster:
    PubMaster(vector[string])
    int send(string, char *, size_t)
