#pragma once

#include "Vec3.h"

// The light as an object in the WORLD. It's still a directional light (like
// the sun): it shines from its position toward the world origin.
struct Light {
  Vec3D position = {-3, 3, 3}; // gives the old default direction {1, -1, -1}

  // direction the light travels (what Shade_triangle expects)
  Vec3D direction() const { return (Vec3D{0, 0, 0} - position).normalized(); }
};
