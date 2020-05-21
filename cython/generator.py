#!/usr/bin/env python3

import capnp
from cereal import car, log

PXD = """from libcpp cimport bool
from libc.stdint cimport *

cdef extern from "capnp_wrapper.h":
    cdef T ReaderFromBytes[T](char* dat, size_t sz)

"""

TYPE_LOOKUP = {
  'float32': 'float',
  'float64': 'double',
  'bool': 'bool',
  'int8': 'int8_t',
  'int16': 'int16_t',
  'int32': 'int32_t',
  'int64': 'int64_t',
  'uint8': 'uint8_t',
  'uint16': 'uint16_t',
  'uint32': 'uint32_t',
  'uint64': 'uint64_t',
}

seen = []


def gen_code(definition, node, name=None):
  global seen

  pyx, pxd = "", ""

  if name is None:
    name = node.name

  print(definition, name)
  tp = getattr(definition, name)
  print("tp", tp)

  # Skip constants
  if not hasattr(tp, 'schema'):
    return "", "", None

  _, class_name = tp.schema.node.displayName.split(':')

  # Skip structs with templates
  if class_name in ("Map"):
    return "", "", None

  full_name = class_name.replace('.', '')
  if full_name in seen:
    return "", "", None

  seen.append(full_name)

  pxd += f"    cdef cppclass {full_name}Reader \"cereal::{class_name.replace('.', '::')}::Reader\":\n"

  pyx += f"from {capnp_name} cimport {full_name}Reader\n\n"
  pyx += f"cdef class {full_name}(object):\n"
  pyx += f"    cdef {full_name}Reader reader\n\n"

  pyx += f"    def __init__(self, s=None):\n"
  pyx += f"        if s is not None:"
  pyx += f"            self.reader = ReaderFromBytes[{full_name}Reader](s, len(s))\n\n"

  pyx += f"    cdef set_reader(self, {full_name}Reader reader):\n"
  pyx += f"        self.reader = reader\n\n"

  added_fields = False

  nested_pyx, nested_pxd = "", ""

  print(class_name)

  fields_list = tp.schema.fields_list
  for field in fields_list:
    struct_full_name = None
    name = field.proto.name
    name_cap = name[0].upper() + name[1:]

    try:
      if len(field.schema.union_fields):
        continue

      # Normal struct
      struct_type_name = field.schema.node.displayName.split('.')[-1]
      pyx_, pxd_, struct_full_name = gen_code(tp, field, struct_type_name)
      nested_pyx += pyx_
      nested_pxd += pxd_

    except (capnp.lib.capnp.KjException, AttributeError) as e:
      print(e)

    field_tp = field.proto.slot.type._which_str()

    if field_tp in ['list', 'enum', 'text', 'data']:
      continue

    if field_tp == 'struct':
      if struct_full_name is None:
        continue
      field_tp = struct_full_name + "Reader"
      pyx += 4 * " " + f"@property\n"
      pxd += 8 * " " + f"{field_tp} get{name_cap}()\n"
      pyx += 4 * " " + f"def {name}(self):\n"
      pyx += 8 * " " + f"i = {struct_full_name}()\n\n"
      pyx += 8 * " " + f"i.set_reader(self.reader.get{name_cap}())\n\n"
      pyx += 8 * " " + f"return i\n\n"
    else:
      field_tp = TYPE_LOOKUP[field_tp]

      pyx += 4 * " " + f"@property\n"
      pxd += 8 * " " + f"{field_tp} get{name_cap}()\n"
      pyx += 4 * " " + f"def {name}(self):\n"
      pyx += 8 * " " + f"return self.reader.get{name_cap}()\n\n"
    added_fields = True

  if not added_fields:
    pxd += 8 * " " + "pass\n\n"
  else:
    pxd += "\n"

  pyx = nested_pyx + pyx
  pxd = nested_pxd + pxd

  return pyx, pxd, full_name

if __name__ == "__main__":
  # for capnp_name, definition in [('car', car)]:
  for capnp_name, definition in [('car', car), ('log', log)]:
    pxd = PXD
    pxd += f"cdef extern from \"../gen/cpp/{capnp_name}.capnp.c++\":\n    pass\n\n"
    pxd += f"cdef extern from \"../gen/cpp/{capnp_name}.capnp.h\":\n"

    pyx = f"from {capnp_name} cimport ReaderFromBytes\n\n"

    for node in definition.schema.node.nestedNodes:
      print()
      pyx_, pxd_, _ = gen_code(definition, node)
      pxd += pxd_
      pyx += pyx_

    with open(f'{capnp_name}.pxd', 'w') as f:
      f.write(pxd)

    with open(f'{capnp_name}.pyx', 'w') as f:
      f.write(pyx)
