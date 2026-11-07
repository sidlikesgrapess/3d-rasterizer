#pragma once

#include "Mesh.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// OBJECT LOADER (.obj)
inline Mesh OBJLoader(std::string filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filePath << std::endl;
    return {};
  }
  Mesh mesh;
  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string type;
    ss >> type;
    if (type == "v") {
      float x, y, z;
      ss >> x >> y >> z;
      mesh.vertices.push_back({x, y, z});
    } else if (type == "f") {
      std::vector<int> face_v;
      std::vector<int> face_vn;
      std::string token;
      while (ss >> token) {
        int v_idx = 0, vt_idx = 0, vn_idx = 0;
        if (token.find("//") != std::string::npos) {
          sscanf(token.c_str(), "%d//%d", &v_idx, &vn_idx);
        } else {
          sscanf(token.c_str(), "%d/%d/%d", &v_idx, &vt_idx, &vn_idx);
        }
        if (v_idx > 0)
          face_v.push_back(v_idx - 1);
        if (vn_idx > 0)
          face_vn.push_back(vn_idx - 1);
      }
      for (size_t i = 1; i + 1 < face_v.size(); ++i) {
        Triangle tri;
        tri.p[0] = mesh.vertices[face_v[0]];
        tri.p[1] = mesh.vertices[face_v[i]];
        tri.p[2] = mesh.vertices[face_v[i + 1]];

        if (face_vn.size() == face_v.size() && !mesh.vert_normals.empty()) {
          tri.n[0] = mesh.vert_normals[face_vn[0]];
          tri.n[1] = mesh.vert_normals[face_vn[i]];
          tri.n[2] = mesh.vert_normals[face_vn[i + 1]];
        }

        tri.color = col::RANDOM();
        mesh.tris.push_back(tri);
      }
    } else if (type == "vn") {
      float x, y, z;
      ss >> x >> y >> z;
      mesh.vert_normals.push_back({x, y, z});
    }
  }
  return mesh;
}
