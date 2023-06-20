import os
import time
import random
import unittest
import numpy as np
from cereal.visionipc import VisionIpcServer, VisionIpcClient, VisionStreamType

def zmq_sleep(t=1):
  if "ZMQ" in os.environ:
    time.sleep(t)


class TestVisionIpc(unittest.TestCase):
  def create_vipc_server(self, name, *stream_types, num_buffers=1, rgb=False, width=100, height=100):
    self.server = VisionIpcServer(name)
    for stream_type in stream_types:
      self.server.create_buffers(stream_type, num_buffers, rgb, width, height)
    self.server.start_listener()

  def test_connect(self):
    self.create_vipc_server("camerad", VisionStreamType.VISION_STREAM_ROAD)
    client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, False)
    self.assertTrue(client.connect(True))
    self.assertTrue(client.is_connected)

  def test_available_streams(self):
    stream_types = set(random.choices([x.value for x in VisionStreamType], k=2))
    self.create_vipc_server("camerad", *stream_types)
    available_streams = VisionIpcClient.available_streams("camerad", True)
    self.assertEqual(available_streams, stream_types)

  def test_buffers(self):
    width, height, num_buffers = 100, 200, 5
    self.create_vipc_server("camerad", VisionStreamType.VISION_STREAM_ROAD, num_buffers=num_buffers, width=width, height=height)
    client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, False)
    self.assertEqual(client.width, None)
    self.assertEqual(client.height, None)
    # self.assertEqual(client.buffer_len, None)
    # self.assertEqual(client.num_buffers, 0)

    self.assertTrue(client.connect(True))
    buf = np.zeros(100 * 300, dtype=np.uint8)
    self.server.send(VisionStreamType.VISION_STREAM_ROAD, buf)
    client.recv()  # TODO: Remove this after vipc refactor
    self.assertEqual(client.width, width)
    self.assertEqual(client.height, height)
    # self.assertGreater(client.buffer_len, 0)
    # self.assertEqual(client.num_buffers, num_buffers)

  def test_send_single_buffer(self):
    self.create_vipc_server("camerad", VisionStreamType.VISION_STREAM_ROAD)
    client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, False)
    self.assertTrue(client.connect(True))
    zmq_sleep()

    buf = np.zeros(100 * 150, dtype=np.uint8)
    buf.view('<i4')[0] = 1234

    self.server.send(VisionStreamType.VISION_STREAM_ROAD, buf, frame_id=1337)

    recv_buf = client.recv()
    self.assertIsNot(recv_buf, None)
    self.assertEqual(recv_buf.view('<i4')[0], 1234)
    # self.assertEqual(client.frame_id, 1337)
