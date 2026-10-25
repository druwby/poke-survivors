#ifndef WEAPONS_H
#define WEAPONS_H

#include "types.h"
#include "spatial_grid.h"

void Weapons_Init(WeaponsManager* wm);
void Weapons_InitForStarter(GameState* game, StarterID starter);
void Weapons_Update(GameState* game, float dt);
void Weapons_FireWaterWand(GameState* game);
void Weapons_FireCleave(GameState* game);
void Weapons_FirePulse(GameState* game);
void Weapons_FireVineWhip(GameState* game);
void Weapons_SpawnDamageText(GameState* game, Vec2 pos, float damage, SDL_Color color);
void Weapons_SpawnEnemyProjectile(GameState* game, Vec2 pos, Vec2 vel, float radius, float damage, float lifetime, SDL_Color color);
void Weapons_SpawnGroundHazard(GameState* game, HazardType type, Vec2 pos, float radius, float duration, float damage, bool is_enemy, SDL_Color color);
void Weapons_DrawGroundHazards(GameState* game);
void Weapons_Draw(GameState* game);

#endif // WEAPONS_H
