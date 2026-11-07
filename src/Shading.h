#pragma once

#include "Color.h"
#include "Mesh.h"
#include "Vec3.h"
#include <algorithm>
#include <cmath>
#include <vector>

// SHADING / CULLING

// The 3 shading modes picked in the menu
enum ShadingMode { SHADING_FLAT, SHADING_GOURAUD, SHADING_PHONG };

// How much light hits a surface with this normal (0.15 .. 1)
inline float Light_intensity(Vec3D normal, Vec3D Light_v) {
  float light_intensity = normal.dot(Light_v.normalized());
  return std::clamp(light_intensity, 0.15f, 1.0f);
}

// Final color from the light amount + the surface color (green or texture)
inline Color Shade_intensity(float light_intensity, Color color,
                             float specularity, float exposure) {
  // x^8 as 3 multiplies instead of pow()
  float li2 = light_intensity * light_intensity;
  float li4 = li2 * li2;
  float specular = li4 * li4;
  Color spec = col::WHITE;
  spec = spec * specular;

  // log10 is slow and exposure rarely changes: only recompute when it changes
  static float cached_exposure = -1.0f;
  static float exposure_scale = 0.0f;
  if (exposure != cached_exposure) {
    cached_exposure = exposure;
    exposure_scale = std::log10(exposure * 10 + 1.0f);
  }

  return ((color * light_intensity * exposure_scale) +
          spec * specularity * exposure); // some type of faked exposure ig
}

// Lighting for ONE normal. Flat calls it once per triangle, Phong once per
// pixel. (Gouraud lights the corners with Light_intensity, blends that, then
// calls Shade_intensity per pixel.)
inline Color Shade_normal(Vec3D normal, Vec3D Light_v, Color color,
                          float specularity, float exposure) {
  return Shade_intensity(Light_intensity(normal, Light_v), color, specularity,
                         exposure);
}

// Face normal from the triangle's corners
inline Vec3D face_normal(const Triangle &tri) {
  Vec3D l1 = tri.p[0] - tri.p[1];
  Vec3D l2 = tri.p[2] - tri.p[1];
  return l1.cross(l2).normalized();
}

// Normal at corner i from the .obj, flipped to point the same way as
// face_normal (like the `cam - ...` below). No .obj normals -> face normal.
inline Vec3D vertex_normal(const Triangle &tri, int i) {
  if (tri.n[i].length() == 0.0f)
    return face_normal(tri);
  return (tri.n[i] * -1.0f).normalized();
}

// Flat shading: one color for the whole triangle
inline Color Shade_triangle(const Triangle &tri, Vec3D Light_v,
                            Color color, bool calculateNormals = true,
                            float specularity = 0.8f, float exposure = 1.0f) {
  Vec3D normal;
  Vec3D cam = {0, 0, 0};
  if (calculateNormals) {
    normal = face_normal(tri);
  } else {
    normal = cam - ((tri.n[0] + tri.n[1] + tri.n[2]).normalized());
  }
  return Shade_normal(normal, Light_v, color, specularity, exposure);
}

inline bool isTriangleCulled(const Triangle &tri, float cull_dist,
                             bool backface_cull = true) {
  if (tri.p[0].z < cull_dist || tri.p[1].z < cull_dist ||
      tri.p[2].z < cull_dist) {
    return true;
  }
  if (!backface_cull)
    return false;
  Vec3D v0 = tri.p[0];
  Vec3D v1 = tri.p[1];
  Vec3D v2 = tri.p[2];
  Vec3D cam = {0, 0, 0};
  Vec3D l1 = v0 - v1;
  Vec3D l2 = v2 - v1;

  Vec3D normal = l1.cross(l2);
  normal = normal.normalized();
  float angle = normal.dot(cam - v1);
  return (angle > 0.0f) ? true : false;
}

inline void Painters_algo(std::vector<Triangle> &transformed_tri) {
  std::sort(transformed_tri.begin(), transformed_tri.end(),
            [](const Triangle &t1, const Triangle &t2) {
              float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
              float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
              return z1 > z2; // furthest Z rendered first, closest Z rendered
                              // last
            });
}
