#pragma once

#include "Vec3.h"
#include <algorithm>
#include <cfloat>

// The 2 kinds of light (picked in the menu)
//  - directional: like the sun. Infinitely far away, so every point gets
//    light from the same direction (position -> target) at the same
//    strength, and nothing is "behind" it.
//  - point: like a light bulb at `position`. Each point gets light from
//    the bulb's direction, weaker the further away it is (strength / d^2),
//    and a shadow only counts if the blocker is between the point and bulb.
enum LightType { LIGHT_DIRECTIONAL, LIGHT_POINT };

struct Light {
  int type = LIGHT_DIRECTIONAL;
  Vec3D position = {-3, 3, 3}; // gives the old default direction {1, -1, -1}
  Vec3D target = {0, 0, 0};    // directional: shines from position to here
  float strength = 30.0f;      // point: 30 = full light 5.5 units away

  // directional: the way the light travels (what Light_terms expects)
  Vec3D direction() const { return (target - position).normalized(); }

  // the way the light travels when it arrives at `point`
  Vec3D direction_at(const Vec3D &point) const {
    if (type == LIGHT_POINT)
      return (point - position).normalized();
    return direction();
  }

  // how strong the light is at `point` (1 = like the sun)
  float strength_at(const Vec3D &point) const {
    if (type == LIGHT_POINT) {
      Vec3D d = point - position;
      return strength / std::max(d.dot(d), 1e-4f); // falls off with d^2
    }
    return 1.0f;
  }

  // how far a shadow ray from `point` has to go to reach the light
  float distance_to(const Vec3D &point) const {
    if (type == LIGHT_POINT)
      return (position - point).length();
    return FLT_MAX; // the sun is infinitely far away
  }
};
