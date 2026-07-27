
#include "headers/olcPixelGameEngine.h"
using namespace std;

struct Vec3D {
    float x, y, z;
};

struct Vec2D {
    float x, y;
};

struct Triangle {
    Vec3D p[3];
};

struct Mesh {
    std::vector<Triangle> tris;
};

class Engine3D : public olc::PixelGameEngine {
public:
    Engine3D() {
        sAppName = "3D Rasterizer";
    }

    Mesh mesh;
    float dz = 1.0f/120.0f;
    float angle = 0.0f;
    float z_offset = 0.0f; //!=-1
    float fov = 90.0f;

    

    bool OnUserCreate() override {
       mesh.tris = { 
                    // Back Face (Z = 0.25)
                    { { {0.0f, 0.25f, 0.25f}, {0.25f, -0.25f, 0.25f}, {-0.25f, -0.25f, 0.25f} } },

                    // Front Face (Z = -0.25)
                    { { {0.0f, 0.25f, -0.25f}, {0.25f, -0.25f, -0.25f}, {-0.25f, -0.25f, -0.25f} } },

                    // Right Side (Quad split into 2 triangles)
                    { { {0.0f, 0.25f, 0.25f}, {0.25f, -0.25f, 0.25f}, {0.25f, -0.25f, -0.25f} } },
                    { { {0.0f, 0.25f, 0.25f}, {0.25f, -0.25f, -0.25f}, {0.0f, 0.25f, -0.25f} } },

                    // Left Side (Quad split into 2 triangles)
                    { { {0.0f, 0.25f, 0.25f}, {-0.25f, -0.25f, 0.25f}, {-0.25f, -0.25f, -0.25f} } },
                    { { {0.0f, 0.25f, 0.25f}, {-0.25f, -0.25f, -0.25f}, {0.0f, 0.25f, -0.25f} } },

                    // Bottom Side (Quad split into 2 triangles)
                    { { {0.25f, -0.25f, 0.25f}, {-0.25f, -0.25f, 0.25f}, {-0.25f, -0.25f, -0.25f} } },
                    { { {0.25f, -0.25f, 0.25f}, {-0.25f, -0.25f, -0.25f}, {0.25f, -0.25f, -0.25f} } }
                };
                
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {

        Clear(olc::BLACK);

        if (GetKey(olc::Key::DOWN).bHeld) {
            fov += 0.5f;
        }
        if (GetKey(olc::Key::UP).bHeld) {
            fov -= 0.5f;
        }
        fov = clamp(fov, 10.0f, 170.0f);

        if (GetKey(olc::Key::LEFT).bHeld) {
            angle -= 3.14159f * fElapsedTime;
        }
        if (GetKey(olc::Key::RIGHT).bHeld) {
            angle += 3.14159f * fElapsedTime;
        }

        if (GetKey(olc::Key::S).bHeld) {
            z_offset += dz;
        }
        if (GetKey(olc::Key::W).bHeld) {
            //if(z_offset - dz > -0.99f)
            z_offset -= dz;
        }

       
        //angle += 3.14159f * fElapsedTime; //need try catch

        for (const auto& tri : mesh.tris) {
            Draw_triangle(translate_z_triangle(rotate_xz_triangle(tri, angle), z_offset), olc::WHITE, fov);
        }

        return true;
    }

    Triangle translate_z_triangle(const Triangle& triangle, float z_offset) {
        Triangle translatedTriangle;
        for (int i = 0; i < 3; ++i) {
            translatedTriangle.p[i] = triangle.p[i];
            translatedTriangle.p[i].z += z_offset;
        }
        return translatedTriangle;
    }

    Triangle rotate_xz_triangle(const Triangle& triangle, float angle) {
        Triangle rotatedTriangle;
        for (int i = 0; i < 3; ++i) {
            rotatedTriangle.p[i] = rotate_xz(triangle.p[i], angle);
        }
        return rotatedTriangle;
    }

    void DrawPoint(Vec3D point, olc::Pixel color, float fovDegrees = 90.0f) {
        Vec2D screenPos = screen(point, fovDegrees);
        Draw(screenPos.x, screenPos.y, color);
    }

    void Draw_triangle(const Triangle& tri, olc::Pixel color, float fovDegrees = 90.0f) {
        Vec2D p0 = screen(tri.p[0], fovDegrees);
        Vec2D p1 = screen(tri.p[1], fovDegrees);
        Vec2D p2 = screen(tri.p[2], fovDegrees);
        
        DrawLine(p0.x, p0.y, p1.x, p1.y, color);
        DrawLine(p1.x, p1.y, p2.x, p2.y, color);
        DrawLine(p2.x, p2.y, p0.x, p0.y, color);
    }

    Vec3D rotate_xz(Vec3D point, float angle) {
        float cosA = cos(angle);
        float sinA = sin(angle);
        return {
            point.x * cosA - point.z * sinA,
            point.y,
            point.x * sinA + point.z * cosA
        };
    }

    Vec2D screen(Vec3D point, float fovDegrees = 90.0f) {
        float fovRad = fovDegrees * 0.5f * (3.14159f / 180.0f);
        float f = 1.0f / tan(fovRad);
        float invZ = 1.0f / (point.z + 1.0f);
        
        return {
            static_cast<float>(((point.x * f) * invZ + 1.0f) * (ScreenWidth() / 2.0f)),
            static_cast<float>((1.0f - (((point.y * f) * invZ + 1.0f) / 2.0f)) * ScreenHeight())
        };
    }

};

int main() {
    Engine3D engine;
    
   
    if (engine.Construct(256, 256, 4, 4, false, true, false, false)) {
        engine.Start();
    }
    
    return 0;
}