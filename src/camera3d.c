#include "camera3d.h"
#include "sprite.h"
#include <math.h>
#include <string.h>

#define CLAMP(v, min_v, max_v) ((v) < (min_v) ? (min_v) : ((v) > (max_v) ? (max_v) : (v)))

static float AngleDiff(float a, float b) {
    float diff = fmodf(a - b + (float)M_PI, 2.0f * (float)M_PI);
    if (diff < 0.0f) diff += 2.0f * (float)M_PI;
    return diff - (float)M_PI;
}

void Camera3D_Init(Camera3D* cam, ViewMode initial_mode) {
    if (!cam) return;
    memset(cam, 0, sizeof(Camera3D));
    
    cam->mode = initial_mode;
    cam->aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    cam->near_z = 8.0f;
    cam->far_z = 2400.0f;
    cam->fov = 68.0f * ((float)M_PI / 180.0f);
    cam->focal_length = ((float)WINDOW_HEIGHT * 0.5f) / tanf(cam->fov * 0.5f);
    
    cam->mouse_sensitivity = 0.0045f;
    cam->key_orbit_speed = 2.4f;
    cam->smooth_follow_enabled = true;
    
    Camera3D_SetMode(cam, initial_mode);
}

void Camera3D_SetMode(Camera3D* cam, ViewMode new_mode) {
    if (!cam) return;
    cam->mode = new_mode;
    
    if (cam->mode == VIEW_3D_THIRD_PERSON) {
        cam->target_distance = 420.0f;
        cam->distance = 420.0f;
        cam->target_pitch = 0.54f; // ~31 degrees downward
        cam->pitch = 0.54f;
        cam->height_offset = 26.0f;
        cam->fov = 68.0f * ((float)M_PI / 180.0f);
    } else if (cam->mode == VIEW_3D_TOPDOWN) {
        cam->target_distance = 620.0f;
        cam->distance = 620.0f;
        cam->target_pitch = 0.98f; // ~56 degrees high angle
        cam->pitch = 0.98f;
        cam->height_offset = 20.0f;
        cam->fov = 60.0f * ((float)M_PI / 180.0f);
    } else { // VIEW_2D_ORTHO
        cam->target_distance = 400.0f;
        cam->distance = 400.0f;
        cam->target_pitch = 0.0f;
        cam->pitch = 0.0f;
        cam->height_offset = 0.0f;
    }
    
    cam->focal_length = ((float)WINDOW_HEIGHT * 0.5f) / tanf(cam->fov * 0.5f);
}

void Camera3D_CycleMode(Camera3D* cam) {
    if (!cam) return;
    ViewMode next = (ViewMode)((cam->mode + 1) % VIEW_MODE_COUNT);
    Camera3D_SetMode(cam, next);
}

void Camera3D_Snap(Camera3D* cam, Vec3 target_pos) {
    if (!cam) return;
    cam->target.x = target_pos.x;
    cam->target.y = target_pos.y + cam->height_offset;
    cam->target.z = target_pos.z;
    cam->smooth_target = cam->target;
    cam->distance = cam->target_distance;
    cam->yaw = cam->target_yaw;
    cam->pitch = cam->target_pitch;
    
    float cy = cosf(cam->yaw);
    float sy = sinf(cam->yaw);
    float cp = cosf(cam->pitch);
    float sp = sinf(cam->pitch);
    
    cam->forward.x = sy * cp;
    cam->forward.y = -sp;
    cam->forward.z = -cy * cp;
    
    cam->right.x = cy;
    cam->right.y = 0.0f;
    cam->right.z = sy;
    
    cam->up.x = sy * sp;
    cam->up.y = cp;
    cam->up.z = -cy * sp;
    
    cam->pos.x = cam->smooth_target.x - cam->forward.x * cam->distance;
    cam->pos.y = cam->smooth_target.y - cam->forward.y * cam->distance;
    cam->pos.z = cam->smooth_target.z - cam->forward.z * cam->distance;
}

void Camera3D_ResetBehindPlayer(Camera3D* cam, float player_facing_angle) {
    if (!cam) return;
    cam->target_yaw = player_facing_angle + (float)M_PI * 0.5f;
}

const char* Camera3D_GetModeName(ViewMode mode) {
    switch (mode) {
        case VIEW_3D_THIRD_PERSON: return "3D Third-Person (Over-the-Shoulder)";
        case VIEW_3D_TOPDOWN:      return "3D Top-Down (Perspective)";
        case VIEW_2D_ORTHO:        return "2D Orthogonal (Classic)";
        default:                   return "Unknown View";
    }
}

void Camera3D_Update(Camera3D* cam, Vec3 player_pos, float dt) {
    if (!cam) return;
    
    // Smooth target follow
    cam->target.x = player_pos.x;
    cam->target.y = player_pos.y + cam->height_offset;
    cam->target.z = player_pos.z;
    
    if (cam->smooth_follow_enabled && dt > 0.0001f) {
        float follow_speed = 12.0f * dt;
        if (follow_speed > 1.0f) follow_speed = 1.0f;
        cam->smooth_target.x += (cam->target.x - cam->smooth_target.x) * follow_speed;
        cam->smooth_target.y += (cam->target.y - cam->smooth_target.y) * follow_speed;
        cam->smooth_target.z += (cam->target.z - cam->smooth_target.z) * follow_speed;
    } else {
        cam->smooth_target = cam->target;
    }
    
    // Smooth zoom distance interpolation
    if (dt > 0.0001f) {
        float zoom_speed = 10.0f * dt;
        if (zoom_speed > 1.0f) zoom_speed = 1.0f;
        cam->distance += (cam->target_distance - cam->distance) * zoom_speed;
    }
    
    // Clamp pitch to prevent flipping
    cam->pitch = CLAMP(cam->pitch, 0.12f, 1.35f);
    cam->target_pitch = CLAMP(cam->target_pitch, 0.12f, 1.35f);
    
    // Smooth yaw & pitch
    if (dt > 0.0001f) {
        float rot_speed = 14.0f * dt;
        if (rot_speed > 1.0f) rot_speed = 1.0f;
        cam->yaw += AngleDiff(cam->target_yaw, cam->yaw) * rot_speed;
        cam->pitch += (cam->target_pitch - cam->pitch) * rot_speed;
    }
    
    // Compute Basis Vectors
    float cy = cosf(cam->yaw);
    float sy = sinf(cam->yaw);
    float cp = cosf(cam->pitch);
    float sp = sinf(cam->pitch);
    
    // Forward: looking towards target
    cam->forward.x = sy * cp;
    cam->forward.y = -sp;
    cam->forward.z = -cy * cp;
    
    // Right: horizontal right vector
    cam->right.x = cy;
    cam->right.y = 0.0f;
    cam->right.z = sy;
    
    // Up = Right x Forward
    cam->up.x = sy * sp;
    cam->up.y = cp;
    cam->up.z = -cy * sp;
    
    // Eye position = smooth_target - forward * distance
    cam->pos.x = cam->smooth_target.x - cam->forward.x * cam->distance;
    cam->pos.y = cam->smooth_target.y - cam->forward.y * cam->distance;
    cam->pos.z = cam->smooth_target.z - cam->forward.z * cam->distance;
}

void Camera3D_HandleEvent(Camera3D* cam, const SDL_Event* event) {
    if (!cam || !event) return;
    
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (event->button.button == SDL_BUTTON_RIGHT || event->button.button == SDL_BUTTON_MIDDLE) {
            cam->mouse_orbit_active = true;
            cam->last_mouse_x = event->button.x;
            cam->last_mouse_y = event->button.y;
        }
    } else if (event->type == SDL_MOUSEBUTTONUP) {
        if (event->button.button == SDL_BUTTON_RIGHT || event->button.button == SDL_BUTTON_MIDDLE) {
            cam->mouse_orbit_active = false;
        }
    } else if (event->type == SDL_MOUSEMOTION) {
        if (cam->mouse_orbit_active) {
            int dx = event->motion.x - cam->last_mouse_x;
            int dy = event->motion.y - cam->last_mouse_y;
            cam->last_mouse_x = event->motion.x;
            cam->last_mouse_y = event->motion.y;
            
            cam->target_yaw += (float)dx * cam->mouse_sensitivity;
            cam->target_pitch += (float)dy * cam->mouse_sensitivity;
            cam->target_pitch = CLAMP(cam->target_pitch, 0.12f, 1.35f);
        }
    } else if (event->type == SDL_MOUSEWHEEL) {
        // Zoom in/out with scroll wheel
        if (event->wheel.y != 0) {
            cam->target_distance -= (float)event->wheel.y * 36.0f;
            cam->target_distance = CLAMP(cam->target_distance, 180.0f, 950.0f);
        }
    }
}

void Camera3D_HandleInput(Camera3D* cam, const Uint8* keystate, float dt) {
    if (!cam || !keystate) return;
    
    // Keyboard camera orbit (Arrow keys or '[' / ']')
    float orbit_delta = cam->key_orbit_speed * dt;
    if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_LEFTBRACKET]) {
        cam->target_yaw -= orbit_delta;
    }
    if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_RIGHTBRACKET]) {
        cam->target_yaw += orbit_delta;
    }
    if (keystate[SDL_SCANCODE_UP]) {
        cam->target_pitch -= orbit_delta * 0.65f;
        cam->target_pitch = CLAMP(cam->target_pitch, 0.12f, 1.35f);
    }
    if (keystate[SDL_SCANCODE_DOWN]) {
        cam->target_pitch += orbit_delta * 0.65f;
        cam->target_pitch = CLAMP(cam->target_pitch, 0.12f, 1.35f);
    }
    
    // Zoom in/out via PageUp / PageDown or Keypad +/-
    if (keystate[SDL_SCANCODE_PAGEUP] || keystate[SDL_SCANCODE_KP_PLUS]) {
        cam->target_distance -= 240.0f * dt;
        cam->target_distance = CLAMP(cam->target_distance, 180.0f, 950.0f);
    }
    if (keystate[SDL_SCANCODE_PAGEDOWN] || keystate[SDL_SCANCODE_KP_MINUS]) {
        cam->target_distance += 240.0f * dt;
        cam->target_distance = CLAMP(cam->target_distance, 180.0f, 950.0f);
    }
}

void Camera3D_WorldToView(const Camera3D* cam, Vec3 world, Vec3* out_view) {
    if (!cam || !out_view) return;
    float dx = world.x - cam->pos.x;
    float dy = world.y - cam->pos.y;
    float dz = world.z - cam->pos.z;
    
    out_view->x = dx * cam->right.x   + dy * cam->right.y   + dz * cam->right.z;
    out_view->y = dx * cam->up.x      + dy * cam->up.y      + dz * cam->up.z;
    out_view->z = dx * cam->forward.x + dy * cam->forward.y + dz * cam->forward.z;
}

bool Camera3D_ViewToScreen(const Camera3D* cam, Vec3 view, float* out_screen_x, float* out_screen_y) {
    if (!cam || !out_screen_x || !out_screen_y) return false;
    if (view.z <= cam->near_z) return false;
    
    float inv_z = cam->focal_length / view.z;
    *out_screen_x = ((float)WINDOW_WIDTH * 0.5f) + view.x * inv_z;
    *out_screen_y = ((float)WINDOW_HEIGHT * 0.5f) - view.y * inv_z;
    return true;
}

bool Camera3D_Project(const Camera3D* cam, Vec3 world, float* out_screen_x, float* out_screen_y, float* out_depth) {
    if (!cam) return false;
    Vec3 view;
    Camera3D_WorldToView(cam, world, &view);
    if (out_depth) *out_depth = view.z;
    return Camera3D_ViewToScreen(cam, view, out_screen_x, out_screen_y);
}

bool Camera3D_ScreenToGround(const Camera3D* cam, float screen_x, float screen_y, float ground_y, Vec3* out_world) {
    if (!cam || !out_world) return false;
    
    // View ray
    float vx = (screen_x - (float)WINDOW_WIDTH * 0.5f) / cam->focal_length;
    float vy = -(screen_y - (float)WINDOW_HEIGHT * 0.5f) / cam->focal_length;
    float vz = 1.0f;
    
    // World ray direction
    Vec3 r_dir = {
        vx * cam->right.x   + vy * cam->up.x   + vz * cam->forward.x,
        vx * cam->right.y   + vy * cam->up.y   + vz * cam->forward.y,
        vx * cam->right.z   + vy * cam->up.z   + vz * cam->forward.z
    };
    
    if (fabsf(r_dir.y) < 0.0001f) return false;
    
    float t = (ground_y - cam->pos.y) / r_dir.y;
    if (t <= 0.0f || t > 3500.0f) {
        // Ray points towards or above horizon: project forward along horizontal direction
        float hx = r_dir.x;
        float hz = r_dir.z;
        float hlen = sqrtf(hx * hx + hz * hz);
        if (hlen > 0.001f) {
            hx /= hlen;
            hz /= hlen;
        } else {
            hx = cam->forward.x;
            hz = cam->forward.z;
        }
        out_world->x = cam->smooth_target.x + hx * 900.0f;
        out_world->y = ground_y;
        out_world->z = cam->smooth_target.z + hz * 900.0f;
        return true;
    }
    
    out_world->x = cam->pos.x + r_dir.x * t;
    out_world->y = ground_y;
    out_world->z = cam->pos.z + r_dir.z * t;
    return true;
}

int Camera3D_GetRelativeDirection(const Camera3D* cam, int world_direction) {
    if (!cam || cam->mode == VIEW_2D_ORTHO) {
        return ((world_direction % 8) + 8) % 8;
    }
    
    float world_ang = Sprite_DirectionToAngle(world_direction);
    float dx = cosf(world_ang);
    float dz = sinf(world_ang);
    
    float cy = cosf(cam->yaw);
    float sy = sinf(cam->yaw);
    
    // Project world facing vector onto horizontal camera basis
    float dx_cam = dx * cy + dz * sy;
    float d_fwd  = dx * sy - dz * cy;
    
    float vx_screen = dx_cam;
    float vy_screen = -d_fwd;
    
    return Sprite_VecToDirection(vx_screen, vy_screen, world_direction);
}

void Camera3D_GetMovementVector(const Camera3D* cam, float input_forward, float input_right, float* out_vx, float* out_vz) {
    if (!out_vx || !out_vz) return;
    
    if (!cam || cam->mode == VIEW_2D_ORTHO) {
        *out_vx = input_right;
        *out_vz = -input_forward;
        return;
    }
    
    float cy = cosf(cam->yaw);
    float sy = sinf(cam->yaw);
    
    // Forward is (sy, -cy), Right is (cy, sy)
    *out_vx = input_forward * sy + input_right * cy;
    *out_vz = input_forward * (-cy) + input_right * sy;
}
