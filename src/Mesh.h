#pragma once

#include "Color.h"
#include "Vec3.h"
#include <vector>

struct Triangle {
  Vec3D p[3];
  Vec3D n[3] = {};
  Color color = col::WHITE;
};

struct Mesh {
  std::vector<Vec3D> vertices;
  std::vector<Triangle> tris;
  std::vector<Vec3D> vert_normals;
};
