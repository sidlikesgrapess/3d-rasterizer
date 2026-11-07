#pragma once

#include "Mesh.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// folder part of a path, with the trailing slash ("Object/a.obj" -> "Object/")
inline std::string folder_of(const std::string &path) {
  size_t slash = path.find_last_of("/\\");
  return (slash == std::string::npos) ? "" : path.substr(0, slash + 1);
}

// MATERIAL LOADER (.mtl): for each material, load its map_Kd image (each
// image file only once) and remember which texture the material uses.
inline void load_mtl(const std::string &mtlPath, Mesh &mesh,
                     std::map<std::string, int> &material_texture) {
  std::ifstream file(mtlPath);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << mtlPath << std::endl;
    return;
  }
  std::map<std::string, int> loaded; // image path -> index in mesh.textures
  std::string line, material;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string type;
    ss >> type;
    if (type == "newmtl") {
      ss >> material;
    } else if (type == "map_Kd") {
      std::string path;
      std::getline(ss >> std::ws, path); // rest of line: may contain spaces
      while (!path.empty() && (path.back() == '\r' || path.back() == ' '))
        path.pop_back();
      bool absolute = path.size() > 1 && (path[1] == ':' || path[0] == '/');
      if (!absolute)
        path = folder_of(mtlPath) + path; // relative to the .mtl file

      if (loaded.count(path)) {
        material_texture[material] = loaded[path];
      } else {
        Texture tex;
        if (load_texture(path, tex)) {
          mesh.textures.push_back(tex);
          loaded[path] = (int)mesh.textures.size() - 1;
          material_texture[material] = loaded[path];
        }
      }
    }
  }
}

// OBJECT LOADER (.obj)
inline Mesh OBJLoader(std::string filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filePath << std::endl;
    return {};
  }
  Mesh mesh;
  std::vector<UV> uvs;                         // all vt lines
  std::map<std::string, int> material_texture; // material name -> texture
  int current_texture = -1;                    // set by usemtl
  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string type;
    ss >> type;
    if (type == "v") {
      float x, y, z;
      ss >> x >> y >> z;
      mesh.vertices.push_back({x, y, z});
    } else if (type == "vt") {
      float u, v;
      ss >> u >> v;
      uvs.push_back({u, v});
    } else if (type == "mtllib") {
      std::string name;
      ss >> name;
      load_mtl(folder_of(filePath) + name, mesh, material_texture);
    } else if (type == "usemtl") {
      std::string name;
      ss >> name;
      current_texture =
          material_texture.count(name) ? material_texture[name] : -1;
    } else if (type == "f") {
      std::vector<int> face_v;
      std::vector<int> face_vt;
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
        if (vt_idx > 0)
          face_vt.push_back(vt_idx - 1);
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

        if (face_vt.size() == face_v.size() && !uvs.empty()) {
          tri.uv[0] = uvs[face_vt[0]];
          tri.uv[1] = uvs[face_vt[i]];
          tri.uv[2] = uvs[face_vt[i + 1]];
          tri.texture = current_texture;
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
