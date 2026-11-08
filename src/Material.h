#pragma once

#include "Color.h"
#include <string>

// How a surface reacts to light. One per `newmtl` in the .mtl file.
//   diffuse  = metallic 0, roughness 1   (matte: no shiny spot)
//   glossy   = metallic 0, roughness low (sharp white shiny spot)
//   metallic = metallic 1, roughness low (shiny spot tinted by the color)
struct Material {
  std::string name = "default";
  Color color = col::GREEN; // used when there's no texture (or textures off)
  float metallic = 0.0f;    // 0 = plastic/cloth/skin, 1 = metal
  float roughness = 0.5f;   // 0 = mirror-sharp, 1 = matte
};
