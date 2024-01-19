#include "flying_camera_controller.h"

namespace gfx {
void update_flying_camera_controller(FlyingCameraController* camera, f32 dt) {
    if (!camera->is_enabled)
        return;

    int x, y = 0;
    vec3f forward(0.0f, 0.0f, -1.0f);
    vec3f right(0.0f);

    SDL_GetRelativeMouseState(&x, &y);

    f32 dx = (f32)x * camera->mouse_sensitivity * dt;
    f32 dy = (f32)y * camera->mouse_sensitivity * dt;

    camera->yaw -= dx;
    camera->pitch -= dy;
    camera->pitch = glm::clamp(camera->pitch, -89.99f, 89.99f);
    camera->yaw = std::fmodf(camera->yaw, 360.0f);
    f32 pitch_rad = camera->pitch / 180.0f * (f32)M_PI;
    f32 yaw_rad = camera->yaw / 180.0f * (f32)M_PI;

    glm::mat4 m = glm::rotate(glm::mat4(1.0f), yaw_rad, vec3f(0.0f, 1.0f, 0.0f)) *
                  glm::rotate(glm::mat4(1.0f), pitch_rad, vec3f(1.0f, 0.0f, 0.0f));

    forward = glm::normalize(m * vec4f(forward, 0.0f));
    right = glm::cross(vec3f(forward), vec3f(0.0f, 1.0f, 0.0));
		

    if (IsKeyPressed(SDL_SCANCODE_W))
        camera->position += forward * camera->speed * dt;
    else if (IsKeyPressed(SDL_SCANCODE_S))
        camera->position -= forward * camera->speed * dt;

    if (IsKeyPressed(SDL_SCANCODE_D))
        camera->position += right * camera->speed * dt;
    else if (IsKeyPressed(SDL_SCANCODE_A))
        camera->position -= right * camera->speed * dt;

    camera->view_transform = glm::lookAtRH(camera->position, camera->position + forward, vec3f(0.0f, 1.0f, 0.0));
}
} // namespace gfx
