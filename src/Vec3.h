#pragma once

#include <cmath>

struct Vec3D {
  float x, y, z;

  Vec3D operator+(const Vec3D &other) const {
    return {x + other.x, y + other.y, z + other.z};
  }

  Vec3D operator-(const Vec3D &other) const {
    return {x - other.x, y - other.y, z - other.z};
  }

  Vec3D operator*(const float scalar) const {
    return {x * scalar, y * scalar, z * scalar};
  }

  Vec3D operator/(const float scalar) const {
    return {x / scalar, y / scalar, z / scalar};
  }

  Vec3D cross(const Vec3D &other) const {
    return {y * other.z - z * other.y, z * other.x - x * other.z,
            x * other.y - y * other.x};
  }

  float dot(const Vec3D &other) const {
    return x * other.x + y * other.y + z * other.z;
  }

  float length() const { return std::sqrt(x * x + y * y + z * z); }

  Vec3D normalized() const {
    float len = length();
    if (len == 0.0f)
      return {0, 0, 0};
    return *this / len;
  }
};

struct Px2D {
  int x, y;
  float z;
};
