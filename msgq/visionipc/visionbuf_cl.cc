#include "msgq/visionipc/visionbuf.h"

#include <atomic>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <chrono>
#include <iostream>

std::atomic<int> offset = 0;

static void *malloc_with_fd(size_t len, int *fd) {
  char full_path[0x100];

#ifdef __APPLE__
  snprintf(full_path, sizeof(full_path)-1, "/tmp/visionbuf_%d_%d", getpid(), offset++);
#else
  snprintf(full_path, sizeof(full_path)-1, "/dev/shm/visionbuf_%d_%d", getpid(), offset++);
#endif

  *fd = open(full_path, O_RDWR | O_CREAT, 0664);
  assert(*fd >= 0);

  unlink(full_path);

  ftruncate(*fd, len);
  void *addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
  assert(addr != MAP_FAILED);

  return addr;
}

void VisionBuf::allocate(size_t length) {
  this->len = length;
  this->mmap_len = this->len + sizeof(uint64_t);
  this->addr = malloc_with_fd(this->mmap_len, &this->fd);
  this->frame_id = (uint64_t*)((uint8_t*)this->addr + this->len);
}

void VisionBuf::init_cl(cl_device_id device_id, cl_context ctx){
  int err;

  this->copy_q = clCreateCommandQueue(ctx, device_id, 0, &err);
  assert(err == 0);

  this->buf_cl = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, this->len, this->addr, &err);
  assert(err == 0);
}


void VisionBuf::import(){
  assert(this->fd >= 0);
  this->addr = mmap(NULL, this->mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
  assert(this->addr != MAP_FAILED);

  this->frame_id = (uint64_t*)((uint8_t*)this->addr + this->len);
}


int VisionBuf::sync(int dir) {
  int err = 0;
  if (!this->buf_cl) return 0;

  if (dir == VISIONBUF_SYNC_FROM_DEVICE) {
    err = clEnqueueReadBuffer(this->copy_q, this->buf_cl, CL_FALSE, 0, this->len, this->addr, 0, NULL, NULL);
  } else {
    err = clEnqueueWriteBuffer(this->copy_q, this->buf_cl, CL_FALSE, 0, this->len, this->addr, 0, NULL, NULL);
  }

  if (err == 0){
    err = clFinish(this->copy_q);
  }

  return err;
}

int VisionBuf::free() {
  auto start_total = std::chrono::high_resolution_clock::now();
  int err = 0;

  if (this->buf_cl){
    auto start_release_mem = std::chrono::high_resolution_clock::now();
    err = clReleaseMemObject(this->buf_cl);
    auto end_release_mem = std::chrono::high_resolution_clock::now();
    auto release_mem_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_release_mem - start_release_mem).count();
    std::cout << "[VisionBuf::free] Release OpenCL memory object: " << release_mem_duration << " us" << std::endl;
    if (err != 0) return err;

    auto start_release_queue = std::chrono::high_resolution_clock::now();
    err = clReleaseCommandQueue(this->copy_q);
    auto end_release_queue = std::chrono::high_resolution_clock::now();
    auto release_queue_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_release_queue - start_release_queue).count();
    std::cout << "[VisionBuf::free] Release OpenCL command queue: " << release_queue_duration << " us" << std::endl;
    if (err != 0) return err;
  }

  auto start_munmap = std::chrono::high_resolution_clock::now();
  err = munmap(this->addr, this->mmap_len);
  auto end_munmap = std::chrono::high_resolution_clock::now();
  auto munmap_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_munmap - start_munmap).count();
  std::cout << "[VisionBuf::free] Unmap memory: " << munmap_duration << " us" << std::endl;
  if (err != 0) return err;

  auto start_close = std::chrono::high_resolution_clock::now();
  err = close(this->fd);
  auto end_close = std::chrono::high_resolution_clock::now();
  auto close_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_close - start_close).count();
  std::cout << "[VisionBuf::free] Close file descriptor: " << close_duration << " us" << std::endl;

  auto end_total = std::chrono::high_resolution_clock::now();
  auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_total - start_total).count();
  std::cout << "[VisionBuf::free] Total time: " << total_duration << " us" << std::endl;

  return err;
}
