#include <cassert>
#include <iostream>

#include "cl_helpers.h"

cl_device_id cereal_cl_get_device_id(cl_device_type device_type) {
  bool opencl_platform_found = false;
  cl_device_id device_id = NULL;

  cl_uint num_platforms = 0;
  CL_CHECK(clGetPlatformIDs(0, NULL, &num_platforms));
  cl_platform_id* platform_ids = new cl_platform_id[num_platforms];
  CL_CHECK(clGetPlatformIDs(num_platforms, platform_ids, NULL));

  char cBuffer[1024];
  for (size_t i = 0; i < num_platforms; i++) {
    CL_CHECK(clGetPlatformInfo(platform_ids[i], CL_PLATFORM_NAME, sizeof(cBuffer), &cBuffer, NULL));
    printf("platform[%zu] CL_PLATFORM_NAME: %s\n", i, cBuffer);

    cl_uint num_devices;
    int err = clGetDeviceIDs(platform_ids[i], device_type, 0, NULL, &num_devices);
    if (err != 0 || !num_devices) {
      continue;
    }
    // Get first device
    CL_CHECK(clGetDeviceIDs(platform_ids[i], device_type, 1, &device_id, NULL));
    opencl_platform_found = true;
    break;
  }

  delete[] platform_ids;

  if (!opencl_platform_found) {
    std::cout << "No valid openCL platform found" << std::endl;
    assert(opencl_platform_found);
  }
  return device_id;
}
