#pragma once

#include <string>
#include <variant>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

// Cython accepted both text and bytes for C++ strings. Keep that API while
// exposing the full input type to nanobind's stub generator.
using PythonString = std::variant<std::string, nanobind::bytes>;

inline std::string python_string(const PythonString &value) {
  if (const auto *text = std::get_if<std::string>(&value)) return *text;
  const auto &bytes = std::get<nanobind::bytes>(value);
  return std::string(bytes.c_str(), bytes.size());
}
