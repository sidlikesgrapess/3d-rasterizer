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
#include "Parallel.h"
#include "RayTracer.h"
#include "SDLHeaders.h"
#include "SceneObject.h"
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
  // every .obj in Obj_load/ (mesh + world transform + BVH each)
  std::vector<SceneObject> objects;
  // All objects' triangles counted as one list: triangle `id` is
  // objects[tri_refs[id].object].mesh.tris[tri_refs[id].index]
  struct TriRef {
    int object, index;
  };
  std::vector<TriRef> tri_refs;
  // per object, set every frame: mesh -> world, world -> mesh, mesh -> camera
  std::vector<Mat4> models, to_models, model_views;
  // Per-triangle data, rebuilt every frame (memory kept between frames).
  // Filled in parallel in step 1, then only read by the other steps.
  std::vector<Triangle> transformed_tri; // triangle relative to the camera
  std::vector<char> visible;             // 0 = culled
  std::vector<Px2D> projected;           // 3 screen corners per triangle
  std::vector<Color> flat_colors;        // flat: the triangle's color
  std::vector<LightTerms> corner_terms; // Gouraud: light at each corner
  std::vector<Vec3D> corner_normals; // Gouraud/Phong: normal at each corner
  float render_scale = 1.0f; // 1 = full resolution, 0.5 = half (menu)
  bool antialias = false; // on = 2x2 samples per pixel (menu)
  Camera camera;         // where the camera sits in the world (+ fov)
  std::vector<Line> grid; // floor, fixed in the world (not in the menu)
  std::vector<Line> axes; // x/y/z axes, fixed in the world (not in the menu)
  float exposure = 1.0f;
  float exposure_scale = 1.0f; // log10(exposure * 10 + 1), set every frame
  Light light; // directional or point (Light.h), in world coordinates
  int shading = SHADING_FLAT; // SHADING_FLAT / _GOURAUD / _PHONG (Shading.h)
  bool calculateNormals = true; // flat only
  bool backface_cull = true;
  bool use_textures = true; // off = plain green
  bool raytrace = false;    // on = ray traced image (shading modes unused)
  Px2D mousepos;
#pragma endregion

  bool OnUserCreate() {
    // every .obj in Obj_load/, textures in Obj_load/<name>_tex/
    objects = load_folder("Obj_load");
    for (int o = 0; o < (int)objects.size(); ++o)
      for (int i = 0; i < (int)objects[o].mesh.tris.size(); ++i)
        tri_refs.push_back({o, i});
    // orbit camera: with the default yaw/pitch this puts it at ~(-2, 3, -2)
    camera.target = {0, 0.62f, 0};
    camera.distance = 3.7f;
    grid = make_grid(10);
    axes = make_axes(10.0f);
    return true;
  }

  // Called by main() for every SDL event. Blender-style camera, main window
  // only: middle-drag = orbit, Shift + middle-drag = pan, wheel = zoom.
  void HandleEvent(const SDL_Event &e) {
    Uint32 main_id = SDL_GetWindowID(window);

    if (e.type == SDL_MOUSEMOTION && e.motion.windowID == main_id &&
        (e.motion.state & SDL_BUTTON_MMASK)) {
      if (SDL_GetModState() & KMOD_SHIFT) {
        // pan: slide the target along the camera's right/up axes.
        // px = world units per pixel at the target, so the scene follows
        // the mouse at any zoom.
        float px = 2.0f * camera.distance *
                   std::tan(camera.fov * 0.5f * 3.14159265f / 180.0f) /
                   screenH;
        Vec3D right = camera.to_world_dir({1, 0, 0});
        Vec3D up = camera.to_world_dir({0, 1, 0});
        camera.target = camera.target - right * (e.motion.xrel * px) +
                        up * (e.motion.yrel * px);
      } else {
        // orbit: left/right turns around, up/down tilts
        camera.yaw -= e.motion.xrel * 0.01f;
        camera.pitch += e.motion.yrel * 0.01f;
        camera.pitch = std::clamp(camera.pitch, -1.55f, 1.55f); // no flipping
      }
    } else if (e.type == SDL_MOUSEWHEEL && e.wheel.windowID == main_id) {
      // zoom: multiply so it feels the same near and far
      if (e.wheel.y > 0)
        camera.distance *= 0.9f;
      else if (e.wheel.y < 0)
        camera.distance *= 1.1f;
    }
  }

  // Surface color at texture coordinate (u, v): the texture, or the
  // material's color if there's no texture (or textures are off)
  Color base_color(const Mesh &mesh, const Triangle &tri, float u,
                   float v) const {
    if (use_textures && tri.texture >= 0)
      return mesh.textures[tri.texture].sample(u, v);
    return mesh.materials[tri.material].color;
  }

  // RAY TRACING (used instead of filling triangles when `raytrace` is on).
  // Rays are in world coordinates.

  // Closest hit over all objects (any_hit: stop at the first one). Each
  // BVH is in its object's own coordinates, so the ray is moved into each.
  // Moving/rotating doesn't change lengths, so hit.t can be compared
  // between objects (the BVH only accepts hits closer than hit.t).
  bool intersect_scene(const Ray &ray, Hit &hit, int &object,
                       bool any_hit) const {
    bool found = false;
    for (int o = 0; o < (int)objects.size(); ++o) {
      Ray local = {to_models[o].transform_point(ray.origin),
                   to_models[o].transform_dir(ray.dir)};
      if (objects[o].bvh.intersect(objects[o].mesh.tris, local, hit,
                                   any_hit)) {
        found = true;
        object = o;
        if (any_hit)
          return true;
      }
    }
    return found;
  }

  // Shadow ray from `point` toward the light: is anything in the way?
  // It starts a tiny bit off the surface so it doesn't hit the triangle it
  // starts on, and only counts blockers closer than the light (a point
  // light's shadow ray stops at the bulb).
  bool in_shadow(const Vec3D &point) const {
    Vec3D to_light = light.direction_at(point) * -1.0f;
    Ray shadow = {point + to_light * 1e-3f, to_light};
    Hit blocker;
    blocker.t = light.distance_to(point) - 1e-3f; // only hits before this
    int blocker_object;
    return intersect_scene(shadow, blocker, blocker_object, true);
  }

  // Color seen along one ray, into `out`.
  // Returns false if the ray misses everything (out is left as it was).
  // On a hit: light the closest hit point, with a shadow ray toward the
  // light (if something is in the way, it's in shadow). Smooth materials
  // also send a reflection ray, which calls trace() again (bounces = depth).
  bool trace(const Ray &ray, int depth, Color &out) const {
    Hit hit;
    int o = -1;
    if (!intersect_scene(ray, hit, o, false))
      return false;
    const Mesh &mesh = objects[o].mesh;
    const Triangle &tri = mesh.tris[hit.tri];
    const Material &mat = mesh.materials[tri.material];
    float w0 = 1.0f - hit.u - hit.v, w1 = hit.u, w2 = hit.v;
    // smooth normal, like Phong (in the mesh's coordinates -> world)
    Vec3D normal = models[o]
                       .transform_dir(vertex_normal(tri, 0) * w0 +
                                      vertex_normal(tri, 1) * w1 +
                                      vertex_normal(tri, 2) * w2)
                       .normalized();
    Vec3D point = ray.origin + ray.dir * hit.t;

    LightTerms terms =
        Light_terms(normal, light.direction_at(point), ray.dir, mat.roughness,
                    light.strength_at(point));
    if (in_shadow(point)) {
      terms.diffuse = 0.15f; // only the ambient light
      terms.highlight = 0.0f;
    }

    // reflection: bounce the ray off the surface like a mirror. Only for
    // smooth materials (rough ones would need many rays to look blurry;
    // they reflect the plain background instead).
    Color reflection = col::GREY;
    float sharp = std::clamp(1.0f - mat.roughness * 2.0f, 0.0f, 1.0f);
    if (depth < 2 && sharp > 0.0f) {
      Vec3D dir = ray.dir - normal * (2.0f * ray.dir.dot(normal));
      Color seen = col::GREY; // stays grey if the bounce hits nothing
      trace({point + dir * 1e-3f, dir}, depth + 1, seen);
      reflection = seen * sharp + col::GREY * (1.0f - sharp);
    }

    // texture: the hit weights are exact, no perspective fix needed
    Color base = base_color(
        mesh, tri, w0 * tri.uv[0].u + w1 * tri.uv[1].u + w2 * tri.uv[2].u,
        w0 * tri.uv[0].v + w1 * tri.uv[1].v + w2 * tri.uv[2].v);
    out = Shade_material(terms, base, mat, reflection, exposure_scale);
    return true;
  }

  // One ray per pixel from the camera (per sample, with antialiasing).
  // Rays that miss everything but hit the floor (y = 0) inside the grid
  // check the light too: if an object blocks it, that floor pixel is
  // darkened (a cast shadow).
  void render_raytraced() {
    float f = 1.0f / std::tan(camera.fov * 0.5f * 3.14159265f / 180.0f);
    float aspect = (float)fb.width / (float)fb.height;

    auto darken = [](uint32_t p, float k) { // scale r, g, b; keep alpha
      uint32_t r = (uint32_t)(((p >> 24) & 255) * k);
      uint32_t g = (uint32_t)(((p >> 16) & 255) * k);
      uint32_t b = (uint32_t)(((p >> 8) & 255) * k);
      return (r << 24) | (g << 16) | (b << 8) | (p & 255);
    };

    const int STRIP = 32;
    int strips = (fb.height + STRIP - 1) / STRIP;
    parallel_for(strips, [&](int s) {
      int y0 = s * STRIP;
      int y1 = std::min(fb.height, y0 + STRIP);
      for (int y = y0; y < y1; ++y) {
        for (int x = 0; x < fb.width; ++x) {
          // pixel -> direction: the opposite of screen() in Projection.h
          float x_ndc = ((x + 0.5f) / fb.width) * 2.0f - 1.0f;
          float y_ndc = 1.0f - ((y + 0.5f) / fb.height) * 2.0f;
          Vec3D dir =
              camera.to_world_dir({x_ndc * aspect / f, y_ndc / f, 1.0f})
                  .normalized();
          Ray ray = {camera.position, dir};
          int i = y * fb.width + x;

          Color color;
          if (trace(ray, 0, color)) {
            fb.screen_buffer[i] = color.to_hex();
          } else if (dir.y < 0.0f && camera.position.y > 0.0f) {
            // where the ray meets the floor (y = 0)
            float t = -camera.position.y / dir.y;
            Vec3D floor = camera.position + dir * t;
            if (std::fabs(floor.x) <= 10.0f && std::fabs(floor.z) <= 10.0f &&
                in_shadow(floor))
              fb.screen_buffer[i] = darken(fb.screen_buffer[i], 0.6f);
          }
        }
      }
    });
  }

  // Everything else is changed from the menu window.
  bool OnUserUpdate(float fElapsedTime) {
    camera.update_orbit(); // position from target + distance + yaw/pitch

    // render scale: draw into a smaller buffer, SDL stretches it to the window
    // antialias: draw 2x wider and taller, fb.Draw() averages it back down
    fb.aa = antialias ? 2 : 1;
    int w = std::max(1, (int)(screenW * render_scale)) * fb.aa;
    int h = std::max(1, (int)(screenH * render_scale)) * fb.aa;
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
      // move the light around its target (same distance) along dir
      light.position =
          light.target - dir * (light.position - light.target).length();
    }

#pragma endregion

#pragma region rendering
    // model: mesh coords -> world.  view: world -> relative to camera.
    // Combined, read right to left: model first, then view. One per object.
    Mat4 view = camera.view_matrix();
    models.resize(objects.size());
    to_models.resize(objects.size());
    model_views.resize(objects.size());
    for (int o = 0; o < (int)objects.size(); ++o) {
      models[o] = objects[o].transform.matrix();
      to_models[o] = models[o].inverse_rigid();
      model_views[o] = view * models[o];
    }
    // the same light, moved to camera coordinates like the mesh
    Light light_view = light;
    light_view.position = view.transform_point(light.position);
    light_view.target = view.transform_point(light.target);

    // floor + axes first. Lines don't use the z-buffer, so the car is always
    // drawn on top of them.
    draw_lines(fb, grid, view, camera.fov);
    draw_lines(fb, axes, view, camera.fov);

    // MULTITHREADING: every step below is split across all CPU cores with
    // parallel_for (Parallel.h). Rule: a thread only writes data that belongs
    // to its own job (its own triangles, or its own strip of screen rows).

    // screen() keeps a fov cache in statics. Fill it HERE, before any
    // threads start, so threads only read it.
    screen({0, 0, 1}, camera.fov, fb.width, fb.height);
    exposure_scale = std::log10(exposure * 10.0f + 1.0f); // log10 is slow

    if (raytrace) {
      render_raytraced(); // replaces all the triangle filling below
    } else {

      int n = (int)tri_refs.size(); // all objects' triangles
      transformed_tri.resize(n); // memory is reused between frames
      visible.resize(n);
      projected.resize(n * 3);
      if (shading == SHADING_FLAT) {
        flat_colors.resize(n);
      } else {
        fb.ClearTriangleIds();
        corner_terms.resize(n * 3);
        corner_normals.resize(n * 3);
      }

      // 1. Per triangle, in parallel: transform, cull, project to the screen,
      //    then its color (flat) or its corner data (Gouraud/Phong).
      parallel_for_chunks(n, 4096, [&](int begin, int end) {
        for (int id = begin; id < end; ++id) {
          const TriRef &ref = tri_refs[id];
          const Mesh &mesh = objects[ref.object].mesh;
          Triangle &tri = transformed_tri[id];
          tri = transform_triangle(mesh.tris[ref.index],
                                   model_views[ref.object]);
          visible[id] = !isTriangleCulled(tri, 0.1f, backface_cull);
          if (!visible[id])
            continue;
          for (int k = 0; k < 3; ++k)
            projected[id * 3 + k] =
                screen(tri.p[k], camera.fov, fb.width, fb.height);

          const Material &mat = mesh.materials[tri.material];
          if (shading == SHADING_FLAT) {
            // one color for the whole triangle (texture: read at its center)
            Color base =
                base_color(mesh, tri, (tri.uv[0].u + tri.uv[1].u + tri.uv[2].u) / 3,
                           (tri.uv[0].v + tri.uv[1].v + tri.uv[2].v) / 3);
            // camera is at (0,0,0), so the center is also the view direction
            Vec3D center = (tri.p[0] + tri.p[1] + tri.p[2]) / 3.0f;
            LightTerms terms = Light_terms(
                flat_normal(tri, calculateNormals),
                light_view.direction_at(center), center, mat.roughness,
                light_view.strength_at(center));
            // no reflection rays here: mirrors just see the grey background
            flat_colors[id] =
                Shade_material(terms, base, mat, col::GREY, exposure_scale);
          } else {
            for (int k = 0; k < 3; ++k) {
              corner_normals[id * 3 + k] = vertex_normal(tri, k);
              if (shading == SHADING_GOURAUD)
                corner_terms[id * 3 + k] = Light_terms(
                    corner_normals[id * 3 + k],
                    light_view.direction_at(tri.p[k]), tri.p[k],
                    mat.roughness, light_view.strength_at(tri.p[k]));
            }
          }
        }
      });

      // 2. Fill, in parallel: the screen is cut into strips of STRIP rows and
      //    each thread fills one strip at a time, clipped to those rows. Two
      //    threads never touch the same pixel, so the z-buffer stays correct.
      //    Triangles are still drawn in the same order, so the image is the
      //    same as with one thread.
      const int STRIP = 32;
      int strips = (fb.height + STRIP - 1) / STRIP;
      parallel_for(strips, [&](int s) {
        int y0 = s * STRIP;
        int y1 = std::min(fb.height, y0 + STRIP);
        for (int id = 0; id < n; ++id) {
          if (!visible[id])
            continue;
          const Px2D *p = &projected[id * 3];
          int top = std::min({p[0].y, p[1].y, p[2].y});
          int bottom = std::max({p[0].y, p[1].y, p[2].y});
          if (bottom < y0 || top >= y1)
            continue; // doesn't touch this strip

          if (shading == SHADING_FLAT)
            fb.Fill_triangle(p[0], p[1], p[2], flat_colors[id], y0, y1);
          else // only depth + which triangle won each pixel; colors in step 3
            fb.Fill_triangle_deferred(p[0], p[1], p[2], id, y0, y1);
        }
      });

      // 3. Gouraud/Phong: color every pixel ONCE, from the triangle that won
      //    it. In parallel by strips again (each pixel is independent).
      if (shading != SHADING_FLAT) {
        parallel_for(strips, [&](int s) {
          int y0 = s * STRIP;
          int y1 = std::min(fb.height, y0 + STRIP);
          for (int i = y0 * fb.width; i < y1 * fb.width; ++i) {
          int id = fb.tri_id[i];
          if (id < 0)
            continue; // no triangle here (background, lines)
          float w0 = fb.w0_buffer[i];
          float w1 = fb.w1_buffer[i];
          float w2 = 1.0f - w0 - w1;
          const Triangle &tri = transformed_tri[id];
          const Mesh &mesh = objects[tri_refs[id].object].mesh;

          // surface color: the material's, or the texture at this pixel
          const Material &mat = mesh.materials[tri.material];
          Color base = mat.color;
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

          LightTerms terms;
          if (shading == SHADING_GOURAUD) {
            // blend the 3 corners' light, then color this pixel
            const LightTerms *L = &corner_terms[id * 3];
            terms.diffuse =
                L[0].diffuse * w0 + L[1].diffuse * w1 + L[2].diffuse * w2;
            terms.highlight =
                L[0].highlight * w0 + L[1].highlight * w1 + L[2].highlight * w2;
            terms.facing =
                L[0].facing * w0 + L[1].facing * w1 + L[2].facing * w2;
          } else {
            // Phong: blend the 3 corner normals, then light this pixel
            const Vec3D *n = &corner_normals[id * 3];
            Vec3D normal = (n[0] * w0 + n[1] * w1 + n[2] * w2).normalized();
            Vec3D point = tri.p[0] * w0 + tri.p[1] * w1 + tri.p[2] * w2;
            terms = Light_terms(normal, light_view.direction_at(point), point,
                                mat.roughness, light_view.strength_at(point));
          }
          Color color =
              Shade_material(terms, base, mat, col::GREY, exposure_scale);
          fb.screen_buffer[i] = color.to_hex();
          }
        });
      }
    } // end of: if (raytrace) ... else

    // light marker last, so it's always visible. Directional: a line to its
    // target shows which way it shines.
    if (light.type == LIGHT_DIRECTIONAL)
      draw_lines(fb, {{light.position, light.target, col::YELLOW}}, view,
                 camera.fov);
    draw_circle(fb, light.position, view, camera.fov,
                10.0f * render_scale * fb.aa,
                col::YELLOW);

    fb.Draw();
#pragma endregion

    return true;
  }
};
