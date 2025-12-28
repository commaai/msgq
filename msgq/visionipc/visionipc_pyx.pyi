"""Type stubs for msgq.visionipc.visionipc_pyx Cython module."""

from enum import IntEnum
from typing import Any, Optional
from typing_extensions import TypeAlias

import numpy as np
from numpy.typing import NDArray

# Cython with c_string_encoding=ascii accepts both str and bytes for string params
_String: TypeAlias = str | bytes
# Cython typed memoryview accepts bytes, bytearray, and numpy arrays
_BufferLike: TypeAlias = bytes | bytearray | NDArray[Any]

def get_endpoint_name(name: _String, stream: VisionStreamType) -> str: ...

class VisionStreamType(IntEnum):
    VISION_STREAM_ROAD = 0
    VISION_STREAM_DRIVER = 1
    VISION_STREAM_WIDE_ROAD = 2
    VISION_STREAM_MAP = 3

class VisionBuf:
    @property
    def data(self) -> NDArray[np.uint8]: ...
    @property
    def width(self) -> int: ...
    @property
    def height(self) -> int: ...
    @property
    def stride(self) -> int: ...
    @property
    def uv_offset(self) -> int: ...
    @property
    def idx(self) -> int: ...
    @property
    def fd(self) -> int: ...

class VisionIpcServer:
    def __init__(self, name: _String) -> None: ...
    def create_buffers(
        self, tp: VisionStreamType, num_buffers: int, width: int, height: int
    ) -> None: ...
    def create_buffers_with_sizes(
        self,
        tp: VisionStreamType,
        num_buffers: int,
        width: int,
        height: int,
        size: int,
        stride: int,
        uv_offset: int,
    ) -> None: ...
    def send(
        self,
        tp: VisionStreamType,
        data: _BufferLike,
        frame_id: int = 0,
        timestamp_sof: int = 0,
        timestamp_eof: int = 0,
    ) -> None: ...
    def start_listener(self) -> None: ...

class VisionIpcClient:
    def __init__(
        self,
        name: _String,
        stream: VisionStreamType,
        conflate: bool,
        context: Optional[object] = None,
    ) -> None: ...
    @property
    def width(self) -> Optional[int]: ...
    @property
    def height(self) -> Optional[int]: ...
    @property
    def stride(self) -> Optional[int]: ...
    @property
    def uv_offset(self) -> Optional[int]: ...
    @property
    def buffer_len(self) -> Optional[int]: ...
    @property
    def num_buffers(self) -> int: ...
    @property
    def frame_id(self) -> int: ...
    @property
    def timestamp_sof(self) -> int: ...
    @property
    def timestamp_eof(self) -> int: ...
    @property
    def valid(self) -> bool: ...
    def recv(self, timeout_ms: int = 100) -> Optional[VisionBuf]: ...
    def connect(self, blocking: bool) -> bool: ...
    def is_connected(self) -> bool: ...
    @staticmethod
    def available_streams(name: _String, block: bool) -> set[int]: ...
