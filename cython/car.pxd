from libcpp cimport bool

cdef extern from "capnp_wrapper.h":
    cdef T ReaderFromBytes[T](char* dat, size_t sz)

cdef extern from "../gen/cpp/car.capnp.c++":
    pass

cdef extern from "../gen/cpp/car.capnp.h":
    cdef cppclass CarStateReader "cereal::CarState::Reader":
        float getBrake()
