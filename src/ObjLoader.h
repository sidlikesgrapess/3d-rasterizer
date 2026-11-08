#pragma once

#include "Mesh.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
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

// MATERIAL LOADER (.mtl): for each material, read its color and shininess
// into mesh.materials, load its map_Kd image (each image file only once)
// and remember which texture the material uses. Images are looked for in
// tex_folder first (by file name), then where the .mtl says.
inline void load_mtl(const std::string &mtlPath, Mesh &mesh,
                     std::map<std::string, int> &material_texture,
                     std::map<std::string, int> &material_id,
                     const std::string &tex_folder) {
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
      Material m;
      m.name = material;
      mesh.materials.push_back(m);
      material_id[material] = (int)mesh.materials.size() - 1;
    } else if (mesh.materials.empty()) {
      continue; // lines before the first newmtl
    } else if (type == "Kd") { // color, 0..1
      float r, g, b;
      ss >> r >> g >> b;
      mesh.materials.back().color = {(Uint8)(r * 255), (Uint8)(g * 255),
                                     (Uint8)(b * 255), 255};
    } else if (type == "Ns") { // shininess 0..1000 (Blender: (1-rough)^2*1000)
      float ns;
      ss >> ns;
      mesh.materials.back().roughness =
          1.0f - std::sqrt(std::clamp(ns / 1000.0f, 0.0f, 1.0f));
    } else if (type == "Pr") { // roughness (newer .mtl files)
      ss >> mesh.materials.back().roughness;
    } else if (type == "Pm") { // metallic (newer .mtl files)
      ss >> mesh.materials.back().metallic;
    } else if (type == "map_Kd") {
      std::string path;
      std::getline(ss >> std::ws, path); // rest of line: may contain spaces
      while (!path.empty() && (path.back() == '\r' || path.back() == ' '))
        path.pop_back();
      std::string file_name = path.substr(folder_of(path).size());
      bool absolute = path.size() > 1 && (path[1] == ':' || path[0] == '/');
      if (!tex_folder.empty() &&
          std::filesystem::exists(tex_folder + file_name))
        path = tex_folder + file_name; // e.g. Obj_load/car_tex/paint.png
      else if (!absolute)
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

// OBJECT LOADER (.obj). tex_folder: where to look for its textures first.
inline Mesh OBJLoader(std::string filePath, std::string tex_folder = "") {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filePath << std::endl;
    return {};
  }
  Mesh mesh;
  std::vector<UV> uvs;                         // all vt lines
  std::map<std::string, int> material_texture; // material name -> texture
  std::map<std::string, int> material_id;      // material name -> material
  int current_texture = -1;                    // set by usemtl
  int current_material = -1;                   // set by usemtl
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
      load_mtl(folder_of(filePath) + name, mesh, material_texture,
               material_id, tex_folder);
    } else if (type == "usemtl") {
      std::string name;
      ss >> name;
      current_texture =
          material_texture.count(name) ? material_texture[name] : -1;
      current_material = material_id.count(name) ? material_id[name] : -1;
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
        tri.material = current_material;
        mesh.tris.push_back(tri);
      }
    } else if (type == "vn") {
      float x, y, z;
      ss >> x >> y >> z;
      mesh.vert_normals.push_back({x, y, z});
    }
  }
  // triangles with no (known) material get a default one
  bool missing = false;
  for (const Triangle &tri : mesh.tris)
    missing |= (tri.material < 0);
  if (missing || mesh.materials.empty()) {
    mesh.materials.push_back(Material{});
    for (Triangle &tri : mesh.tris)
      if (tri.material < 0)
        tri.material = (int)mesh.materials.size() - 1;
  }
  return mesh;
}
