#include <thread>
#include <chrono>
#include <cassert>

#include "visionipc.h"
#include "visionipc_server.h"
#include "visionipc_client.h"


int main(void){
  auto server = VisionIpcServer("camerad", {VISION_STREAM_RGB_BACK, VISION_STREAM_RGB_FRONT});

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  auto client = VisionIpcClient("camerad", VISION_STREAM_RGB_FRONT);

  VisionBuf * buf = server.get_buffer(VISION_STREAM_RGB_FRONT);
  server.send(buf);

  VisionBuf * recv_buf = client.recv();
  assert(recv_buf);


  return 0;
}
