#pragma once

#include "Mat4.h"
#include "Vec3.h"

// Where an object sits in the WORLD (not relative to the camera).
// matrix() takes the object's own mesh coordinates into world coordinates.
struct WorldTransform {
  Vec3D position = {0, 0, 0}; // world position
  float rot_xz = 0.0f;        // rotations in radians
  float rot_yz = 0.0f;
  float rot_xy = 0.0f;

  Mat4 matrix() const {
    // read right to left: rotate xz -> yz -> xy, then move to world position
    return translation(position.x, position.y, position.z) *
           rotation_xy(rot_xy) * rotation_yz(rot_yz) * rotation_xz(rot_xz);
  }
};
