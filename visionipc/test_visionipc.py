#!/usr/bin/env python3

from cereal.visionipc.visionipc_pyx import VisionIpcServer, VisionStreamType

if __name__ == "__main__":
  server = VisionIpcServer("camerad")
  w, h = 100, 100
  server.create_buffers(VisionStreamType.VISION_STREAM_RGB_BACK, 5, True, w, h)
  server.send(VisionStreamType.VISION_STREAM_RGB_BACK, b"A" * w * h * 3)
