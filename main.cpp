#include "headers/SDL2/SDL_mouse.h"
#include "headers/SDL2/SDL_scancode.h"
#include <cfloat>
#include <cstdint>
#define SDL_MAIN_HANDLED
#include "headers/SDL2/SDL.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

const int screenW = 600;
const int screenH = 600;
SDL_Renderer *renderer = nullptr;
SDL_Texture *frameTexture = nullptr;

#pragma region structures

struct Color {
  Uint8 r = 0, g = 0, b = 0, a = 255;
  uint32_t to_hex() {
    return (static_cast<uint32_t>(r) << 24) | (static_cast<uint32_t>(g) << 16) |
           (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
  }
  Color operator+(const Color &other) const {
    return {static_cast<Uint8>((r + other.r) > 255 ? 255 : r + other.r),
            static_cast<Uint8>((g + other.g) > 255 ? 255 : g + other.g),
            static_cast<Uint8>((b + other.b) > 255 ? 255 : b + other.b),
            static_cast<Uint8>((a + other.a) > 255 ? 255 : a + other.a)};
  }
  Color operator-(const Color &other) const {
    return {static_cast<Uint8>((r - other.r) < 0 ? 0 : r - other.r),
            static_cast<Uint8>((g - other.g) < 0 ? 0 : g - other.g),
            static_cast<Uint8>((b - other.b) < 0 ? 0 : b - other.b),
            static_cast<Uint8>((a - other.a) < 0 ? 0 : a - other.a)};
  }
  Color operator*(const float scalar) const {
    return {static_cast<Uint8>((r * scalar) > 255 ? 255 : r * scalar),
            static_cast<Uint8>((g * scalar) > 255 ? 255 : g * scalar),
            static_cast<Uint8>((b * scalar) > 255 ? 255 : b * scalar),
            static_cast<Uint8>((a * scalar) > 255 ? 255 : a * scalar)};
  }
  Color operator/(const float scalar) const {
    return {static_cast<Uint8>(r / scalar > 255 ? 255 : r / scalar),
            static_cast<Uint8>(g / scalar > 255 ? 255 : g / scalar),
            static_cast<Uint8>(b / scalar > 255 ? 255 : b / scalar),
            static_cast<Uint8>(a / scalar > 255 ? 255 : a / scalar)};
  }
};

namespace col {
const Color BLACK = {0, 0, 0, 255};
const Color WHITE = {255, 255, 255, 255};
const Color GREEN = {0, 255, 0, 255};
inline Color RANDOM() {
  return {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256),
          static_cast<Uint8>(rand() % 256), 255};
}
} // namespace col

struct Vec3D {
  float x, y, z;

  Vec3D operator+(const Vec3D &other) const {
    return {x + other.x, y + other.y, z + other.z};
  }

  Vec3D operator-(const Vec3D &other) const {
    return {x - other.x, y - other.y, z - other.z};
  }

  Vec3D operator*(const float scalar) const {
    return {x * scalar, y * scalar, z * scalar};
  }

  Vec3D operator/(const float scalar) const {
    return {x / scalar, y / scalar, z / scalar};
  }

  Vec3D cross(const Vec3D &other) const {
    return {y * other.z - z * other.y, z * other.x - x * other.z,
            x * other.y - y * other.x};
  }

  float dot(const Vec3D &other) const {
    return x * other.x + y * other.y + z * other.z;
  }

  float length() const { return std::sqrt(x * x + y * y + z * z); }

  Vec3D normalized() const {
    float len = length();
    if (len == 0.0f)
      return {0, 0, 0};
    return *this / len;
  }
};

struct Px2D {
  int x, y;
  float z;
};

struct Triangle {
  Vec3D p[3];
  Vec3D n[3] = {};
  Color color = col::WHITE;
};

struct Mesh {
  std::vector<Vec3D> vertices;
  std::vector<Triangle> tris;
  std::vector<Vec3D> vert_normals;
};

#pragma endregion

class Engine3D {
public:
  Engine3D() {}
#pragma region vars
  vector<uint32_t> screen_buffer =
      vector<uint32_t>(screenW * screenH); // y *screenW+x = index of pixel
  vector<float> z_buffer = vector<float>(screenW * screenH, FLT_MAX);
  const Uint8 *keystate = nullptr;
  Mesh mesh;
  float dz = 1.0f / 120.0f;
  float angle = 0.0f;
  float angle2 = 0.0f;
  float z_offset = 2.0f;
  float fov = 90.0f;
  float y_offset = 0.0f;
  float dy = 1.0f / 120.0f;
  Vec3D light_dir = {1, -1, -1};
  Vec3D z_dir = {0, 0, 1};
  bool calculateNormals = true;
  Px2D mousepos;
#pragma endregion

  bool OnUserCreate() {
    mesh = OBJLoader("Object/full_model.obj");
    return true;
  }

  bool OnUserUpdate(float fElapsedTime) {
    keystate = SDL_GetKeyboardState(nullptr);
    Clear(col::BLACK);
#pragma region Keyboard input
    if (keystate[SDL_SCANCODE_Q]) {
      fov += 0.5f;
    }
    if (keystate[SDL_SCANCODE_E]) {
      fov -= 0.5f;
    }
    fov = clamp(fov, 10.0f, 170.0f);

    if (keystate[SDL_SCANCODE_LEFT]) {
      angle -= 3.14159f * fElapsedTime;
    }
    if (keystate[SDL_SCANCODE_RIGHT]) {
      angle += 3.14159f * fElapsedTime;
    }
    if (keystate[SDL_SCANCODE_UP]) {
      angle2 += 3.14159f * fElapsedTime;
    }
    if (keystate[SDL_SCANCODE_DOWN]) {
      angle2 -= 3.14159f * fElapsedTime;
    }

    if (keystate[SDL_SCANCODE_S]) {
      z_offset += dz;
    }
    if (keystate[SDL_SCANCODE_W]) {
      if (z_offset - dz > -0.99f)
        z_offset -= dz;
    }
    if (keystate[SDL_SCANCODE_Z]) {
      y_offset += dy;
    }
    if (keystate[SDL_SCANCODE_X]) {
      y_offset -= dy;
    }

    if (keystate[SDL_SCANCODE_C]) {
      calculateNormals = !calculateNormals;
      SDL_Delay(200);
    }

    if (SDL_GetMouseState(&mousepos.x, &mousepos.y) &
        SDL_BUTTON(SDL_BUTTON_LEFT)) {
      light_dir = {-(((float)mousepos.x / screenW) * 4.0f - 2.0f) * z_dir.z,
                   (((float)mousepos.y / screenH) * 4.0f - 2.0f), z_dir.z};
      light_dir = light_dir.normalized();
    }

#pragma endregion

#pragma region rendering
    vector<Triangle> transformed_tri;
    Vec3D light = {1, -1, -1};
    light = rotate_yz(rotate_xz(light_dir, angle), angle2);
    z_dir = rotate_yz(rotate_xz({0, 0, 1}, angle), angle2);

    // 1. Transform mesh
    for (const auto &tri : mesh.tris) {
      Triangle transformed = translate_y_triangle(
          translate_z_triangle(
              rotate_yz_triangle(rotate_xz_triangle(tri, angle), angle2),
              z_offset),
          y_offset);
      transformed_tri.push_back(transformed);
    }

    // 2. painters algo(slow)
    // Painters_algo(transformed_tri);

    // 3. Render
    for (const auto &tri : transformed_tri) {
      if (!isTriangleCulled(tri, 0.1f)) {
        Color shaded = Shade_triangle(tri, light, col::GREEN,
                                      calculateNormals); // 2nd sector = -1 -1
        Fill_triangle(tri, shaded, fov);
      }
      // Draw_triangle(tri, col::WHITE, fov);
    }

    Draw();
#pragma endregion

    return true;
  }

#pragma region helper functions
  // OBJECT LOADER (.obj)
  Mesh OBJLoader(string filePath) {
    ifstream file(filePath);
    if (!file.is_open()) {
      cerr << "Error opening file: " << filePath << endl;
      return {};
    }
    Mesh mesh;
    string line;
    while (getline(file, line)) {
      stringstream ss(line);
      string type;
      ss >> type;
      if (type == "v") {
        float x, y, z;
        ss >> x >> y >> z;
        mesh.vertices.push_back({x, y, z});
      } else if (type == "f") {
        vector<int> face_v;
        vector<int> face_vn;
        string token;
        while (ss >> token) {
          int v_idx = 0, vt_idx = 0, vn_idx = 0;
          if (token.find("//") != string::npos) {
            sscanf(token.c_str(), "%d//%d", &v_idx, &vn_idx);
          } else {
            sscanf(token.c_str(), "%d/%d/%d", &v_idx, &vt_idx, &vn_idx);
          }
          if (v_idx > 0)
            face_v.push_back(v_idx - 1);
          if (vn_idx > 0)
            face_vn.push_back(vn_idx - 1);
        }
        for (size_t i = 1; i + 1 < face_v.size(); ++i) {
          Triangle tri;
          tri.p[0] = mesh.vertices[face_v[0]];
          tri.p[1] = mesh.vertices[face_v[i]];
          tri.p[2] = mesh.vertices[face_v[i + 1]];

          if (face_vn.size() == face_v.size() && !mesh.vert_normals.empty()) {
            tri.n[0] = mesh.vert_normals[face_vn[0]];
            tri.n[1] = mesh.vert_normals[face_vn[i]];
            tri.n[2] = mesh.vert_normals[face_vn[i + 1]];
          }

          tri.color = col::RANDOM();
          mesh.tris.push_back(tri);
        }
      } else if (type == "vn") {
        float x, y, z;
        ss >> x >> y >> z;
        mesh.vert_normals.push_back({x, y, z});
      }
    }
    return mesh;
  }
  // ---------------------------------------------------------------

  // ALL TRANSFORM FUNCTIONS
  Triangle translate_z_triangle(const Triangle &triangle, float z_offset) {
    Triangle translatedTriangle;
    translatedTriangle.color = triangle.color;
    for (int i = 0; i < 3; ++i) {
      translatedTriangle.p[i] = triangle.p[i];
      translatedTriangle.p[i].z += z_offset;
      translatedTriangle.n[i] = triangle.n[i];
    }
    return translatedTriangle;
  }
  Triangle translate_y_triangle(const Triangle &triangle, float y_offset) {
    Triangle translatedTriangle;
    translatedTriangle.color = triangle.color;
    for (int i = 0; i < 3; ++i) {
      translatedTriangle.p[i] = triangle.p[i];
      translatedTriangle.p[i].y += y_offset;
      translatedTriangle.n[i] = triangle.n[i];
    }
    return translatedTriangle;
  }

  Triangle rotate_xz_triangle(const Triangle &triangle, float angle) {
    Triangle rotatedTriangle;
    rotatedTriangle.color = triangle.color;
    for (int i = 0; i < 3; ++i) {
      rotatedTriangle.p[i] = rotate_xz(triangle.p[i], angle);
      rotatedTriangle.n[i] = rotate_xz(triangle.n[i], angle);
    }
    return rotatedTriangle;
  }
  Triangle rotate_yz_triangle(const Triangle &triangle, float angle) {
    Triangle rotatedTriangle;
    rotatedTriangle.color = triangle.color;
    for (int i = 0; i < 3; ++i) {
      rotatedTriangle.p[i] = rotate_yz(triangle.p[i], angle);
      rotatedTriangle.n[i] = rotate_yz(triangle.n[i], angle);
    }
    return rotatedTriangle;
  }

  Vec3D rotate_xz(Vec3D point, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    return {point.x * cosA - point.z * sinA, point.y,
            point.x * sinA + point.z * cosA};
  }

  Vec3D rotate_yz(Vec3D point, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    return {point.x, point.y * cosA - point.z * sinA,
            point.y * sinA + point.z * cosA};
  }

  void DrawPoint(Vec3D point, Color color, float fovDegrees = 90.0f) {
    Px2D screenPos = screen(point, fovDegrees);
    if (screenPos.x >= 0 && screenPos.y >= 0 && screenPos.x < screenW &&
        screenPos.y < screenH) {
      screen_buffer[screenPos.y * screenW + screenPos.x] = color.to_hex();
    }
  }
  //---------------------------------------------------------------------

  // ALL DRAW FUNCTIONS
  void Draw() { // once per frame
    SDL_UpdateTexture(frameTexture, nullptr, screen_buffer.data(),
                      screenW * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, frameTexture, nullptr, nullptr);
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
      if (x0 >= 0 && x0 < screenW && y0 >= 0 && y0 < screenH) {
        screen_buffer[y0 * screenW + x0] = hexColor;
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

  void
  DrawHorizontalLine(int x1, int x2, int y, float z1, float z2,
                     Color color) { // rasterize horizontal scanline with z-test
    if (y < 0 || y >= screenH)
      return;
    if (x1 > x2) {
      std::swap(x1, x2);
      std::swap(z1, z2);
    }

    int origX1 = x1;
    int origX2 = x2;

    int startX = std::max(0, x1);
    int endX = std::min(screenW - 1, x2);

    uint32_t hexColor = color.to_hex();
    int rowOffset = y * screenW;
    float dx = (float)(origX2 - origX1);

    for (int x = startX; x <= endX; ++x) {
      float t = (dx != 0.0f) ? (float)(x - origX1) / dx : 0.0f; // fuck this
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

  void Draw_triangle(const Triangle &tri, Color color,
                     float fovDegrees = 90.0f) {
    Px2D p0 = screen(tri.p[0], fovDegrees);
    Px2D p1 = screen(tri.p[1], fovDegrees);
    Px2D p2 = screen(tri.p[2], fovDegrees);

    DrawLine(p0.x, p0.y, p1.x, p1.y, color);
    DrawLine(p1.x, p1.y, p2.x, p2.y, color);
    DrawLine(p2.x, p2.y, p0.x, p0.y, color);
  }

  void Fill_flat_bottom_triangle(Px2D p0, Px2D p1, Px2D p2, Color color) {
    if (p1.y == p0.y)
      return;
    float invslope1 = (float)(p1.x - p0.x) / (float)(p1.y - p0.y);
    float invslope2 = (float)(p2.x - p0.x) / (float)(p2.y - p0.y);

    int startY = p0.y;
    int endY = p1.y;
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

  void Fill_flat_top_triangle(Px2D p0, Px2D p1, Px2D p2, Color color) {
    if (p2.y == p0.y)
      return;
    float invslope1 = (float)(p2.x - p0.x) / (float)(p2.y - p0.y); // slope
    float invslope2 = (float)(p2.x - p1.x) / (float)(p2.y - p1.y);

    int startY = p0.y;
    int endY = p2.y;
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

  void Fill_triangle(const Triangle &tri, Color color,
                     float fovDegrees = 90.0f) {
    Px2D p0 = screen(tri.p[0], fovDegrees);
    Px2D p1 = screen(tri.p[1], fovDegrees);
    Px2D p2 = screen(tri.p[2], fovDegrees);

    if (p0.y > p1.y)
      std::swap(p0, p1);
    if (p0.y > p2.y)
      std::swap(p0, p2);
    if (p1.y > p2.y)
      std::swap(p1, p2);

    if (p0.y == p2.y)
      return;

    if (p1.y == p2.y) {
      Fill_flat_bottom_triangle(p0, p1, p2, color);
    } else if (p0.y == p1.y) {
      Fill_flat_top_triangle(p0, p1, p2, color);
    } else {
      float t = (float)(p1.y - p0.y) / (float)(p2.y - p0.y);
      int p3_x = p0.x + (int)(t * (float)(p2.x - p0.x));
      float p3_z = p0.z + t * (p2.z - p0.z);
      Px2D p3 = {p3_x, p1.y, p3_z};
      Fill_flat_bottom_triangle(p0, p1, p3, color);
      Fill_flat_top_triangle(p1, p3, p2, color);
    }
  }
  // --------------------------------------------------------------------

  // PROJECTION AND SHADING MATH
  Px2D screen(Vec3D point, float fovDegrees = 90.0f) {
    float fovRad = fovDegrees * 0.5f * (3.14159265f / 180.0f);
    float f = 1.0f / tan(fovRad);
    float aspect = (float)screenW / (float)screenH;

    float z = point.z;
    if (z <= 0.001f) // fallback for now
      z = 0.001f;
    float invZ = 1.0f / z;

    float x_ndc = (point.x * f / aspect) * invZ; // normalize to aspect ratio
    float y_ndc = (point.y * f) * invZ;

    int screenX = (int)((x_ndc + 1.0f) * 0.5f *
                        screenW); // formula = (x+1)/(2*z*tan(fov/2))
    int screenY = (int)((1.0f - y_ndc) * 0.5f * screenH);

    return {screenX, screenY, z};
  }

  Color Shade_triangle(Triangle tri, Vec3D Light_v, Color color,
                       bool calculateNormals = true) {
    Vec3D normal;
    Vec3D cam = {0, 0, 0};
    if (calculateNormals) {
      Vec3D v0 = tri.p[0];
      Vec3D v1 = tri.p[1];
      Vec3D v2 = tri.p[2];

      Vec3D l1 = v0 - v1;
      Vec3D l2 = v2 - v1;

      normal = l1.cross(l2).normalized();
    } else {
      normal = cam - ((tri.n[0] + tri.n[1] + tri.n[2]).normalized());
    }

    float light_intensity = normal.dot(Light_v.normalized());
    light_intensity = clamp(light_intensity, 0.15f, 1.0f);

    return color * light_intensity;
  }

  bool isTriangleCulled(Triangle tri, float cull_dist) {
    if (tri.p[0].z < cull_dist || tri.p[1].z < cull_dist ||
        tri.p[2].z < cull_dist) {
      return true;
    }
    Vec3D v0 = tri.p[0];
    Vec3D v1 = tri.p[1];
    Vec3D v2 = tri.p[2];
    Vec3D cam = {0, 0, 0};
    Vec3D l1 = v0 - v1;
    Vec3D l2 = v2 - v1;

    Vec3D normal = l1.cross(l2);
    normal = normal.normalized();
    float angle = normal.dot(cam - v1);
    return (angle > 0.0f) ? true : false;
  }

  void Painters_algo(vector<Triangle> &transformed_tri) {
    sort(transformed_tri.begin(), transformed_tri.end(),
         [](const Triangle &t1, const Triangle &t2) {
           float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
           float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
           return z1 > z2; // furthest Z rendered first, closest Z rendered last
         });
  }

#pragma endregion
};

int main() {

  srand(time(NULL)); // for making colors not random every frame

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("3D Rasterizer", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, screenW,
                                        screenH, SDL_WINDOW_ALLOW_HIGHDPI);
  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  SDL_RenderSetLogicalSize(renderer, screenW, screenH);
  frameTexture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_STREAMING, screenW, screenH);

  Engine3D engine;
  engine.OnUserCreate();

  Uint64 lastTime = SDL_GetPerformanceCounter();
  bool running = true;

  float frameTimer = 0.0f;
  int frameCount = 0;

  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        running = false;
    }

    Uint64 currentTime = SDL_GetPerformanceCounter();
    float fElapsedTime =
        (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
    lastTime = currentTime;
    engine.OnUserUpdate(fElapsedTime);

    frameTimer += fElapsedTime;
    frameCount++;
    // Update title once every 1.0 second
    if (frameTimer >= 1.0f) {
      float fps = (float)frameCount / frameTimer;
      std::string title = "3D Rasterizer | FPS: " + std::to_string((int)fps);
      SDL_SetWindowTitle(window, title.c_str());
      // Reset counters for next second
      frameCount = 0;
      frameTimer = 0.0f;
    }
  }
  SDL_DestroyTexture(frameTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}