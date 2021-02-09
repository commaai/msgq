# distutils: language = c++
# cython: c_string_encoding=ascii, language_level=3

import sys
from libcpp.string cimport string
from libcpp cimport bool


from .visionipc cimport VisionIpcServer as cppVisionIpcServer

class VisionIpcServer:
  pass
