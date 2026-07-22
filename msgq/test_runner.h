#pragma once

#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_runner {

struct TestCase {
  const char *name;
  void (*function)();
};

inline std::vector<TestCase> &tests() {
  static std::vector<TestCase> registered_tests;
  return registered_tests;
}

struct Registrar {
  Registrar(const char *name, void (*function)()) {
    tests().push_back({name, function});
  }
};

class Failure : public std::runtime_error {
public:
  Failure(const char *expression, const char *file, int line)
      : std::runtime_error(std::string(file) + ":" + std::to_string(line) +
                           ": REQUIRE(" + expression + ") failed") {}
};

inline std::size_t &assertion_count() {
  static std::size_t count = 0;
  return count;
}

inline void require(bool result, const char *expression, const char *file, int line) {
  assertion_count()++;
  if (!result) {
    throw Failure(expression, file, line);
  }
}

inline int run_all() {
  if (tests().empty()) {
    std::cerr << "[FAIL] no tests registered\n";
    return 1;
  }
  std::size_t passed = 0;

  for (const TestCase &test : tests()) {
    try {
      assertion_count() = 0;
      test.function();
      if (assertion_count() == 0) {
        throw std::runtime_error("no assertions executed");
      }
      ++passed;
      std::cout << "[PASS] " << test.name << '\n';
    } catch (const std::exception &error) {
      std::cerr << "[FAIL] " << test.name << "\n  " << error.what() << '\n';
    } catch (...) {
      std::cerr << "[FAIL] " << test.name << "\n  unknown exception\n";
    }
  }

  std::cout << '\n' << passed << "/" << tests().size() << " tests passed\n";
  return passed == tests().size() ? 0 : 1;
}

}  // namespace test_runner

#define TEST_RUNNER_JOIN_IMPL(left, right) left##right
#define TEST_RUNNER_JOIN(left, right) TEST_RUNNER_JOIN_IMPL(left, right)

#define TEST_CASE(name)                                                         \
  static void TEST_RUNNER_JOIN(test_case_, __LINE__)();                         \
  static const test_runner::Registrar TEST_RUNNER_JOIN(test_registrar_,         \
                                                        __LINE__)(               \
      name, TEST_RUNNER_JOIN(test_case_, __LINE__));                            \
  static void TEST_RUNNER_JOIN(test_case_, __LINE__)()

#define REQUIRE(expression)                                                     \
  do {                                                                          \
    test_runner::require(static_cast<bool>(expression), #expression, __FILE__,  \
                         __LINE__);                                              \
  } while (false)

int main() {
  return test_runner::run_all();
}
