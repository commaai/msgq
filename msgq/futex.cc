#include "msgq/futex.h"

#include <fcntl.h>
#include <limits.h>
#include <linux/futex.h>
#include <stdio.h>
#include <sys/mman.h>
#include <syscall.h>
#include <unistd.h>

#include <cassert>
#include <stdexcept>

Futex::Futex(const std::string &path) {
  auto fd = open(path.c_str(), O_RDWR | O_CREAT, 0664);
  if (fd < 0) {
    throw std::runtime_error("Failed to open file: " + path);
  }

  if (ftruncate(fd, sizeof(uint32_t)) < 0) {
    close(fd);
    throw std::runtime_error("Failed to truncate file: " + path);
  }

  int *mem = (int *)mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  if (mem == MAP_FAILED) {
    throw std::runtime_error("Failed to mmap file: " + path);
  }

  futex = reinterpret_cast<std::atomic<uint32_t> *>(mem);
}

Futex::~Futex() {
  munmap(futex, sizeof(uint32_t));
}

void Futex::broadcast() {
  // Increment the futex value to signal waiting threads
  futex->fetch_add(1, std::memory_order_relaxed);

  // Wake up all threads waiting on the futex
  syscall(SYS_futex, futex, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
}

bool Futex::wait(uint32_t expected, int timeout_ms) {
  if (futex->load(std::memory_order_relaxed) != expected) {
    return true;  // Already not equal, no need to wait
  }

  if (timeout_ms <= 0) {
    return false;  // Timeout immediately
  }

  // Perform the futex wait syscall
  struct timespec ts;
  ts.tv_sec = timeout_ms / 1000;
  ts.tv_nsec = (timeout_ms % 1000) * 1000 * 1000;
  syscall(SYS_futex, futex, FUTEX_WAIT, expected, &ts, nullptr, 0);

  return futex->load(std::memory_order_relaxed) != expected;
}
