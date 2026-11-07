#pragma once

#include "../headers/imgui/backends/imgui_impl_sdl2.h"
#include "../headers/imgui/backends/imgui_impl_sdlrenderer2.h"
#include "../headers/imgui/imgui.h"
#include "Engine3D.h"
#include "SDLHeaders.h"

// A second SDL window. Lists the objects; clicking one shows its transform.
// The fields edit engine.object / engine.camera directly, nothing to sync.
struct Menu {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  int selected = -1; // -1 = nothing, 0 = car, 1 = camera, 2 = light
  int fps = 0;       // set by main() once per second

  void Init() {
    window = SDL_CreateWindow("Menu", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, 300, 400, 0);
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

    if (ImGui::Selectable("Car", selected == 0))
      selected = 0;
    if (ImGui::Selectable("Light", selected == 2))
      selected = 2;
    if (ImGui::Selectable("Camera", selected == 1))
      selected = 1;

    if (selected == 0) {
      ImGui::DragFloat3("position", &engine.object.position.x, 0.01f);
      ImGui::DragFloat("rot_xz", &engine.object.rot_xz, 0.01f);
      ImGui::DragFloat("rot_yz", &engine.object.rot_yz, 0.01f);
      ImGui::DragFloat("rot_xy", &engine.object.rot_xy, 0.01f);
    } else if (selected == 1) {
      ImGui::DragFloat3("position", &engine.camera.position.x, 0.01f);
      ImGui::DragFloat("yaw", &engine.camera.yaw, 0.01f);
      ImGui::DragFloat("pitch", &engine.camera.pitch, 0.01f);
      ImGui::DragFloat("fov", &engine.camera.fov, 0.5f, 10.0f, 170.0f);
    } else if (selected == 2) {
      ImGui::DragFloat3("position", &engine.light.position.x, 0.01f);
    }

    // render settings (these used to be keybinds)
    ImGui::Text("");
    ImGui::Text("Settings");
    ImGui::RadioButton("flat", &engine.shading, SHADING_FLAT);
    ImGui::SameLine();
    ImGui::RadioButton("gouraud", &engine.shading, SHADING_GOURAUD);
    ImGui::SameLine();
    ImGui::RadioButton("phong", &engine.shading, SHADING_PHONG);
    ImGui::Checkbox("calculate normals (flat only)", &engine.calculateNormals);
    ImGui::Checkbox("backface cull", &engine.backface_cull);
    ImGui::SliderFloat("render scale", &engine.render_scale, 0.25f, 1.0f);
    ImGui::DragFloat("specularity", &engine.specularity, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("exposure", &engine.exposure, 0.01f, 0.0f, 10.0f);
    ImGui::Text("Left mouse in main window: move light");

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
