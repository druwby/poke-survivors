#ifndef CAMERA3D_H
#define CAMERA3D_H

#include <SDL.h>
#include <stdbool.h>
#include "config.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declarations
struct GameState;

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef enum {
    VIEW_3D_THIRD_PERSON = 0, // Over-the-shoulder / chase camera behind player (Default)
    VIEW_3D_TOPDOWN      = 1, // High-angle elevated 3D perspective / isometric
    VIEW_2D_ORTHO        = 2, // Classic 2D orthogonal top-down
    VIEW_MODE_COUNT
} ViewMode;

typedef struct Camera3D {
    ViewMode mode;
    
    // Camera Eye & Target in 3D World Space (X = East/West, Y = Elevation/Up, Z = North/South)
    Vec3 pos;              // Eye position
    Vec3 target;           // Look-at point
    Vec3 smooth_target;    // Smoothed interpolation target
    
    // Spherical Coordinates / Orientation
    float yaw;             // Horizontal angle in radians (0 = looking North along -Z)
    float target_yaw;      // Desired yaw for smooth rotation
    float pitch;           // Vertical tilt in radians (downward looking: ~0.45 - 0.75 rad)
    float target_pitch;    // Desired pitch
    float distance;        // Distance from eye to target
    float target_distance; // Desired distance for zoom
    float height_offset;   // Vertical look-at height above target ground (e.g. 24.0f)
    
    // Lens / Frustum / Projection
    float fov;             // Field of view in radians (default 68 deg)
    float aspect;          // Aspect ratio (width / height)
    float near_z;          // Near clipping plane
    float far_z;           // Far clipping plane
    float focal_length;    // (screen_height * 0.5) / tan(fov * 0.5)
    
    // Current Frame Basis Vectors (computed each frame)
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    
    // Input & Orbit Controls
    bool mouse_orbit_active;
    int last_mouse_x;
    int last_mouse_y;
    float mouse_sensitivity;
    float key_orbit_speed;
    bool smooth_follow_enabled;
    bool auto_align_yaw;
} Camera3D;

// Lifecycle
void Camera3D_Init(Camera3D* cam, ViewMode initial_mode);
void Camera3D_SetMode(Camera3D* cam, ViewMode new_mode);
void Camera3D_CycleMode(Camera3D* cam);
void Camera3D_Snap(Camera3D* cam, Vec3 target_pos);
void Camera3D_ResetBehindPlayer(Camera3D* cam, float player_facing_angle);
const char* Camera3D_GetModeName(ViewMode mode);

// Frame Update
void Camera3D_Update(Camera3D* cam, Vec3 player_pos, float dt);

// Input & Event Handling
void Camera3D_HandleEvent(Camera3D* cam, const SDL_Event* event);
void Camera3D_HandleInput(Camera3D* cam, const Uint8* keystate, float dt);

// 3D Math & Projection
void Camera3D_WorldToView(const Camera3D* cam, Vec3 world, Vec3* out_view);
bool Camera3D_ViewToScreen(const Camera3D* cam, Vec3 view, float* out_screen_x, float* out_screen_y);
bool Camera3D_Project(const Camera3D* cam, Vec3 world, float* out_screen_x, float* out_screen_y, float* out_depth);
bool Camera3D_ScreenToGround(const Camera3D* cam, float screen_x, float screen_y, float ground_y, Vec3* out_world);

// Sprite Billboard Direction Mapping (Relative to 3D Camera Yaw)
int Camera3D_GetRelativeDirection(const Camera3D* cam, int world_direction);

// Camera-Relative WASD Movement
void Camera3D_GetMovementVector(const Camera3D* cam, float input_forward, float input_right, float* out_vx, float* out_vz);

#endif // CAMERA3D_H
