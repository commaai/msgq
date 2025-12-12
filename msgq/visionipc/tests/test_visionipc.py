import os
import sys
import time
import random
import numpy as np
import pytest
from typing import Optional, cast
from msgq.visionipc import VisionIpcServer, VisionIpcClient, VisionStreamType

def zmq_sleep(t=1):
  if "ZMQ" in os.environ:
    time.sleep(t)


class TestVisionIpc:
  server: Optional[VisionIpcServer]
  client: Optional[VisionIpcClient]

  def setup_method(self):
    self.server = None
    self.client = None

  def teardown_method(self):
    if self.client:
      self.client.close()
    if self.server:
      self.server.close()

  def setup_vipc(self, name, *stream_types, num_buffers=1, width=100, height=100, conflate=False):
    # Ensure previous server is closed if calling setup_vipc multiple times
    if self.server:
      self.server.close()
    if self.client:
      self.client.close()

    self.server = VisionIpcServer(name)
    for stream_type in stream_types:
      self.server.create_buffers(stream_type, num_buffers, width, height)
    self.server.start_listener()

    if len(stream_types):
      self.client = VisionIpcClient(name, stream_types[0], conflate)
      assert self.client.connect(True)
    else:
      self.client = None

    zmq_sleep()
    return self.server, self.client

  def test_connect(self):
    self.setup_vipc("camerad", VisionStreamType.VISION_STREAM_ROAD)
    assert self.client
    assert self.client.is_connected

  def test_available_streams(self):
    for k in range(4):
      stream_types = set(random.choices(list(VisionStreamType), k=k))
      self.setup_vipc("camerad", *stream_types)
      available_streams = VisionIpcClient.available_streams("camerad", True)
      print(f"k={k}, expected={stream_types}, got={available_streams}")
      assert available_streams == stream_types

  def test_buffers(self):
    width, height, num_buffers = 100, 200, 5
    self.setup_vipc("camerad", VisionStreamType.VISION_STREAM_ROAD, num_buffers=num_buffers, width=width, height=height)
    assert self.client
    assert self.client.width == width
    assert self.client.height == height
    assert self.client.buffer_len > 0
    assert self.client.num_buffers == num_buffers

  def test_send_single_buffer(self):
    self.setup_vipc("camerad", VisionStreamType.VISION_STREAM_ROAD)
    assert self.client
    assert self.server

    buf = np.zeros(self.client.buffer_len, dtype=np.uint8)
    buf.view('<i4')[0] = 1234
    self.server.send(VisionStreamType.VISION_STREAM_ROAD, buf, frame_id=1337)

    recv_buf = self.client.recv()
    assert recv_buf is not None
    assert recv_buf.data.view('<i4')[0] == 1234
    assert self.client.frame_id == 1337

  def test_no_conflate(self):
    self.setup_vipc("camerad", VisionStreamType.VISION_STREAM_ROAD)
    assert self.client
    assert self.server

    buf = np.zeros(self.client.buffer_len, dtype=np.uint8)
    self.server.send(VisionStreamType.VISION_STREAM_ROAD, buf, frame_id=1)
    self.server.send(VisionStreamType.VISION_STREAM_ROAD, buf, frame_id=2)

    recv_buf = self.client.recv()
    assert recv_buf is not None
    assert self.client.frame_id == 1

    recv_buf = self.client.recv()
    assert recv_buf is not None
    assert self.client.frame_id == 2

    recv_buf = self.client.recv()
    assert recv_buf is None

  def test_conflate(self):
    self.setup_vipc("camerad", VisionStreamType.VISION_STREAM_ROAD, conflate=True)
    assert self.client
    assert self.server

    buf = np.zeros(self.client.buffer_len, dtype=np.uint8)
    self.server.send(VisionStreamType.VISION_STREAM_ROAD, buf, frame_id=1)
    self.server.send(VisionStreamType.VISION_STREAM_ROAD, buf, frame_id=2)

    recv_buf = self.client.recv()
    assert recv_buf is not None
    assert self.client.frame_id == 2

    recv_buf = self.client.recv()
    assert recv_buf is None

  @pytest.mark.skipif(sys.platform == "darwin", reason="SocketEventHandle not supported on macOS")
  def test_server_no_start_listener(self):
    server = VisionIpcServer("test_no_start")
    server.create_buffers(VisionStreamType.VISION_STREAM_ROAD, 1, 100, 100)
    server.close()

  @pytest.mark.skipif(sys.platform == "darwin", reason="SocketEventHandle not supported on macOS")
  def test_connect_fail(self):
    client = VisionIpcClient("nonexistent_server", VisionStreamType.VISION_STREAM_ROAD, False)
    assert not client.connect(False)
    client.close()

  def test_recv_timeout(self):
    self.setup_vipc("camerad", VisionStreamType.VISION_STREAM_ROAD)
    assert self.client
    start_time = time.monotonic()
    recv_buf = self.client.recv(100)
    end_time = time.monotonic()
    assert recv_buf is None
    assert (end_time - start_time) < 0.5

  @pytest.mark.skipif(sys.platform == "darwin", reason="SocketEventHandle not supported on macOS")
  def test_data_correctness(self):
    self.setup_vipc("camerad", VisionStreamType.VISION_STREAM_ROAD)
    assert self.client
    assert self.server

    buf_size = self.client.buffer_len
    data_pattern = np.arange(buf_size, dtype=np.uint8)

    buf = np.zeros(buf_size, dtype=np.uint8)
    buf[:] = data_pattern[:]

    self.server.send(VisionStreamType.VISION_STREAM_ROAD, buf, frame_id=123)

    recv_buf = self.client.recv()
    assert recv_buf is not None
    assert np.array_equal(recv_buf.data, data_pattern)

    recv_buf.data[0] = 255
    assert recv_buf.data[0] == 255

  @pytest.mark.skipif(sys.platform == "darwin", reason="SocketEventHandle not supported on macOS")
  def test_concurrency(self):
    self.setup_vipc("camerad", VisionStreamType.VISION_STREAM_ROAD)
    assert self.client
    assert self.server

    client2 = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, False)
    assert client2.connect(True)

    buf = np.zeros(self.client.buffer_len, dtype=np.uint8)
    self.server.send(VisionStreamType.VISION_STREAM_ROAD, buf, frame_id=999)

    check1 = self.client.recv()
    check2 = client2.recv()

    assert check1 is not None and check1.data is not None
    assert check2 is not None and check2.data is not None
    assert self.client.frame_id == 999
    assert client2.frame_id == 999

    client2.close()

  @pytest.mark.skipif(sys.platform == "darwin", reason="SocketEventHandle not supported on macOS")
  def test_invalid_inputs(self):
    # Test invalid stream type
    self.client = VisionIpcClient("camerad", cast(VisionStreamType, 9999), False)
    connected = self.client.connect(False)
    assert not connected

    # Test CLContext inputs validation
    with pytest.raises(TypeError):
      self.client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, False, cl_context="invalid")
