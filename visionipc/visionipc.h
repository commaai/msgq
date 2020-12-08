#pragma once

#include <unistd.h>
#include <stdint.h>

#define VISIONIPC_MAX_FDS 64

#ifdef __cplusplus
extern "C" {
#endif


struct VIPCBufExtra {
  uint32_t frame_id;
  uint64_t timestamp_sof;
  uint64_t timestamp_eof;
};

struct VisionIpcPacket {
  size_t idx;
  VIPCBufExtra extra;
};

#ifdef __cplusplus
}
#endif
