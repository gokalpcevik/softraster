#pragma once
#include "SDL2/SDL_mouse.h"
#include "input.h"
#include "types.h"
#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>

namespace gfx {
struct FlyingCameraController {
    vec3f position = vec3f{0.0f, 0.0f, 0.0f};
    glm::mat4 view_transform = glm::lookAtRH({0.0f, 0.0f, 0.0f}, vec3f{0.0f, 0.0f, -10.0f}, {0.0, 1.0f, 0.0f});
    float pitch, yaw = 0.0f;
    float speed = 10.0f;
    float mouse_sensitivity = 1.0f;
    bool is_enabled = false;
};

void update_flying_camera_controller(FlyingCameraController* camera, f32 dt);

} // namespace gfx
