#include "src/Config.h"
#include "src/Display.h"
#include "src/Engine3D.h"
#include "src/Menu.h"
#include "src/SDLHeaders.h"
#include <ctime>
#include <string>

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
  engine.window = window;
  engine.OnUserCreate();

  Menu menu;
  menu.Init();

  Uint64 lastTime = SDL_GetPerformanceCounter();
  bool running = true;

  float frameTimer = 0.0f;
  int frameCount = 0;

  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        running = false;
      // with 2 windows, closing one sends this instead of SDL_QUIT
      if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)
        running = false;
      menu.HandleEvent(e);
    }

    Uint64 currentTime = SDL_GetPerformanceCounter();
    float fElapsedTime =
        (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
    lastTime = currentTime;
    engine.OnUserUpdate(fElapsedTime);
    menu.Draw(engine);

    frameTimer += fElapsedTime;
    frameCount++;
    // Update title once every 1.0 second
    if (frameTimer >= 1.0f) {
      float fps = (float)frameCount / frameTimer;
      menu.fps = (int)fps;
      // Reset counters for next second
      frameCount = 0;
      frameTimer = 0.0f;
    }
  }
  menu.Shutdown();
  SDL_DestroyTexture(frameTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}