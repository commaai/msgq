#pragma once

#include <unistd.h>
#include <cstdint>
#include "visionbuf.h"

#define VISIONIPC_MAX_FDS 64

struct VIPCBufExtra {
  uint32_t frame_id;
  uint64_t timestamp_sof;
  uint64_t timestamp_eof;
};

struct VisionIpcPacket {
  size_t idx;
  // TODO: add metadata
};

void visionipc_start_server(void);
