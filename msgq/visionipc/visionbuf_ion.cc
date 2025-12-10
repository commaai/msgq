#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ion.h>
#include <CL/cl_ext.h>
#include <chrono>
#include <iostream>

#include <msm_ion.h>

#include "msgq/visionipc/visionbuf.h"

// keep trying if x gets interrupted by a signal
#define HANDLE_EINTR(x)                                       \
  ({                                                          \
    decltype(x) ret;                                          \
    int try_cnt = 0;                                          \
    do {                                                      \
      ret = (x);                                              \
    } while (ret == -1 && errno == EINTR && try_cnt++ < 100); \
    ret;                                                      \
  })

// just hard-code these for convenience
// size_t device_page_size = 0;
// clGetDeviceInfo(device_id, CL_DEVICE_PAGE_SIZE_QCOM,
//                 sizeof(device_page_size), &device_page_size,
//                 NULL);

// size_t padding_cl = 0;
// clGetDeviceInfo(device_id, CL_DEVICE_EXT_MEM_PADDING_IN_BYTES_QCOM,
//                 sizeof(padding_cl), &padding_cl,
//                 NULL);
#define DEVICE_PAGE_SIZE_CL 4096
#define PADDING_CL 0

struct IonFileHandle {
  IonFileHandle() {
    fd = open("/dev/ion", O_RDWR | O_NONBLOCK);
    assert(fd >= 0);
  }
  ~IonFileHandle() {
    close(fd);
  }
  int fd = -1;
};

int ion_fd() {
  static IonFileHandle fh;
  return fh.fd;
}

void VisionBuf::allocate(size_t length) {
  struct ion_allocation_data ion_alloc = {0};
  ion_alloc.len = length + PADDING_CL + sizeof(uint64_t);
  ion_alloc.align = 4096;
  ion_alloc.heap_id_mask = 1 << ION_IOMMU_HEAP_ID;
  ion_alloc.flags = ION_FLAG_CACHED;

  int err = HANDLE_EINTR(ioctl(ion_fd(), ION_IOC_ALLOC, &ion_alloc));
  assert(err == 0);

  struct ion_fd_data ion_fd_data = {0};
  ion_fd_data.handle = ion_alloc.handle;
  err = HANDLE_EINTR(ioctl(ion_fd(), ION_IOC_SHARE, &ion_fd_data));
  assert(err == 0);

  void *mmap_addr = mmap(NULL, ion_alloc.len,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED, ion_fd_data.fd, 0);
  assert(mmap_addr != MAP_FAILED);

  memset(mmap_addr, 0, ion_alloc.len);

  this->len = length;
  this->mmap_len = ion_alloc.len;
  this->addr = mmap_addr;
  this->handle = ion_alloc.handle;
  this->fd = ion_fd_data.fd;
  this->frame_id = (uint64_t*)((uint8_t*)this->addr + this->len + PADDING_CL);
}

void VisionBuf::import(){
  int err;
  assert(this->fd >= 0);

  // Get handle
  struct ion_fd_data fd_data = {0};
  fd_data.fd = this->fd;
  err = HANDLE_EINTR(ioctl(ion_fd(), ION_IOC_IMPORT, &fd_data));
  assert(err == 0);

  this->handle = fd_data.handle;
  this->addr = mmap(NULL, this->mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
  assert(this->addr != MAP_FAILED);

  this->frame_id = (uint64_t*)((uint8_t*)this->addr + this->len + PADDING_CL);
}

void VisionBuf::init_cl(cl_device_id device_id, cl_context ctx) {
  int err;

  assert(((uintptr_t)this->addr % DEVICE_PAGE_SIZE_CL) == 0);

  cl_mem_ion_host_ptr ion_cl = {0};
  ion_cl.ext_host_ptr.allocation_type = CL_MEM_ION_HOST_PTR_QCOM;
  ion_cl.ext_host_ptr.host_cache_policy = CL_MEM_HOST_UNCACHED_QCOM;
  ion_cl.ion_filedesc = this->fd;
  ion_cl.ion_hostptr = this->addr;

  this->buf_cl = clCreateBuffer(ctx,
                              CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM,
                              this->len, &ion_cl, &err);
  assert(err == 0);
}


int VisionBuf::sync(int dir) {
  struct ion_flush_data flush_data = {0};
  flush_data.handle = this->handle;
  flush_data.vaddr = this->addr;
  flush_data.offset = 0;
  flush_data.length = this->len;

  // ION_IOC_INV_CACHES ~= DMA_FROM_DEVICE
  // ION_IOC_CLEAN_CACHES ~= DMA_TO_DEVICE
  // ION_IOC_CLEAN_INV_CACHES ~= DMA_BIDIRECTIONAL

  struct ion_custom_data custom_data = {0};

   assert(dir == VISIONBUF_SYNC_FROM_DEVICE || dir == VISIONBUF_SYNC_TO_DEVICE);
   custom_data.cmd = (dir == VISIONBUF_SYNC_FROM_DEVICE) ?
     ION_IOC_INV_CACHES : ION_IOC_CLEAN_CACHES;

  custom_data.arg = (unsigned long)&flush_data;
  return HANDLE_EINTR(ioctl(ion_fd(), ION_IOC_CUSTOM, &custom_data));
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
  if (err != 0) return err;

  auto start_free_ion = std::chrono::high_resolution_clock::now();
  struct ion_handle_data handle_data = {.handle = this->handle};
  err = HANDLE_EINTR(ioctl(ion_fd(), ION_IOC_FREE, &handle_data));
  auto end_free_ion = std::chrono::high_resolution_clock::now();
  auto free_ion_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_free_ion - start_free_ion).count();
  std::cout << "[VisionBuf::free] Free ION handle: " << free_ion_duration << " us" << std::endl;

  auto end_total = std::chrono::high_resolution_clock::now();
  auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_total - start_total).count();
  std::cout << "[VisionBuf::free] Total time: " << total_duration << " us" << std::endl;

  return err;
}
