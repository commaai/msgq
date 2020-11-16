#include <cassert>
#include <iostream>

#include "cl_helpers.h"

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
    std::cout << "No valid openCL platform found" << std::endl;
    assert(opencl_platform_found);
  }
  return device_id;
}
