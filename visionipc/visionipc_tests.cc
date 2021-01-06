#include "catch2/catch.hpp"
#include "visionipc_server.h"
#include "visionipc_client.h"

TEST_CASE("Connecting"){
  VisionIpcServer server("camerad");
  server.create_buffers(VISION_STREAM_YUV_BACK, 1, false, 100, 100);
  server.start_listener();

  VisionIpcClient client = VisionIpcClient("camerad", VISION_STREAM_YUV_BACK, false);
  client.connect();

  REQUIRE(client.connected);
}

TEST_CASE("Check buffers"){
  size_t width = 100, height = 200, num_buffers = 5;
  VisionIpcServer server("camerad");
  server.create_buffers(VISION_STREAM_YUV_BACK, num_buffers, false, width, height);
  server.start_listener();

  VisionIpcClient client = VisionIpcClient("camerad", VISION_STREAM_YUV_BACK, false);
  client.connect();

  REQUIRE(client.buffers[0].width == width);
  REQUIRE(client.buffers[0].height == height);
  REQUIRE(client.buffers[0].len);
  REQUIRE(client.num_buffers == num_buffers);
}

TEST_CASE("Check yuv/rgb"){
  VisionIpcServer server("camerad");
  server.create_buffers(VISION_STREAM_YUV_BACK, 1, false, 100, 100);
  server.create_buffers(VISION_STREAM_RGB_BACK, 1, true, 100, 100);
  server.start_listener();

  VisionIpcClient client_yuv = VisionIpcClient("camerad", VISION_STREAM_YUV_BACK, false);
  VisionIpcClient client_rgb = VisionIpcClient("camerad", VISION_STREAM_RGB_BACK, false);
  client_yuv.connect();
  client_rgb.connect();

  REQUIRE(client_rgb.buffers[0].rgb == true);
  REQUIRE(client_yuv.buffers[0].rgb == false);
}

TEST_CASE("Send single buffer"){
  VisionIpcServer server("camerad");
  server.create_buffers(VISION_STREAM_YUV_BACK, 1, true, 100, 100);
  server.start_listener();

  VisionIpcClient client = VisionIpcClient("camerad", VISION_STREAM_YUV_BACK, false);
  client.connect();

  VisionBuf * buf = server.get_buffer(VISION_STREAM_YUV_BACK);
  REQUIRE(buf != nullptr);

  *((uint64_t*)buf->addr) = 1234;

  VisionIpcBufExtra extra = {0};
  extra.frame_id = 1337;

  server.send(buf, &extra);

  VisionIpcBufExtra extra_recv = {0};
  VisionBuf * recv_buf = client.recv(&extra_recv);
  REQUIRE(recv_buf != nullptr);
  REQUIRE(*(uint64_t*)recv_buf->addr == 1234);
  REQUIRE(extra_recv.frame_id == extra.frame_id);
}
