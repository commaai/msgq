#include "msgq/visionipc/visionbuf.h"

#include <atomic>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "msgq/msgq.h"

#ifdef _WIN32
#include <windows.h>

// An anonymous section; the fd field carries its handle, which fits in 32 bits
static void *malloc_with_fd(size_t len, int *fd) {
  HANDLE section = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)((uint64_t)len >> 32), (DWORD)len, NULL);
  assert(section != NULL);
  void *addr = MapViewOfFile(section, FILE_MAP_ALL_ACCESS, 0, 0, len);
  assert(addr != NULL);

  *fd = (int)(intptr_t)section;
  return addr;
}
#else
#include <sys/mman.h>

std::atomic<int> offset = 0;

static void *malloc_with_fd(size_t len, int *fd) {
  char full_path[0x100];

  snprintf(full_path, sizeof(full_path)-1, "%s/msgq_visionbuf_%d_%d", msgq_shm_dir().c_str(), getpid(), offset++);

  *fd = open(full_path, O_RDWR | O_CREAT, 0664);
  assert(*fd >= 0);

  unlink(full_path);

  int ret = ftruncate(*fd, len);
  assert(ret == 0);
  void *addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
  assert(addr != MAP_FAILED);

  return addr;
}
#endif

void VisionBuf::allocate(size_t length) {
  this->len = length;
  this->mmap_len = this->len + sizeof(uint64_t);
  this->addr = malloc_with_fd(this->mmap_len, &this->fd);
  this->frame_id = (uint64_t*)((uint8_t*)this->addr + this->len);
}

void VisionBuf::import(){
  assert(this->fd >= 0);
#ifdef _WIN32
  this->addr = MapViewOfFile((HANDLE)(intptr_t)this->fd, FILE_MAP_ALL_ACCESS, 0, 0, this->mmap_len);
  assert(this->addr != NULL);
#else
  this->addr = mmap(NULL, this->mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
  assert(this->addr != MAP_FAILED);
#endif

  this->frame_id = (uint64_t*)((uint8_t*)this->addr + this->len);
}

void VisionBuf::init_yuv(size_t init_width, size_t init_height, size_t init_stride, size_t init_uv_offset){
  this->width = init_width;
  this->height = init_height;
  this->stride = init_stride;
  this->uv_offset = init_uv_offset;

  this->y = (uint8_t *)this->addr;
  this->uv = this->y + this->uv_offset;
}

int VisionBuf::sync(int dir) {
  return 0;
}

int VisionBuf::free() {
#ifdef _WIN32
  if (!UnmapViewOfFile(this->addr)) return -1;
  return CloseHandle((HANDLE)(intptr_t)this->fd) ? 0 : -1;
#else
  int err = munmap(this->addr, this->mmap_len);
  if (err != 0) return err;

  err = close(this->fd);
  return err;
#endif
}

uint64_t VisionBuf::get_frame_id() {
  return *frame_id;
}

void VisionBuf::set_frame_id(uint64_t id) {
  *frame_id = id;
}
