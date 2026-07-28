#include "headers/SDL2/SDL_scancode.h"
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
vector<uint32_t>
screen_buffer(screenW *screenH); // y *screenW+x = index of pixel
SDL_Texture *frameTexture = nullptr;

#pragma region structures

struct Color {
  Uint8 r = 0, g = 0, b = 0, a = 255;
  uint32_t to_hex() {
    return (static_cast<uint32_t>(r) << 24) | (static_cast<uint32_t>(g) << 16) |
           (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
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
};

struct Px2D {
  int x, y;
};

struct Triangle {
  Vec3D p[3];
  Color color = col::WHITE;
};

struct Mesh {
  std::vector<Vec3D> vertices;
  std::vector<Triangle> tris;
};

#pragma endregion

class Engine3D {
public:
  Engine3D() {}
  const Uint8 *keystate = nullptr;

  Mesh mesh;
  float dz = 1.0f / 120.0f;
  float angle = 0.0f;
  float angle2 = 0.0f;
  float z_offset = 2.0f;
  float fov = 90.0f;
  float y_offset = 0.0f;
  float dy = 1.0f / 120.0f;

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
        string token;
        while (ss >> token) {
          stringstream token_ss(token);
          int v_idx;
          token_ss >> v_idx;
          face_v.push_back(v_idx - 1);
        }
        for (size_t i = 1; i + 1 < face_v.size(); ++i) {
          Triangle tri;
          tri.p[0] = mesh.vertices[face_v[0]];
          tri.p[1] = mesh.vertices[face_v[i]];
          tri.p[2] = mesh.vertices[face_v[i + 1]];
          tri.color = col::RANDOM();
          mesh.tris.push_back(tri);
        }
      }
    }
    return mesh;
  }
#pragma region defs
  Mesh cube;
#pragma endregion

  bool OnUserCreate() {
    cube = OBJLoader("Object/full_model.obj");
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
      // if(z_offset - dz > -0.99f)
      z_offset -= dz;
    }
    if (keystate[SDL_SCANCODE_Z]) {
      y_offset += dy;
    }
    if (keystate[SDL_SCANCODE_X]) {
      y_offset -= dy;
    }
#pragma endregion

#pragma region rendering
    vector<Triangle> transformed_tri;

    for (const auto &tri : cube.tris) {
      Triangle transformed = translate_y_triangle(
          translate_z_triangle(
              rotate_yz_triangle(rotate_xz_triangle(tri, angle), angle2),
              z_offset),
          y_offset);
      transformed_tri.push_back(transformed);
    }

    // 2. painters algo
    sort(transformed_tri.begin(), transformed_tri.end(),
         [](const Triangle &t1, const Triangle &t2) {
           float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
           float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
           return z1 > z2; // furthest Z rendered first, closest Z rendered last
         });

    // 3. Render
    for (const auto &tri : transformed_tri) {
      Fill_triangle(tri, col::GREEN, fov);
      Draw_triangle(tri, col::BLACK, fov);
    }

    Draw();
#pragma endregion

    return true;
  }

#pragma region helper functions
  // ALL TRANSFORM FUNCTIONS
  Triangle translate_z_triangle(const Triangle &triangle, float z_offset) {
    Triangle translatedTriangle;
    translatedTriangle.color = triangle.color;
    for (int i = 0; i < 3; ++i) {
      translatedTriangle.p[i] = triangle.p[i];
      translatedTriangle.p[i].z += z_offset;
    }
    return translatedTriangle;
  }
  Triangle translate_y_triangle(const Triangle &triangle, float y_offset) {
    Triangle translatedTriangle;
    translatedTriangle.color = triangle.color;
    for (int i = 0; i < 3; ++i) {
      translatedTriangle.p[i] = triangle.p[i];
      translatedTriangle.p[i].y += y_offset;
    }
    return translatedTriangle;
  }

  Triangle rotate_xz_triangle(const Triangle &triangle, float angle) {
    Triangle rotatedTriangle;
    rotatedTriangle.color = triangle.color;
    for (int i = 0; i < 3; ++i) {
      rotatedTriangle.p[i] = rotate_xz(triangle.p[i], angle);
    }
    return rotatedTriangle;
  }
  Triangle rotate_yz_triangle(const Triangle &triangle, float angle) {
    Triangle rotatedTriangle;
    rotatedTriangle.color = triangle.color;
    for (int i = 0; i < 3; ++i) {
      rotatedTriangle.p[i] = rotate_yz(triangle.p[i], angle);
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
    if (screenPos.x >= 0 && screenPos.y >= 0 && screenPos.x < screenW && screenPos.y < screenH) {
      screen_buffer[screenPos.y * screenW + screenPos.x] = color.to_hex();
    }
  }

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

  void DrawHorizontalLine(int x1, int x2, int y,
                          Color color) { // faster for flat triangles
    if (y < 0 || y >= screenH)
      return;
    if (x1 > x2)
      std::swap(x1, x2);

    x1 = std::max(0, x1);
    x2 = std::min(screenW - 1, x2);

    uint32_t hexColor = color.to_hex();
    int rowOffset = y * screenW;

    for (int x = x1; x <= x2; ++x) {
      screen_buffer[rowOffset + x] = hexColor;
    }
  }

  void Clear(Color color) {
    std::fill(screen_buffer.begin(), screen_buffer.end(), color.to_hex());
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

    for (int y = startY; y <= endY; y++) {
      float dy = (float)(y - p0.y);
      float x1 = (float)p0.x + dy * invslope1;
      float x2 = (float)p0.x + dy * invslope2;
      DrawHorizontalLine((int)x1, (int)x2, y, color);
    }
  }

  void Fill_flat_top_triangle(Px2D p0, Px2D p1, Px2D p2, Color color) {
    if (p2.y == p0.y)
      return;
    float invslope1 = (float)(p2.x - p0.x) / (float)(p2.y - p0.y); // slope
    float invslope2 = (float)(p2.x - p1.x) / (float)(p2.y - p1.y);

    int startY = p0.y;
    int endY = p2.y;

    for (int y = startY; y <= endY; y++) {
      float dy = (float)(y - p2.y);
      float x1 = (float)p2.x + dy * invslope1; // eqn of line along slope
      float x2 = (float)p2.x + dy * invslope2;
      DrawHorizontalLine((int)x1, (int)x2, y, color);
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
      int p3_x = p0.x + (int)(((float)(p1.y - p0.y) / (float)(p2.y - p0.y)) * (float)(p2.x - p0.x));
      Px2D p3 = {p3_x, p1.y};
      Fill_flat_bottom_triangle(p0, p1, p3, color);
      Fill_flat_top_triangle(p1, p3, p2, color);
    }
  }
  // PROJECTION AND SHADING MATH
  Px2D screen(Vec3D point, float fovDegrees = 90.0f) {
    float fovRad = fovDegrees * 0.5f * (3.14159265f / 180.0f);
    float f = 1.0f / tan(fovRad);
    float aspect = (float)screenW / (float)screenH;

    float z = point.z;
    if (z <= 0.0001f) // fallback for now
      z = 0.0001f;
    float invZ = 1.0f / z;

    float x_ndc = (point.x * f / aspect) * invZ; // normalize to aspect ratio
    float y_ndc = (point.y * f) * invZ;

    int screenX = (int)((x_ndc + 1.0f) * 0.5f * screenW); // formula = (x+1)/(2*z*tan(fov/2))
    int screenY = (int)((1.0f - y_ndc) * 0.5f * screenH);

    return {screenX, screenY};
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