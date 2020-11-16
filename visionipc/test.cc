#include "visionipc.h"
#include "visionipc_server.h"
#include "visionipc_client.h"


#include <stdio.h>
#include <assert.h>
#include <stdbool.h>


int main(void){
  auto server = VisionIpcServer("camerad", {VISION_STREAM_RGB_BACK, VISION_STREAM_RGB_FRONT});
  auto client = VisionIpcClient("camerad", VISION_STREAM_RGB_BACK);

  // cl_device_id device_id = cl_get_device_id(CL_DEVICE_TYPE_CPU);
  // cl_context ctx = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
  // assert(err == 0);


  // VisionBuf b = visionbuf_allocate(100);
  // b = visionbuf_init_cl(b, device_id, ctx);

  return 0;
}
