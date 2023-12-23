/*
 * 	Interpolation
 *	Depth
 *	Shading
 *	Texturing
 *	Multi-threading <--> Tiled rendering (Pretty much same thing)
 *	Font rendering
 *	Multi-sampling
 * */

#pragma once

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
struct Renderer {
    SDL_Texture* framebuffer = nullptr;
    f32 aspect_ratio = 800.0f / 600.0f;
    s32 fb_pitch = 0;
    SDL_Renderer* sdl_renderer = nullptr;
    u32* color_buffer = nullptr;
    f32* w_buffer = nullptr;
    f32* clear_depth_buffer = nullptr;
			
		size_t color_buffer_width = 0;
    size_t color_buffer_height = 0;
    // In bytes
    size_t color_buffer_size_in_bytes = 0;
    size_t depth_buffer_size_in_bytes = 0;
};

__forceinline constexpr u32 RGBA(u8 R, u8 G, u8 B, u8 A = 255) {
    return (u32)B | (u32)(G << 8) | (u32)(R << 16) | (u32)(A << 24);
}

// all u8 -> A|R|G|B = 32 bits
__forceinline constexpr u32 RGB(u8 R, u8 G, u8 B) { return (u32)B | (u32)(G << 8) | (u32)(R << 16) | (u32)(255 << 24); }

void InitImGui(SDL_Window* wnd, SDL_Renderer* renderer);
bool InitRenderer(SDL_Window* window, Renderer* renderer);
void CleanupRenderer(Renderer* renderer);
void ClearBuffers(Renderer* renderer);
void Present(Renderer* renderer);
void PutPixel(Renderer* renderer, s32 x, s32 y, uint32_t color);
void PutPixel(Renderer* renderer, s32 x, s32 y, vec3f const& color);

void DrawRect(Renderer* renderer, s32 x0, s32 y0, s32 w, s32 h, u32 color);
void DrawRect(Renderer* renderer, vec2i const& position, vec2i const& size, u32 color);
void DrawTriangle2DUniformColor(Renderer* renderer, vec2f const& v0, vec2f const& v1, vec2f const& v2, u32 color);

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

void DrawTriangle3D(Renderer* renderer, InterpolatedTriangle* tri);
void DrawMeshUniformColor(Renderer* renderer, Mesh* mesh, glm::mat4 const& ModelToWorld, u32 color);


} // namespace gfx
