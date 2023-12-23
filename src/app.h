#pragma once
#include "SDL2/SDL.h"
#include "logger.h"
#include "renderer.h"
#include <cstdint>

namespace gfx {

struct App {
    bool is_running = true;
    SDL_Window* window;
    Renderer renderer;
    struct {
        f64 frame_time_s = 0.0;
        f64 frame_time_ms = 0.0;
    } timestep;
    u64 perf_counter = 0;
};

static App* app;
__forceinline App* GetApp() { return app; }

int Run(int argc, char** argv);
void Cleanup(App* app);
void ProcessWindowEvents(App* app, SDL_Event& e);
} // namespace gfx
