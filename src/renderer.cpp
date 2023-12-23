#include "renderer.h"
#include "logger.h"

namespace gfx {

void InitImGui(SDL_Window* wnd, SDL_Renderer* renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui_ImplSDL2_InitForSDLRenderer(wnd, renderer);
    ImGui_ImplSDLRenderer_Init(renderer);
    ImGui::StyleColorsDark();
}

bool InitRenderer(SDL_Window* window, Renderer* renderer) {

    // Create SDL Renderer
    renderer->sdl_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    if (renderer->sdl_renderer == nullptr) {
        gfx_error(SDL_GetError());
        return false;
    }

    // Create SDL Texture (framebuffer)
    s32 w, h;
    SDL_GetWindowSize(window, &w, &h);

    // Going to ignore high DpI stuff for now.
    renderer->framebuffer =
        SDL_CreateTexture(renderer->sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

    if (renderer->framebuffer == nullptr) {
        gfx_error(SDL_GetError());
        return false;
    }

    renderer->fb_pitch = sizeof(u32) * w;

    // Allocate color buffer (CpU)
    renderer->color_buffer = new u32[w * h];

    renderer->w_buffer = new f32[w * h];
    renderer->clear_depth_buffer = new f32[w * h];

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            renderer->clear_depth_buffer[y * w + x] = 0.0f;
        }
    }

    if (renderer->color_buffer == nullptr) {
        gfx_error("Color buffer could not be allocated.");
        return false;
    }

    renderer->color_buffer_width = w;
    renderer->color_buffer_height = h;
    renderer->color_buffer_size_in_bytes = sizeof(u32) * (size_t)w * (size_t)h;
    renderer->depth_buffer_size_in_bytes = sizeof(f32) * (size_t)w * (size_t)h;

    return true;
}

void CleanupRenderer(Renderer* renderer) {
    if (renderer->color_buffer != nullptr) {
        delete[] renderer->color_buffer;
    }

    if (renderer->w_buffer != nullptr) {
        delete[] renderer->w_buffer;
    }

    if (renderer->clear_depth_buffer != nullptr) {
        delete[] renderer->clear_depth_buffer;
    }

    if (renderer->framebuffer != nullptr) {
        SDL_DestroyTexture(renderer->framebuffer);
    }

    if (renderer->sdl_renderer != nullptr) {
        SDL_DestroyRenderer(renderer->sdl_renderer);
    }
}

void ClearBuffers(Renderer* renderer) {
    std::memset(renderer->color_buffer, 0x00, renderer->color_buffer_size_in_bytes);
    std::memcpy(renderer->w_buffer, renderer->clear_depth_buffer, renderer->depth_buffer_size_in_bytes);
}

void Present(Renderer* renderer) {
    SDL_UpdateTexture(renderer->framebuffer, nullptr, renderer->color_buffer, renderer->fb_pitch);
    SDL_RenderCopy(renderer->sdl_renderer, renderer->framebuffer, nullptr, nullptr);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer->sdl_renderer);
}

void PutPixel(Renderer* renderer, s32 x, s32 y, u32 color) {
    renderer->color_buffer[y * renderer->color_buffer_width + x] = color;
}

void PutPixel(Renderer* renderer, s32 x, s32 y, vec3f const& color) {
    vec3f color_scaled = color * 255.0f;
    u32 color_8bpc = RGB((u8)color_scaled.x, (u8)color_scaled.y, (u8)color_scaled.z);
    renderer->color_buffer[y * renderer->color_buffer_width + x] = color_8bpc;
}

void DrawRect(Renderer* renderer, s32 x0, s32 y0, s32 w, s32 h, u32 color) {
    // Most naive approach.
    for (s32 y = y0; y < (y0 + h); ++y) {
        for (s32 x = x0; x < (x0 + w); ++x) {
            PutPixel(renderer, x, y, color);
        }
    }
}

void DrawRect(Renderer* renderer, vec2i const& position, vec2i const& size, u32 color) {
    // Most naive approach.
    for (s32 y = position.y; y < (position.y + size.y); ++y) {
        for (s32 x = position.x; x < (position.x + size.x); ++x) {
            PutPixel(renderer, x, y, color);
        }
    }
}

__forceinline float EvaluateEdge(vec2f const& p, vec2f const& v0, vec2f const& v1) {
    return ((v1.x - v0.x) * (p.y - v0.y)) - ((v1.y - v0.y) * (p.x - v0.x));
}

/*
 * Winding order is expected to be CCW.
 * */
__forceinline bool IsPointInsideTriangle(vec2f const& p, vec2f const& v0, vec2f const& v1, vec2f const& v2) {
    float E01 = EvaluateEdge(p, v0, v1);
    float E12 = EvaluateEdge(p, v1, v2);
    float E20 = EvaluateEdge(p, v2, v0);
    // If the results return 0.0f (highly unlikely but possible), it means the point lies exactly on the edge
    return E01 >= 0.0f && E12 >= 0.0f && E20 >= 0.0f;
}

__forceinline void GetTriangleAABB(vec2f const& p0, vec2f const& p1, vec2f const& p2, vec2f& origin, vec2f& size) {
    float x_min = std::min(p0[0], p1[0]);
    float x_max = std::max(p1[0], p2[0]);

    // Check if the 3rd point's X coord is smaller
    if (p2[0] < x_min)
        x_min = p2[0];

    // Check if the first point's X coord is larger
    if (p0[0] > x_max)
        x_max = p0[0];

    float y_min = std::min(p0[1], p1[1]);
    float y_max = std::max(p0[1], p2[1]);

    // Check if the 3rd point's Y coord is smaller
    if (p2[1] < y_min)
        y_min = p2[1];

    // Check if the first point's Y coord is larger
    if (p1[1] > y_max)
        y_max = p1[1];

    origin = vec2f{x_min, y_min};
    size = vec2f{x_max - x_min, y_max - y_min};
}

void DrawTriangle2DUniformColor(Renderer* renderer, const vec2f& v0, const vec2f& v1, const vec2f& v2, u32 color) {
    // tri AABB
    vec2f origin, size;
    GetTriangleAABB(v0, v1, v2, origin, size);
    // origin + size
    vec2f origin_plus_size = origin + size;
    // Go over the triangle rect.
    for (u32 y = (u32)(origin.y - 0.5f); y <= (u32)origin_plus_size.y; ++y) {
        for (u32 x = (u32)(origin.x - 0.5f); x <= (u32)origin_plus_size.x; ++x) {
            vec2f sample{(f32)x + 0.5f, (f32)y + 0.5f};
            // Check if the point in the AABB rect lies in the triangle, if so put the pixel in it.
            if (IsPointInsideTriangle(sample, v0, v1, v2)) {
                PutPixel(renderer, (s32)sample.x, (s32)sample.y, color);
            }
        }
    }
}

void DrawMeshUniformColor(Renderer* renderer, Mesh* mesh, const glm::mat4& ModelToWorld, u32 color) {

    vec3f eye(0.0f, 8.0f, -8.0f);
    glm::mat4 view = glm::lookAtRH(eye, vec3f(0.0f), vec3f(0.0f, 1.0f, 0.0));
    glm::mat4 projection = glm::perspective(glm::pi<float>() * 0.25f, renderer->aspect_ratio, 0.1f, 100.0f);
    glm::mat4 MVP = projection * view * ModelToWorld;

    for (size_t triangle_index = 0; triangle_index < mesh->triangles.size(); ++triangle_index) {

        u32 index0 = mesh->triangles[triangle_index].indices[0];
        u32 index1 = mesh->triangles[triangle_index].indices[1];
        u32 index2 = mesh->triangles[triangle_index].indices[2];

        vec4f v0_clip = MVP * vec4f(mesh->vertices[index0], 1.0f);
        vec4f v1_clip = MVP * vec4f(mesh->vertices[index1], 1.0f);
        vec4f v2_clip = MVP * vec4f(mesh->vertices[index2], 1.0f);

        vec2f v0_ndc = vec2f(v0_clip / v0_clip.w);
        vec2f v1_ndc = vec2f(v1_clip / v1_clip.w);
        vec2f v2_ndc = vec2f(v2_clip / v2_clip.w);

        InterpolatedTriangle triangle{};

        triangle.screen_space.p0 = (v0_ndc + vec2f(1.0f) / 2.0f) * vec2f(800.0f, 600.0f);
        triangle.screen_space.p1 = (v1_ndc + vec2f(1.0f) / 2.0f) * vec2f(800.0f, 600.0f);
        triangle.screen_space.p2 = (v2_ndc + vec2f(1.0f) / 2.0f) * vec2f(800.0f, 600.0f);

        // Store 1/ndc.w for all vertices (after clipping)
        // Divide all of our vertex attributes and depth by ndc.w, call it U~
        // Interpolate 1/ndc.w according to barycentric coordinates
        // Interpolate all divided(i.e. U~) vertex attributes according to barycentric coordinates
        // Divide the interpolated divided vertex attributes by 1/ndc.w
        // Profit ??

        // Step 1
        triangle.v0_pw_rcp = 1.0f / v0_clip.w;
        triangle.v1_pw_rcp = 1.0f / v1_clip.w;
        triangle.v2_pw_rcp = 1.0f / v2_clip.w;

        // Step 2
        triangle.attributes_w.v0_color = vec3f(1.0f, 0.0f, 0.0f) / v0_clip.w;
        triangle.attributes_w.v1_color = vec3f(0.0f, 1.0f, 0.0f) / v1_clip.w;
        triangle.attributes_w.v2_color = vec3f(0.0f, 0.0f, 1.0f) / v2_clip.w;

        DrawTriangle3D(renderer, &triangle);
    }
}

void DrawTriangle3D(Renderer* renderer, InterpolatedTriangle* tri) {
    // tri AABB
    vec2f origin, size;
    GetTriangleAABB(tri->screen_space.p0, tri->screen_space.p1, tri->screen_space.p2, origin, size);
    // origin + size
    vec2f origin_plus_size = origin + size;
    // Go over the triangle rect.
    for (u32 y = (u32)(origin.y - 0.5f); y <= (u32)origin_plus_size.y; ++y) {
        for (u32 x = (u32)(origin.x - 0.5f); x <= (u32)origin_plus_size.x; ++x) {
            vec2f sample{(f32)x + 0.5f, (f32)y + 0.5f};

            f32 E01 = EvaluateEdge(sample, tri->screen_space.p0, tri->screen_space.p1);
            f32 E12 = EvaluateEdge(sample, tri->screen_space.p1, tri->screen_space.p2);
            f32 E20 = EvaluateEdge(sample, tri->screen_space.p2, tri->screen_space.p0);

            bool is_point_inside_triangle = E01 >= 0.0f && E12 >= 0.0f && E20 >= 0.0f;

            if (is_point_inside_triangle) {
                f32 parallelogram_area = EvaluateEdge(tri->screen_space.p0, tri->screen_space.p1, tri->screen_space.p2);
                // Barycentric coordinates
                f32 lambda0 = E12 / parallelogram_area;
                f32 lambda1 = E20 / parallelogram_area;
                f32 lambda2 = E01 / parallelogram_area;

                // 1/P~w interpolated
                f32 rcp_pw_interp = lambda0 * tri->v0_pw_rcp + lambda1 * tri->v1_pw_rcp + lambda2 * tri->v2_pw_rcp;

                // 1/z
                f32* w_at_pixel = &renderer->w_buffer[y * renderer->color_buffer_width + x];

                if (rcp_pw_interp >= *w_at_pixel) {
                    *w_at_pixel = rcp_pw_interp;
                    vec3f color_interp = lambda0 * tri->attributes_w.v0_color + lambda1 * tri->attributes_w.v1_color +
                                         lambda2 * tri->attributes_w.v2_color;
                    color_interp = color_interp / rcp_pw_interp;
                    PutPixel(renderer, x, y, color_interp);
                }
            }
        }
    }
}

} // namespace gfx
