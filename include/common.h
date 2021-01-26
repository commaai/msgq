#pragma once

#include <atomic>
#include <cassert>
#include <csignal>
#include <mutex>

class ExitSignalHandler {
 public:
  static void init() { std::call_once(init_sig_flag, init_sig_handler); }
  inline operator bool() const { return do_exit; }

 private:
  static void vipc_sig_handler(int signal) {
    assert(signal == SIGINT || signal == SIGTERM);
    do_exit = true;

    if (signal == SIGINT && old_sigint_handler) old_sigint_handler(signal);
    if (signal == SIGTERM && old_sigterm_handler) old_sigterm_handler(signal);
  }

  static void init_sig_handler() {
    old_sigint_handler = old_sigterm_handler = nullptr;
    struct sigaction old;
    if (sigaction(SIGINT, NULL, &old) == 0) old_sigint_handler = old.sa_handler;
    if (sigaction(SIGTERM, NULL, &old) == 0) old_sigterm_handler = old.sa_handler;

    struct sigaction sa = {};
    sa.sa_handler = vipc_sig_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
  }
  inline static std::once_flag init_sig_flag;
  inline static std::atomic<bool> do_exit = false;
  inline static void (*old_sigint_handler)(int) = nullptr;
  inline static void (*old_sigterm_handler)(int) = nullptr;
};
