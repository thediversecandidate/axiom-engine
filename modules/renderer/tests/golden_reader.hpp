#pragma once
// Test-only reader for the flat numeric fields of tests/golden/*.json. It returns every number in the
// value that follows "key": (nested arrays are flattened). Strings with escaped quotes are not supported.

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace axiom::test {

inline std::string readGolden(std::string_view name) {
  const std::string path = std::string(AXIOM_GOLDEN_DIR) + "/" + std::string(name);
  std::ifstream in(path);
  REQUIRE(in.good());
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

inline std::vector<double> goldenNumbers(const std::string &json, std::string_view key) {
  const std::string quoted = "\"" + std::string(key) + "\"";
  std::size_t pos = json.find(quoted);
  REQUIRE(pos != std::string::npos);
  pos = json.find(':', pos + quoted.size());
  REQUIRE(pos != std::string::npos);
  ++pos;
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n')) {
    ++pos;
  }
  std::vector<double> out;
  int depth = 0;
  do {
    const char c = json[pos];
    if (c == '[') {
      ++depth;
      ++pos;
    } else if (c == ']') {
      --depth;
      ++pos;
    } else if (c == '-' || (c >= '0' && c <= '9')) {
      char *end = nullptr;
      out.push_back(std::strtod(json.c_str() + pos, &end));
      pos = static_cast<std::size_t>(end - json.c_str());
    } else {
      ++pos; // separators and whitespace
    }
  } while (depth > 0 && pos < json.size());
  REQUIRE(depth == 0);
  REQUIRE(!out.empty());
  return out;
}

} // namespace axiom::test
