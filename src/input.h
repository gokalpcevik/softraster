#pragma once
#include "types.h"
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>

namespace gfx {
bool IsKeyPressed(SDL_Scancode key);
bool IsLeftMouseButtonPressed();
bool IsRightMouseButtonPressed();
vec2i GetCursorPosition();
} // namespace gfx
