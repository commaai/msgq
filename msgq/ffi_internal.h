#pragma once

#include <cerrno>
#include <cstdio>
#include <exception>
#include <utility>

namespace msgq_ffi {

extern thread_local char last_error[512];

inline void set_error(const char *error) {
  std::snprintf(last_error, sizeof(last_error), "%s", error);
  errno = EIO;
}

template <typename Result, typename Callable>
Result translate_exceptions(Result failure, Callable &&callable) noexcept {
  last_error[0] = '\0';
  try {
    return std::forward<Callable>(callable)();
  } catch (const std::exception &error) {
    set_error(error.what());
  } catch (...) {
    set_error("unknown C++ exception");
  }
  return failure;
}

template <typename Callable>
void translate_exceptions(Callable &&callable) noexcept {
  translate_exceptions(false, [&]() {
    std::forward<Callable>(callable)();
    return true;
  });
}

}  // namespace msgq_ffi
