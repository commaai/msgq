#include "visionipc.h"
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

cl_device_id cl_get_device_id(cl_device_type device_type) {
  bool opencl_platform_found = false;
  cl_device_id device_id = NULL;

  cl_uint num_platforms = 0;
  int err = clGetPlatformIDs(0, NULL, &num_platforms);
  assert(err == 0);
  cl_platform_id* platform_ids = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
  err = clGetPlatformIDs(num_platforms, platform_ids, NULL);
  assert(err == 0);

  char cBuffer[1024];
  for (size_t i = 0; i < num_platforms; i++) {
    err = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_NAME, sizeof(cBuffer), &cBuffer, NULL);
    assert(err == 0);

    cl_uint num_devices;
    err = clGetDeviceIDs(platform_ids[i], device_type, 0, NULL, &num_devices);
    if (err != 0 || !num_devices) {
      continue;
    }
    // Get first device
    err = clGetDeviceIDs(platform_ids[i], device_type, 1, &device_id, NULL);
    assert(err == 0);
    opencl_platform_found = true;
    break;
  }
  free(platform_ids);

  if (!opencl_platform_found) {
    printf("No valid openCL platform found\n");
    assert(opencl_platform_found);
  }
  return device_id;
}

int main(void){
  int err;

  cl_device_id device_id = cl_get_device_id(CL_DEVICE_TYPE_CPU);
  cl_context ctx = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
  assert(err == 0);


  VisionBuf b = visionbuf_allocate(100);
  b = visionbuf_init_cl(b, device_id, ctx);

  return 0;
}
