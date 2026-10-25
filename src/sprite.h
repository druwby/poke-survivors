#ifndef SPRITE_H
#define SPRITE_H

#include <SDL.h>
#include <stdbool.h>
#include "types.h"

SpriteSheet Sprite_Load(SDL_Renderer* renderer, const char* filepath, int frame_w, int frame_h, int num_frames, int num_directions, float frame_duration, float scale);
int Sprite_VecToDirection(float vx, float vy, int current_direction);
void Sprite_UpdateAnim(AnimState* anim, const SpriteSheet* sheet, float dt, bool is_moving);
void Sprite_Draw(SDL_Renderer* renderer, const SpriteSheet* sheet, const AnimState* anim, Vec2 pos, Vec2 camera, SDL_Color fallback_color, float fallback_radius, bool hit_flash);
void Sprite_DrawRotated(SDL_Renderer* renderer, const SpriteSheet* sheet, const AnimState* anim, Vec2 pos, Vec2 camera, double angle_deg, SDL_Color fallback_color, float fallback_radius, bool hit_flash);
void Sprite_DrawProceduralFallback(SDL_Renderer* renderer, Vec2 pos, Vec2 camera, float radius, int direction, SDL_Color color, bool hit_flash);

#endif // SPRITE_H
