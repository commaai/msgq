#include "visionbuf.h"

#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

int offset = 0;

static void *malloc_with_fd(size_t len, int *fd) {
  char full_path[0x100];

#ifdef __APPLE__
  snprintf(full_path, sizeof(full_path)-1, "/tmp/visionbuf_%d_%d", getpid(), offset++);
#else
  snprintf(full_path, sizeof(full_path)-1, "/dev/shm/visionbuf_%d_%d", getpid(), offset++);
#endif

  *fd = open(full_path, O_RDWR | O_CREAT, 0777);
  assert(*fd >= 0);

  unlink(full_path);

  ftruncate(*fd, len);
  void *addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
  assert(addr != MAP_FAILED);

  return addr;
}

void visionbuf_init_cl(VisionBuf* buf, cl_device_id device_id, cl_context ctx){
  int err;

  buf->copy_q = clCreateCommandQueue(ctx, device_id, 0, &err);
  assert(err == 0);

  buf->buf_cl = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, buf->len, buf->addr, &err);
  assert(err == 0);
}

VisionBuf visionbuf_allocate(size_t len) {
  int fd;
  void *addr = malloc_with_fd(len, &fd);

  VisionBuf buf = {0};
  buf.len = len;
  buf.mmap_len = len;
  buf.addr = addr;
  buf.fd = fd;

  return buf;
}

void visionbuf_import(VisionBuf* buf){
  assert(buf->fd >= 0);
  buf->addr = mmap(NULL, buf->mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, buf->fd, 0);
  assert(buf->addr != MAP_FAILED);
}

void visionbuf_sync(const VisionBuf* buf, int dir) {
  int err = 0;
  if (!buf->buf_cl) return;

  if (dir == VISIONBUF_SYNC_FROM_DEVICE) {
    err = clEnqueueReadBuffer(buf->copy_q, buf->buf_cl, CL_FALSE, 0, buf->len, buf->addr, 0, NULL, NULL);
  } else {
    err = clEnqueueWriteBuffer(buf->copy_q, buf->buf_cl, CL_FALSE, 0, buf->len, buf->addr, 0, NULL, NULL);
  }
  assert(err == 0);
  clFinish(buf->copy_q);
}

void visionbuf_free(const VisionBuf* buf) {
  if (buf->buf_cl){
    int err = clReleaseMemObject(buf->buf_cl);
    assert(err == 0);

    clReleaseCommandQueue(buf->copy_q);
  }

  munmap(buf->addr, buf->len);
  close(buf->fd);
}
