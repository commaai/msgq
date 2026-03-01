#include "msgq/logger/logger.h"

#include <atomic>
#include <cstdio>
#include <string>

namespace {

void default_logger(int level, const char *file, int line, const char *msg) {
  std::fprintf(stderr, "[msgq] %d %s:%d %s\n",
               level,
               file != nullptr ? file : "<unknown>",
               line,
               msg != nullptr ? msg : "");
}

std::atomic<msgq_logger_callback_t> logger_callback(default_logger);

}  // namespace

extern "C" void msgq_set_logger(msgq_logger_callback_t callback) {
  logger_callback.store(callback != nullptr ? callback : default_logger, std::memory_order_release);
}

extern "C" void msgq_logv(int level, const char *file, int line, const char *fmt, va_list args) {
  if (fmt == nullptr) return;

  va_list args_copy;
  va_copy(args_copy, args);
  const int needed = std::vsnprintf(nullptr, 0, fmt, args_copy);
  va_end(args_copy);
  if (needed < 0) return;

  std::string message(static_cast<size_t>(needed), '\0');
  std::vsnprintf(message.data(), message.size() + 1, fmt, args);

  msgq_logger_callback_t callback = logger_callback.load(std::memory_order_acquire);
  callback(level, file, line, message.c_str());
}

extern "C" void msgq_log(int level, const char *file, int line, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  msgq_logv(level, file, line, fmt, args);
  va_end(args);
}
