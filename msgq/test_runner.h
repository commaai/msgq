#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_runner {

using TestFunction = void (*)();

struct TestCase {
  const char *name;
  TestFunction function;
};

inline std::vector<TestCase> &tests() {
  static std::vector<TestCase> registered_tests;
  return registered_tests;
}

class Registrar {
public:
  Registrar(const char *name, TestFunction function) {
    tests().push_back({name, function});
  }
};

class Failure : public std::runtime_error {
public:
  Failure(const char *expression, const char *file, int line)
      : std::runtime_error(std::string(file) + ":" + std::to_string(line) +
                           ": REQUIRE(" + expression + ") failed") {}
};

struct SectionState {
  std::size_t selected = 0;
  std::size_t encountered = 0;
};

inline SectionState &section_state() {
  static SectionState state;
  return state;
}

inline bool enter_section() {
  SectionState &state = section_state();
  return state.encountered++ == state.selected;
}

int run_all();

}  // namespace test_runner

#define TEST_RUNNER_JOIN_IMPL(left, right) left##right
#define TEST_RUNNER_JOIN(left, right) TEST_RUNNER_JOIN_IMPL(left, right)

#define TEST_CASE(name, ...)                                                    \
  static void TEST_RUNNER_JOIN(test_case_, __LINE__)();                         \
  static const test_runner::Registrar TEST_RUNNER_JOIN(test_registrar_,         \
                                                        __LINE__)(               \
      name, TEST_RUNNER_JOIN(test_case_, __LINE__));                            \
  static void TEST_RUNNER_JOIN(test_case_, __LINE__)()

#define SECTION(name) if (test_runner::enter_section())

#define REQUIRE(expression)                                                     \
  do {                                                                          \
    if (!(expression)) {                                                        \
      throw test_runner::Failure(#expression, __FILE__, __LINE__);              \
    }                                                                           \
  } while (false)
