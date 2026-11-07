#pragma once

#include "Camera.h"
#include "Color.h"
#include "Config.h"
#include "Framebuffer.h"
#include "Light.h"
#include "Lines.h"
#include "Mat4.h"
#include "Mesh.h"
#include "ObjLoader.h"
#include "SDLHeaders.h"
#include "Shading.h"
#include "Transform.h"
#include "Vec3.h"
#include "WorldTransform.h"
#include <algorithm>
#include <cstdint>
#include <vector>

class Engine3D {
public:
  Engine3D() {}
#pragma region vars
  SDL_Window *window = nullptr; // main window, so input from the menu is ignored
  Framebuffer fb; // screen_buffer + z_buffer + all drawing functions
  Mesh mesh;
  std::vector<Triangle> transformed_tri; // kept between frames (no realloc)
  // Gouraud/Phong: per-triangle corner data, 3 entries per triangle
  std::vector<float> corner_light;   // Gouraud: light amount at each corner
  std::vector<Vec3D> corner_normals; // normal at each corner
  std::vector<char> corners_ready;   // 1 per triangle: corner data computed?
  float render_scale = 1.0f; // 1 = full resolution, 0.5 = half (menu)
  WorldTransform object; // where the mesh sits in the world
  Camera camera;         // where the camera sits in the world (+ fov)
  std::vector<Line> grid; // floor, fixed in the world (not in the menu)
  std::vector<Line> axes; // x/y/z axes, fixed in the world (not in the menu)
  float specularity = 0.1f;
  float exposure = 1.0f;
  Light light; // world position; shines toward the world origin
  int shading = SHADING_FLAT; // SHADING_FLAT / _GOURAUD / _PHONG (Shading.h)
  bool calculateNormals = true; // flat only
  bool backface_cull = true;
  bool use_textures = true; // off = plain green
  Px2D mousepos;
#pragma endregion

  bool OnUserCreate() {
    // mesh = OBJLoader("Object/bmw.obj"); // no textures
    mesh = OBJLoader("Object/full_model.obj"); // textured (full_model.mtl)
    object.position = {0, 0, 0}; // was z_offset = 2
    camera.position = {-2, 3, -2}; // above the floor, so the grid isn't edge-on
    grid = make_grid(10);
    axes = make_axes(10.0f);
    return true;
  }

  // No keyboard controls: everything is changed from the menu window.
  bool OnUserUpdate(float fElapsedTime) {
    // render scale: draw into a smaller buffer, SDL stretches it to the window
    int w = std::max(1, (int)(screenW * render_scale));
    int h = std::max(1, (int)(screenH * render_scale));
    if (w != fb.width || h != fb.height)
      fb.Resize(w, h);

    fb.Clear(col::GREY);
#pragma region Mouse input
    // Left mouse (hold + drag) in the main window: set light direction
    if (SDL_GetMouseFocus() == window && // clicks in the menu don't move light
        (SDL_GetMouseState(&mousepos.x, &mousepos.y) &
         SDL_BUTTON(SDL_BUTTON_LEFT))) {
      // mouse picks a direction as seen from the camera...
      Vec3D seen = {-(((float)mousepos.x / screenW) * 4.0f - 2.0f),
                    (((float)mousepos.y / screenH) * 4.0f - 2.0f), 1.0f};
      Vec3D dir = camera.to_world_dir(seen).normalized();
      // move the light (same distance from origin) so it shines along dir
      light.position = dir * -light.position.length();
    }

#pragma endregion

#pragma region rendering
    // reuse last frame's memory instead of allocating ~29 MB every frame
    transformed_tri.clear();
    transformed_tri.reserve(mesh.tris.size());

    // model: mesh coords -> world.  view: world -> relative to camera.
    // Combined, read right to left: model first, then view.
    Mat4 model = object.matrix();
    Mat4 view = camera.view_matrix();
    Mat4 model_view = view * model;
    // world -> camera, like the mesh
    Vec3D light_view = view.transform_dir(light.direction());

    // floor + axes first. Lines don't use the z-buffer, so the car is always
    // drawn on top of them.
    draw_lines(fb, grid, view, camera.fov);
    draw_lines(fb, axes, view, camera.fov);

    // 1. Transform mesh
    for (const auto &tri : mesh.tris) {
      transformed_tri.push_back(transform_triangle(tri, model_view));
    }

    // 2. painters algo(slow)
    // Painters_algo(transformed_tri);

    // 3. Render
    if (shading != SHADING_FLAT) {
      fb.ClearTriangleIds();
      corner_light.resize(transformed_tri.size() * 3);
      corner_normals.resize(transformed_tri.size() * 3);
      corners_ready.assign(transformed_tri.size(), 0);
    }

    for (size_t id = 0; id < transformed_tri.size(); ++id) {
      const Triangle &tri = transformed_tri[id];
      if (isTriangleCulled(tri, 0.1f, backface_cull))
        continue;

      if (shading == SHADING_FLAT) {
        // one color for the whole triangle (texture: read at its center)
        Color base = col::GREEN;
        if (use_textures && tri.texture >= 0)
          base = mesh.textures[tri.texture].sample(
              (tri.uv[0].u + tri.uv[1].u + tri.uv[2].u) / 3.0f,
              (tri.uv[0].v + tri.uv[1].v + tri.uv[2].v) / 3.0f);
        Color shaded =
            Shade_triangle(tri, light_view, base, calculateNormals,
                           specularity, exposure); // 2nd sector = -1 -1
        fb.Fill_triangle(tri, shaded, camera.fov);

      } else {
        // only depth + which triangle won each pixel; colors come below
        fb.Fill_triangle_deferred(tri, (int)id, camera.fov);
      }
      // fb.Draw_triangle(tri, col::WHITE, camera.fov);
    }

    // 4. Gouraud/Phong: color every pixel ONCE, from the triangle that won it
    if (shading != SHADING_FLAT) {
      for (int i = 0; i < fb.width * fb.height; ++i) {
        int id = fb.tri_id[i];
        if (id < 0)
          continue; // no triangle here (background, lines)
        float w0 = fb.w0_buffer[i];
        float w1 = fb.w1_buffer[i];
        float w2 = 1.0f - w0 - w1;

        // corner data, worked out the first time this triangle is needed
        // (most triangles are hidden or too small to win any pixel)
        const Triangle &tri = transformed_tri[id];
        if (!corners_ready[id]) {
          for (int k = 0; k < 3; ++k) {
            corner_normals[id * 3 + k] = vertex_normal(tri, k);
            if (shading == SHADING_GOURAUD)
              corner_light[id * 3 + k] =
                  Light_intensity(corner_normals[id * 3 + k], light_view);
          }
          corners_ready[id] = 1;
        }

        // surface color: green, or the texture at this pixel
        Color base = col::GREEN;
        if (use_textures && tri.texture >= 0) {
          // Perspective correction: w0/w1/w2 are even steps on the SCREEN,
          // but far parts of a triangle are squeezed. Dividing each weight
          // by its corner's depth (and rescaling to add up to 1) undoes that,
          // otherwise the texture bends and slides on big triangles.
          float p0 = w0 / tri.p[0].z;
          float p1 = w1 / tri.p[1].z;
          float p2 = w2 / tri.p[2].z;
          float sum = p0 + p1 + p2;
          p0 /= sum;
          p1 /= sum;
          p2 /= sum;
          float u = p0 * tri.uv[0].u + p1 * tri.uv[1].u + p2 * tri.uv[2].u;
          float v = p0 * tri.uv[0].v + p1 * tri.uv[1].v + p2 * tri.uv[2].v;
          base = mesh.textures[tri.texture].sample(u, v);
        }

        Color color;
        if (shading == SHADING_GOURAUD) {
          // blend the 3 corner light amounts, then color this pixel
          const float *L = &corner_light[id * 3];
          float light_amount = L[0] * w0 + L[1] * w1 + L[2] * w2;
          color = Shade_intensity(light_amount, base, specularity, exposure);
        } else {
          // Phong: blend the 3 corner normals, then light this pixel
          const Vec3D *n = &corner_normals[id * 3];
          Vec3D normal = (n[0] * w0 + n[1] * w1 + n[2] * w2).normalized();
          color = Shade_normal(normal, light_view, base, specularity, exposure);
        }
        fb.screen_buffer[i] = color.to_hex();
      }
    }

    // light marker last, so it's always visible
    draw_circle(fb, light.position, view, camera.fov, 10.0f * render_scale,
                col::YELLOW);

    fb.Draw();
#pragma endregion

    return true;
  }
};
