#pragma once

#include <unistd.h>
#include "visionbuf.h"

#define VISIONIPC_MAX_FDS 64

struct VisionIpcPacket {
  size_t idx;
  // TODO: add metadata
};

void visionipc_start_server(void);
