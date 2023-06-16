import unittest
from cereal.visionipc import VisionIpcServer, VisionIpcClient, VisionStreamType

class TestVisionIpc(unittest.TestCase):
  def test_connect(self):
    server = VisionIpcServer("camerad")
    server.create_buffers(VisionStreamType.VISION_STREAM_ROAD, 1, False, 100, 100)
    server.start_listener()

    client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, False)
    self.assertTrue(client.connect(True))
    self.assertTrue(client.is_connected)
