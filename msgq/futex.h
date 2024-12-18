#pragma once

#include <cstdint>
#include <atomic>
#include <string>


class Futex {
public:
  Futex(const std::string &path);
  ~Futex();
  void broadcast();
  bool wait(uint32_t expected, int timeout_ms);
  inline uint32_t value() const { return futex->load(); }

private:
  std::atomic<uint32_t> *futex = nullptr;
};
