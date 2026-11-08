#pragma once

#include "Mesh.h"
#include "Vec3.h"
#include <cmath>

// 4x4 matrix, row-major: m[row][col]. Starts as the identity (does nothing).
// A point is treated as (x, y, z, 1): the 1 lets the 4th column translate.
struct Mat4 {
  float m[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};

  // a * b = "apply b first, then a"
  Mat4 operator*(const Mat4 &b) const {
    Mat4 r;
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j) {
        r.m[i][j] = 0;
        for (int k = 0; k < 4; ++k)
          r.m[i][j] += m[i][k] * b.m[k][j];
      }
    return r;
  }

  // positions: w = 1, so translation applies
  Vec3D transform_point(const Vec3D &v) const {
    return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3],
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3],
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3]};
  }

  // Undo this matrix, if it's only rotations + a move (no scaling):
  // the rotation part flips across its diagonal, the move is undone after.
  Mat4 inverse_rigid() const {
    Mat4 r;
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        r.m[i][j] = m[j][i];
    for (int i = 0; i < 3; ++i)
      r.m[i][3] = -(r.m[i][0] * m[0][3] + r.m[i][1] * m[1][3] +
                    r.m[i][2] * m[2][3]);
    return r;
  }

  // directions (normals): w = 0, so translation is ignored
  Vec3D transform_dir(const Vec3D &v) const {
    return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
  }
};

// Same math as rotate_xz / rotate_yz / rotate_xy in Transform.h, as matrices.
inline Mat4 translation(float x, float y, float z) {
  Mat4 r;
  r.m[0][3] = x;
  r.m[1][3] = y;
  r.m[2][3] = z;
  return r;
}

inline Mat4 rotation_xz(float angle) {
  Mat4 r;
  float c = std::cos(angle), s = std::sin(angle);
  r.m[0][0] = c;  r.m[0][2] = -s;
  r.m[2][0] = s;  r.m[2][2] = c;
  return r;
}

inline Mat4 rotation_yz(float angle) {
  Mat4 r;
  float c = std::cos(angle), s = std::sin(angle);
  r.m[1][1] = c;  r.m[1][2] = -s;
  r.m[2][1] = s;  r.m[2][2] = c;
  return r;
}

inline Mat4 rotation_xy(float angle) {
  Mat4 r;
  float c = std::cos(angle), s = std::sin(angle);
  r.m[0][0] = c;  r.m[0][1] = -s;
  r.m[1][0] = s;  r.m[1][1] = c;
  return r;
}

// One call replaces the whole chain of translate_*/rotate_*_triangle calls.
inline Triangle transform_triangle(const Triangle &tri, const Mat4 &mat) {
  Triangle out;
  out.color = tri.color;
  out.texture = tri.texture;
  out.material = tri.material;
  for (int i = 0; i < 3; ++i) {
    out.uv[i] = tri.uv[i];
    out.p[i] = mat.transform_point(tri.p[i]);
    out.n[i] = mat.transform_dir(tri.n[i]);
  }
  return out;
}
