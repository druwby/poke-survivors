#ifndef POKEDEX_H
#define POKEDEX_H

#include "types.h"

void Pokedex_Init(SDL_Renderer* renderer);
const PokedexEntry* Pokedex_GetEntry(int pokedex_id);
SpriteSheet Pokedex_LoadSprite(SDL_Renderer* renderer, int pokedex_id, float custom_scale);
SDL_Texture* Pokedex_LoadPortrait(SDL_Renderer* renderer, int pokedex_id);
const char* Pokedex_GetTypeName(PokeType type);
SDL_Color Pokedex_GetTypeColor(PokeType type);
void Pokedex_Cleanup(void);

#endif // POKEDEX_H
