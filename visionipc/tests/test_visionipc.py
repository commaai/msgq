import os
import time
import unittest
import numpy as np
from cereal.visionipc import VisionIpcServer, VisionIpcClient, VisionStreamType

def zmq_sleep(t=1):
  if "ZMQ" in os.environ:
    time.sleep(t)

class TestVisionIpc(unittest.TestCase):
  def test_connect(self):
    server = VisionIpcServer("camerad")
    server.create_buffers(VisionStreamType.VISION_STREAM_ROAD, 1, False, 100, 100)
    server.start_listener()

    client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, False)
    self.assertTrue(client.connect(True))
    self.assertTrue(client.is_connected)

  def test_available_streams(self):
    server = VisionIpcServer("camerad")
    server.create_buffers(VisionStreamType.VISION_STREAM_ROAD, 1, False, 100, 100)
    server.create_buffers(VisionStreamType.VISION_STREAM_WIDE_ROAD, 1, False, 100, 100)
    server.start_listener()

    available_streams = VisionIpcClient.available_streams("camerad", True)
    self.assertEqual(len(available_streams), 2)
    self.assertIn(VisionStreamType.VISION_STREAM_ROAD, available_streams)
    self.assertIn(VisionStreamType.VISION_STREAM_WIDE_ROAD, available_streams)

  def test_buffers(self):
    width, height, num_buffers = 100, 200, 5
    server = VisionIpcServer("camerad")
    server.create_buffers(VisionStreamType.VISION_STREAM_ROAD, num_buffers, False, width, height)
    server.start_listener()

    client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, False)
    self.assertEqual(client.width, None)
    self.assertEqual(client.height, None)
    self.assertEqual(client.buffer_len, None)
    self.assertEqual(client.num_buffers, 0)

    self.assertTrue(client.connect(True))
    self.assertEqual(client.width, width)
    self.assertEqual(client.height, height)
    self.assertGreater(client.buffer_len, 0)
    self.assertEqual(client.num_buffers, num_buffers)

  def test_yuv_rgb(self):
    server = VisionIpcServer("camerad")
    server.create_buffers(VisionStreamType.VISION_STREAM_ROAD, 1, False, 100, 100)
    server.create_buffers(VisionStreamType.VISION_STREAM_MAP, 1, True, 100, 100)
    server.start_listener()

    client_yuv = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, False)
    client_rgb = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_MAP, False)
    self.assertTrue(client_yuv.connect(True))
    self.assertTrue(client_rgb.connect(True))

    self.assertTrue(client_rgb.rgb)
    self.assertFalse(client_yuv.rgb)

  def test_send_single_buffer(self):
    server = VisionIpcServer("camerad")
    server.create_buffers(VisionStreamType.VISION_STREAM_ROAD, 1, True, 100, 100)
    server.start_listener()

    client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, False)
    self.assertTrue(client.connect(True))
    zmq_sleep()

    buf = np.zeros(client.buffer_len, dtype=np.uint8)
    buf.view('<i4')[0] = 1234

    server.send(VisionStreamType.VISION_STREAM_ROAD, buf, frame_id=1337)

    recv_buf = client.recv()
    self.assertIsNot(recv_buf, None)
    self.assertEqual(recv_buf.view('<i4')[0], 1234)
