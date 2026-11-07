#pragma once

#include "Color.h"
#include "Framebuffer.h"
#include "Mat4.h"
#include "Projection.h"
#include "Vec3.h"
#include <vector>

// A 3D line segment in WORLD coordinates (used for the floor grid and axes).
struct Line {
  Vec3D a, b;
  Color color;
};

// Floor grid on the xz plane (y = 0), one line every unit from -size to +size.
inline std::vector<Line> make_grid(int size) {
  std::vector<Line> lines;
  Color c = {90, 90, 90, 255};
  float s = (float)size;
  for (int i = -size; i <= size; ++i) {
    float f = (float)i;
    lines.push_back({{f, 0, -s}, {f, 0, s}, c}); // runs along z
    lines.push_back({{-s, 0, f}, {s, 0, f}, c}); // runs along x
  }
  return lines;
}

// X red, Y green, Z blue, through the world origin.
inline std::vector<Line> make_axes(float length) {
  return {{{-length, 0, 0}, {length, 0, 0}, col::RED},
          {{0, -length, 0}, {0, length, 0}, col::GREEN},
          {{0, 0, -length}, {0, 0, length}, col::BLUE}};
}

// A fixed-size circle on screen around a world point (used to show the light)
inline void draw_circle(Framebuffer &fb, Vec3D world, const Mat4 &view,
                        float fov, float radius_px, Color color) {
  Vec3D p = view.transform_point(world);
  if (p.z < 0.1f)
    return; // behind the camera
  Px2D c = screen(p, fov, fb.width, fb.height);
  const int segments = 24;
  for (int i = 0; i < segments; ++i) {
    float a0 = 2.0f * 3.14159265f * i / segments;
    float a1 = 2.0f * 3.14159265f * (i + 1) / segments;
    fb.DrawLine(c.x + radius_px * std::cos(a0), c.y + radius_px * std::sin(a0),
                c.x + radius_px * std::cos(a1), c.y + radius_px * std::sin(a1),
                color);
  }
}

// world -> camera with the view matrix, then project and draw like triangles do
inline void draw_lines(Framebuffer &fb, const std::vector<Line> &lines,
                       const Mat4 &view, float fov) {
  const float near_z = 0.1f; // same cull distance the triangles use
  for (const Line &l : lines) {
    Vec3D a = view.transform_point(l.a);
    Vec3D b = view.transform_point(l.b);

    if (a.z < near_z && b.z < near_z)
      continue; // fully behind the camera
    // partly behind: cut it off at near_z, otherwise screen() blows up
    if (a.z < near_z)
      a = a + (b - a) * ((near_z - a.z) / (b.z - a.z));
    if (b.z < near_z)
      b = b + (a - b) * ((near_z - b.z) / (a.z - b.z));

    Px2D p0 = screen(a, fov, fb.width, fb.height);
    Px2D p1 = screen(b, fov, fb.width, fb.height);
    fb.DrawLine(p0.x, p0.y, p1.x, p1.y, l.color);
  }
}
