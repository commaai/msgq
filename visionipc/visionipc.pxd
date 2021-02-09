# distutils: language = c++
#cython: language_level=3

from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp cimport bool

cdef extern from "visionbuf.h":
  cdef enum VisionStreamType:
    VISION_STREAM_RGB_BACK "VISION_STREAM_RGB_BACK"

  cdef cppclass VisionBuf:
    void * addr

cdef extern from "visionipc_server.h":
  cdef cppclass VisionIpcServer:
    VisionIpcServer(string, void*, void*)
    VisionBuf * get_buffer(VisionStreamType)
