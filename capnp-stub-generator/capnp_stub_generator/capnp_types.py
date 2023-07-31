"""Types definitions that are common in capnproto schemas."""
from __future__ import annotations

from types import ModuleType
from typing import Dict
from typing import Tuple

CAPNP_TYPE_TO_PYTHON = {
    "void": "None",
    "bool": "bool",
    "int8": "int",
    "int16": "int",
    "int32": "int",
    "int64": "int",
    "uint8": "int",
    "uint16": "int",
    "uint32": "int",
    "uint64": "int",
    "float32": "float",
    "float64": "float",
    "text": "str",
    "data": "bytes",
}


class CapnpFieldType:
    """Types of capnproto fields."""

    GROUP = "group"
    SLOT = "slot"


class CapnpElementType:
    """Types of capnproto elements."""

    BOOL = "bool"
    ENUM = "enum"
    STRUCT = "struct"
    CONST = "const"
    VOID = "void"
    LIST = "list"
    ANY_POINTER = "anyPointer"


ModuleRegistryType = Dict[int, Tuple[str, ModuleType]]
