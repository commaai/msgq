#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "msgq/test_runner.h"
#include "msgq/visionipc/visionbuf.h"

TEST_CASE("VisionBuf allocation and fd import") {
  // Include a frame id beyond a page boundary to exercise the full mapping.
  constexpr size_t length = 4096;
  VisionBuf producer;
  producer.allocate(length);
  REQUIRE(producer.len == length);
  REQUIRE(producer.mmap_len >= length + sizeof(uint64_t));

  REQUIRE(producer.sync(VISIONBUF_SYNC_FROM_DEVICE) == 0);
  REQUIRE(producer.get_frame_id() == 0);
  for (size_t i = 0; i < length; ++i) {
    REQUIRE(static_cast<uint8_t *>(producer.addr)[i] == 0);
  }
  memset(producer.addr, 0xa5, length);
  producer.set_frame_id(1234);
  REQUIRE(producer.sync(VISIONBUF_SYNC_TO_DEVICE) == 0);

  VisionBuf consumer;
  consumer.fd = dup(producer.fd);
  REQUIRE(consumer.fd >= 0);
  consumer.len = producer.len;
  consumer.mmap_len = producer.mmap_len;
  consumer.import();
  consumer.init_yuv(64, 32, 64, 2048);
  REQUIRE(consumer.y == consumer.addr);
  REQUIRE(consumer.uv == consumer.y + 2048);

  // The imported fd must keep the allocation alive after the producer exits.
  REQUIRE(producer.free() == 0);
  REQUIRE(consumer.sync(VISIONBUF_SYNC_FROM_DEVICE) == 0);
  REQUIRE(consumer.get_frame_id() == 1234);
  for (size_t i = 0; i < length; ++i) {
    REQUIRE(static_cast<uint8_t *>(consumer.addr)[i] == 0xa5);
  }
  REQUIRE(consumer.sync(VISIONBUF_SYNC_TO_DEVICE) == 0);
  int fd = consumer.fd;
  REQUIRE(consumer.free() == 0);
  REQUIRE(fcntl(fd, F_GETFD) == -1);
}
