#include "app.h"
#include "logger.h"
#include "renderer.h"
#include <SDL_timer.h>
#include <SDL_video.h>

gfx::App* app;

gfx::Mesh cube;

void DrawStats(gfx::App* app) {
    ImGui::Begin("Dummy", 0,
                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);

    ImGui::Text("Frame Time: %0.1fms", app->timestep.frame_time_ms);
    ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
}

int gfx::Run(int argc, char** argv) {

    app = new App();

    gfx::InitLogger();

    // Initialize SDL
    // Create an SDL Window
    // Create the main loop
    // Handle events
    // Render (...)
    // Display results

    int sdl_init = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);
    if (sdl_init < 0) {
        delete app;
        gfx_error(SDL_GetError());
        return EXIT_FAILURE;
    }

    int pos = SDL_WINDOWPOS_CENTERED;
    u32 flags = 0;
    app->window = SDL_CreateWindow("3D Graphics from Scratch", pos, pos, 800, 600, flags);

    if (app->window == nullptr) {
        gfx_error(SDL_GetError());
        return false;
    }

    if (!InitRenderer(app->window, &app->renderer)) {
        return false;
    }

    InitImGui(app->window, app->renderer.sdl_renderer);

    if (!ImportMeshFromSceneFile(&cube, "meshes/cube.obj")) {
        return false;
    }

    while (app->is_running) {
        app->perf_counter = SDL_GetPerformanceCounter();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            ProcessWindowEvents(app, e);
        }

        Renderer* renderer = &app->renderer;

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ClearBuffers(renderer);

				f32 angle = (f32)(SDL_GetTicks());
				DrawMeshUniformColor(renderer, &cube, glm::rotate(glm::mat4(1.0f), angle * 0.0001f, vec3f(0.0f, 1.0f, 0.0)), RGB(127,255,127));
        DrawStats(app);

        ImGui::Render();
        Present(renderer);

        u64 counter_delta = SDL_GetPerformanceCounter() - app->perf_counter;
        app->timestep.frame_time_s = (f64)counter_delta / (f64)SDL_GetPerformanceFrequency();
        app->timestep.frame_time_ms = (f64)counter_delta / (f64)SDL_GetPerformanceFrequency() * 1000.0;
    }

    Cleanup(app);
    SDL_Quit();

    return EXIT_SUCCESS;
}

void gfx::Cleanup(App* app) {
    CleanupRenderer(&app->renderer);
    if (app->window != nullptr) {
        SDL_DestroyWindow(app->window);
    }
    delete app;
}

void gfx::ProcessWindowEvents(App* app, SDL_Event& e) {
    if (e.type == SDL_QUIT) {
        app->is_running = false;
    } else if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_q) {
            app->is_running = false;
        }
    }
}
