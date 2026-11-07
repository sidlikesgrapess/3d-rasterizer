#pragma once

#include "SDLHeaders.h"
#include <cstdint>
#include <cstdlib>

struct Color {
  Uint8 r = 0, g = 0, b = 0, a = 255;
  uint32_t to_hex() {
    return (static_cast<uint32_t>(r) << 24) | (static_cast<uint32_t>(g) << 16) |
           (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
  }
  Color operator+(const Color &other) const {
    return {static_cast<Uint8>((r + other.r) > 255 ? 255 : r + other.r),
            static_cast<Uint8>((g + other.g) > 255 ? 255 : g + other.g),
            static_cast<Uint8>((b + other.b) > 255 ? 255 : b + other.b),
            static_cast<Uint8>((a + other.a) > 255 ? 255 : a + other.a)};
  }
  Color operator-(const Color &other) const {
    return {static_cast<Uint8>((r - other.r) < 0 ? 0 : r - other.r),
            static_cast<Uint8>((g - other.g) < 0 ? 0 : g - other.g),
            static_cast<Uint8>((b - other.b) < 0 ? 0 : b - other.b),
            static_cast<Uint8>((a - other.a) < 0 ? 0 : a - other.a)};
  }
  Color operator*(const float scalar) const {
    return {static_cast<Uint8>((r * scalar) > 255 ? 255 : r * scalar),
            static_cast<Uint8>((g * scalar) > 255 ? 255 : g * scalar),
            static_cast<Uint8>((b * scalar) > 255 ? 255 : b * scalar),
            static_cast<Uint8>((a * scalar) > 255 ? 255 : a * scalar)};
  }
  Color operator*(const Color other) const {
    return {static_cast<Uint8>((r * other.r) > 255 ? 255 : r * other.r),
            static_cast<Uint8>((g * other.g) > 255 ? 255 : g * other.g),
            static_cast<Uint8>((b * other.b) > 255 ? 255 : b * other.b),
            static_cast<Uint8>((a * other.a) > 255 ? 255 : a * other.a)};
  }
  Color operator/(const float scalar) const {
    return {static_cast<Uint8>(r / scalar > 255 ? 255 : r / scalar),
            static_cast<Uint8>(g / scalar > 255 ? 255 : g / scalar),
            static_cast<Uint8>(b / scalar > 255 ? 255 : b / scalar),
            static_cast<Uint8>(a / scalar > 255 ? 255 : a / scalar)};
  }
};

namespace col {
const Color BLACK = {0, 0, 0, 255};
const Color WHITE = {255, 255, 255, 255};
const Color GREEN = {0, 255, 0, 255};
const Color RED = {255, 0, 0, 255};
const Color BLUE = {0, 0, 255, 255};
const Color YELLOW = {255, 255, 0, 255};
const Color CYAN = {0, 255, 255, 255};
const Color MAGENTA = {255, 0, 255, 255};
const Color GREY = {128, 128, 128, 255};
inline Color RANDOM() {
  return {static_cast<Uint8>(std::rand() % 256),
          static_cast<Uint8>(std::rand() % 256),
          static_cast<Uint8>(std::rand() % 256), 255};
}
} // namespace col
