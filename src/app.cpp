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
    ImGui::End();
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
    app->window = SDL_CreateWindow("Rasterizer", pos, pos, 800, 600, flags);

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

        if (IsKeyPressed(SDL_SCANCODE_F1)) {
            app->camera_controller.is_enabled = !app->camera_controller.is_enabled;
            app->trap_mouse = !app->trap_mouse;
            SDL_SetRelativeMouseMode((SDL_bool)app->trap_mouse);
        }

        update_flying_camera_controller(&app->camera_controller, app->timestep.frame_time_s);

        Renderer* renderer = &app->renderer;

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ClearBuffers(renderer);

        {
            static vec2f vtx_pos0 = {150.0f, 100.0f};
            static vec2f vtx_pos1 = {400.0f, 400.0f};
            static vec2f vtx_pos2 = {550.0f, 200.0f};
            static Triangle2D test_triangle{ 0,  vtx_pos0, vtx_pos1, vtx_pos2, "Test Triangle"};
						ImGui::Begin("Triangle Settings");
						ImGui::DragFloat2("V0", (float*)&test_triangle.vtx_pos0, 1.f, 0.0f,10000.0f);
						ImGui::DragFloat2("V1", (float*)&test_triangle.vtx_pos1, 1.f, 0.0f,10000.0f);
						ImGui::DragFloat2("V2", (float*)&test_triangle.vtx_pos2, 1.f, 0.0f,10000.0f);
						ImGui::End();
            DrawTriangle2D(renderer, &test_triangle);
            BinTriangle2D_L0(renderer, &test_triangle);
        }

        // Options
        // {
        //     ImGui::Begin("Options");
        //     static bool enable_debug_grid = true;
        //     ImGui::Checkbox("Enable Debug Grid", &enable_debug_grid);
        //     if (enable_debug_grid) {
        //         DrawTileGrid(renderer);
        //     }
        //     ImGui::End();
        // }

        // Mesh
        // {
        //     f32 angle = (f32)(SDL_GetTicks()) * 0.001f;
        //     glm::mat4 mvp =
        //         glm::perspective(glm::pi<float>() / 2.0f, renderer->aspect_ratio, 0.1f, 100.0f) * // Projection
        //         app->camera_controller.view_transform *                                           // View
        //         glm::translate(glm::mat4(1.0f), vec3f(0.0f, 0.0f, -15.0f));                       // Model
        //     DrawMesh(renderer, &cube, mvp);
        // }
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
