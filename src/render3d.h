#ifndef RENDER3D_H
#define RENDER3D_H

#include "types.h"

void Render3D_Init(GameState* game);
void Render3D_DrawWorld(GameState* game);
float Render3D_GetTerrainHeight(const TileMap* map, float world_x, float world_z);

#endif // RENDER3D_H
