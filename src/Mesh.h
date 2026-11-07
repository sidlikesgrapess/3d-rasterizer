#pragma once

#include "Color.h"
#include "Texture.h"
#include "Vec3.h"
#include <vector>

// texture coordinate (vt in the .obj)
struct UV {
  float u = 0, v = 0;
};

struct Triangle {
  Vec3D p[3];
  Vec3D n[3] = {};
  Color color = col::WHITE;
  UV uv[3] = {};    // texture coordinate at each corner
  int texture = -1; // index into Mesh::textures, -1 = no texture
};

struct Mesh {
  std::vector<Vec3D> vertices;
  std::vector<Triangle> tris;
  std::vector<Vec3D> vert_normals;
  std::vector<Texture> textures; // loaded from the .mtl file
};
