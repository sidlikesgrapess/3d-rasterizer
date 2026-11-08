#pragma once

#include "Mesh.h"
#include "Vec3.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <numeric>
#include <vector>

// SIMPLE RAY TRACING
// A ray = a start point + a direction. We find the closest triangle it hits.

struct Ray {
  Vec3D origin;
  Vec3D dir;
};

struct Hit {
  float t = FLT_MAX; // distance along the ray
  int tri = -1;      // which triangle (index into mesh.tris)
  float u = 0, v = 0; // weights of corners 1 and 2 (corner 0 = 1 - u - v)
};

// Ray vs one triangle (Moller-Trumbore). On a hit, gives the distance t and
// the weights u, v of where on the triangle it hit.
inline bool ray_triangle(const Ray &ray, const Triangle &tri, float &t,
                         float &u, float &v) {
  Vec3D e1 = tri.p[1] - tri.p[0];
  Vec3D e2 = tri.p[2] - tri.p[0];
  Vec3D pv = ray.dir.cross(e2);
  float det = e1.dot(pv);
  if (std::fabs(det) < 1e-20f)
    return false; // ray is parallel to the triangle
  float inv_det = 1.0f / det;
  Vec3D tv = ray.origin - tri.p[0];
  u = tv.dot(pv) * inv_det;
  if (u < 0.0f || u > 1.0f)
    return false;
  Vec3D qv = tv.cross(e1);
  v = ray.dir.dot(qv) * inv_det;
  if (v < 0.0f || u + v > 1.0f)
    return false;
  t = e2.dot(qv) * inv_det;
  return t > 1e-4f; // in front of the start point
}

// Ray vs a box: does the ray pass through it closer than max_t?
inline bool ray_box(const Vec3D &origin, const Vec3D &inv_dir,
                    const Vec3D &bmin, const Vec3D &bmax, float max_t) {
  float tx1 = (bmin.x - origin.x) * inv_dir.x;
  float tx2 = (bmax.x - origin.x) * inv_dir.x;
  float tmin = std::min(tx1, tx2), tmax = std::max(tx1, tx2);
  float ty1 = (bmin.y - origin.y) * inv_dir.y;
  float ty2 = (bmax.y - origin.y) * inv_dir.y;
  tmin = std::max(tmin, std::min(ty1, ty2));
  tmax = std::min(tmax, std::max(ty1, ty2));
  float tz1 = (bmin.z - origin.z) * inv_dir.z;
  float tz2 = (bmax.z - origin.z) * inv_dir.z;
  tmin = std::max(tmin, std::min(tz1, tz2));
  tmax = std::min(tmax, std::max(tz1, tz2));
  return tmax >= std::max(tmin, 0.0f) && tmin < max_t;
}

// BVH (bounding volume hierarchy): a tree of boxes around the triangles.
// A ray skips any box it misses, so it only tests a few dozen triangles
// instead of all of them. Built once, in the mesh's own coordinates.
struct BVHNode {
  Vec3D min, max;      // the box
  int left = -1, right = -1; // child nodes (if not a leaf)
  int first = 0, count = 0;  // leaf: tri_index[first .. first+count)
};

struct BVH {
  std::vector<BVHNode> nodes;
  std::vector<int> tri_index; // triangle indices, grouped by leaf

  void build(const std::vector<Triangle> &tris) {
    nodes.clear();
    tri_index.resize(tris.size());
    std::iota(tri_index.begin(), tri_index.end(), 0); // 0, 1, 2, ...
    if (tris.empty())
      return;
    nodes.reserve(2 * tris.size());
    build_node(tris, 0, (int)tris.size());
  }

  // closest hit (any_hit = false) or just "is anything in the way" (true)
  bool intersect(const std::vector<Triangle> &tris, const Ray &ray, Hit &hit,
                 bool any_hit) const {
    if (nodes.empty())
      return false;
    Vec3D inv_dir = {1.0f / ray.dir.x, 1.0f / ray.dir.y, 1.0f / ray.dir.z};
    bool found = false;
    int stack[64]; // nodes still to visit
    int sp = 0;
    stack[sp++] = 0;
    while (sp > 0) {
      const BVHNode &node = nodes[stack[--sp]];
      if (!ray_box(ray.origin, inv_dir, node.min, node.max, hit.t))
        continue; // missed this box: skip everything inside it
      if (node.count > 0) { // leaf: test its triangles
        for (int i = node.first; i < node.first + node.count; ++i) {
          float t, u, v;
          int id = tri_index[i];
          if (ray_triangle(ray, tris[id], t, u, v) && t < hit.t) {
            hit = {t, id, u, v};
            found = true;
            if (any_hit)
              return true;
          }
        }
      } else {
        stack[sp++] = node.left;
        stack[sp++] = node.right;
      }
    }
    return found;
  }

private:
  static float axis_value(const Vec3D &p, int axis) {
    return axis == 0 ? p.x : (axis == 1 ? p.y : p.z);
  }
  static Vec3D centroid(const Triangle &t) {
    return (t.p[0] + t.p[1] + t.p[2]) / 3.0f;
  }

  // Box around triangles first..first+count, then split them in half along
  // the box's longest side, until a node has 4 or fewer triangles.
  int build_node(const std::vector<Triangle> &tris, int first, int count) {
    BVHNode node;
    node.min = {FLT_MAX, FLT_MAX, FLT_MAX};
    node.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (int i = first; i < first + count; ++i)
      for (const Vec3D &p : tris[tri_index[i]].p) {
        node.min = {std::min(node.min.x, p.x), std::min(node.min.y, p.y),
                    std::min(node.min.z, p.z)};
        node.max = {std::max(node.max.x, p.x), std::max(node.max.y, p.y),
                    std::max(node.max.z, p.z)};
      }
    int index = (int)nodes.size();
    nodes.push_back(node);

    if (count <= 4) { // small enough: leaf
      nodes[index].first = first;
      nodes[index].count = count;
      return index;
    }

    Vec3D size = node.max - node.min;
    int axis = (size.x > size.y && size.x > size.z) ? 0
               : (size.y > size.z)                  ? 1
                                                    : 2;
    int mid = first + count / 2;
    // put the half with the smaller centers first (no full sort needed)
    std::nth_element(tri_index.begin() + first, tri_index.begin() + mid,
                     tri_index.begin() + first + count, [&](int a, int b) {
                       return axis_value(centroid(tris[a]), axis) <
                              axis_value(centroid(tris[b]), axis);
                     });
    int left = build_node(tris, first, mid - first);
    int right = build_node(tris, mid, first + count - mid);
    nodes[index].left = left;
    nodes[index].right = right;
    return index;
  }
};
