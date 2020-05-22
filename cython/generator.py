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
  'text': '_',
  'data': '_',
}

seen = []


def to_capnp_enum_name(name):
  s = ""
  for c in name:
    if c.isupper():
      s += '_' + c
    else:
      s += c.upper()
  return s


def gen_code(definition, node, name=None):
  global seen

  pyx, pxd = "", ""

  if name is None:
    name = node.name

  tp = getattr(definition, name)

  # Skip constants
  if not hasattr(tp, 'schema'):
    return "", "", None

  _, class_name = tp.schema.node.displayName.split(':')

  # Skip structs with templates
  if class_name in ("Map"):
    return "", "", None

  full_name = class_name.replace('.', '')
  if full_name in seen:
    return "", "", full_name

  seen.append(full_name)

  pxd += f"    cdef cppclass {full_name}Reader \"cereal::{class_name.replace('.', '::')}::Reader\":\n"

  pyx += f"from log cimport {full_name}Reader\n\n"
  pyx += f"cdef class {full_name}(object):\n"
  pyx += f"    cdef {full_name}Reader reader\n\n"

  pyx += f"    def __init__(self, s=None):\n"
  pyx += f"        if s is not None:\n"
  pyx += f"            self.reader = ReaderFromBytes[{full_name}Reader](s, len(s))\n\n"

  pyx += f"    cdef set_reader(self, {full_name}Reader reader):\n"
  pyx += f"        self.reader = reader\n\n"

  added_fields = False

  nested_pyx, nested_pxd = "", ""

  fields_list = tp.schema.fields_list
  for field in fields_list:
    struct_full_name = None
    name = field.proto.name
    name_cap = name[0].upper() + name[1:]

    try:
      field_tp = field.proto.slot.type._which_str()
    except capnp.lib.capnp.KjException:
      print('no type', field)
      field_tp = None

    if field_tp not in TYPE_LOOKUP:
      if isinstance(field.schema, capnp.lib.capnp._EnumSchema):
        struct_type_name = field.schema.node.displayName.split(':')[-1]
        struct_type_name = struct_type_name.split('.')
        full_struct_name = "".join(struct_type_name)
        qualified_struct_name = "::".join(struct_type_name)

        enumerants = field.schema.enumerants
        if full_struct_name not in seen:
          print("first time", full_struct_name)
          seen.append(full_struct_name)

          nested_pxd += f"    cdef cppclass {full_struct_name}:\n"
          nested_pxd += f"        pass\n\n"
          for enum_name, val in enumerants.items():
            c_name = to_capnp_enum_name(enum_name)
            nested_pxd += f"    cdef {full_struct_name} {full_struct_name}_{enum_name} \"{qualified_struct_name}::{c_name}\"\n"
          nested_pxd += "\n"
        else:
          continue

      if isinstance(field.schema, capnp.lib.capnp._StructSchema):
        if len(field.schema.union_fields):
          # Some unions seem to have issues. Why not event?
          print("unions", field)
          continue
        else:
          # Struct
          struct_type_name = field.schema.node.displayName.split(':')[-1]
          struct_type_name = struct_type_name.split('.')[-1]

          if struct_type_name == "Map": # Map has a weird template, we don't support it
            continue

          # Nested struct
          if hasattr(tp, struct_type_name):
            pyx_, pxd_, struct_full_name = gen_code(tp, field, struct_type_name)
            nested_pyx += pyx_
            nested_pxd += pxd_
          else:
            struct_full_name = struct_type_name

    field_tp = field.proto.slot.type._which_str()

    if field_tp in ['list', 'text', 'data']:
      continue

    if field_tp == 'struct':
      if struct_full_name is None:
        continue
      field_tp = struct_full_name + "Reader"
      pxd += 8 * " " + f"{field_tp} get{name_cap}()\n"

      pyx += 4 * " " + f"@property\n"
      pyx += 4 * " " + f"def {name}(self):\n"
      pyx += 8 * " " + f"i = {struct_full_name}()\n"
      pyx += 8 * " " + f"i.set_reader(self.reader.get{name_cap}())\n"
      pyx += 8 * " " + f"return i\n\n"
    elif field_tp == 'enum':
      pxd += 8 * " " + f"{full_struct_name} get{name_cap}()\n"

      pyx += 4 * " " + f"@property\n"
      pyx += 4 * " " + f"def {name}(self):\n"
      pyx += 8 * " " + f"return <int>self.reader.get{name_cap}()\n\n"
    else:
      field_tp = TYPE_LOOKUP[field_tp]
      pxd += 8 * " " + f"{field_tp} get{name_cap}()\n"

      pyx += 4 * " " + f"@property\n"
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
  pxd = PXD
  pyx = ""
  for capnp_name, definition in [('car', car), ('log', log)]:
    pxd += f"cdef extern from \"../gen/cpp/{capnp_name}.capnp.c++\":\n    pass\n\n"
    pxd += f"cdef extern from \"../gen/cpp/{capnp_name}.capnp.h\":\n"

    pyx += f"from log cimport ReaderFromBytes\n\n"

    for node in definition.schema.node.nestedNodes:
      pyx_, pxd_, _ = gen_code(definition, node)
      pxd += pxd_
      pyx += pyx_

  with open(f'log.pxd', 'w') as f:
    f.write(pxd)

  with open(f'log.pyx', 'w') as f:
    f.write(pyx)
