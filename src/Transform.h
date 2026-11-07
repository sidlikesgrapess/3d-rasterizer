#pragma once

#include "Mesh.h"
#include "Vec3.h"
#include <cmath>

// ALL TRANSFORM FUNCTIONS
inline Vec3D rotate_xz(Vec3D point, float angle) {
  float cosA = std::cos(angle);
  float sinA = std::sin(angle);
  return {point.x * cosA - point.z * sinA, point.y,
          point.x * sinA + point.z * cosA};
}

inline Vec3D rotate_yz(Vec3D point, float angle) {
  float cosA = std::cos(angle);
  float sinA = std::sin(angle);
  return {point.x, point.y * cosA - point.z * sinA,
          point.y * sinA + point.z * cosA};
}
inline Vec3D rotate_xy(Vec3D point, float angle) {
  float cosA = std::cos(angle);
  float sinA = std::sin(angle);
  return {point.x * cosA - point.y * sinA, point.x * sinA + point.y * cosA,
          point.z};
}

inline Triangle translate_x_triangle(const Triangle &triangle, float x_offset) {
  Triangle translatedTriangle;
  translatedTriangle.color = triangle.color;
  for (int i = 0; i < 3; ++i) {
    translatedTriangle.p[i] = triangle.p[i];
    translatedTriangle.p[i].x += x_offset;
    translatedTriangle.n[i] = triangle.n[i];
  }
  return translatedTriangle;
}

inline Triangle translate_z_triangle(const Triangle &triangle, float z_offset) {
  Triangle translatedTriangle;
  translatedTriangle.color = triangle.color;
  for (int i = 0; i < 3; ++i) {
    translatedTriangle.p[i] = triangle.p[i];
    translatedTriangle.p[i].z += z_offset;
    translatedTriangle.n[i] = triangle.n[i];
  }
  return translatedTriangle;
}

inline Triangle translate_y_triangle(const Triangle &triangle, float y_offset) {
  Triangle translatedTriangle;
  translatedTriangle.color = triangle.color;
  for (int i = 0; i < 3; ++i) {
    translatedTriangle.p[i] = triangle.p[i];
    translatedTriangle.p[i].y += y_offset;
    translatedTriangle.n[i] = triangle.n[i];
  }
  return translatedTriangle;
}

inline Triangle rotate_xz_triangle(const Triangle &triangle, float angle) {
  Triangle rotatedTriangle;
  rotatedTriangle.color = triangle.color;
  for (int i = 0; i < 3; ++i) {
    rotatedTriangle.p[i] = rotate_xz(triangle.p[i], angle);
    rotatedTriangle.n[i] = rotate_xz(triangle.n[i], angle);
  }
  return rotatedTriangle;
}

inline Triangle rotate_yz_triangle(const Triangle &triangle, float angle) {
  Triangle rotatedTriangle;
  rotatedTriangle.color = triangle.color;
  for (int i = 0; i < 3; ++i) {
    rotatedTriangle.p[i] = rotate_yz(triangle.p[i], angle);
    rotatedTriangle.n[i] = rotate_yz(triangle.n[i], angle);
  }
  return rotatedTriangle;
}

inline Triangle rotate_xy_triangle(const Triangle &triangle, float angle) {
  Triangle rotatedTriangle;
  rotatedTriangle.color = triangle.color;
  for (int i = 0; i < 3; ++i) {
    rotatedTriangle.p[i] = rotate_xy(triangle.p[i], angle);
    rotatedTriangle.n[i] = rotate_xy(triangle.n[i], angle);
  }
  return rotatedTriangle;
}
