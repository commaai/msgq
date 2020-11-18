#pragma once

#include <unistd.h>
#include "visionbuf.h"

struct VisionIpcPacket {
  size_t idx;
  // TODO: add metadata
};

void visionipc_start_server(void);
