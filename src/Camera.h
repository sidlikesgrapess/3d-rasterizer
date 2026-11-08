#pragma once

#include "Mat4.h"
#include "Vec3.h"

// The camera lives IN THE WORLD like any other object (position + rotation).
// Rendering needs everything relative to the camera, so view_matrix() is the
// camera's placement undone: it moves the whole world so the camera ends up at
// the origin looking down +z (which is what screen() in Projection.h expects).
// Blender-style orbit camera: it always looks at `target` from `distance`
// away. yaw/pitch say which side it's looking from; position is worked out
// from those by update_orbit() every frame.
struct Camera {
  Vec3D position = {0, 0, 0}; // world position (set by update_orbit)
  float yaw = -0.780f;           // rotation in xz (radians)
  float pitch = 0.700f;         // rotation in yz (radians)
  float fov = 90.0f;
  Vec3D target = {0, 0, 0}; // point the camera orbits around
  float distance = 5.0f;    // how far the camera is from the target

  // a direction relative to the camera -> the same direction in the world
  Vec3D to_world_dir(const Vec3D &dir) const {
    return (rotation_xz(yaw) * rotation_yz(pitch)).transform_dir(dir);
  }

  // place the camera `distance` behind the target, looking at it
  void update_orbit() {
    Vec3D forward = to_world_dir({0, 0, 1}); // where the camera looks
    position = target - forward * distance;
  }

  Mat4 view_matrix() const {
    // opposite of how the camera is placed: un-translate, then un-rotate
    return rotation_yz(-pitch) * rotation_xz(-yaw) *
           translation(-position.x, -position.y, -position.z);
  }
};
