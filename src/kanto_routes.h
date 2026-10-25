#ifndef KANTO_ROUTES_H
#define KANTO_ROUTES_H

#include "types.h"

typedef struct {
    const char* name;
    const char* section_name;
    int pokemon_ids[6];
    int pokemon_count;
    int boss_pokemon_id; // 0 if none
    const char* boss_name;
    float duration_sec;
    int kill_quota;
    bool is_gym;
} RouteConfig;

void KantoRoutes_Init(GameState* game);
void KantoRoutes_LoadRoute(GameState* game, int section_idx, int route_idx);
void KantoRoutes_Update(GameState* game, float dt);
void KantoRoutes_SpawnEnemies(GameState* game, float dt);
void KantoRoutes_AdvanceRoute(GameState* game);
const RouteConfig* KantoRoutes_GetCurrentRoute(const GameState* game);
int KantoRoutes_GetTotalSections(void);
int KantoRoutes_GetRoutesInSection(int section_idx);

#endif // KANTO_ROUTES_H
