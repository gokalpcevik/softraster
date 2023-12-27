#include "input.h"

namespace gfx {
bool IsKeyPressed(SDL_Scancode key) {
    const Uint8* state = SDL_GetKeyboardState(NULL);
    return state[key];
}

bool IsLeftMouseButtonPressed() {
    int64_t buttons = SDL_GetMouseState(NULL, NULL);
    return (buttons & SDL_BUTTON_LMASK) != 0;
}

bool IsRightMouseButtonPressed() {
    int64_t buttons = SDL_GetMouseState(NULL, NULL);
    return (buttons & SDL_BUTTON_RMASK) != 0;
}

vec2i GetCursorPosition() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return vec2i{x, y};
}
} // namespace gfx
