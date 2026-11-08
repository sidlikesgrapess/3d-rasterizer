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

// MATERIALS
// The light at a point is split in two steps:
//  1. Light_terms: the parts that only depend on directions (normal, light,
//     camera) and roughness. Gouraud works these out at the 3 corners and
//     blends them; flat once per triangle; Phong/ray tracing once per pixel.
//  2. Shade_material: mixes those with the surface color and the material.

struct LightTerms {
  float diffuse = 0;   // how much light hits (0.15 = ambient, 1 = full sun)
  float highlight = 0; // the shiny spot's strength (before its color)
  float facing = 1;    // 1 = faces the camera, 0 = seen edge-on
};

// normal: inward, like face_normal(). light_v: the way the light travels.
// view_dir: from the camera to the point. intensity: how strong the light is
// here (1 = sun; a point light gets weaker with distance, see Light.h).
inline LightTerms Light_terms(Vec3D normal, Vec3D light_v, Vec3D view_dir,
                              float roughness, float intensity = 1.0f) {
  Vec3D n = normal * -1.0f; // outward
  Vec3D to_light = (light_v * -1.0f).normalized();
  Vec3D to_eye = (view_dir * -1.0f).normalized();
  LightTerms t;
  float n_dot_l = n.dot(to_light);
  // at least 0.15 (ambient). A strong point light can go above 1.
  t.diffuse = std::max(n_dot_l * intensity, 0.15f);
  t.facing = std::clamp(n.dot(to_eye), 0.0f, 1.0f);
  if (n_dot_l > 0.0f) {
    // Blinn-Phong: "half" is halfway between the light and the eye. The
    // closer the normal is to it, the brighter the shiny spot.
    Vec3D half = (to_light + to_eye).normalized();
    float r = std::max(roughness, 0.05f);
    float shininess = 2.0f / (r * r) - 2.0f; // smooth = small, sharp spot
    // (shininess + 8) / 8: a smaller spot gets brighter, so smooth and
    // rough surfaces reflect the same total amount of light
    t.highlight = (shininess + 8.0f) / 8.0f *
                  std::pow(std::max(n.dot(half), 0.0f), shininess) * n_dot_l *
                  intensity;
  }
  return t;
}

// Final color. base = texture or material color. reflection = what a mirror
// would see here: the grey background, or what a reflection ray hit (ray
// tracing). exposure_scale = log10(exposure * 10 + 1), worked out per frame.
inline Color Shade_material(const LightTerms &t, Color base,
                            const Material &m, Color reflection,
                            float exposure_scale) {
  // Fresnel: everything reflects more when seen edge-on (Schlick's
  // (1 - cos)^5). Rough surfaces less so.
  float e = 1.0f - t.facing;
  float edge = e * e * e * e * e;
  float c[3] = {base.r / 255.0f, base.g / 255.0f, base.b / 255.0f};
  float env[3] = {reflection.r / 255.0f, reflection.g / 255.0f,
                  reflection.b / 255.0f};
  float out[3];
  for (int k = 0; k < 3; ++k) { // red, green, blue
    // how much is reflected head-on: 4% white for non-metals, the surface
    // color for metals
    float f0 = 0.04f + (c[k] - 0.04f) * m.metallic;
    float fresnel = f0 + (std::max(1.0f - m.roughness, f0) - f0) * edge;
    float diffuse = c[k] * t.diffuse * (1.0f - m.metallic); // metals: none
    float v = (diffuse + f0 * t.highlight + fresnel * env[k]) * exposure_scale;
    out[k] = std::min(v, 1.0f) * 255.0f;
  }
  return {(Uint8)out[0], (Uint8)out[1], (Uint8)out[2], 255};
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

// Flat shading: one normal for the whole triangle (calculated, or the
// average of the .obj normals flipped inward)
inline Vec3D flat_normal(const Triangle &tri, bool calculateNormals) {
  if (calculateNormals)
    return face_normal(tri);
  Vec3D cam = {0, 0, 0};
  return cam - ((tri.n[0] + tri.n[1] + tri.n[2]).normalized());
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
