#include "msgq/test_runner.h"

#include <exception>
#include <iostream>

namespace test_runner {

int run_all() {
  std::size_t passed = 0;

  for (const TestCase &test : tests()) {
    bool failed = false;
    std::size_t section = 0;
    std::size_t section_count = 0;

    do {
      SectionState &state = section_state();
      state = {.selected = section, .encountered = 0};

      try {
        test.function();
      } catch (const std::exception &error) {
        failed = true;
        std::cerr << "[FAIL] " << test.name << "\n  " << error.what() << '\n';
      } catch (...) {
        failed = true;
        std::cerr << "[FAIL] " << test.name << "\n  unknown exception\n";
      }

      section_count = state.encountered;
      ++section;
    } while (section < section_count);

    if (!failed) {
      ++passed;
      std::cout << "[PASS] " << test.name << '\n';
    }
  }

  const std::size_t total = tests().size();
  std::cout << "\n" << passed << "/" << total << " tests passed\n";
  return passed == total ? 0 : 1;
}

}  // namespace test_runner

int main() {
  return test_runner::run_all();
}
