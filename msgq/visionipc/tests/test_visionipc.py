import gc
import struct
import unittest
from typing import Optional
from msgq.visionipc import VisionIpcServer, VisionIpcClient


class TestVisionIpc(unittest.TestCase):
  server: Optional[VisionIpcServer]
  client: Optional[VisionIpcClient]

  def setUp(self):
    self.server = None
    self.client = None

  def tearDown(self):
    self.client = None
    self.server = None

  def setup_vipc(self, name, *stream_types, num_buffers=1, width=100, height=100, conflate=False):
    self.server = VisionIpcServer(name)
    for stream_type in stream_types:
      self.server.create_buffers(stream_type, num_buffers, width, height)
    self.server.start_listener()

    if len(stream_types):
      self.client = VisionIpcClient(name, stream_types[0], conflate)
      assert self.client.connect(True)
    else:
      self.client = None

    return self.server, self.client

  def test_connect(self):
    self.setup_vipc("camerad", 0)
    assert self.client is not None
    assert self.client.is_connected()

  def test_available_streams(self):
    stream_types = (0, 2)
    self.setup_vipc("camerad", *stream_types)
    available_streams = VisionIpcClient.available_streams("camerad", True)
    assert available_streams == set(stream_types)

  def test_buffers(self):
    width, height, num_buffers = 100, 200, 5
    self.setup_vipc("camerad", 0, num_buffers=num_buffers, width=width, height=height)
    assert self.client is not None
    assert self.client.width == width
    assert self.client.height == height
    assert self.client.buffer_len is not None and self.client.buffer_len > 0
    assert self.client.num_buffers == num_buffers

  def test_send_single_buffer(self):
    self.setup_vipc("camerad", 0)
    assert self.server is not None
    assert self.client is not None
    assert self.client.buffer_len is not None
    buf = bytearray(self.client.buffer_len)
    struct.pack_into("<Q", buf, 0, 1234)
    self.server.send(0, buf, frame_id=1337)

    recv_buf = self.client.recv()
    assert recv_buf is not None
    data = recv_buf.data
    assert isinstance(data, memoryview)
    assert struct.unpack_from("<Q", data, 0)[0] == 1234
    assert len(data) == self.client.buffer_len
    assert data[8:].nbytes == self.client.buffer_len - 8
    assert self.client.frame_id == 1337
    assert recv_buf.frame_id == 1337

  def test_no_conflate(self):
    self.setup_vipc("camerad", 0)
    assert self.server is not None
    assert self.client is not None
    assert self.client.buffer_len is not None
    buf = bytearray(self.client.buffer_len)
    self.server.send(0, buf, frame_id=1)
    self.server.send(0, buf, frame_id=2)

    recv_buf = self.client.recv()
    assert recv_buf is not None
    assert self.client.frame_id == 1

    recv_buf = self.client.recv()
    assert recv_buf is not None
    assert self.client.frame_id == 2

  def test_conflate(self):
    self.setup_vipc("camerad", 0, conflate=True)
    assert self.server is not None
    assert self.client is not None
    assert self.client.buffer_len is not None
    buf = bytearray(self.client.buffer_len)
    self.server.send(0, buf, frame_id=1)
    self.server.send(0, buf, frame_id=2)

    recv_buf = self.client.recv()
    assert recv_buf is not None
    assert self.client.frame_id == 2

    recv_buf = self.client.recv(timeout_ms=5)
    assert recv_buf is None

  def test_send_buffer_types(self):
    self.setup_vipc("camerad", 0)
    assert self.server is not None
    assert self.client is not None
    assert self.client.buffer_len is not None
    data = bytes(i % 256 for i in range(self.client.buffer_len))
    strided = bytearray(len(data) * 2)
    strided[::2] = data
    for payload in (data, bytearray(data), memoryview(data), memoryview(strided)[::2], memoryview(data[::-1])[::-1]):
      with self.subTest(type=type(payload)):
        self.server.send(0, payload, frame_id=42, timestamp_sof=123, timestamp_eof=456)
        buf = self.client.recv()
        assert buf is not None
        self.assertEqual(bytes(buf.data), data)
        self.assertEqual(buf.frame_id, 42)
        self.assertEqual(self.client.timestamp_sof, 123)
        self.assertEqual(self.client.timestamp_eof, 456)
        self.assertFalse(self.client.valid)

  def test_send_wrong_size(self):
    self.setup_vipc("camerad", 0)
    assert self.server is not None
    with self.assertRaises(AssertionError):
      self.server.send(0, b"short")

  def test_buffer_keeps_client_alive(self):
    self.setup_vipc("camerad", 0)
    assert self.server is not None
    assert self.client is not None
    assert self.client.buffer_len is not None
    data = b"a" * self.client.buffer_len
    self.server.send(0, data, frame_id=42)
    buf = self.client.recv()
    assert buf is not None
    self.client = None
    gc.collect()
    self.assertEqual(buf.frame_id, 42)
    view = buf.data
    del buf
    gc.collect()
    self.assertEqual(bytes(view), data)
    self.assertFalse(view.readonly)
    view[0] = ord("b")
    self.assertEqual(view[0], ord("b"))

  def test_unconnected_properties(self):
    client = VisionIpcClient("unconnected", 0, False)
    for value in (client.width, client.height, client.stride, client.uv_offset, client.buffer_len):
      self.assertIsNone(value)
    self.assertEqual(client.num_buffers, 0)
    self.assertFalse(client.is_connected())
    self.assertFalse(client.connect(False))
    self.assertIsNone(client.recv(timeout_ms=0))
