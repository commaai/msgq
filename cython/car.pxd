from libcpp cimport bool

cdef extern from "../gen/cpp/car.capnp.c++":
    pass

cdef extern from "car_wrapper.cc":
    pass


cdef extern from "../gen/cpp/car.capnp.h":
    cdef cppclass CarStateReader "cereal::CarState::Reader":
        float getBrake()

cdef extern from "car_wrapper.h":
    cdef CarStateReader CarStateFromBytes(char* dat, size_t sz)
