#pragma once
// Test-only image I/O and comparison for reference-image tests (ROADMAP Stage 1).
// Images are binary PAM (P7, TUPLTYPE RGB_ALPHA, MAXVAL 255): readable without an image library.
// References are created by a person (tests/golden/README.md), never by a test.

#include "axiom/renderer/offscreen_target.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace axiom::test {

struct Image {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::vector<renderer::Rgba8> pixels;
};

inline bool writePam(const std::filesystem::path &path, const Image &img) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  out << "P7\nWIDTH " << img.width << "\nHEIGHT " << img.height
      << "\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n";
  out.write(reinterpret_cast<const char *>(img.pixels.data()),
            static_cast<std::streamsize>(img.pixels.size() * sizeof(renderer::Rgba8)));
  return out.good();
}

/// Returns nullopt if the file is missing or is not an RGBA PAM with MAXVAL 255 and a complete payload.
inline std::optional<Image> readPam(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }
  std::string line;
  if (!std::getline(in, line) || line != "P7") {
    return std::nullopt;
  }
  Image img;
  int depth = 0, maxval = 0;
  std::string tupltype;
  while (std::getline(in, line) && line != "ENDHDR") {
    std::istringstream fields(line);
    std::string key;
    fields >> key;
    if (key == "WIDTH")
      fields >> img.width;
    else if (key == "HEIGHT")
      fields >> img.height;
    else if (key == "DEPTH")
      fields >> depth;
    else if (key == "MAXVAL")
      fields >> maxval;
    else if (key == "TUPLTYPE")
      fields >> tupltype;
  }
  if (line != "ENDHDR" || depth != 4 || maxval != 255 || tupltype != "RGB_ALPHA" || img.width == 0 || img.height == 0) {
    return std::nullopt;
  }
  img.pixels.resize(std::size_t{img.width} * img.height);
  in.read(reinterpret_cast<char *>(img.pixels.data()),
          static_cast<std::streamsize>(img.pixels.size() * sizeof(renderer::Rgba8)));
  if (in.gcount() != static_cast<std::streamsize>(img.pixels.size() * sizeof(renderer::Rgba8))) {
    return std::nullopt;
  }
  return img;
}

struct Comparison {
  std::size_t exceeding = 0; // pixels where any channel differs by more than the tolerance
  int maxDelta = 0;
  Image diff; // red = exceeding; otherwise the reference at quarter brightness
};

/// Precondition: equal dimensions (checked by the caller).
inline Comparison compareImages(const Image &actual, const Image &reference, int tolerance) {
  Comparison c;
  c.diff = Image{reference.width, reference.height, std::vector<renderer::Rgba8>(reference.pixels.size())};
  for (std::size_t i = 0; i < reference.pixels.size(); ++i) {
    const auto &a = actual.pixels[i];
    const auto &r = reference.pixels[i];
    const int d = std::max({std::abs(a.r - r.r), std::abs(a.g - r.g), std::abs(a.b - r.b), std::abs(a.a - r.a)});
    c.maxDelta = std::max(c.maxDelta, d);
    if (d > tolerance) {
      ++c.exceeding;
      c.diff.pixels[i] = {255, 0, 0, 255};
    } else {
      c.diff.pixels[i] = {static_cast<std::uint8_t>(r.r / 4), static_cast<std::uint8_t>(r.g / 4),
                          static_cast<std::uint8_t>(r.b / 4), 255};
    }
  }
  return c;
}

} // namespace axiom::test
