#include "visionbuf.h"

void visionbuf_init_rgb(VisionBuf* buf, size_t width, size_t height) {
  buf->rgb = true;
  buf->width = width;
  buf->height = height;

  // Is BGR correct?
  buf->b = (uint8_t *)buf->addr;
  buf->g = buf->b + (width * height);
  buf->r = buf->g + (width * height);
}
void visionbuf_init_yuv(VisionBuf* buf, size_t width, size_t height){
  buf->rgb = false;
  buf->width = width;
  buf->height = height;

  buf->y = (uint8_t *)buf->addr;
  buf->u = buf->y + (width * height);
  buf->v = buf->u + (width / 2 * height / 2);
}
