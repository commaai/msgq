import os
import ctypes
import socket
import struct
import mmap
import time
import platform
import numpy as np
from typing import List, Optional, Set, Dict, Union, cast

import msgq
from msgq.visionipc.constants import VisionStreamType

# --- OpenCL Bindings ---
_cl_lib = None
try:
  if platform.system() == "Darwin":
    _cl_lib = ctypes.CDLL("/System/Library/Frameworks/OpenCL.framework/OpenCL")
  else:
    try:
        _cl_lib = ctypes.CDLL("libOpenCL.so.1")
    except OSError:
        _cl_lib = ctypes.CDLL("libOpenCL.so")
except OSError:
  pass

# CL Types
cl_device_id = ctypes.c_void_p
cl_context = ctypes.c_void_p
cl_command_queue = ctypes.c_void_p
cl_mem = ctypes.c_void_p
cl_int = ctypes.c_int
cl_bool = ctypes.c_uint

CL_MEM_READ_WRITE = 1
CL_MEM_USE_HOST_PTR = 8
CL_FALSE = 0
CL_TRUE = 1

if _cl_lib:
  _cl_lib.clCreateBuffer.argtypes = [cl_context, ctypes.c_uint64, ctypes.c_size_t, ctypes.c_void_p, ctypes.POINTER(cl_int)]
  _cl_lib.clCreateBuffer.restype = cl_mem

  _cl_lib.clCreateCommandQueue.argtypes = [cl_context, cl_device_id, ctypes.c_uint64, ctypes.POINTER(cl_int)]
  _cl_lib.clCreateCommandQueue.restype = cl_command_queue

  _cl_lib.clEnqueueReadBuffer.argtypes = [
    cl_command_queue, cl_mem, cl_bool, ctypes.c_size_t, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_void_p
  ]
  _cl_lib.clEnqueueReadBuffer.restype = cl_int

  _cl_lib.clEnqueueWriteBuffer.argtypes = [
    cl_command_queue, cl_mem, cl_bool, ctypes.c_size_t, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_void_p
  ]
  _cl_lib.clEnqueueWriteBuffer.restype = cl_int

  _cl_lib.clFinish.argtypes = [cl_command_queue]
  _cl_lib.clFinish.restype = cl_int

  _cl_lib.clReleaseMemObject.argtypes = [cl_mem]
  _cl_lib.clReleaseMemObject.restype = cl_int

  _cl_lib.clReleaseCommandQueue.argtypes = [cl_command_queue]
  _cl_lib.clReleaseCommandQueue.restype = cl_int


# --- ION Support ---
class IOCTL:
  # Python implementation of standard Linux _IOC macros
  @staticmethod
  def _IOC(direction, ioctl_type, nr, size):
    return (direction << 30) | (ioctl_type << 8) | nr | (size << 16)

  @staticmethod
  def _IOWR(ioctl_type, nr, struct_or_size):
    size = ctypes.sizeof(struct_or_size) if isinstance(struct_or_size, type) else struct_or_size
    return IOCTL._IOC(3, ioctl_type, nr, size)

# ion.h
ION_IOC_MAGIC = ord('I')
ION_FLAG_CACHED = 1

class ion_allocation_data(ctypes.Structure):
  _fields_ = [
    ("len", ctypes.c_size_t),
    ("align", ctypes.c_size_t),
    ("heap_id_mask", ctypes.c_uint32),
    ("flags", ctypes.c_uint32),
    ("handle", ctypes.c_uint32), # ion_user_handle_t
  ]

class ion_fd_data(ctypes.Structure):
  _fields_ = [
    ("handle", ctypes.c_uint32),
    ("fd", ctypes.c_int32),
  ]

class ion_handle_data(ctypes.Structure):
  _fields_ = [
    ("handle", ctypes.c_uint32),
  ]

class ion_flush_data(ctypes.Structure):
  _fields_ = [
    ("handle", ctypes.c_uint32),
    ("vaddr", ctypes.c_void_p),
    ("offset", ctypes.c_uint32),
    ("length", ctypes.c_uint32),
  ]

class ion_custom_data(ctypes.Structure):
  _fields_ = [
    ("cmd", ctypes.c_uint32),
    ("arg", ctypes.c_ulong),
  ]

# msm_ion.h
ION_IOMMU_HEAP_ID = 25
ION_IOC_CUSTOM = IOCTL._IOWR(ION_IOC_MAGIC, 6, ion_custom_data)
ION_IOC_CLEAN_CACHES = 0
ION_IOC_INV_CACHES = 1
ION_IOC_CLEAN_INV_CACHES = 2

# Standard ION ioctls
ION_IOC_ALLOC = IOCTL._IOWR(ION_IOC_MAGIC, 0, ion_allocation_data)
ION_IOC_FREE = IOCTL._IOWR(ION_IOC_MAGIC, 1, ion_handle_data)
ION_IOC_SHARE = IOCTL._IOWR(ION_IOC_MAGIC, 4, ion_fd_data)
ION_IOC_IMPORT = IOCTL._IOWR(ION_IOC_MAGIC, 5, ion_fd_data)

# OpenCL ION
CL_MEM_EXT_HOST_PTR_QCOM = 0x20000000
CL_MEM_ION_HOST_PTR_QCOM = 0x20000001
CL_MEM_HOST_UNCACHED_QCOM = 0x20000003

class cl_mem_ext_host_ptr(ctypes.Structure):
  _fields_ = [
    ("allocation_type", ctypes.c_uint),
    ("host_cache_policy", ctypes.c_uint),
  ]

class cl_mem_ion_host_ptr(ctypes.Structure):
  _fields_ = [
    ("ext_host_ptr", cl_mem_ext_host_ptr),
    ("ion_filedesc", ctypes.c_int),
    ("ion_hostptr", ctypes.c_void_p),
  ]


class VisionIpcBufExtra(ctypes.Structure):
  _fields_ = [
    ("frame_id", ctypes.c_uint32),
    ("timestamp_sof", ctypes.c_uint64),
    ("timestamp_eof", ctypes.c_uint64),
    ("valid", ctypes.c_bool),
  ]

class VisionIpcPacket(ctypes.Structure):
  _fields_ = [
    ("server_id", ctypes.c_uint64),
    ("idx", ctypes.c_size_t),
    ("extra", VisionIpcBufExtra),
  ]


# --- Helpers ---
def get_endpoint_name(name: str, stream: VisionStreamType) -> str:
  if "ZMQ" in os.environ:
    return str(9000 + int(stream))
  else:
    return f"visionipc_{name}_{int(stream)}"

def get_ipc_path(name: str) -> str:
  prefix = os.environ.get("OPENPILOT_PREFIX", "")
  if prefix:
    prefix += "_"
  return f"/tmp/{prefix}visionipc_{name}"


class VisionBuf:
  def __init__(self):
    self.len = 0
    self.mmap_len = 0
    self.addr = 0
    self.fd = -1
    self.width = 0
    self.height = 0
    self.stride = 0
    self.uv_offset = 0
    self.type = 0
    self.idx = 0
    self.server_id = 0
    self.buf_cl = None
    self.copy_q = None
    self.ctx = None
    self.device_id = None
    self._mmap = None

    self.handle = 0 # ION handle
    self.ion_fd = -1 # ION device fd

  @property
  def using_ion(self):
    return self.ion_fd >= 0

  def _open_ion(self):
    try:
      self.ion_fd = os.open("/dev/ion", os.O_RDWR | os.O_NONBLOCK)
    except OSError:
      self.ion_fd = -1

  def allocate(self, length: int):
    self.len = length

    # Try ION first
    if os.path.exists("/dev/ion"):
      self._open_ion()

    if self.using_ion:
      # PADDING_CL = 0, sizeof(uint64_t) = 8
      alloc_len = length + 8
      alloc_len = (alloc_len + 4095) & ~4095 # Align 4096

      alloc = ion_allocation_data()
      alloc.len = alloc_len
      alloc.align = 4096
      alloc.heap_id_mask = 1 << ION_IOMMU_HEAP_ID
      alloc.flags = ION_FLAG_CACHED

      try:
         if ctypes.pythonapi.ioctl(self.ion_fd, ION_IOC_ALLOC, ctypes.byref(alloc)) != 0:
            raise OSError("ION alloc failed")
      except Exception:
         self.ion_fd = -1
         raise

      self.handle = alloc.handle
      self.mmap_len = alloc.len

      # Share to get fd
      fd_data = ion_fd_data()
      fd_data.handle = self.handle
      if ctypes.pythonapi.ioctl(self.ion_fd, ION_IOC_SHARE, ctypes.byref(fd_data)) != 0:
         raise OSError("ION share failed")

      self.fd = fd_data.fd
      self._mmap = mmap.mmap(self.fd, self.mmap_len)
      self.addr = ctypes.addressof(ctypes.c_char.from_buffer(self._mmap))

      # Clear memory
      ctypes.memset(self.addr, 0, self.mmap_len)
      return

    # Create shm file
    # We need a unique name. Logic from visionbuf_cl.cc: visionbuf_{pid}_{offset}
    base_dir = "/dev/shm" if platform.system() != "Darwin" else "/tmp"
    self.mmap_len = self.len + 8
    name = f"{base_dir}/visionbuf_{os.getpid()}_{id(self)}"
    self.fd = os.open(name, os.O_RDWR | os.O_CREAT | os.O_EXCL, 0o664)
    os.unlink(name)
    os.ftruncate(self.fd, self.mmap_len)

    self._mmap = mmap.mmap(self.fd, self.mmap_len)
    self.addr = ctypes.addressof(ctypes.c_char.from_buffer(self._mmap))

  def import_fd(self, fd: int, length: int):
    self.fd = fd
    self.len = length

    # Try to import as ION first if available
    if os.path.exists("/dev/ion"):
       self._open_ion()

    if self.using_ion:
       # Import
       fd_data = ion_fd_data()
       fd_data.fd = self.fd
       if ctypes.pythonapi.ioctl(self.ion_fd, ION_IOC_IMPORT, ctypes.byref(fd_data)) != 0:
          raise OSError("ION import failed")

       self.handle = fd_data.handle
       self.mmap_len = self.len + 8
       self.mmap_len = (self.mmap_len + 4095) & ~4095

       self._mmap = mmap.mmap(self.fd, self.mmap_len)
       self.addr = ctypes.addressof(ctypes.c_char.from_buffer(self._mmap))
       return

    self.mmap_len = self.len + 8
    self._mmap = mmap.mmap(self.fd, self.mmap_len)
    self.addr = ctypes.addressof(ctypes.c_char.from_buffer(self._mmap))

  def init_cl(self, device_id, ctx):
    if not _cl_lib:
        return

    if self.using_ion:
       err = ctypes.c_int()
       ion_cl = cl_mem_ion_host_ptr()
       ion_cl.ext_host_ptr.allocation_type = CL_MEM_ION_HOST_PTR_QCOM
       ion_cl.ext_host_ptr.host_cache_policy = CL_MEM_HOST_UNCACHED_QCOM
       ion_cl.ion_filedesc = self.fd
       ion_cl.ion_hostptr = self.addr

       self.buf_cl = _cl_lib.clCreateBuffer(ctx, CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM, self.len, ctypes.byref(ion_cl), ctypes.byref(err))
       assert err.value == 0
       return

    self.device_id = device_id
    self.ctx = ctx
    err = ctypes.c_int()
    self.copy_q = _cl_lib.clCreateCommandQueue(ctx, device_id, 0, ctypes.byref(err))
    assert err.value == 0
    self.buf_cl = _cl_lib.clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, self.len, self.addr, ctypes.byref(err))
    assert err.value == 0

  def init_msgq(self, width, height, stride, uv_offset):
    self.width = width
    self.height = height
    self.stride = stride
    self.uv_offset = uv_offset

  def sync(self, to_device: bool):
    if not self.buf_cl or not _cl_lib:
        return
    err = 0
    if to_device: # SYNC_TO_DEVICE = 1
         # Write host -> device (EnqueueWriteBuffer)
         err = _cl_lib.clEnqueueWriteBuffer(self.copy_q, self.buf_cl, CL_FALSE, 0, self.len, self.addr, 0, None, None)
    else:
         err = _cl_lib.clEnqueueReadBuffer(self.copy_q, self.buf_cl, CL_FALSE, 0, self.len, self.addr, 0, None, None)

    if err == 0:
        _cl_lib.clFinish(self.copy_q)

    if self.using_ion:
       # Flush cache defined in generic sync
       flush = ion_flush_data()
       flush.handle = self.handle
       flush.vaddr = self.addr
       flush.offset = 0
       flush.length = self.len

       custom = ion_custom_data()
       custom.cmd = ION_IOC_INV_CACHES if not to_device else ION_IOC_CLEAN_CACHES
       custom.arg = ctypes.addressof(flush)

       ctypes.pythonapi.ioctl(self.ion_fd, ION_IOC_CUSTOM, ctypes.byref(custom))

  def free(self):
    if self.buf_cl and _cl_lib:
      _cl_lib.clReleaseMemObject(self.buf_cl)
      _cl_lib.clReleaseCommandQueue(self.copy_q)
      self.buf_cl = None
    if self._mmap:
      self._mmap.close()
    if self.fd >= 0:
      os.close(self.fd)
      self.fd = -1

    if self.using_ion:
       handle_data = ion_handle_data()
       handle_data.handle = self.handle
       try:
           ctypes.pythonapi.ioctl(self.ion_fd, ION_IOC_FREE, ctypes.byref(handle_data))
       except Exception:
           pass
       os.close(self.ion_fd)
       self.ion_fd = -1

  def set_frame_id(self, frame_id: int):
    if self._mmap is not None:
      struct.pack_into("Q", self._mmap, self.len, frame_id)

  def get_frame_id(self) -> int:
    if self._mmap is not None:
      val = struct.unpack_from("Q", self._mmap, self.len)[0]
      return int(val)
    return 0

  @property
  def data(self):
    return np.frombuffer(self._mmap, dtype=np.uint8, count=self.len)


class VisionIpcServer:
  def __init__(self, name: str, device_id=None, cl_context=None):
    self.name = name
    self.device_id = device_id
    self.cl_context = cl_context
    self.buffers: Dict[VisionStreamType, List[VisionBuf]] = {}
    self.sockets: Dict[VisionStreamType, msgq.PubSocket] = {}
    self.cur_idx: Dict[VisionStreamType, int] = {}
    self.server_id = os.urandom(8)
    self.server_id_int = struct.unpack("Q", self.server_id)[0]
    self.running = False
    self.listener_socket: Optional[socket.socket] = None

  def create_buffers(self, stream: VisionStreamType, num_buffers: int, width: int, height: int):
      size = width * height * 3 // 2
      stride = width
      uv_offset = width * height
      self.create_buffers_with_sizes(stream, num_buffers, width, height, size, stride, uv_offset)

  def create_buffers_with_sizes(self, stream: VisionStreamType, num_buffers: int, width: int, height: int, size: int, stride: int, uv_offset: int):
      bufs = []
      for i in range(num_buffers):
          buf = VisionBuf()
          buf.allocate(size)
          buf.init_msgq(width, height, stride, uv_offset)
          buf.idx = i
          buf.type = stream
          buf.server_id = self.server_id_int
          if self.device_id and self.cl_context:
              buf.init_cl(self.device_id, self.cl_context)
          bufs.append(buf)

      self.buffers[stream] = bufs
      self.cur_idx[stream] = 0

      # Create PubSocket
      ctx = msgq.Context()
      sock = msgq.PubSocket()
      sock.connect(ctx, get_endpoint_name(self.name, stream))
      self.sockets[stream] = sock

  def get_buffer(self, stream: VisionStreamType) -> VisionBuf:
      bufs = self.buffers[stream]
      idx = self.cur_idx[stream]
      self.cur_idx[stream] = (idx + 1) % len(bufs)
      return bufs[idx]

  def send(
    self,
    stream_or_buf: Union[VisionStreamType, VisionBuf],
    data: Union[bytes, np.ndarray, bool] = False,
    frame_id: int = 0,
    timestamp_sof: int = 0,
    timestamp_eof: int = 0
  ):
      if isinstance(stream_or_buf, VisionBuf):
          buf = stream_or_buf
          extra_valid = data # 2nd arg is extra_valid bool
      else:
          stream = stream_or_buf
          buf = self.get_buffer(stream)
          if isinstance(data, (bytes, bytearray)):
             ctypes.memmove(buf.addr, data, min(len(data), buf.len))
          elif isinstance(data, np.ndarray):
             ctypes.memmove(buf.addr, data.ctypes.data, min(data.nbytes, buf.len))
          extra_valid = True

      # Sync if needed (from device to host)
      buf.sync(False)

      packet = VisionIpcPacket()
      packet.server_id = self.server_id_int
      packet.idx = buf.idx
      packet.extra.valid = extra_valid
      packet.extra.frame_id = frame_id
      packet.extra.timestamp_sof = timestamp_sof
      packet.extra.timestamp_eof = timestamp_eof

      self.sockets[VisionStreamType(buf.type)].send(bytes(packet))

  def start_listener(self):
      self.running = True
      path = get_ipc_path(self.name)
      if os.path.exists(path):
          os.unlink(path)

      self.listener_socket = socket.socket(socket.AF_UNIX, socket.SOCK_SEQPACKET if platform.system() != "Darwin" else socket.SOCK_STREAM)
      # mypy: listen and settimeout are methods of socket.socket, but if listener_socket is Optional check it.
      assert self.listener_socket is not None
      self.listener_socket.bind(path)
      self.listener_socket.listen(3)
      self.listener_socket.settimeout(0.1)

      import threading
      self.thread = threading.Thread(target=self._listener_loop, daemon=True)
      self.thread.start()

  def _listener_loop(self):
      while self.running and self.listener_socket:
          try:
              conn, _ = self.listener_socket.accept()
          except socket.timeout:
              continue
          except OSError:
              break

          try:
              # Receive stream type request
              data = conn.recv(1024)
              if not data:
                conn.close()
                continue

              requested_stream = struct.unpack("i", data[:4])[0] # VisionStreamType is int

              if requested_stream == 4: # VISION_STREAM_MAX -> Request available streams
                   available = list(self.buffers.keys())
                   # Pack available streams (int array)
                   resp = struct.pack(f"{len(available)}i", *available)
                   conn.send(resp)

              elif requested_stream in self.buffers:
                  bufs = self.buffers[requested_stream]
                  fds = [b.fd for b in bufs]

                  # Send FDs and metadata

                  meta = b""
                  for b in bufs:
                      meta += struct.pack("QQQQQ", b.width, b.height, b.stride, b.uv_offset, b.len)

                  ancillary = [(socket.SOL_SOCKET, socket.SCM_RIGHTS, struct.pack(f"{len(fds)}i", *fds))]
                  conn.sendmsg([meta], ancillary)

              conn.close()
          except Exception:
              conn.close()

  def close(self):
      self.running = False
      if self.listener_socket:
          self.listener_socket.close()
          self.listener_socket = None
      for sock in self.sockets.values():
          sock.close()
      self.sockets.clear()

  def __del__(self):
      self.close()


class VisionIpcClient:
  def __init__(self, name: str, stream: VisionStreamType, conflate: bool, device_id=None, cl_context=None):
    self.sock: Optional[msgq.SubSocket] = None
    self.poller: Optional[msgq.Poller] = None
    self.connected = False
    self.frame_id = 0
    self.timestamp_sof = 0
    self.timestamp_eof = 0

    self.name = name
    self.stream = stream
    self.conflate = conflate
    self.device_id = device_id
    self.cl_context = cl_context
    if cl_context is not None and not isinstance(cl_context, (ctypes.c_void_p, int)):
      raise TypeError("cl_context must be a pointer or int")
    self.buffers: List[VisionBuf] = []

  def close(self):
      sock = getattr(self, 'sock', None)
      if sock:
          # SubSocket doesn't need explicit close usually, but good practice if wrapper supported it?
          # wrapper (SubSocket) doesn't have close...
          # But we have poller.
          pass
      poller = getattr(self, 'poller', None)
      if poller:
          # Poller has destroy in __del__
          pass
      # We should clear buffers to free memory/FDs
      buffers = getattr(self, 'buffers', [])
      for buf in buffers:
          buf.free()
      self.buffers = []

  def __del__(self):
      self.close()

  @property
  def is_connected(self):
    return self.connected

  @property
  def width(self):
    return self.buffers[0].width if self.buffers else 0

  @property
  def height(self):
    return self.buffers[0].height if self.buffers else 0

  @property
  def buffer_len(self):
    return self.buffers[0].len if self.buffers else 0

  @property
  def num_buffers(self):
    return len(self.buffers)

  def connect(self, blocking: bool = True):
    # Connect to server to get FDs
    path = get_ipc_path(self.name)
    start = time.time()

    while True:
        try:
            sock_type = socket.SOCK_SEQPACKET if platform.system() != "Darwin" else socket.SOCK_STREAM
            s_sock = socket.socket(socket.AF_UNIX, sock_type)
            s_sock.connect(path)

            # Request stream
            s_sock.send(struct.pack("i", int(self.stream)))

            # Recv metadata and FDs
            iov, ancillary, flags, addr = s_sock.recvmsg(8192, 4096)
            s_sock.close()

            fds: List[int] = []
            for cmsg_level, cmsg_type, cmsg_data in ancillary:
                if cmsg_level == socket.SOL_SOCKET and cmsg_type == socket.SCM_RIGHTS:
                    fds.extend(struct.unpack(f"{len(cmsg_data)//4}i", cmsg_data))

            num_buffers = len(fds)
            if num_buffers == 0:
                if blocking and (time.time() - start < 5): # Retry for a bit?
                   time.sleep(0.1)
                   continue
                return False

            # Parse metadata
            # 40 bytes per buffer
            self.buffers = []
            for i in range(num_buffers):
                width, height, stride, uv_offset, length = struct.unpack_from("QQQQQ", iov, i*40)
                buf = VisionBuf()
                buf.import_fd(fds[i], length)
                buf.init_msgq(width, height, stride, uv_offset)
                buf.idx = i
                if self.device_id and self.cl_context:
                    buf.init_cl(self.device_id, self.cl_context)
                self.buffers.append(buf)

            # Connect to msgq
            ctx = msgq.Context()
            self.sock = msgq.SubSocket()
            assert self.sock is not None
            self.sock.connect(ctx, get_endpoint_name(self.name, self.stream), "127.0.0.1", self.conflate)
            self.poller = msgq.Poller()
            assert self.poller is not None
            self.poller.registerSocket(self.sock)

            self.connected = True
            return True

        except (OSError, ConnectionRefusedError, FileNotFoundError):
            if not blocking:
                return False
            time.sleep(0.1)

  def recv(self, timeout_ms: int = 100) -> Optional[VisionBuf]:
      if not self.connected:
          return None

      assert self.poller is not None
      polled = self.poller.poll(timeout_ms)
      if not polled:
          return None

      assert self.sock is not None
      msg = self.sock.receive(non_blocking=True)
      if not msg:
          return None

      # Parse VisionIpcPacket
      if len(msg) < ctypes.sizeof(VisionIpcPacket):
          return None

      packet = VisionIpcPacket.from_buffer_copy(msg)

      # Update client state from packet
      self.frame_id = packet.extra.frame_id
      self.timestamp_sof = packet.extra.timestamp_sof
      self.timestamp_eof = packet.extra.timestamp_eof

      if packet.idx >= len(self.buffers):
          return None

      buf = self.buffers[packet.idx]

      if buf.server_id == 0:
        buf.server_id = packet.server_id
      elif buf.server_id != packet.server_id:
        # Server restarted?
        self.connected = False
        return None

      # Sync host -> device if using OpenCL
      # Server writes to shared memory (host), client must update device buffer.
      if self.device_id and self.cl_context:
          buf.sync(True) # To device

      return cast(VisionBuf, buf)


  @staticmethod
  def available_streams(name: str, blocking: bool = True) -> Set[VisionStreamType]:
      path = get_ipc_path(name)
      try:
            sock_type = socket.SOCK_SEQPACKET if platform.system() != "Darwin" else socket.SOCK_STREAM
            s_sock = socket.socket(socket.AF_UNIX, sock_type)
            s_sock.connect(path)
            s_sock.send(struct.pack("i", 4)) # VISION_STREAM_MAX

            data = s_sock.recv(1024)
            s_sock.close()
            if not data:
                return set()

            num = len(data) // 4
            streams = struct.unpack(f"{num}i", data)
            return {VisionStreamType(s) for s in streams}
      except OSError:
          return set()

