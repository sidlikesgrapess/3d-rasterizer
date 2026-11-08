#pragma once

#include "Mesh.h"
#include "ObjLoader.h"
#include "RayTracer.h"
#include "WorldTransform.h"
#include <algorithm>
#include <cfloat>
#include <filesystem>
#include <string>
#include <vector>

// One loaded .obj file: its mesh, where it sits in the world, and its BVH
struct SceneObject {
  std::string name; // file name without ".obj"
  Mesh mesh;
  WorldTransform transform;
  BVH bvh; // for ray tracing, in the mesh's own coordinates
};

// Loads every .obj in `folder` (in name order). The textures of "car.obj"
// are looked for in "car_tex/" next to it. The objects are placed side by
// side along x (0.5 apart), with the whole row centered on x = 0.
inline std::vector<SceneObject> load_folder(const std::string &folder) {
  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  if (fs::is_directory(folder))
    for (const auto &entry : fs::directory_iterator(folder))
      if (entry.path().extension() == ".obj")
        files.push_back(entry.path());
  std::sort(files.begin(), files.end());

  std::vector<SceneObject> objects;
  float cursor = 0.0f; // where the next object's left side goes
  for (const fs::path &file : files) {
    SceneObject obj;
    obj.name = file.stem().string();
    std::string tex_folder =
        (file.parent_path() / (obj.name + "_tex")).string() + "/";
    obj.mesh = OBJLoader(file.string(), tex_folder);
    if (obj.mesh.tris.empty())
      continue;

    // how wide it is along x, to put it right after the previous one
    float min_x = FLT_MAX, max_x = -FLT_MAX;
    for (const Triangle &tri : obj.mesh.tris)
      for (const Vec3D &p : tri.p) {
        min_x = std::min(min_x, p.x);
        max_x = std::max(max_x, p.x);
      }
    obj.transform.position.x = cursor - min_x;
    cursor += (max_x - min_x) + 0.5f;

    obj.bvh.build(obj.mesh.tris); // once per mesh
    objects.push_back(std::move(obj));
  }
  float row_width = std::max(0.0f, cursor - 0.5f);
  for (SceneObject &obj : objects)
    obj.transform.position.x -= row_width / 2.0f;
  return objects;
}
