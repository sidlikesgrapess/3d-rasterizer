#pragma once

#include "Color.h"
#include "Config.h"
#include "Display.h"
#include "Mesh.h"
#include "Parallel.h"
#include "Projection.h"
#include "Vec3.h"
#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <cstdlib>
#include <vector>

// Owns the pixel + depth buffers and everything that writes into them
// (lines, scanlines, triangle fill) plus presenting the frame to SDL.
// It can be smaller than the window (render scale): SDL stretches it to fit.
struct Framebuffer {
  int width = screenW;
  int height = screenH;
  std::vector<uint32_t> screen_buffer = std::vector<uint32_t>(
      screenW * screenH); // y *width+x = index of pixel
  std::vector<float> z_buffer = std::vector<float>(screenW * screenH, FLT_MAX);

  // Gouraud/Phong only: which triangle won each pixel (-1 = none) and its
  // weights there (w2 = 1 - w0 - w1). Shading happens once at the end.
  std::vector<int> tri_id = std::vector<int>(screenW * screenH, -1);
  std::vector<float> w0_buffer = std::vector<float>(screenW * screenH);
  std::vector<float> w1_buffer = std::vector<float>(screenW * screenH);

  // Antialiasing (supersampling): the buffers above are drawn aa times wider
  // and taller, then every aa x aa block is averaged into one shown pixel.
  int aa = 1;
  std::vector<uint32_t> aa_buffer; // the averaged (shown) image

  // change the render size (render scale in the menu)
  void Resize(int w, int h) {
    width = w;
    height = h;
    screen_buffer.assign(w * h, 0);
    z_buffer.assign(w * h, FLT_MAX);
    tri_id.assign(w * h, -1);
    w0_buffer.assign(w * h, 0.0f);
    w1_buffer.assign(w * h, 0.0f);
  }

  void DrawPoint(Vec3D point, Color color, float fovDegrees = 90.0f) {
    Px2D screenPos = screen(point, fovDegrees, width, height);
    if (screenPos.x >= 0 && screenPos.y >= 0 && screenPos.x < width &&
        screenPos.y < height) {
      screen_buffer[screenPos.y * width + screenPos.x] = color.to_hex();
    }
  }
  //---------------------------------------------------------------------

  // ALL DRAW FUNCTIONS
  void Draw() { // once per frame
    int out_w = width / aa, out_h = height / aa;
    const uint32_t *pixels = screen_buffer.data();
    if (aa > 1) {
      aa_buffer.resize(out_w * out_h);
      parallel_for(out_h, [&](int y) {
        for (int x = 0; x < out_w; ++x) {
          // add up each color channel (byte) of the aa x aa block
          uint32_t sum[4] = {0, 0, 0, 0};
          for (int sy = 0; sy < aa; ++sy)
            for (int sx = 0; sx < aa; ++sx) {
              uint32_t p = screen_buffer[(y * aa + sy) * width + x * aa + sx];
              for (int c = 0; c < 4; ++c)
                sum[c] += (p >> (c * 8)) & 0xFF;
            }
          uint32_t out = 0;
          for (int c = 0; c < 4; ++c)
            out |= (sum[c] / (aa * aa)) << (c * 8); // average
          aa_buffer[y * out_w + x] = out;
        }
      });
      pixels = aa_buffer.data();
    }

    // the texture must match the shown size: recreate it if the size changed
    int tw = 0, th = 0;
    SDL_QueryTexture(frameTexture, nullptr, nullptr, &tw, &th);
    if (tw != out_w || th != out_h) {
      SDL_DestroyTexture(frameTexture);
      frameTexture =
          SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                            SDL_TEXTUREACCESS_STREAMING, out_w, out_h);
    }
    SDL_UpdateTexture(frameTexture, nullptr, pixels, out_w * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, frameTexture, nullptr, nullptr); // stretches
    SDL_RenderPresent(renderer);
  }

  void DrawLine(float x0_f, float y0_f, float x1_f, float y1_f,
                Color color) { // asssumes coords are screen coords
    int x0 = (int)x0_f;        // Bresenham's algo for lines in int
    int y0 = (int)y0_f;
    int x1 = (int)x1_f;
    int y1 = (int)y1_f;
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    uint32_t hexColor = color.to_hex();
    while (true) {
      // Bounds check
      if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
        screen_buffer[y0 * width + x0] = hexColor;
      }
      if (x0 == x1 && y0 == y1)
        break;
      int e2 = 2 * err;
      if (e2 > -dy) {
        err -= dy;
        x0 += sx;
      }
      if (e2 < dx) {
        err += dx;
        y0 += sy;
      }
    }
  }

  void DrawHorizontalLine(int x1, int x2, int y, float z1, float z2,
                          Color color) { // horizontal scanline with z
    if (y < 0 || y >= height)
      return;
    if (x1 > x2) {
      std::swap(x1, x2);
      std::swap(z1, z2);
    }

    int origX1 = x1;
    int origX2 = x2;

    int startX = std::max(0, x1);        // outside screen
    int endX = std::min(width - 1, x2); // outside screen

    uint32_t hexColor = color.to_hex();
    int rowOffset = y * width;
    float dx = (float)(origX2 - origX1);

    for (int x = startX; x <= endX; ++x) {
      float t =
          (dx != 0.0f) ? (float)(x - origX1) / dx : 0.0f; // fuck this % line
      float z_current = z1 + t * (z2 - z1);

      int index = rowOffset + x;
      if (z_current < z_buffer[index]) {
        z_buffer[index] = z_current;
        screen_buffer[index] = hexColor;
      }
    }
  }

  void Clear(Color color) {
    std::fill(screen_buffer.begin(), screen_buffer.end(), color.to_hex());
    std::fill(z_buffer.begin(), z_buffer.end(), FLT_MAX);
  }

  // only needed for Gouraud/Phong, so it's separate from Clear()
  void ClearTriangleIds() { std::fill(tri_id.begin(), tri_id.end(), -1); }

  void Draw_triangle(const Triangle &tri, Color color,
                     float fovDegrees = 90.0f) {
    Px2D p0 = screen(tri.p[0], fovDegrees, width, height);
    Px2D p1 = screen(tri.p[1], fovDegrees, width, height);
    Px2D p2 = screen(tri.p[2], fovDegrees, width, height);

    DrawLine(p0.x, p0.y, p1.x, p1.y, color);
    DrawLine(p1.x, p1.y, p2.x, p2.y, color);
    DrawLine(p2.x, p2.y, p0.x, p0.y, color);
  }

  // y0..y1-1 = the rows this call may draw (a thread's strip of the screen)
  void Fill_flat_bottom_triangle(Px2D p0, Px2D p1, Px2D p2, Color color,
                                 int y0, int y1) {
    if (p1.y == p0.y)
      return;
    float invslope1 = (float)(p1.x - p0.x) / (float)(p1.y - p0.y);
    float invslope2 = (float)(p2.x - p0.x) / (float)(p2.y - p0.y);

    int startY = std::max(p0.y, y0);   // only rows inside the strip
    int endY = std::min(p1.y, y1 - 1);
    float totalDy = (float)(p1.y - p0.y);

    for (int y = startY; y <= endY; y++) {
      float dy = (float)(y - p0.y);
      float t = (totalDy != 0.0f) ? dy / totalDy : 0.0f; // fuck this too
      float x1 = (float)p0.x + dy * invslope1;
      float x2 = (float)p0.x + dy * invslope2;
      float z1 = p0.z + t * (p1.z - p0.z);
      float z2 = p0.z + t * (p2.z - p0.z);
      DrawHorizontalLine((int)x1, (int)x2, y, z1, z2, color);
    }
  }

  void Fill_flat_top_triangle(Px2D p0, Px2D p1, Px2D p2, Color color, int y0,
                              int y1) {
    if (p2.y == p0.y)
      return;
    float invslope1 = (float)(p2.x - p0.x) / (float)(p2.y - p0.y); // slope
    float invslope2 = (float)(p2.x - p1.x) / (float)(p2.y - p1.y);

    int startY = std::max(p0.y, y0);   // only rows inside the strip
    int endY = std::min(p2.y, y1 - 1);
    float totalDy = (float)(p2.y - p0.y);

    for (int y = startY; y <= endY; y++) {
      float dy = (float)(y - p2.y);
      float t =
          (totalDy != 0.0f) ? (float)(y - p0.y) / totalDy : 0.0f; // fuck this
      float x1 = (float)p2.x + dy * invslope1; // eqn of line along slope
      float x2 = (float)p2.x + dy * invslope2;
      float z1 = p0.z + t * (p2.z - p0.z);
      float z2 = p1.z + t * (p2.z - p1.z);
      DrawHorizontalLine((int)x1, (int)x2, y, z1, z2, color);
    }
  }

  // Used by Gouraud and Phong. Visits every pixel inside the triangle and,
  // where it's closest so far, remembers the triangle (id) and the pixel's
  // weights (w0, w1, w2: how close it is to each corner, they add up to 1).
  // No color is computed here: the engine shades each pixel ONCE afterwards,
  // so pixels that get covered by a closer triangle cost nothing extra.
  // p0..p2 = corners already projected with screen().
  // y0..y1-1 = the rows this call may draw (a thread's strip of the screen).
  void Fill_triangle_deferred(Px2D p0, Px2D p1, Px2D p2, int id, int y0,
                              int y1) {
    // edge(a, b, x, y): which side of the line a->b the point (x, y) is on
    auto edge = [](Px2D a, Px2D b, float x, float y) {
      return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
    };
    float area = edge(p0, p1, (float)p2.x, (float)p2.y);
    if (area == 0.0f)
      return; // triangle is a line on screen
    float sign = (area > 0) ? 1.0f : -1.0f; // works for either winding

    // The mesh has tiny cracks between triangles. Like the scanline fill,
    // draw half a pixel past each edge so they get covered.
    // edge value / edge length = distance from that edge in pixels.
    auto len = [](Px2D a, Px2D b) {
      return std::sqrt((float)((b.x - a.x) * (b.x - a.x) +
                               (b.y - a.y) * (b.y - a.y)));
    };
    float pad0 = 0.5f * len(p1, p2);
    float pad1 = 0.5f * len(p2, p0);
    float pad2 = 0.5f * len(p0, p1);

    // only check pixels in the triangle's bounding box (clipped to screen).
    // Pixels outside the box are >= 1 px from the triangle, so the half-pixel
    // padding never reaches them.
    int minX = std::max(0, std::min({p0.x, p1.x, p2.x}));
    int maxX = std::min(width - 1, std::max({p0.x, p1.x, p2.x}));
    int minY = std::max({0, y0, std::min({p0.y, p1.y, p2.y})});
    int maxY = std::min({height - 1, y1 - 1, std::max({p0.y, p1.y, p2.y})});

    for (int y = minY; y <= maxY; ++y) {
      for (int x = minX; x <= maxX; ++x) {
        float e0 = edge(p1, p2, (float)x, (float)y) * sign;
        float e1 = edge(p2, p0, (float)x, (float)y) * sign;
        float e2 = edge(p0, p1, (float)x, (float)y) * sign;
        if (e0 < -pad0 || e1 < -pad1 || e2 < -pad2)
          continue; // more than half a pixel outside the triangle

        // weights: negative ones (pixel in the padding) count as 0
        float w0 = std::max(e0, 0.0f);
        float w1 = std::max(e1, 0.0f);
        float w2 = std::max(e2, 0.0f);
        float sum = w0 + w1 + w2;
        if (sum == 0.0f)
          continue;
        w0 /= sum;
        w1 /= sum;
        w2 /= sum;

        float z = w0 * p0.z + w1 * p1.z + w2 * p2.z;
        int index = y * width + x;
        if (z < z_buffer[index]) {
          z_buffer[index] = z;
          tri_id[index] = id;
          w0_buffer[index] = w0;
          w1_buffer[index] = w1;
        }
      }
    }
  }

  // p0..p2 = corners already projected with screen().
  // y0..y1-1 = the rows this call may draw (a thread's strip of the screen).
  void Fill_triangle(Px2D p0, Px2D p1, Px2D p2, Color color, int y0, int y1) {
    if (p0.y > p1.y)
      std::swap(p0, p1);
    if (p0.y > p2.y)
      std::swap(p0, p2);
    if (p1.y > p2.y)
      std::swap(p1, p2);

    if (p0.y == p2.y)
      return;

    if (p1.y == p2.y) {
      Fill_flat_bottom_triangle(p0, p1, p2, color, y0, y1);
    } else if (p0.y == p1.y) {
      Fill_flat_top_triangle(p0, p1, p2, color, y0, y1);
    } else {
      float t = (float)(p1.y - p0.y) / (float)(p2.y - p0.y);
      int p3_x = p0.x + (int)(t * (float)(p2.x - p0.x));
      float p3_z = p0.z + t * (p2.z - p0.z);
      Px2D p3 = {p3_x, p1.y, p3_z};
      Fill_flat_bottom_triangle(p0, p1, p3, color, y0, y1);
      Fill_flat_top_triangle(p1, p3, p2, color, y0, y1);
    }
  }
};
