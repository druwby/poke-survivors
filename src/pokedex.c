#include "pokedex.h"
#include "pokedex_data.h"
#include "image_loader.h"
#include "sprite.h"
#include <stdio.h>
#include <string.h>

static SDL_Texture* s_walk_cache[152];
static SDL_Texture* s_portrait_cache[152];
static bool s_pokedex_initialized = false;

void Pokedex_Init(SDL_Renderer* renderer) {
    (void)renderer;
    if (s_pokedex_initialized) return;
    memset(s_walk_cache, 0, sizeof(s_walk_cache));
    memset(s_portrait_cache, 0, sizeof(s_portrait_cache));
    s_pokedex_initialized = true;
}

const PokedexEntry* Pokedex_GetEntry(int pokedex_id) {
    if (pokedex_id < 1 || pokedex_id > 151) return NULL;
    return &g_pokedex[pokedex_id];
}

SpriteSheet Pokedex_LoadSprite(SDL_Renderer* renderer, int pokedex_id, float custom_scale) {
    SpriteSheet sheet;
    memset(&sheet, 0, sizeof(SpriteSheet));
    const PokedexEntry* entry = Pokedex_GetEntry(pokedex_id);
    if (!entry) return sheet;

    if (!s_walk_cache[pokedex_id]) {
        char path[256];
        snprintf(path, sizeof(path), "assets/sprites/sprite/%04d/Walk-Anim.png", pokedex_id);
        s_walk_cache[pokedex_id] = Image_LoadTexture(renderer, path);
    }

    sheet.texture = s_walk_cache[pokedex_id];
    sheet.frame_w = entry->frame_w;
    sheet.frame_h = entry->frame_h;
    sheet.num_frames = entry->frame_count > 0 ? entry->frame_count : 4;
    sheet.num_directions = 8;
    sheet.frame_duration = 0.12f;
    sheet.scale = (custom_scale > 0.0f) ? custom_scale : entry->scale;
    sheet.loaded = (sheet.texture != NULL);
    if (sheet.loaded && sheet.texture) {
        int tw = 0, th = 0;
        if (SDL_QueryTexture(sheet.texture, NULL, NULL, &tw, &th) == 0 && tw > 0 && th > 0) {
            if (sheet.num_frames > 0 && (sheet.frame_w <= 0 || (sheet.frame_w * sheet.num_frames != tw && tw % sheet.num_frames == 0))) {
                sheet.frame_w = tw / sheet.num_frames;
            } else if (sheet.num_frames <= 0 && sheet.frame_w > 0 && tw % sheet.frame_w == 0) {
                sheet.num_frames = tw / sheet.frame_w;
            }
            if (sheet.num_directions > 0 && (sheet.frame_h <= 0 || (sheet.frame_h * sheet.num_directions != th && th % sheet.num_directions == 0))) {
                sheet.frame_h = th / sheet.num_directions;
            } else if (sheet.num_directions <= 0 && sheet.frame_h > 0 && th % sheet.frame_h == 0) {
                sheet.num_directions = th / sheet.frame_h;
            }
        }
    }
    return sheet;
}

SDL_Texture* Pokedex_LoadPortrait(SDL_Renderer* renderer, int pokedex_id) {
    if (pokedex_id < 1 || pokedex_id > 151) return NULL;
    if (!s_portrait_cache[pokedex_id]) {
        char path[256];
        snprintf(path, sizeof(path), "assets/sprites/portrait/%04d/Normal.png", pokedex_id);
        s_portrait_cache[pokedex_id] = Image_LoadTexture(renderer, path);
    }
    return s_portrait_cache[pokedex_id];
}

const char* Pokedex_GetTypeName(PokeType type) {
    switch (type) {
        case POKE_TYPE_NORMAL:   return "NORMAL";
        case POKE_TYPE_FIRE:     return "FIRE";
        case POKE_TYPE_WATER:    return "WATER";
        case POKE_TYPE_GRASS:    return "GRASS";
        case POKE_TYPE_ELECTRIC: return "ELECTRIC";
        case POKE_TYPE_ICE:      return "ICE";
        case POKE_TYPE_FIGHTING: return "FIGHTING";
        case POKE_TYPE_POISON:   return "POISON";
        case POKE_TYPE_GROUND:   return "GROUND";
        case POKE_TYPE_FLYING:   return "FLYING";
        case POKE_TYPE_PSYCHIC:  return "PSYCHIC";
        case POKE_TYPE_BUG:      return "BUG";
        case POKE_TYPE_ROCK:     return "ROCK";
        case POKE_TYPE_GHOST:    return "GHOST";
        case POKE_TYPE_DRAGON:   return "DRAGON";
        default:                 return "NONE";
    }
}

SDL_Color Pokedex_GetTypeColor(PokeType type) {
    switch (type) {
        case POKE_TYPE_NORMAL:   return (SDL_Color){ 165, 165, 145, 255 };
        case POKE_TYPE_FIRE:     return (SDL_Color){ 240, 100, 40, 255 };
        case POKE_TYPE_WATER:    return (SDL_Color){ 70, 150, 240, 255 };
        case POKE_TYPE_GRASS:    return (SDL_Color){ 80, 200, 80, 255 };
        case POKE_TYPE_ELECTRIC: return (SDL_Color){ 245, 215, 50, 255 };
        case POKE_TYPE_ICE:      return (SDL_Color){ 120, 210, 230, 255 };
        case POKE_TYPE_FIGHTING: return (SDL_Color){ 190, 60, 40, 255 };
        case POKE_TYPE_POISON:   return (SDL_Color){ 170, 80, 200, 255 };
        case POKE_TYPE_GROUND:   return (SDL_Color){ 205, 160, 80, 255 };
        case POKE_TYPE_FLYING:   return (SDL_Color){ 160, 150, 230, 255 };
        case POKE_TYPE_PSYCHIC:  return (SDL_Color){ 235, 75, 130, 255 };
        case POKE_TYPE_BUG:      return (SDL_Color){ 150, 190, 50, 255 };
        case POKE_TYPE_ROCK:     return (SDL_Color){ 170, 150, 90, 255 };
        case POKE_TYPE_GHOST:    return (SDL_Color){ 110, 80, 160, 255 };
        case POKE_TYPE_DRAGON:   return (SDL_Color){ 110, 60, 210, 255 };
        default:                 return (SDL_Color){ 180, 180, 180, 255 };
    }
}

void Pokedex_Cleanup(void) {
    for (int i = 1; i <= 151; i++) {
        if (s_walk_cache[i]) {
            SDL_DestroyTexture(s_walk_cache[i]);
            s_walk_cache[i] = NULL;
        }
        if (s_portrait_cache[i]) {
            SDL_DestroyTexture(s_portrait_cache[i]);
            s_portrait_cache[i] = NULL;
        }
    }
    s_pokedex_initialized = false;
}
