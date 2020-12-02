#include "visionbuf.h"

void visionbuf_init_rgb(VisionBuf* buf, size_t width, size_t height) {
  buf->rgb = true;
  buf->width = width;
  buf->height = height;
  buf->stride = 3 * width; // TODO: deal with alignment
}

void visionbuf_init_yuv(VisionBuf* buf, size_t width, size_t height){
  buf->rgb = false;
  buf->width = width;
  buf->height = height;

  buf->y = (uint8_t *)buf->addr;
  buf->u = buf->y + (width * height);
  buf->v = buf->u + (width / 2 * height / 2);
}
