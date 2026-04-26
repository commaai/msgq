# distutils: language = c++
#cython: language_level=3

from .visionipc cimport VisionBuf as cppVisionBuf

cdef class VisionBuf:
  cdef cppVisionBuf * buf
  cdef bint owner

  @staticmethod
  cdef create(cppVisionBuf*)
