#pragma once

#include "../headers/imgui/backends/imgui_impl_sdl2.h"
#include "../headers/imgui/backends/imgui_impl_sdlrenderer2.h"
#include "../headers/imgui/imgui.h"
#include "Engine3D.h"
#include "SDLHeaders.h"

// A second SDL window. Lists the light, the camera and every loaded object;
// clicking one shows its settings. Each object's arrow opens its materials.
// The fields edit the engine directly, nothing to sync.
struct Menu {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  enum Selected { NONE, OBJECT, CAMERA, LIGHT, MATERIAL };
  Selected selected = NONE;
  int object = 0;   // which object (OBJECT / MATERIAL)
  int material = 0; // which of that object's materials (MATERIAL)
  int fps = 0;      // set by main() once per second

  void Init() {
    window = SDL_CreateWindow("Menu", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, 300, 600, 0);
    // no vsync here: the main window already waits for it once per frame
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr; // don't write imgui.ini
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
  }

  // ImGui ignores events that belong to the main window by itself
  void HandleEvent(const SDL_Event &e) { ImGui_ImplSDL2_ProcessEvent(&e); }

  void Draw(Engine3D &engine) {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // one panel filling the whole menu window
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Objects", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    ImGui::Text("FPS: %d", fps);

    if (ImGui::Selectable("Light", selected == LIGHT))
      selected = LIGHT;
    if (ImGui::Selectable("Camera", selected == CAMERA))
      selected = CAMERA;

    // one row per object: click the name = its transform,
    // click the arrow = show its materials
    for (int o = 0; o < (int)engine.objects.size(); ++o) {
      SceneObject &obj = engine.objects[o];
      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
      if (selected == OBJECT && object == o)
        flags |= ImGuiTreeNodeFlags_Selected;
      bool open = ImGui::TreeNodeEx((void *)(intptr_t)o, flags, "%s",
                                    obj.name.c_str());
      if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selected = OBJECT;
        object = o;
      }
      if (open) {
        for (int i = 0; i < (int)obj.mesh.materials.size(); ++i) {
          ImGui::PushID(i); // materials could share a name
          if (ImGui::Selectable(obj.mesh.materials[i].name.c_str(),
                                selected == MATERIAL && object == o &&
                                    material == i)) {
            selected = MATERIAL;
            object = o;
            material = i;
          }
          ImGui::PopID();
        }
        ImGui::TreePop();
      }
    }

    if (selected == OBJECT && object < (int)engine.objects.size()) {
      WorldTransform &t = engine.objects[object].transform;
      ImGui::DragFloat3("position", &t.position.x, 0.01f);
      ImGui::DragFloat("rot_xz", &t.rot_xz, 0.01f);
      ImGui::DragFloat("rot_yz", &t.rot_yz, 0.01f);
      ImGui::DragFloat("rot_xy", &t.rot_xy, 0.01f);
    } else if (selected == CAMERA) {
      // position is worked out from these every frame (orbit camera)
      ImGui::DragFloat3("target", &engine.camera.target.x, 0.01f);
      ImGui::DragFloat("distance", &engine.camera.distance, 0.01f, 0.05f,
                       1000.0f);
      ImGui::DragFloat("yaw", &engine.camera.yaw, 0.01f);
      ImGui::DragFloat("pitch", &engine.camera.pitch, 0.01f, -1.55f, 1.55f);
      ImGui::DragFloat("fov", &engine.camera.fov, 0.5f, 10.0f, 170.0f);
    } else if (selected == LIGHT) {
      ImGui::RadioButton("directional", &engine.light.type, LIGHT_DIRECTIONAL);
      ImGui::SameLine();
      ImGui::RadioButton("point", &engine.light.type, LIGHT_POINT);
      ImGui::DragFloat3("position", &engine.light.position.x, 0.01f);
      if (engine.light.type == LIGHT_DIRECTIONAL)
        ImGui::DragFloat3("target", &engine.light.target.x, 0.01f);
      else
        ImGui::DragFloat("strength", &engine.light.strength, 0.1f, 0.0f,
                         10000.0f);
    } else if (selected == MATERIAL && object < (int)engine.objects.size() &&
               material < (int)engine.objects[object].mesh.materials.size()) {
      Material &m = engine.objects[object].mesh.materials[material];
      // ImGui edits colors as 0..1 floats
      float c[3] = {m.color.r / 255.0f, m.color.g / 255.0f, m.color.b / 255.0f};
      if (ImGui::ColorEdit3("color (no texture)", c))
        m.color = {(Uint8)(c[0] * 255), (Uint8)(c[1] * 255),
                   (Uint8)(c[2] * 255), 255};
      ImGui::SliderFloat("metallic", &m.metallic, 0.0f, 1.0f);
      ImGui::SliderFloat("roughness", &m.roughness, 0.0f, 1.0f);
    }

    // render settings (these used to be keybinds)
    ImGui::Text("");
    ImGui::Text("Settings");
    ImGui::Checkbox("ray tracing", &engine.raytrace);
    // these only matter for triangle filling: greyed out while ray tracing
    ImGui::BeginDisabled(engine.raytrace);
    ImGui::RadioButton("flat", &engine.shading, SHADING_FLAT);
    ImGui::SameLine();
    ImGui::RadioButton("gouraud", &engine.shading, SHADING_GOURAUD);
    ImGui::SameLine();
    ImGui::RadioButton("phong", &engine.shading, SHADING_PHONG);
    ImGui::Checkbox("calculate normals (flat only)", &engine.calculateNormals);
    ImGui::Checkbox("backface cull", &engine.backface_cull);
    ImGui::EndDisabled();
    ImGui::Checkbox("textures", &engine.use_textures);
    ImGui::SliderFloat("render scale", &engine.render_scale, 0.25f, 1.0f);
    ImGui::Checkbox("antialiasing (2x2)", &engine.antialias);
    ImGui::DragFloat("exposure", &engine.exposure, 0.01f, 0.0f, 10.0f);
    ImGui::Text("Main window mouse:");
    ImGui::Text("  left drag: move light");
    ImGui::Text("  middle drag: orbit camera");
    ImGui::Text("  shift + middle drag: pan");
    ImGui::Text("  wheel: zoom");

    ImGui::End();
    ImGui::Render();
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  }

  void Shutdown() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
  }
};
