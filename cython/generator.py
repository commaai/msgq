#!/usr/bin/env python3

from cereal import car

pxd = """from libcpp cimport bool

cdef extern from "capnp_wrapper.h":
    cdef T ReaderFromBytes[T](char* dat, size_t sz)

"""

TYPE_LOOKUP = {
  'float32': 'float',
  'float64': 'double',
  'bool': 'bool',
}

if __name__ == "__main__":
  tp = car.CarState

  capnp_name, class_name = tp.schema.node.displayName.split(':')
  capnp_name, _ = capnp_name.split('.')

  pxd += f"cdef extern from \"../gen/cpp/{capnp_name}.capnp.c++\":\n    pass\n\n"
  pxd += f"cdef extern from \"../gen/cpp/{capnp_name}.capnp.h\":\n"
  pxd += f"    cdef cppclass {class_name}Reader \"cereal::{class_name}::Reader\":\n"

  pyx = f"from {capnp_name} cimport ReaderFromBytes, {class_name}Reader\n\n"
  pyx += f"cdef class {class_name}(object):\n"
  pyx += f"    cdef {class_name}Reader reader\n\n"
  pyx += f"    def __init__(self, s):\n"
  pyx += f"        self.reader = ReaderFromBytes[{class_name}Reader](s, len(s))\n\n"

  print(class_name)
  for field in tp.schema.fields_list:

    name = field.proto.name
    tp = field.proto.slot.type._which_str()

    if tp == 'list':
      continue

    if tp == 'struct':
      continue

    if tp == 'enum':
      continue

    name_cap = name[0].upper() + name[1:]

    tp = TYPE_LOOKUP[tp]
    pxd += 8 * " " + f"{tp} get{name_cap}()\n"
    pyx += 4 * " " + f"@property\n"
    pyx += 4 * " " + f"def {name}(self):\n"
    pyx += 8 * " " + f"return self.reader.get{name_cap}()\n\n"

    print(tp, name)

  with open('car.pxd', 'w') as f:
    f.write(pxd)

  with open('car.pyx', 'w') as f:
    f.write(pyx)
