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

    renderer->color_buffer_pitch = sizeof(u32) * w;

    // Allocate color buffer (CpU)
    renderer->color_buffer = new u32[w * h];

    renderer->w_buffer = new f32[w * h];
    renderer->clear_w_buffer = new f32[w * h];

    // Clear W buffer is where all values are 0.0f.
    // We W buffer for the visibility problem to avoid computing the actual depth value (camera space Z value of a given
    // pixel)
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            renderer->clear_w_buffer[y * w + x] = 0.0f;
        }
    }

    if (renderer->color_buffer == nullptr) {
        gfx_error("Color buffer could not be allocated.");
        return false;
    }

    renderer->buffer_width = w;
    renderer->buffer_height = h;
    renderer->fBuffer_width = (f32)w;
    renderer->fBuffer_heigth = (f32)h;
    renderer->buffer_size_in_pixels = w * h;
    renderer->color_buffer_size_in_bytes = sizeof(u32) * (size_t)w * (size_t)h;
    renderer->w_buffer_size_in_bytes = sizeof(f32) * (size_t)w * (size_t)h;

    GenerateL0Tiles(renderer, L0_TILE_SIZE);
    return true;
}

Corner GetTrivialRejectCorner(f32 A, f32 B) {
    if (A == 0.0f || B == 0.0f) {
        return Corner::BottomLeft;
    }

    bool A_is_positive = A > 0.0f;
    bool B_is_positive = B > 0.0f;

    if (A_is_positive) {
        if (B_is_positive) {
            return Corner::TopLeft;
        } else {
            return Corner::TopRight;
        }
    } else {
        if (B_is_positive) {
            return Corner::BottomLeft;
        } else {
            return Corner::BottomRight;
        }
    }
}

u64 BinTriangle2D_L0(Renderer* renderer, Triangle2D* triangle) {
    // These can be pre-computed when triangle is constructed. (Also should be recomputed when triangle is transformed)
    f32 A01, B01, A12, B12, A20, B20 = 0.0f;

    GetEdgeCoefficients(triangle->vtx_pos0, triangle->vtx_pos1, A01, B01);
    GetEdgeCoefficients(triangle->vtx_pos1, triangle->vtx_pos2, A12, B12);
    GetEdgeCoefficients(triangle->vtx_pos2, triangle->vtx_pos0, A20, B20);

    Corner E01_TR_corner = GetTrivialRejectCorner(A01, B01);
    Corner E12_TR_corner = GetTrivialRejectCorner(A12, B12);
    Corner E20_TR_corner = GetTrivialRejectCorner(A20, B20);

    ImGui::Begin("Dummy", 0,
                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
    ImGui::SetWindowSize(ImVec2(renderer->fBuffer_width, renderer->fBuffer_heigth));
    ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (Tile const& tile : renderer->l0_tiles) {

        ImVec2 tile_text_pos = {(f32)tile.orig_x0, (f32)tile.orig_y0};

        // @TODO: clean tis shi up
        // Edge 01 -- TRC = Trivial reject corner
        vec2f E01_TRC_pos = GetTileCornerPosition(E01_TR_corner, tile);
        f32 E01_TRC_value = A01 * (E01_TRC_pos.y - triangle->vtx_pos0.y) + B01 * (E01_TRC_pos.x - triangle->vtx_pos0.x);

        if (E01_TRC_value > 0.0f) {
            draw_list->AddText(tile_text_pos, IM_COL32(255, 0, 0, 255),
                               fmt::format("Tile {0} (TR)", tile.index).c_str());
            continue;
        }

        // Edge 12
        vec2f E12_TRC_pos = GetTileCornerPosition(E12_TR_corner, tile);
        f32 E12_TRC_value = A12 * (E12_TRC_pos.y - triangle->vtx_pos1.y) + B12 * (E12_TRC_pos.x - triangle->vtx_pos1.x);

        if (E12_TRC_value > 0.0f) {
            draw_list->AddText(tile_text_pos, IM_COL32(255, 0, 0, 255),
                               fmt::format("Tile {0} (TR)", tile.index).c_str());
            continue;
        }

        vec2f E20_TRC_pos = GetTileCornerPosition(E20_TR_corner, tile);
        f32 E20_TRC_value = A20 * (E20_TRC_pos.y - triangle->vtx_pos2.y) + B20 * (E20_TRC_pos.x - triangle->vtx_pos2.x);

        if (E20_TRC_value > 0.0f) {
            draw_list->AddText(tile_text_pos, IM_COL32(255, 0, 0, 255),
                               fmt::format("Tile {0} (TR)", tile.index).c_str());
            continue;
        }

        // I'm sure there is some clever shit we can do to avoid calculating this all over.
        vec2f E01_TAC_pos = GetTileCornerPosition(GetOppositeCorner(E01_TR_corner), tile);
        vec2f E12_TAC_pos = GetTileCornerPosition(GetOppositeCorner(E12_TR_corner), tile);
        vec2f E20_TAC_pos = GetTileCornerPosition(GetOppositeCorner(E20_TR_corner), tile);

        f32 E01_TAC_value = A01 * (E01_TAC_pos.y - triangle->vtx_pos0.y) + B01 * (E01_TAC_pos.x - triangle->vtx_pos0.x);

        f32 E12_TAC_value = A12 * (E12_TAC_pos.y - triangle->vtx_pos1.y) + B12 * (E12_TAC_pos.x - triangle->vtx_pos1.x);

        f32 E20_TAC_value = A20 * (E20_TAC_pos.y - triangle->vtx_pos2.y) + B20 * (E20_TAC_pos.x - triangle->vtx_pos2.x);

        if (E01_TAC_value < 0.0f && E12_TAC_value < 0.0f && E20_TAC_value < 0.0f) {
            draw_list->AddText(tile_text_pos, IM_COL32(0, 255, 0, 255),
                               fmt::format("Tile {0} (TA)", tile.index).c_str());
            continue;
        }

        draw_list->AddText(tile_text_pos, IM_COL32_WHITE, fmt::format("Tile {0}", tile.index).c_str());
    }

    constexpr u32 GRID_COLOR = IM_COL32(50, 50, 125, 255);
    size_t const pitch = renderer->l0_tile_count_pitch;

    for (size_t tile_y = 0; tile_y < pitch; ++tile_y) {
        // Horizontal lines
        ImVec2 horizontal_start = {0.0f, static_cast<f32>(tile_y * L0_TILE_SIZE)};
        ImVec2 horizontal_end = {renderer->fBuffer_width, horizontal_start.y};
        draw_list->AddLine(horizontal_start, horizontal_end, GRID_COLOR);

        // Vertical lines + Tile ID text
        for (size_t tile_x = 0; tile_x < pitch; ++tile_x) {
            ImVec2 vertical_start = {static_cast<f32>(tile_x * L0_TILE_SIZE), 0.0f};
            ImVec2 vertical_end = {vertical_start.x, renderer->fBuffer_heigth};
            draw_list->AddLine(vertical_start, vertical_end, GRID_COLOR);
        }
    }
    ImGui::PopStyleVar(3);

    ImGui::End();
    return 0;
}

void GenerateL0Tiles(Renderer* renderer, size_t tile_size) {
    renderer->l0_tile_count_pitch =
        std::ceil((f32)std::max(renderer->buffer_width, renderer->buffer_height) / L0_TILE_SIZE);
    renderer->l0_tile_count = renderer->l0_tile_count_pitch * renderer->l0_tile_count_pitch;

    renderer->l0_tiles.resize(renderer->l0_tile_count);

    for (size_t tile_y = 0; tile_y < renderer->l0_tile_count_pitch; ++tile_y) {
        for (size_t tile_x = 0; tile_x < renderer->l0_tile_count_pitch; ++tile_x) {
            u32 index = tile_y * renderer->l0_tile_count_pitch + tile_x;
            u64 id = (1 << index);

            Tile& t = renderer->l0_tiles[index];
            t.index = index;
            t.id = id;
            t.tile_x = tile_x;
            t.tile_y = tile_y;
            // Top-left
            t.orig_x0 = tile_x * L0_TILE_SIZE;
            t.orig_y0 = tile_y * L0_TILE_SIZE;
            // Top-right
            t.orig_x1 = t.orig_x0 + L0_TILE_SIZE;
            t.orig_y1 = t.orig_y0;
            // Bottom-left
            t.orig_x2 = t.orig_x0;
            t.orig_y2 = t.orig_y0 + L0_TILE_SIZE;
            // Bottom-right
            t.orig_x3 = t.orig_x1;
            t.orig_y3 = t.orig_y1 + L0_TILE_SIZE;
        }
    }
}

void DrawTileGrid(Renderer* renderer) {

    ImGui::Begin("Dummy", 0,
                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
    ImGui::SetWindowSize(ImVec2(renderer->fBuffer_width, renderer->fBuffer_heigth));
    ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));

    constexpr u32 GRID_COLOR = IM_COL32(50, 50, 125, 255);
    size_t const pitch = renderer->l0_tile_count_pitch;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    for (size_t tile_y = 0; tile_y < pitch; ++tile_y) {
        // Horizontal lines
        ImVec2 horizontal_start = {0.0f, static_cast<f32>(tile_y * L0_TILE_SIZE)};
        ImVec2 horizontal_end = {renderer->fBuffer_width, horizontal_start.y};
        draw_list->AddLine(horizontal_start, horizontal_end, GRID_COLOR);

        // Vertical lines + Tile ID text
        for (size_t tile_x = 0; tile_x < pitch; ++tile_x) {
            ImVec2 vertical_start = {static_cast<f32>(tile_x * L0_TILE_SIZE), 0.0f};
            ImVec2 vertical_end = {vertical_start.x, renderer->fBuffer_heigth};
            draw_list->AddLine(vertical_start, vertical_end, GRID_COLOR);

            u32 index = tile_y * pitch + tile_x;
            ImVec2 text_loc = {vertical_start.x + 4.0f, horizontal_start.y};
            draw_list->AddText(text_loc, IM_COL32_WHITE, fmt::format("Tile {0}", (index)).c_str());
        }
    }
    ImGui::End();
}

void ClearBuffers(Renderer* renderer) {
    std::memset(renderer->color_buffer, 0x00, renderer->color_buffer_size_in_bytes);
    std::memcpy(renderer->w_buffer, renderer->clear_w_buffer, renderer->w_buffer_size_in_bytes);
}

void Present(Renderer* renderer) {
    SDL_UpdateTexture(renderer->framebuffer, nullptr, renderer->color_buffer, renderer->color_buffer_pitch);
    SDL_RenderCopy(renderer->sdl_renderer, renderer->framebuffer, nullptr, nullptr);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer->sdl_renderer);
}

void PutPixel(Renderer* renderer, u32 x, u32 y, u32 color) {
    renderer->color_buffer[y * renderer->buffer_width + x] = color;
}

void PutPixel(Renderer* renderer, u32 x, u32 y, vec3f const& color) {
    vec3f color_scaled = color * 255.0f;
    u32 color_8bpc = RGB((u8)color_scaled.x, (u8)color_scaled.y, (u8)color_scaled.z);
    renderer->color_buffer[y * renderer->buffer_width + x] = color_8bpc;
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

void DrawMesh(Renderer* renderer, Mesh* mesh, glm::mat4 const& mvp) {

    for (size_t triangle_index = 0; triangle_index < mesh->triangles.size(); ++triangle_index) {
        u32 index0 = mesh->triangles[triangle_index].indices[0];
        u32 index1 = mesh->triangles[triangle_index].indices[1];
        u32 index2 = mesh->triangles[triangle_index].indices[2];

        vec4f v0_clip = mvp * vec4f(mesh->vertices[index0], 1.0f);
        vec4f v1_clip = mvp * vec4f(mesh->vertices[index1], 1.0f);
        vec4f v2_clip = mvp * vec4f(mesh->vertices[index2], 1.0f);

        if (v0_clip.w < 1.0f || v1_clip.w < 1.0f || v2_clip.w < 1.0f)
            continue;

        vec3f v0_ndc = vec3f(v0_clip / v0_clip.w);
        vec3f v1_ndc = vec3f(v1_clip / v1_clip.w);
        vec3f v2_ndc = vec3f(v2_clip / v2_clip.w);

        vec2f v0_screen_space_pretransform = vec2f(v0_ndc) + vec2f(1.0f) / 2.0f;
        vec2f v1_screen_space_pretransform = vec2f(v1_ndc) + vec2f(1.0f) / 2.0f;
        vec2f v2_screen_space_pretransform = vec2f(v2_ndc) + vec2f(1.0f) / 2.0f;

        // Screen-space (or viewport space) is top-left origin, the upper code has bottom-left origin
        v0_screen_space_pretransform.y = 1.0f - v0_screen_space_pretransform.y;
        v1_screen_space_pretransform.y = 1.0f - v1_screen_space_pretransform.y;
        v2_screen_space_pretransform.y = 1.0f - v2_screen_space_pretransform.y;

        InterpolatedTriangle triangle{};
        triangle.screen_space.p0 = (v0_screen_space_pretransform)*vec2f(800.0f, 600.0f);
        triangle.screen_space.p1 = (v1_screen_space_pretransform)*vec2f(800.0f, 600.0f);
        triangle.screen_space.p2 = (v2_screen_space_pretransform)*vec2f(800.0f, 600.0f);

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

        // ImGui::PushID(triangle_index);
        // ImGui::Text("Triangle Index: %zu", triangle_index);
        // ImGui::Text("Vertex 0: [z: %.4f] [1/w: %.4f]", v0_ndc.z, triangle.v0_pw_rcp);
        // ImGui::Text("Vertex 1: [z: %.4f] [1/w: %.4f]", v1_ndc.z, triangle.v1_pw_rcp);
        // ImGui::Text("Vertex 2: [z: %.4f] [1/w: %.4f]", v2_ndc.z, triangle.v2_pw_rcp);
        // ImGui::Separator();
        // ImGui::PopID();

        // Step 2
        triangle.attributes_w.v0_color = vec3f(1.0f, 0.0f, 0.0f) / v0_clip.w;
        triangle.attributes_w.v1_color = vec3f(0.0f, 1.0f, 0.0f) / v1_clip.w;
        triangle.attributes_w.v2_color = vec3f(0.0f, 0.0f, 1.0f) / v2_clip.w;

        DrawTriangle3D(renderer, &triangle);
    }
}

void DrawTriangle2D(Renderer* renderer, Triangle2D* tri) {
    // tri AABB
    vec2f origin, size;
    GetTriangleAABB(tri->vtx_pos0, tri->vtx_pos1, tri->vtx_pos2, origin, size);
    // origin + size
    vec2f origin_plus_size = origin + size;
    // Go over the triangle rect.
    for (u32 y = (u32)(origin.y - 0.5f); y <= (u32)origin_plus_size.y; ++y) {
        for (u32 x = (u32)(origin.x - 0.5f); x <= (u32)origin_plus_size.x; ++x) {
            vec2f sample{(f32)x + 0.5f, (f32)y + 0.5f};
            // Check if the point in the AABB rect lies in the triangle, if so put the pixel in it.
            if (IsPointInsideTriangle(sample, tri->vtx_pos0, tri->vtx_pos1, tri->vtx_pos2)) {
                PutPixel(renderer, (s32)sample.x, (s32)sample.y, RGB(50, 50, 50));
            }
        }
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

            if (x >= renderer->buffer_width || y >= renderer->buffer_height)
                continue;

            f32 E01 = EvaluateEdge(sample, tri->screen_space.p0, tri->screen_space.p1);
            f32 E12 = EvaluateEdge(sample, tri->screen_space.p1, tri->screen_space.p2);
            f32 E20 = EvaluateEdge(sample, tri->screen_space.p2, tri->screen_space.p0);

            // @TODO: check inside-outside with tie-breaking rules
            bool is_point_inside_triangle = E01 <= 0.0f && E12 <= 0.0f && E20 <= 0.0f;

            if (is_point_inside_triangle) {
                f32 parallelogram_area = EvaluateEdge(tri->screen_space.p0, tri->screen_space.p1, tri->screen_space.p2);
                // Barycentric coordinates
                f32 lambda0 = E12 / parallelogram_area;
                f32 lambda1 = E20 / parallelogram_area;
                f32 lambda2 = E01 / parallelogram_area;

                // 1/P~w interpolated
                f32 rcp_pw_interp = lambda0 * tri->v0_pw_rcp + lambda1 * tri->v1_pw_rcp + lambda2 * tri->v2_pw_rcp;

                // 1/z
                f32* w_at_pixel = &renderer->w_buffer[y * renderer->buffer_width + x];

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

void CleanupRenderer(Renderer* renderer) {

    if (renderer->color_buffer != nullptr) {
        delete[] renderer->color_buffer;
    }

    if (renderer->w_buffer != nullptr) {
        delete[] renderer->w_buffer;
    }

    if (renderer->clear_w_buffer != nullptr) {
        delete[] renderer->clear_w_buffer;
    }

    if (renderer->framebuffer != nullptr) {
        SDL_DestroyTexture(renderer->framebuffer);
    }

    if (renderer->sdl_renderer != nullptr) {
        SDL_DestroyRenderer(renderer->sdl_renderer);
    }
}
} // namespace gfx
