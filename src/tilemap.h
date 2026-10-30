#ifndef TILEMAP_H
#define TILEMAP_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"

#define TILE_WORLD_SIZE 64.0f
#define TILE_SRC_SIZE   24

#define MAP_TILES_X GRID_COLS // 128
#define MAP_TILES_Y GRID_ROWS // 128

// Collision & Feature Flags
#define TILE_FLAG_NONE       0x00
#define TILE_FLAG_SOLID      0x01 // Impassable for ground entities (cliffs, deep water)
#define TILE_FLAG_RAMP       0x02 // Ramp / stairs connecting elevation levels (always passable)
#define TILE_FLAG_WATER      0x04 // Water obstacle
#define TILE_FLAG_PATH       0x08 // Pathway / road
#define TILE_FLAG_PLATEAU    0x10 // Elevated plateau flat ground
#define TILE_FLAG_DECO       0x20 // Decorative overlay (pebbles, flora)

// Tile indices corresponding to exact 24x24 slices of assets/tilesets/tileset1.png (432x192)
typedef enum {
    // Block 0: Rock / Cliff / Plateau (Cols 0..5, Rows 0..7 of 24x24)
    TILE_ROCK_TL = 0,       // src {  0,   0, 24, 24 } - Top-Left outer cliff corner
    TILE_ROCK_T,            // src { 24,   0, 24, 24 } - Top cliff face edge
    TILE_ROCK_TR,           // src { 48,   0, 24, 24 } - Top-Right outer cliff corner
    TILE_ROCK_L,            // src {  0,  24, 24, 24 } - West / Left cliff face edge
    TILE_ROCK_CENTER,       // src { 24,  24, 24, 24 } - Flat elevated plateau rock surface
    TILE_ROCK_R,            // src { 48,  24, 24, 24 } - East / Right cliff face edge
    TILE_ROCK_BL,           // src {  0,  48, 24, 24 } - Bottom-Left outer cliff corner
    TILE_ROCK_B,            // src { 24,  48, 24, 24 } - South / Bottom cliff face
    TILE_ROCK_BR,           // src { 48,  48, 24, 24 } - Bottom-Right outer cliff corner
    TILE_ROCK_INNER_TL,     // src { 24, 144, 24, 24 } - Inner / concave top-left corner
    TILE_ROCK_INNER_TR,     // src {  0, 144, 24, 24 } - Inner / concave top-right corner
    TILE_ROCK_INNER_BL,     // src { 24, 120, 24, 24 } - Inner / concave bottom-left corner
    TILE_ROCK_INNER_BR,     // src {  0, 120, 24, 24 } - Inner / concave bottom-right corner
    TILE_ROCK_RAMP_S,       // src { 96,   0, 24, 24 } - Carved stone ramp through South cliff face
    TILE_ROCK_RAMP_N,       // src { 96,  72, 24, 24 } - Carved stone ramp through North cliff face
    TILE_ROCK_RAMP_W,       // src { 72,  24, 24, 24 } - Carved stone ramp through West cliff face
    TILE_ROCK_RAMP_E,       // src { 120, 24, 24, 24 } - Carved stone ramp through East cliff face
    TILE_ROCK_BOULDER,      // src { 96,  24, 24, 24 } - Natural standalone rock boulder

    // Block 1: Water / Pond / Shoreline (Cols 6..11, Rows 0..7 of 24x24)
    TILE_WATER_TL,          // src { 144,   0, 24, 24 } - Top-Left shoreline corner
    TILE_WATER_T,           // src { 168,   0, 24, 24 } - Top shoreline edge
    TILE_WATER_TR,          // src { 192,   0, 24, 24 } - Top-Right shoreline corner
    TILE_WATER_L,           // src { 144,  24, 24, 24 } - West shoreline edge
    TILE_WATER_CENTER,      // src { 168,  24, 24, 24 } - Deep open water body
    TILE_WATER_R,           // src { 192,  24, 24, 24 } - East shoreline edge
    TILE_WATER_BL,          // src { 144,  48, 24, 24 } - Bottom-Left shoreline corner
    TILE_WATER_B,           // src { 168,  48, 24, 24 } - South shoreline edge
    TILE_WATER_BR,          // src { 192,  48, 24, 24 } - Bottom-Right shoreline corner
    TILE_WATER_INNER_TL,    // src { 168, 144, 24, 24 } - Inner / concave top-left shoreline
    TILE_WATER_INNER_TR,    // src { 144, 144, 24, 24 } - Inner / concave top-right shoreline
    TILE_WATER_INNER_BL,    // src { 168, 120, 24, 24 } - Inner / concave bottom-left shoreline
    TILE_WATER_INNER_BR,    // src { 144, 120, 24, 24 } - Inner / concave bottom-right shoreline
    TILE_WATER_SHALLOW_1,   // src { 192,  72, 24, 24 } - Shallow water / current ripple 1
    TILE_WATER_SHALLOW_2,   // src { 216,  72, 24, 24 } - Shallow water / current ripple 2

    // Block 2: Ground / Pathways / Decorative (Cols 12..17, Rows 0..7 of 24x24)
    TILE_GROUND_1,          // src { 288,  24, 24, 24 } - Natural sandy dirt ground 1
    TILE_GROUND_2,          // src { 312,   0, 24, 24 } - Sandy dirt ground variation 2
    TILE_GROUND_3,          // src { 336,   0, 24, 24 } - Sandy dirt ground variation 3
    TILE_PATH_TL,           // src { 288,   0, 24, 24 } - Pathway Top-Left corner
    TILE_PATH_T,            // src { 312,   0, 24, 24 } - Pathway Top edge
    TILE_PATH_TR,           // src { 336,   0, 24, 24 } - Pathway Top-Right corner
    TILE_PATH_L,            // src { 288,  24, 24, 24 } - Pathway Left edge
    TILE_PATH_CENTER,       // src { 312,  24, 24, 24 } - Packed dirt walkway center
    TILE_PATH_R,            // src { 336,  24, 24, 24 } - Pathway Right edge
    TILE_PATH_BL,           // src { 288,  48, 24, 24 } - Pathway Bottom-Left corner
    TILE_PATH_B,            // src { 312,  48, 24, 24 } - Pathway Bottom edge
    TILE_PATH_BR,           // src { 336,  48, 24, 24 } - Pathway Bottom-Right corner
    TILE_PATH_VERT,         // src { 384,   0, 24, 24 } - Single-tile wide vertical road / path
    TILE_PATH_HORIZ,        // src { 384,  24, 24, 24 } - Single-tile wide horizontal road / path
    TILE_PATH_CROSS,        // src { 384,  48, 24, 24 } - Crossroad pathway junction
    TILE_DECO_PEBBLE_1,     // src { 384,  72, 24, 24 } - Natural pebble / gravel terrain 1
    TILE_DECO_PEBBLE_2,     // src { 360,  72, 24, 24 } - Natural pebble / gravel terrain 2
    TILE_DECO_PEBBLE_3,     // src { 384,  96, 24, 24 } - Natural pebble / gravel terrain 3
    TILE_DECO_FLOWER,       // src { 312,  72, 24, 24 } - Wild roadside flora / decorative patch

    TILE_TOTAL_COUNT
} TileIndex;

typedef struct {
    uint8_t tile_id;    // TileIndex
    uint8_t flags;      // TILE_FLAG_*
    uint8_t elevation;  // 0: ground level, 1: elevated plateau
    uint8_t overlay_id; // 255 for none, or TileIndex for overlay (e.g. decorative pebbles)
} MapCell;

typedef struct {
    uint16_t dist[MAP_TILES_Y][MAP_TILES_X]; // Step distance to target (0xFFFF = unreachable)
    int target_tile_x;                      // Target tile coordinate X
    int target_tile_y;                      // Target tile coordinate Y
    bool valid;                             // True if flow field is computed
} PathField;

typedef struct TileMap {
    MapCell cells[MAP_TILES_Y][MAP_TILES_X];
    SDL_Rect src_rects[TILE_TOTAL_COUNT];
    int current_section;
    int current_route;
    PathField path_field;
} TileMap;

void Tilemap_Init(TileMap* map);
void Tilemap_GenerateRouteMap(TileMap* map, int section_idx, int route_idx);
void Tilemap_Render(const TileMap* map, SDL_Renderer* renderer, SDL_Texture* tileset_tex, float camera_x, float camera_y);
bool Tilemap_CheckCollisionCircle(const TileMap* map, float* pos_x, float* pos_y, float radius, bool can_fly);
bool Tilemap_IsSolid(const TileMap* map, int tile_x, int tile_y, bool can_fly);
const SDL_Rect* Tilemap_GetSrcRect(TileIndex index);

// Dynamic Flow Field Pathfinding & Line-of-Sight
void Tilemap_UpdateFlowField(TileMap* map, float target_x, float target_y);
bool Tilemap_GetPathDirection(TileMap* map, float from_x, float from_y, float target_x, float target_y, float* out_dir_x, float* out_dir_y);
bool Tilemap_HasLineOfSight(const TileMap* map, float x1, float y1, float x2, float y2);

#endif // TILEMAP_H
