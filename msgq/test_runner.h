#pragma once

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

struct Section {
  std::size_t selected = 0;
  std::size_t encountered = 0;
  bool active = false;
};

inline Section &section() {
  static Section state;
  return state;
}

class SectionGuard {
public:
  SectionGuard(const char *file, int line) {
    if (section().active) {
      throw Failure("nested SECTION is unsupported", file, line);
    }
    entered = section().encountered++ == section().selected;
    section().active = entered;
  }
  ~SectionGuard() { section().active = false; }
  explicit operator bool() const { return entered; }

private:
  bool entered;
};

inline int run_all() {
  if (tests().empty()) {
    std::cerr << "[FAIL] no tests registered\n";
    return 1;
  }
  std::size_t passed = 0;

  for (const TestCase &test : tests()) {
    bool failed = false;
    std::size_t section_index = 0;
    std::size_t section_count = 0;

    do {
      section() = Section{section_index, 0, false};
      try {
        test.function();
      } catch (const std::exception &error) {
        failed = true;
        std::cerr << "[FAIL] " << test.name << "\n  " << error.what() << '\n';
      } catch (...) {
        failed = true;
        std::cerr << "[FAIL] " << test.name << "\n  unknown exception\n";
      }
      section_count = test_runner::section().encountered;
      ++section_index;
    } while (section_index < section_count);

    if (!failed) {
      ++passed;
      std::cout << "[PASS] " << test.name << '\n';
    }
  }

  std::cout << '\n' << passed << "/" << tests().size() << " tests passed\n";
  return passed == tests().size() ? 0 : 1;
}

}  // namespace test_runner

#define TEST_RUNNER_JOIN_IMPL(left, right) left##right
#define TEST_RUNNER_JOIN(left, right) TEST_RUNNER_JOIN_IMPL(left, right)

#define TEST_CASE(name, ...)                                                    \
  static void TEST_RUNNER_JOIN(test_case_, __LINE__)();                         \
  static const test_runner::Registrar TEST_RUNNER_JOIN(test_registrar_,         \
                                                        __LINE__)(               \
      name, TEST_RUNNER_JOIN(test_case_, __LINE__));                            \
  static void TEST_RUNNER_JOIN(test_case_, __LINE__)()

#define SECTION(name)                                                           \
  if (test_runner::SectionGuard TEST_RUNNER_JOIN(section_, __LINE__){           \
          __FILE__, __LINE__};                                                  \
      TEST_RUNNER_JOIN(section_, __LINE__))

#define REQUIRE(expression)                                                     \
  do {                                                                          \
    if (!(expression)) {                                                        \
      throw test_runner::Failure(#expression, __FILE__, __LINE__);              \
    }                                                                           \
  } while (false)

int main() {
  return test_runner::run_all();
}
