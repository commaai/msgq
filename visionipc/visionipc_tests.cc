#include "catch2/catch.hpp"
#include "visionipc_server.h"
#include "visionipc_client.h"

TEST_CASE("CONNECT"){
  VisionIpcServer vipc_server("camerad");
  vipc_server.create_buffers(VISION_STREAM_YUV_BACK, 1, true, 100, 100);
  vipc_server.start_listener();


  VisionIpcClient vipc_client = VisionIpcClient("camerad", VISION_STREAM_YUV_BACK, false);
  vipc_client.connect();
  REQUIRE(true);
}
