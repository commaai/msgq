#pragma once
#include "visionipc.h"

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


enum VisionStreamType {
  VISION_STREAM_RGB_BACK,
  VISION_STREAM_RGB_FRONT,
  VISION_STREAM_RGB_WIDE,
  VISION_STREAM_YUV,
  VISION_STREAM_YUV_FRONT,
  VISION_STREAM_YUV_WIDE,
  VISION_STREAM_MAX,
};

typedef struct VisionBuf {
  size_t len;
  size_t mmap_len;
  void* addr;
  int fd;

  // Visionipc
  size_t idx;
  VisionStreamType type;

  // OpenCL
  cl_mem buf_cl;
  cl_command_queue copy_q;
} VisionBuf;

#define VISIONBUF_SYNC_FROM_DEVICE 0
#define VISIONBUF_SYNC_TO_DEVICE 1

VisionBuf visionbuf_allocate(size_t len);
void visionbuf_import(VisionBuf* buf);
void visionbuf_init_cl(VisionBuf* buf, cl_device_id device_id, cl_context ctx);

void visionbuf_sync(const VisionBuf* buf, int dir);
void visionbuf_free(const VisionBuf* buf);
