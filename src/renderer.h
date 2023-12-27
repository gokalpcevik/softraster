#pragma once

/*
 * 	Interpolation
 *	Depth
 *	Shading
 *	Texturing
 *	Multi-threading <--> Tiled rendering (Pretty much same thing)
 *	Font rendering
 *	Multi-sampling
 * */

#include "SDL2/SDL.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include "logger.h"

#include "mesh.h"
#include "types.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>

namespace gfx {

static size_t constexpr L0_TILE_SIZE = 128;
static size_t constexpr L1_TILE_SIZE = 16;

static constexpr u8 CORNER_VERTICAL_BIT   = (1 << 0);
static constexpr u8 CORNER_HORIZONTAL_BIT = (1 << 1);
static constexpr u8 CORNER_OPPOSITE_MASK  = (CORNER_VERTICAL_BIT | CORNER_HORIZONTAL_BIT);

enum class Corner : u8 { TopLeft = 0b00, TopRight = 0b10, BottomLeft = 0b01, BottomRight = 0b11 };

__forceinline Corner GetOppositeCorner(Corner corner) {
		return static_cast<Corner>(static_cast<u8>(corner) ^ CORNER_OPPOSITE_MASK);
}

struct Tile {
    u64 id;
    u32 index;
    u32 tile_x, tile_y;

    // Tile top-left corner in raster space
    u32 orig_x0, orig_y0;
    // Tile top-right corner
    u32 orig_x1, orig_y1;
    // Tile bottom-left corner
    u32 orig_x2, orig_y2;
    // Tile bottom-right corner
    u32 orig_x3, orig_y3;
};

struct Triangle2D {
    // Which tiles this triangle covers
    u64 coverage_mask = 0x0;

    // Vertex positions (screen-space)
    vec2f vtx_pos0 = vec2f(0.0f);
    vec2f vtx_pos1 = vec2f(0.0f);
    vec2f vtx_pos2 = vec2f(0.0f);

    std::string debug_name = "Triangle";
};

struct Renderer {
    SDL_Texture* framebuffer = nullptr;
    f32 aspect_ratio = 800.0f / 600.0f;
    s32 color_buffer_pitch = 0;
    SDL_Renderer* sdl_renderer = nullptr;
    u32* color_buffer = nullptr;
    f32* w_buffer = nullptr;
    f32* clear_w_buffer = nullptr;

    size_t buffer_width = 0;
    size_t buffer_height = 0;
    size_t buffer_size_in_pixels = 0;
    f32 fBuffer_width = 0.0f;
    f32 fBuffer_heigth = 0.0f;
    // In bytes
    size_t color_buffer_size_in_bytes = 0;
    size_t w_buffer_size_in_bytes = 0;

    // Tile
    std::vector<Tile> l0_tiles;
    size_t l0_tile_count_pitch = 0;
    size_t l0_tile_count = 0;
};

struct InterpolatedTriangle {
    InterpolatedTriangle() = default;

    struct {
        vec2f p0;
        vec2f p1;
        vec2f p2;
    } screen_space;

    f32 v0_pw_rcp = 1.0f;
    f32 v1_pw_rcp = 1.0f;
    f32 v2_pw_rcp = 1.0f;

    struct {
        vec3f v0_color;
        vec3f v1_color;
        vec3f v2_color;
    } attributes_w;
};

void InitImGui(SDL_Window* wnd, SDL_Renderer* renderer);
bool InitRenderer(SDL_Window* window, Renderer* renderer);
void CleanupRenderer(Renderer* renderer);
void ClearBuffers(Renderer* renderer);
void Present(Renderer* renderer);
void PutPixel(Renderer* renderer, u32 x, u32 y, uint32_t color);
void PutPixel(Renderer* renderer, u32 x, u32 y, vec3f const& color);

Corner GetTrivialRejectCorner(f32 A, f32 B);
u64 BinTriangle2D_L0(Renderer* renderer, Triangle2D* triangle);
void GenerateL0Tiles(Renderer* renderer, size_t tile_size = L0_TILE_SIZE);
void DrawTileGrid(Renderer* renderer);
void DrawRect(Renderer* renderer, s32 x0, s32 y0, s32 w, s32 h, u32 color);
void DrawRect(Renderer* renderer, vec2i const& position, vec2i const& size, u32 color);
void DrawTriangle2D(Renderer* renderer, Triangle2D* tri);
void DrawTriangle3D(Renderer* renderer, InterpolatedTriangle* tri);
void DrawMesh(Renderer* renderer, Mesh* mesh, glm::mat4 const& mvp);

__forceinline constexpr u32 RGBA(u8 R, u8 G, u8 B, u8 A = 255) {
    return (u32)B | (u32)(G << 8) | (u32)(R << 16) | (u32)(A << 24);
}

// all u8 -> A|R|G|B = 32 bits
__forceinline constexpr u32 RGB(u8 R, u8 G, u8 B) { return (u32)B | (u32)(G << 8) | (u32)(R << 16) | (u32)(255 << 24); }

__forceinline bool is_point_within_buffer_bounds(s32 x, s32 y) { return x >= 0 && x < 800 && y < 600 && y >= 0; }

// f(x,y)=A*(y-y0)+B*(x-x0), A = (x1-x0), B=(y0-y1)
__forceinline void GetEdgeCoefficients(vec2f const& v0, vec2f const& v1, f32& A, f32& B) {
    A = (v1.x - v0.x);
    B = (v0.y - v1.y);
}

__forceinline float EvaluateEdge(vec2f const& p, vec2f const& v0, vec2f const& v1) {
    return ((v1.x - v0.x) * (p.y - v0.y) - (v1.y - v0.y) * (p.x - v0.x));
}

__forceinline vec2f GetTileCornerPosition(Corner corner, Tile const& tile) {
    switch (corner) {
    case Corner::TopLeft:
				return {(f32)tile.orig_x0, (f32)tile.orig_y0};
    case Corner::TopRight:
				return {(f32)tile.orig_x1, (f32)tile.orig_y1};
    case Corner::BottomLeft:
				return {(f32)tile.orig_x2, (f32)tile.orig_y2};
    case Corner::BottomRight:
				return {(f32)tile.orig_x3, (f32)tile.orig_y3};
    }
}

/*
 * Winding order is expected to be CCW.
 * */
__forceinline bool IsPointInsideTriangle(vec2f const& p, vec2f const& v0, vec2f const& v1, vec2f const& v2) {
    float E01 = EvaluateEdge(p, v0, v1);
    float E12 = EvaluateEdge(p, v1, v2);
    float E20 = EvaluateEdge(p, v2, v0);
    return E01 <= 0.0f && E12 <= 0.0f && E20 <= 0.0f;
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

} // namespace gfx
