#pragma once

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define CL_CHECK(_expr)                         \
  do {                                          \
    assert(CL_SUCCESS == _expr);                \
  } while (0)

#define CL_CHECK_ERR(_expr)                     \
  ({                                            \
    cl_int err = CL_INVALID_VALUE;              \
    __typeof__(_expr) _ret = _expr;             \
    assert(_ret&& err == CL_SUCCESS);           \
    _ret;                                       \
  })

cl_device_id cl_get_device_id(cl_device_type device_type);
