#include "msgq/visionipc/visionbuf.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>

namespace {

int dma_ioctl(int fd, unsigned long request, void *data) {
  int ret;
  do {
    ret = ioctl(fd, request, data);
  } while (ret == -1 && (errno == EINTR || errno == EAGAIN));
  return ret;
}

struct DmaHeapFileHandle {
  DmaHeapFileHandle() {
    fd = open("/dev/dma_heap/system", O_RDWR | O_CLOEXEC);
    assert(fd >= 0);
  }
  ~DmaHeapFileHandle() {
    close(fd);
  }
  int fd = -1;
};

}  // namespace

void VisionBuf::allocate(size_t length) {
  static DmaHeapFileHandle heap;
  struct dma_heap_allocation_data alloc = {};
  alloc.len = length + sizeof(uint64_t);
  alloc.fd_flags = O_RDWR | O_CLOEXEC;

  int err = dma_ioctl(heap.fd, DMA_HEAP_IOCTL_ALLOC, &alloc);
  assert(err == 0);

  this->len = length;
  this->mmap_len = alloc.len;
  this->fd = alloc.fd;
  this->import();

  err = sync(VISIONBUF_SYNC_FROM_DEVICE);
  assert(err == 0);
  memset(this->addr, 0, this->mmap_len);
  err = sync(VISIONBUF_SYNC_TO_DEVICE);
  assert(err == 0);
}

void VisionBuf::import() {
  assert(this->fd >= 0);
  this->addr = mmap(NULL, this->mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
  assert(this->addr != MAP_FAILED);
  this->frame_id = (uint64_t*)((uint8_t*)this->addr + this->len);
}

void VisionBuf::init_yuv(size_t init_width, size_t init_height, size_t init_stride, size_t init_uv_offset) {
  this->width = init_width;
  this->height = init_height;
  this->stride = init_stride;
  this->uv_offset = init_uv_offset;

  this->y = (uint8_t *)this->addr;
  this->uv = this->y + this->uv_offset;
}

int VisionBuf::sync(int dir) {
  assert(dir == VISIONBUF_SYNC_FROM_DEVICE || dir == VISIONBUF_SYNC_TO_DEVICE);
  // Bracket CPU access: invalidate before reading, flush after writing.
  struct dma_buf_sync sync_data = {};
  sync_data.flags = DMA_BUF_SYNC_RW | ((dir == VISIONBUF_SYNC_FROM_DEVICE) ? DMA_BUF_SYNC_START : DMA_BUF_SYNC_END);
  return dma_ioctl(this->fd, DMA_BUF_IOCTL_SYNC, &sync_data);
}

int VisionBuf::free() {
  int err = munmap(this->addr, this->mmap_len);
  if (err != 0) return err;
  return close(this->fd);
}

uint64_t VisionBuf::get_frame_id() {
  return *frame_id;
}

void VisionBuf::set_frame_id(uint64_t id) {
  *frame_id = id;
}
