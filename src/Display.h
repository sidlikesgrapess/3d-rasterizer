#pragma once

#include "SDLHeaders.h"

// Shared SDL objects (created in main, used by Framebuffer::Draw)
inline SDL_Renderer *renderer = nullptr;
inline SDL_Texture *frameTexture = nullptr;
