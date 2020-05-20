from car cimport ReaderFromBytes, CarStateReader


cdef class CarState(object):
    cdef CarStateReader reader

    def __init__(self, s):
        self.reader = ReaderFromBytes[CarStateReader](s, len(s))

    @property
    def brake(self):
        return self.reader.getBrake()
