#include "sprite.h"
#include "image_loader.h"
#include "balance_sim.h"
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SpriteSheet Sprite_Load(SDL_Renderer* renderer, const char* filepath, int frame_w, int frame_h, int num_frames, int num_directions, float frame_duration, float scale) {
    SpriteSheet sheet;
    sheet.frame_w = frame_w;
    sheet.frame_h = frame_h;
    sheet.num_frames = num_frames;
    sheet.num_directions = num_directions;
    sheet.frame_duration = frame_duration;
    sheet.scale = scale;
    sheet.texture = Image_LoadTexture(renderer, filepath);
    sheet.loaded = (sheet.texture != NULL);
    if (sheet.loaded && !g_headless_sim) {
        printf("[Sprite] Loaded '%s' (%dx%d, %d frames, %d dirs, scale %.2f)\n", 
               filepath, frame_w, frame_h, num_frames, num_directions, scale);
    }
    return sheet;
}

int Sprite_VecToDirection(float vx, float vy, int current_direction) {
    if (fabsf(vx) < 0.01f && fabsf(vy) < 0.01f) {
        return current_direction;
    }
    
    float angle = atan2f(vy, vx); // -PI to +PI
    if (angle < 0.0f) {
        angle += 2.0f * (float)M_PI; // 0 to 2PI
    }
    
    float deg = angle * (180.0f / (float)M_PI);
    int sector = (int)floorf((deg + 22.5f) / 45.0f) % 8;
    
    // Sector map to PMD sprite row order:
    // 0: Right (0 deg)      -> Row 2
    // 1: Down-Right (45 deg)-> Row 1
    // 2: Down (90 deg)      -> Row 0
    // 3: Down-Left (135 deg)-> Row 7
    // 4: Left (180 deg)     -> Row 6
    // 5: Up-Left (225 deg)  -> Row 5
    // 6: Up (270 deg)       -> Row 4
    // 7: Up-Right (315 deg) -> Row 3
    static const int sector_to_row[8] = { 2, 1, 0, 7, 6, 5, 4, 3 };
    return sector_to_row[sector];
}

void Sprite_UpdateAnim(AnimState* anim, const SpriteSheet* sheet, float dt, bool is_moving) {
    anim->is_moving = is_moving;
    if (!sheet || sheet->num_frames <= 1) return;
    
    if (is_moving) {
        anim->anim_timer += dt;
        if (anim->anim_timer >= sheet->frame_duration) {
            anim->anim_timer -= sheet->frame_duration;
            anim->current_frame = (anim->current_frame + 1) % sheet->num_frames;
        }
    } else {
        // When stationary, keep facing direction but reset to idle frame (frame 0)
        anim->current_frame = 0;
        anim->anim_timer = 0.0f;
    }
}

void Sprite_DrawProceduralFallback(SDL_Renderer* renderer, Vec2 pos, Vec2 camera, float radius, int direction, SDL_Color color, bool hit_flash) {
    int screen_x = (int)(pos.x - camera.x);
    int screen_y = (int)(pos.y - camera.y);
    int r = (int)radius;
    
    if (hit_flash) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }
    
    // Draw body (filled circle approximation using horizontal lines)
    for (int dy = -r; dy <= r; dy++) {
        int dx = (int)sqrtf((float)(r * r - dy * dy));
        SDL_RenderDrawLine(renderer, screen_x - dx, screen_y + dy, screen_x + dx, screen_y + dy);
    }
    
    // Direction eye/pointer (bright white/yellow dot indicating facing)
    float angle = 0.0f;
    // Reverse row map to angle
    switch (direction) {
        case 0: angle = (float)M_PI * 0.5f; break;  // Down
        case 1: angle = (float)M_PI * 0.25f; break; // Down-Right
        case 2: angle = 0.0f; break;                // Right
        case 3: angle = -(float)M_PI * 0.25f; break;// Up-Right
        case 4: angle = -(float)M_PI * 0.5f; break; // Up
        case 5: angle = -(float)M_PI * 0.75f; break;// Up-Left
        case 6: angle = (float)M_PI; break;         // Left
        case 7: angle = (float)M_PI * 0.75f; break; // Down-Left
        default: break;
    }
    
    int eye_x = screen_x + (int)(cosf(angle) * (radius * 0.6f));
    int eye_y = screen_y + (int)(sinf(angle) * (radius * 0.6f));
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_Rect eye_rect = { eye_x - 2, eye_y - 2, 4, 4 };
    SDL_RenderFillRect(renderer, &eye_rect);
}

void Sprite_Draw(SDL_Renderer* renderer, const SpriteSheet* sheet, const AnimState* anim, Vec2 pos, Vec2 camera, SDL_Color fallback_color, float fallback_radius, bool hit_flash) {
    if (!sheet || !sheet->loaded || !sheet->texture) {
        Sprite_DrawProceduralFallback(renderer, pos, camera, fallback_radius, anim->direction, fallback_color, hit_flash);
        return;
    }
    
    int frame = anim->current_frame % sheet->num_frames;
    int dir = anim->direction % sheet->num_directions;
    
    SDL_Rect src = {
        frame * sheet->frame_w,
        dir * sheet->frame_h,
        sheet->frame_w,
        sheet->frame_h
    };
    
    int dest_w = (int)(sheet->frame_w * sheet->scale);
    int dest_h = (int)(sheet->frame_h * sheet->scale);
    
    SDL_Rect dst = {
        (int)(pos.x - camera.x) - dest_w / 2,
        (int)(pos.y - camera.y) - dest_h / 2,
        dest_w,
        dest_h
    };
    
    if (hit_flash) {
        SDL_SetTextureColorMod(sheet->texture, 255, 100, 100);
    } else {
        SDL_SetTextureColorMod(sheet->texture, 255, 255, 255);
    }
    
    SDL_RenderCopy(renderer, sheet->texture, &src, &dst);
}

void Sprite_DrawRotated(SDL_Renderer* renderer, const SpriteSheet* sheet, const AnimState* anim, Vec2 pos, Vec2 camera, double angle_deg, SDL_Color fallback_color, float fallback_radius, bool hit_flash) {
    if (!sheet || !sheet->loaded || !sheet->texture) {
        Sprite_DrawProceduralFallback(renderer, pos, camera, fallback_radius, anim->direction, fallback_color, hit_flash);
        return;
    }
    
    int frame = anim->current_frame % sheet->num_frames;
    int dir = anim->direction % sheet->num_directions;
    
    SDL_Rect src = {
        frame * sheet->frame_w,
        dir * sheet->frame_h,
        sheet->frame_w,
        sheet->frame_h
    };
    
    int dest_w = (int)(sheet->frame_w * sheet->scale);
    int dest_h = (int)(sheet->frame_h * sheet->scale);
    
    SDL_Rect dst = {
        (int)(pos.x - camera.x) - dest_w / 2,
        (int)(pos.y - camera.y) - dest_h / 2,
        dest_w,
        dest_h
    };
    
    if (hit_flash) {
        SDL_SetTextureColorMod(sheet->texture, 255, 100, 100);
    } else {
        SDL_SetTextureColorMod(sheet->texture, 255, 255, 255);
    }
    
    SDL_RenderCopyEx(renderer, sheet->texture, &src, &dst, angle_deg, NULL, SDL_FLIP_NONE);
}
