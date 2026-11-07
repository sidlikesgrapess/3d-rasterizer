#pragma once

#include "Config.h"
#include "Vec3.h"
#include <cmath>

// PROJECTION MATH
// w, h = size of the framebuffer being drawn into (smaller with render scale)
inline Px2D screen(Vec3D point, float fovDegrees, int w, int h) {
  // tan() is slow and fov rarely changes: only recompute f when fov changes
  static float cached_fov = -1.0f;
  static float f = 0.0f;
  if (fovDegrees != cached_fov) {
    cached_fov = fovDegrees;
    float fovRad = fovDegrees * 0.5f * (3.14159265f / 180.0f);
    f = 1.0f / std::tan(fovRad);
  }
  float aspect = (float)w / (float)h;

  float z = point.z;
  if (z <= 0.001f) // fallback for now
    z = 0.001f;
  float invZ = 1.0f / z;

  float x_ndc = (point.x * f / aspect) * invZ; // normalize to aspect ratio
  float y_ndc = (point.y * f) * invZ;

  int screenX = (int)((x_ndc + 1.0f) * 0.5f *
                      w); // formula = (x+1)/(2*z*tan(fov/2))
  int screenY = (int)((1.0f - y_ndc) * 0.5f * h);

  return {screenX, screenY, z};
}
