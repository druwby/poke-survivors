#ifndef WEAPONS_H
#define WEAPONS_H

#include "types.h"
#include "spatial_grid.h"

// Evolution recipe structure
typedef struct {
    WeaponType ingredient_a;
    WeaponType ingredient_b;
    WeaponType result;
    const char* name;
    const char* desc;
} EvolutionRecipe;

extern const EvolutionRecipe g_evolution_recipes[];
extern const int g_evolution_recipe_count;

// Weapon metadata
const char* Weapons_GetName(WeaponType type);
const char* Weapons_GetDescription(WeaponType type);
SDL_Color Weapons_GetColor(WeaponType type);
bool Weapons_IsEvolved(WeaponType type);

// Weapon Lifecycle & Inventory
void Weapons_Init(WeaponsManager* wm);
void Weapons_InitForStarter(GameState* game, StarterID starter);
bool Weapons_AddWeapon(GameState* game, WeaponType type);
void Weapons_UpgradeWeapon(GameState* game, int slot_index);
void Weapons_EvolveWeapon(GameState* game, int slot_a, int slot_b, WeaponType evolved_type);
void Weapons_Update(GameState* game, float dt);

// Firing routines
void Weapons_FireSlot(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FireVineWhip(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FireFlamethrower(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FireBubbleBeam(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FireRazorLeaf(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FireSwift(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FireThunderShock(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FireToxicSpore(GameState* game, ActiveWeaponSlot* slot);

// Evolved fusions
void Weapons_FireSolarFrenzy(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FireFireBlast(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FireHydroVortex(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FirePlasmaTempest(GameState* game, ActiveWeaponSlot* slot);
void Weapons_FireVenomCataclysm(GameState* game, ActiveWeaponSlot* slot);

// Legacy stubs (for dev menu or existing calls)
void Weapons_FireWaterWand(GameState* game);
void Weapons_FireCleave(GameState* game);
void Weapons_FirePulse(GameState* game);

// Utilities & Hazards
void Weapons_SpawnDamageText(GameState* game, Vec2 pos, float damage, SDL_Color color);
void Weapons_SpawnEnemyProjectile(GameState* game, Vec2 pos, Vec2 vel, float radius, float damage, float lifetime, SDL_Color color);
void Weapons_SpawnGroundHazard(GameState* game, HazardType type, Vec2 pos, float radius, float duration, float damage, bool is_enemy, SDL_Color color);
void Weapons_DrawGroundHazards(GameState* game);
void Weapons_Draw(GameState* game);

#endif // WEAPONS_H
