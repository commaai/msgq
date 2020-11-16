#pragma once

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VisionBuf {
  size_t len;
  size_t mmap_len;
  void* addr;
  int fd;

  // OpenCL
  cl_context ctx;
  cl_device_id device_id;
  cl_mem buf_cl;
  cl_command_queue copy_q;
} VisionBuf;

#define VISIONBUF_SYNC_FROM_DEVICE 0
#define VISIONBUF_SYNC_TO_DEVICE 1

VisionBuf visionbuf_allocate(size_t len);
VisionBuf visionbuf_import(VisionBuf buf);
VisionBuf visionbuf_init_cl(VisionBuf buf, cl_device_id device_id, cl_context ctx);

void visionbuf_sync(const VisionBuf* buf, int dir);
void visionbuf_free(const VisionBuf* buf);

#ifdef __cplusplus
}
#endif
