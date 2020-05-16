from car cimport CarStateFromBytes, CarStateReader


cdef class CarState(object):
    cdef CarStateReader reader

    def __init__(self, s):
        self.reader = CarStateFromBytes(s, len(s))

    @property
    def brake(self):
        return self.reader.getBrake()
