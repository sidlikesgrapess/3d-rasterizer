#pragma once

#include "../headers/stb/stb_image.h" // PNG/JPG loading (implementation in main.cpp)
#include "Color.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// An image the mesh is painted with (from map_Kd in the .mtl file)
struct Texture {
  int width = 0, height = 0;
  std::vector<Color> pixels; // row by row, top row first

  // u, v from the .obj (0..1). Nearest pixel, wraps outside 0..1.
  Color sample(float u, float v) const {
    u -= std::floor(u);
    v -= std::floor(v);
    int x = std::min((int)(u * width), width - 1);
    int y = std::min((int)((1.0f - v) * height), height - 1); // obj v=0 = bottom
    return pixels[y * width + x];
  }
};

inline bool load_texture(const std::string &path, Texture &tex) {
  int channels;
  unsigned char *data =
      stbi_load(path.c_str(), &tex.width, &tex.height, &channels, 4); // RGBA
  if (!data) {
    std::cerr << "Error loading texture: " << path << std::endl;
    return false;
  }
  tex.pixels.resize(tex.width * tex.height);
  for (int i = 0; i < tex.width * tex.height; ++i)
    tex.pixels[i] = {data[i * 4], data[i * 4 + 1], data[i * 4 + 2], 255};
  stbi_image_free(data);
  return true;
}
