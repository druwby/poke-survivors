#include "tilemap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Exact 24x24 pixel slices within assets/tilesets/tileset1.png (432x192)
static const SDL_Rect s_tile_src_rects[TILE_TOTAL_COUNT] = {
    // Block 0: Rock / Cliff / Plateau (Cols 0..5, Rows 0..7 of 24x24)
    [TILE_ROCK_TL]          = {   0,   0, 24, 24 }, // Outer top-left corner
    [TILE_ROCK_T]           = {  24,   0, 24, 24 }, // North cliff edge
    [TILE_ROCK_TR]          = {  48,   0, 24, 24 }, // Outer top-right corner
    [TILE_ROCK_L]           = {   0,  24, 24, 24 }, // West cliff face
    [TILE_ROCK_CENTER]      = {  24,  24, 24, 24 }, // Plateau top flat surface
    [TILE_ROCK_R]           = {  48,  24, 24, 24 }, // East cliff face
    [TILE_ROCK_BL]          = {   0,  48, 24, 24 }, // Outer bottom-left corner
    [TILE_ROCK_B]           = {  24,  48, 24, 24 }, // South cliff face
    [TILE_ROCK_BR]          = {  48,  48, 24, 24 }, // Outer bottom-right corner
    [TILE_ROCK_INNER_TL]    = {  24, 144, 24, 24 }, // Inner concave top-left corner
    [TILE_ROCK_INNER_TR]    = {   0, 144, 24, 24 }, // Inner concave top-right corner
    [TILE_ROCK_INNER_BL]    = {  24, 120, 24, 24 }, // Inner concave bottom-left corner
    [TILE_ROCK_INNER_BR]    = {   0, 120, 24, 24 }, // Inner concave bottom-right corner
    [TILE_ROCK_RAMP_S]      = {  96,   0, 24, 24 }, // Carved stone ramp through South cliff face
    [TILE_ROCK_RAMP_N]      = {  96,  72, 24, 24 }, // Carved stone ramp through North cliff face
    [TILE_ROCK_RAMP_W]      = {  72,  24, 24, 24 }, // Carved stone ramp through West cliff face
    [TILE_ROCK_RAMP_E]      = { 120,  24, 24, 24 }, // Carved stone ramp through East cliff face
    [TILE_ROCK_BOULDER]     = {  96,  24, 24, 24 }, // Standalone rock boulder

    // Block 1: Water / Pond / Shoreline (Cols 6..11, Rows 0..7 of 24x24)
    [TILE_WATER_TL]         = { 144,   0, 24, 24 }, // Shoreline top-left corner
    [TILE_WATER_T]          = { 168,   0, 24, 24 }, // Shoreline top edge
    [TILE_WATER_TR]         = { 192,   0, 24, 24 }, // Shoreline top-right corner
    [TILE_WATER_L]          = { 144,  24, 24, 24 }, // Shoreline west edge
    [TILE_WATER_CENTER]     = { 168,  24, 24, 24 }, // Deep water center
    [TILE_WATER_R]          = { 192,  24, 24, 24 }, // Shoreline east edge
    [TILE_WATER_BL]         = { 144,  48, 24, 24 }, // Shoreline bottom-left corner
    [TILE_WATER_B]          = { 168,  48, 24, 24 }, // Shoreline south edge
    [TILE_WATER_BR]         = { 192,  48, 24, 24 }, // Shoreline bottom-right corner
    [TILE_WATER_INNER_TL]   = { 168, 144, 24, 24 }, // Inner concave top-left shoreline
    [TILE_WATER_INNER_TR]   = { 144, 144, 24, 24 }, // Inner concave top-right shoreline
    [TILE_WATER_INNER_BL]   = { 168, 120, 24, 24 }, // Inner concave bottom-left shoreline
    [TILE_WATER_INNER_BR]   = { 144, 120, 24, 24 }, // Inner concave bottom-right shoreline
    [TILE_WATER_SHALLOW_1]  = { 192,  72, 24, 24 }, // Shallow water ripple variation 1
    [TILE_WATER_SHALLOW_2]  = { 216,  72, 24, 24 }, // Shallow water ripple variation 2

    // Block 2: Ground / Pathways / Decorative (Cols 12..17, Rows 0..7 of 24x24)
    [TILE_GROUND_1]         = { 288,  24, 24, 24 }, // Natural sandy dirt ground 1
    [TILE_GROUND_2]         = { 312,   0, 24, 24 }, // Sandy dirt ground variation 2
    [TILE_GROUND_3]         = { 336,   0, 24, 24 }, // Sandy dirt ground variation 3
    [TILE_PATH_TL]          = { 288,   0, 24, 24 }, // Pathway Top-Left corner
    [TILE_PATH_T]           = { 312,   0, 24, 24 }, // Pathway Top edge
    [TILE_PATH_TR]          = { 336,   0, 24, 24 }, // Pathway Top-Right corner
    [TILE_PATH_L]           = { 288,  24, 24, 24 }, // Pathway Left edge
    [TILE_PATH_CENTER]      = { 312,  24, 24, 24 }, // Packed dirt walkway center
    [TILE_PATH_R]           = { 336,  24, 24, 24 }, // Pathway Right edge
    [TILE_PATH_BL]          = { 288,  48, 24, 24 }, // Pathway Bottom-Left corner
    [TILE_PATH_B]           = { 312,  48, 24, 24 }, // Pathway Bottom edge
    [TILE_PATH_BR]          = { 336,  48, 24, 24 }, // Pathway Bottom-Right corner
    [TILE_PATH_VERT]        = { 384,   0, 24, 24 }, // Single-tile wide vertical road / path
    [TILE_PATH_HORIZ]       = { 384,  24, 24, 24 }, // Single-tile wide horizontal road / path
    [TILE_PATH_CROSS]       = { 384,  48, 24, 24 }, // Crossroad pathway junction
    [TILE_DECO_PEBBLE_1]    = { 384,  72, 24, 24 }, // Natural pebble / gravel terrain 1
    [TILE_DECO_PEBBLE_2]    = { 360,  72, 24, 24 }, // Natural pebble / gravel terrain 2
    [TILE_DECO_PEBBLE_3]    = { 384,  96, 24, 24 }, // Natural pebble / gravel terrain 3
    [TILE_DECO_FLOWER]      = { 312,  72, 24, 24 }, // Wild roadside flora / decorative patch
    [TILE_TALL_GRASS]       = { 336,  72, 24, 24 }, // Wild interactive tall grass clump
    [TILE_PORTAL_GATE]      = { 384,  48, 24, 24 }, // Route exit portal gate
    [TILE_LAVA_CENTER]      = { 168,  72, 24, 24 }, // Volcanic molten lava body
};

const SDL_Rect* Tilemap_GetSrcRect(TileIndex index) {
    if (index >= 0 && index < TILE_TOTAL_COUNT) {
        return &s_tile_src_rects[index];
    }
    return &s_tile_src_rects[TILE_GROUND_1];
}

// Fast deterministic LCG PRNG for reproducible route maps
static uint32_t s_route_rng = 12345;
static uint32_t Route_Rand(void) {
    s_route_rng = s_route_rng * 1103515245 + 12345;
    return (s_route_rng >> 16) & 0x7fff;
}
static void Route_Srand(uint32_t seed) {
    s_route_rng = seed ? seed : 1;
}

void Tilemap_Init(TileMap* map) {
    if (!map) return;
    memset(map, 0, sizeof(TileMap));
    memcpy(map->src_rects, s_tile_src_rects, sizeof(s_tile_src_rects));

    // Clear entire map to ground terrain with organic texture variations
    for (int y = 0; y < MAP_TILES_Y; y++) {
        for (int x = 0; x < MAP_TILES_X; x++) {
            MapCell* c = &map->cells[y][x];
            int r = (x * 17 + y * 31 + (x ^ y)) % 12;
            if (r < 7) {
                c->tile_id = TILE_GROUND_1;
            } else if (r < 10) {
                c->tile_id = TILE_GROUND_2;
            } else {
                c->tile_id = TILE_GROUND_3;
            }
            c->flags = TILE_FLAG_NONE;
            c->elevation = 0;
            c->overlay_id = 255;

            // Occasional pebble/flora accents
            if (r == 11 && ((x * y) % 5 == 0)) {
                c->overlay_id = TILE_DECO_PEBBLE_1 + ((x + y) % 3);
                c->flags |= TILE_FLAG_DECO;
            }
        }
    }

    map->boulder_count = 0;
    memset(map->boulders, 0, sizeof(map->boulders));
    map->exit_gate.active = false;
    map->exit_gate.player_entered = false;
    map->current_biome = BIOME_PLAINS;
}

// Helper: Add natural rock boundary walls enclosing the 128x128 map
static void Tilemap_AddBoundaryWalls(TileMap* map) {
    // Top boundary (North): Row 0 is mountain peak, Row 1 is South cliff face facing playfield
    for (int x = 0; x < MAP_TILES_X; x++) {
        map->cells[0][x].tile_id = TILE_ROCK_CENTER;
        map->cells[0][x].flags = TILE_FLAG_SOLID;
        map->cells[1][x].tile_id = TILE_ROCK_B;
        map->cells[1][x].flags = TILE_FLAG_SOLID;
    }

    // Bottom boundary (South): Row 126 is North cliff edge facing playfield, Row 127 is mountain peak
    for (int x = 0; x < MAP_TILES_X; x++) {
        map->cells[MAP_TILES_Y - 2][x].tile_id = TILE_ROCK_T;
        map->cells[MAP_TILES_Y - 2][x].flags = TILE_FLAG_SOLID;
        map->cells[MAP_TILES_Y - 1][x].tile_id = TILE_ROCK_CENTER;
        map->cells[MAP_TILES_Y - 1][x].flags = TILE_FLAG_SOLID;
    }

    // Left boundary (West): Col 0 is mountain peak, Col 1 is East cliff face facing playfield
    for (int y = 0; y < MAP_TILES_Y; y++) {
        map->cells[y][0].tile_id = TILE_ROCK_CENTER;
        map->cells[y][0].flags = TILE_FLAG_SOLID;
        map->cells[y][1].tile_id = TILE_ROCK_R;
        map->cells[y][1].flags = TILE_FLAG_SOLID;
    }

    // Right boundary (East): Col 126 is West cliff face facing playfield, Col 127 is mountain peak
    for (int y = 0; y < MAP_TILES_Y; y++) {
        map->cells[y][MAP_TILES_X - 2].tile_id = TILE_ROCK_L;
        map->cells[y][MAP_TILES_X - 2].flags = TILE_FLAG_SOLID;
        map->cells[y][MAP_TILES_X - 1].tile_id = TILE_ROCK_CENTER;
        map->cells[y][MAP_TILES_X - 1].flags = TILE_FLAG_SOLID;
    }

    // Inner corners connecting boundary walls seamlessly facing the playfield
    map->cells[1][1].tile_id = TILE_ROCK_INNER_BL;
    map->cells[1][MAP_TILES_X - 2].tile_id = TILE_ROCK_INNER_BR;
    map->cells[MAP_TILES_Y - 2][1].tile_id = TILE_ROCK_INNER_TL;
    map->cells[MAP_TILES_Y - 2][MAP_TILES_X - 2].tile_id = TILE_ROCK_INNER_TR;

    // Extreme outer corners
    map->cells[0][0].tile_id = TILE_ROCK_CENTER;
    map->cells[0][MAP_TILES_X - 1].tile_id = TILE_ROCK_CENTER;
    map->cells[MAP_TILES_Y - 1][0].tile_id = TILE_ROCK_CENTER;
    map->cells[MAP_TILES_Y - 1][MAP_TILES_X - 1].tile_id = TILE_ROCK_CENTER;
}

// Helper: Add an elevated rock plateau with properly aligned cliff faces and ramps
static void Tilemap_AddPlateau(TileMap* map, int x1, int y1, int w, int h, int ramp_edge, int ramp_pos) {
    if (w < 4 || h < 4) return;
    int x2 = x1 + w - 1;
    int y2 = y1 + h - 1;
    if (x1 < 3 || x2 >= MAP_TILES_X - 3 || y1 < 3 || y2 >= MAP_TILES_Y - 3) return;

    // Calculate ramp coordinate based on requested edge:
    // 1: South, 2: North, 3: West, 4: East
    int rx = -1, ry = -1;
    if (ramp_edge == 1) { // South edge (bottom)
        rx = x1 + 1 + (ramp_pos % (w - 2));
        ry = y2;
    } else if (ramp_edge == 2) { // North edge (top)
        rx = x1 + 1 + (ramp_pos % (w - 2));
        ry = y1;
    } else if (ramp_edge == 3) { // West edge (left)
        rx = x1;
        ry = y1 + 1 + (ramp_pos % (h - 2));
    } else if (ramp_edge == 4) { // East edge (right)
        rx = x2;
        ry = y1 + 1 + (ramp_pos % (h - 2));
    }

    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            MapCell* c = &map->cells[y][x];
            bool is_top   = (y == y1);
            bool is_bot   = (y == y2);
            bool is_left  = (x == x1);
            bool is_right = (x == x2);

            if (x == rx && y == ry) {
                // Stone ramp cutting through cliff face
                if (ramp_edge == 1)      c->tile_id = TILE_ROCK_RAMP_S;
                else if (ramp_edge == 2) c->tile_id = TILE_ROCK_RAMP_N;
                else if (ramp_edge == 3) c->tile_id = TILE_ROCK_RAMP_W;
                else                     c->tile_id = TILE_ROCK_RAMP_E;
                c->flags = TILE_FLAG_RAMP;
                c->elevation = 1;
            } else if (is_top && is_left) {
                c->tile_id = TILE_ROCK_TL;
                c->flags = TILE_FLAG_SOLID;
            } else if (is_top && is_right) {
                c->tile_id = TILE_ROCK_TR;
                c->flags = TILE_FLAG_SOLID;
            } else if (is_top) {
                c->tile_id = TILE_ROCK_T;
                c->flags = TILE_FLAG_SOLID;
            } else if (is_bot && is_left) {
                c->tile_id = TILE_ROCK_BL;
                c->flags = TILE_FLAG_SOLID;
            } else if (is_bot && is_right) {
                c->tile_id = TILE_ROCK_BR;
                c->flags = TILE_FLAG_SOLID;
            } else if (is_bot) {
                c->tile_id = TILE_ROCK_B;
                c->flags = TILE_FLAG_SOLID;
            } else if (is_left) {
                c->tile_id = TILE_ROCK_L;
                c->flags = TILE_FLAG_SOLID;
            } else if (is_right) {
                c->tile_id = TILE_ROCK_R;
                c->flags = TILE_FLAG_SOLID;
            } else {
                // Flat elevated plateau rock surface
                c->tile_id = TILE_ROCK_CENTER;
                c->flags = TILE_FLAG_PLATEAU;
                c->elevation = 1;
            }
            c->overlay_id = 255;
        }
    }
}

// Helper: Add a water pond with properly aligned shoreline corners and edges
static void Tilemap_AddPond(TileMap* map, int x1, int y1, int w, int h) {
    if (w < 3 || h < 3) return;
    int x2 = x1 + w - 1;
    int y2 = y1 + h - 1;
    if (x1 < 3 || x2 >= MAP_TILES_X - 3 || y1 < 3 || y2 >= MAP_TILES_Y - 3) return;

    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            MapCell* c = &map->cells[y][x];
            bool is_top   = (y == y1);
            bool is_bot   = (y == y2);
            bool is_left  = (x == x1);
            bool is_right = (x == x2);

            c->flags = TILE_FLAG_WATER | TILE_FLAG_SOLID;
            c->elevation = 0;
            c->overlay_id = 255;

            if (is_top && is_left)       c->tile_id = TILE_WATER_TL;
            else if (is_top && is_right)  c->tile_id = TILE_WATER_TR;
            else if (is_top)              c->tile_id = TILE_WATER_T;
            else if (is_bot && is_left)   c->tile_id = TILE_WATER_BL;
            else if (is_bot && is_right)  c->tile_id = TILE_WATER_BR;
            else if (is_bot)              c->tile_id = TILE_WATER_B;
            else if (is_left)             c->tile_id = TILE_WATER_L;
            else if (is_right)            c->tile_id = TILE_WATER_R;
            else {
                int ripple = (x * 13 + y * 23) % 8;
                if (ripple == 0) c->tile_id = TILE_WATER_SHALLOW_1;
                else if (ripple == 1) c->tile_id = TILE_WATER_SHALLOW_2;
                else c->tile_id = TILE_WATER_CENTER;
            }
        }
    }
}

// Helper: Add horizontal pathway
static void Tilemap_AddPathH(TileMap* map, int x1, int x2, int y, int thickness) {
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (x1 < 2) x1 = 2;
    if (x2 >= MAP_TILES_X - 2) x2 = MAP_TILES_X - 3;
    if (y < 2 || y + thickness >= MAP_TILES_Y - 2) return;

    for (int ty = y; ty < y + thickness; ty++) {
        for (int tx = x1; tx <= x2; tx++) {
            MapCell* c = &map->cells[ty][tx];
            // Do not overwrite solid cliffs or water, but allow passing through ramps
            if (c->flags & TILE_FLAG_SOLID) continue;
            if (c->flags & TILE_FLAG_RAMP) continue;

            if (thickness == 1) {
                c->tile_id = TILE_PATH_HORIZ;
            } else if (ty == y) {
                c->tile_id = TILE_PATH_T;
            } else if (ty == y + thickness - 1) {
                c->tile_id = TILE_PATH_B;
            } else {
                c->tile_id = TILE_PATH_CENTER;
            }
            c->flags |= TILE_FLAG_PATH;
            c->overlay_id = 255;
        }
    }
}

// Helper: Add vertical pathway
static void Tilemap_AddPathV(TileMap* map, int x, int y1, int y2, int thickness) {
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    if (y1 < 2) y1 = 2;
    if (y2 >= MAP_TILES_Y - 2) y2 = MAP_TILES_Y - 3;
    if (x < 2 || x + thickness >= MAP_TILES_X - 2) return;

    for (int ty = y1; ty <= y2; ty++) {
        for (int tx = x; tx < x + thickness; tx++) {
            MapCell* c = &map->cells[ty][tx];
            if (c->flags & TILE_FLAG_SOLID) continue;
            if (c->flags & TILE_FLAG_RAMP) continue;

            if (thickness == 1) {
                c->tile_id = TILE_PATH_VERT;
            } else if (tx == x) {
                c->tile_id = TILE_PATH_L;
            } else if (tx == x + thickness - 1) {
                c->tile_id = TILE_PATH_R;
            } else {
                c->tile_id = TILE_PATH_CENTER;
            }
            c->flags |= TILE_FLAG_PATH;
            c->overlay_id = 255;
        }
    }
}

// Helper: Add decorative pebble/flora clusters
static void Tilemap_AddPebbleCluster(TileMap* map, int cx, int cy, int radius) {
    int rad_sq = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > rad_sq) continue;
            int x = cx + dx;
            int y = cy + dy;
            if (x < 2 || x >= MAP_TILES_X - 2 || y < 2 || y >= MAP_TILES_Y - 2) continue;

            MapCell* c = &map->cells[y][x];
            if (c->flags & (TILE_FLAG_SOLID | TILE_FLAG_RAMP | TILE_FLAG_PATH)) continue;

            int r = (x * 7 + y * 13) % 4;
            if (r == 0) c->overlay_id = TILE_DECO_PEBBLE_1;
            else if (r == 1) c->overlay_id = TILE_DECO_PEBBLE_2;
            else if (r == 2) c->overlay_id = TILE_DECO_PEBBLE_3;
            else c->overlay_id = TILE_DECO_FLOWER;
            c->flags |= TILE_FLAG_DECO;
        }
    }
}

BiomeTheme Tilemap_GetBiomeForRoute(int section_idx, int route_idx) {
    if (section_idx == 0 && route_idx == 2) return BIOME_FOREST;     // Viridian Forest
    if (section_idx == 0 && route_idx == 3) return BIOME_GYM_ARENA;  // Pewter Gym
    if (section_idx == 1 && route_idx == 1) return BIOME_CAVERN;     // Mt. Moon
    if (section_idx == 1 && (route_idx == 2 || route_idx == 3)) return (route_idx == 2) ? BIOME_COASTAL : BIOME_GYM_ARENA; // Route 24 / Cerulean Gym
    if (section_idx == 2 && route_idx == 2) return BIOME_CAVERN;     // Diglett's Cave
    if (section_idx == 2 && route_idx == 3) return BIOME_GYM_ARENA;  // Vermilion Gym
    if (section_idx == 3 && route_idx == 1) return BIOME_CAVERN;     // Rock Tunnel
    if (section_idx == 3 && route_idx == 3) return BIOME_GYM_ARENA;  // Celadon Gym
    if (section_idx == 4 && route_idx == 0) return BIOME_CAVERN;     // Pokemon Tower
    if (section_idx == 4 && route_idx == 1) return BIOME_COASTAL;    // Route 12
    if (section_idx == 4 && route_idx == 2) return BIOME_FOREST;     // Safari Zone
    if (section_idx == 4 && route_idx == 3) return BIOME_GYM_ARENA;  // Fuchsia Gym
    if (section_idx == 5 && route_idx == 0) return BIOME_CAVERN;     // Silph Co.
    if (section_idx == 5 && (route_idx == 2 || route_idx == 3)) return BIOME_GYM_ARENA; // Dojo / Saffron Gym
    if (section_idx == 6 && (route_idx == 0 || route_idx == 2)) return BIOME_COASTAL;   // Seafoam / Route 20
    if (section_idx == 6 && (route_idx == 1 || route_idx == 3)) return BIOME_VOLCANIC; // Pokemon Mansion / Cinnabar Gym
    if (section_idx == 7 && route_idx == 0) return BIOME_COASTAL;    // Route 21
    if (section_idx == 7 && route_idx == 1) return BIOME_CAVERN;     // Victory Road
    if (section_idx == 7 && (route_idx == 2 || route_idx == 3)) return BIOME_GYM_ARENA; // Viridian Gym / Indigo Plateau
    return BIOME_PLAINS;
}

const char* Tilemap_GetBiomeName(BiomeTheme biome) {
    switch (biome) {
        case BIOME_FOREST:    return "Viridian Forest / Safari";
        case BIOME_CAVERN:    return "Cavern / Mt. Moon";
        case BIOME_COASTAL:   return "Coastal / Waterway";
        case BIOME_VOLCANIC:  return "Volcanic / Cinnabar";
        case BIOME_GYM_ARENA: return "Gym Arena / Indigo Plateau";
        default:              return "Plains / Meadow";
    }
}

// Helper: Add organic interactive tall grass patches
static void Tilemap_AddTallGrassPatch(TileMap* map, int cx, int cy, int radius) {
    int rad_sq = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > rad_sq) continue;
            int x = cx + dx;
            int y = cy + dy;
            if (x < 3 || x >= MAP_TILES_X - 3 || y < 3 || y >= MAP_TILES_Y - 3) continue;
            if (x >= 58 && x <= 70 && y >= 58 && y <= 70) continue; // Keep player plaza open

            MapCell* c = &map->cells[y][x];
            if (c->flags & (TILE_FLAG_SOLID | TILE_FLAG_RAMP | TILE_FLAG_WATER)) continue;

            c->flags |= TILE_FLAG_TALL_GRASS;
            c->overlay_id = TILE_TALL_GRASS;
        }
    }
}

// Helper: Add a destructible boulder obstacle
void Tilemap_AddBoulder(TileMap* map, int x, int y, float hp) {
    if (x < 3 || x >= MAP_TILES_X - 3 || y < 3 || y >= MAP_TILES_Y - 3) return;
    if (x >= 58 && x <= 70 && y >= 58 && y <= 70) return; // Player plaza
    if (map->boulder_count >= MAX_BOULDERS) return;

    MapCell* c = &map->cells[y][x];
    if (c->flags & (TILE_FLAG_SOLID | TILE_FLAG_RAMP | TILE_FLAG_WATER)) return;
    if (c->elevation != 0) return; // Only place on ground level

    c->tile_id = TILE_ROCK_BOULDER;
    c->flags |= (TILE_FLAG_SOLID | TILE_FLAG_BOULDER);
    c->elevation = 0;
    c->overlay_id = 255;

    DestructibleBoulder* bld = &map->boulders[map->boulder_count++];
    bld->active = true;
    bld->tile_x = x;
    bld->tile_y = y;
    bld->hp = (hp > 0.0f) ? hp : 75.0f;
    bld->max_hp = bld->hp;
    bld->flash_timer = 0.0f;
}

bool Tilemap_DamageBoulder(TileMap* map, int tile_x, int tile_y, float damage, bool* out_destroyed) {
    if (!map) return false;
    if (out_destroyed) *out_destroyed = false;
    for (int i = 0; i < map->boulder_count; i++) {
        DestructibleBoulder* bld = &map->boulders[i];
        if (bld->active && bld->tile_x == tile_x && bld->tile_y == tile_y) {
            bld->hp -= damage;
            bld->flash_timer = 0.15f;
            if (bld->hp <= 0.0f) {
                bld->hp = 0.0f;
                bld->active = false;
                map->cells[tile_y][tile_x].flags &= ~(TILE_FLAG_SOLID | TILE_FLAG_BOULDER);
                map->cells[tile_y][tile_x].tile_id = TILE_GROUND_1;
                map->path_field.valid = false; // Invalidate pathfinding to recalculate through shortcut!
                if (out_destroyed) *out_destroyed = true;
            }
            return true;
        }
    }
    return false;
}

bool Tilemap_DestroyBoulder(TileMap* map, int tile_x, int tile_y) {
    bool destroyed = false;
    return Tilemap_DamageBoulder(map, tile_x, tile_y, 99999.0f, &destroyed);
}

DestructibleBoulder* Tilemap_GetBoulderAt(TileMap* map, int tile_x, int tile_y) {
    if (!map) return NULL;
    for (int i = 0; i < map->boulder_count; i++) {
        if (map->boulders[i].active && map->boulders[i].tile_x == tile_x && map->boulders[i].tile_y == tile_y) {
            return &map->boulders[i];
        }
    }
    return NULL;
}

// Seamless Route Progression Gate
void Tilemap_ActivateExitGate(TileMap* map) {
    if (!map) return;
    map->exit_gate.active = true;
    map->exit_gate.anim_timer = 0.0f;
    map->exit_gate.player_entered = false;

    int gx = map->exit_gate.tile_x;
    int gy = map->exit_gate.tile_y;
    if (gx >= 0 && gx < MAP_TILES_X && gy >= 0 && gy < MAP_TILES_Y) {
        map->cells[gy][gx].flags &= ~TILE_FLAG_SOLID;
        map->cells[gy][gx].flags |= TILE_FLAG_PATH;
        map->cells[gy][gx].tile_id = TILE_PORTAL_GATE;
    }
}

bool Tilemap_CheckExitGateCollision(const TileMap* map, float player_x, float player_y, float player_radius) {
    if (!map || !map->exit_gate.active) return false;
    float dx = player_x - map->exit_gate.x;
    float dy = player_y - map->exit_gate.y;
    float total_r = map->exit_gate.radius + player_radius;
    return (dx * dx + dy * dy <= total_r * total_r);
}

void Tilemap_GetGateDirection(const TileMap* map, float from_x, float from_y, float* out_dir_x, float* out_dir_y, float* out_dist) {
    if (!map || !out_dir_x || !out_dir_y) return;
    float dx = map->exit_gate.x - from_x;
    float dy = map->exit_gate.y - from_y;
    float d = sqrtf(dx * dx + dy * dy);
    if (out_dist) *out_dist = d;
    if (d > 0.001f) {
        *out_dir_x = dx / d;
        *out_dir_y = dy / d;
    } else {
        *out_dir_x = 0.0f;
        *out_dir_y = 0.0f;
    }
}

// Autotiler for 24x24 Rock / Cliff / Plateau tileset rules
static void Tilemap_AutotilePlateaus(TileMap* map) {
    for (int y = 2; y < MAP_TILES_Y - 2; y++) {
        for (int x = 2; x < MAP_TILES_X - 2; x++) {
            MapCell* c = &map->cells[y][x];
            if (c->elevation != 1 || (c->flags & TILE_FLAG_RAMP)) continue;

            bool n = (y > 1 && map->cells[y - 1][x].elevation == 1);
            bool s = (y < MAP_TILES_Y - 2 && map->cells[y + 1][x].elevation == 1);
            bool w = (x > 1 && map->cells[y][x - 1].elevation == 1);
            bool e = (x < MAP_TILES_X - 2 && map->cells[y][x + 1].elevation == 1);

            if (!n && !w) {
                c->tile_id = TILE_ROCK_TL;
                c->flags = TILE_FLAG_SOLID;
            } else if (!n && !e) {
                c->tile_id = TILE_ROCK_TR;
                c->flags = TILE_FLAG_SOLID;
            } else if (!s && !w) {
                c->tile_id = TILE_ROCK_BL;
                c->flags = TILE_FLAG_SOLID;
            } else if (!s && !e) {
                c->tile_id = TILE_ROCK_BR;
                c->flags = TILE_FLAG_SOLID;
            } else if (!n) {
                c->tile_id = TILE_ROCK_T;
                c->flags = TILE_FLAG_SOLID;
            } else if (!s) {
                c->tile_id = TILE_ROCK_B;
                c->flags = TILE_FLAG_SOLID;
            } else if (!w) {
                c->tile_id = TILE_ROCK_L;
                c->flags = TILE_FLAG_SOLID;
            } else if (!e) {
                c->tile_id = TILE_ROCK_R;
                c->flags = TILE_FLAG_SOLID;
            } else {
                // All 4 cardinal neighbors are rock. Check diagonals for inner concave corners!
                bool nw = (map->cells[y - 1][x - 1].elevation == 1);
                bool ne = (map->cells[y - 1][x + 1].elevation == 1);
                bool sw = (map->cells[y + 1][x - 1].elevation == 1);
                bool se = (map->cells[y + 1][x + 1].elevation == 1);

                if (!se) {
                    c->tile_id = TILE_ROCK_INNER_BL;
                    c->flags = TILE_FLAG_SOLID;
                } else if (!sw) {
                    c->tile_id = TILE_ROCK_INNER_BR;
                    c->flags = TILE_FLAG_SOLID;
                } else if (!ne) {
                    c->tile_id = TILE_ROCK_INNER_TL;
                    c->flags = TILE_FLAG_SOLID;
                } else if (!nw) {
                    c->tile_id = TILE_ROCK_INNER_TR;
                    c->flags = TILE_FLAG_SOLID;
                } else {
                    c->tile_id = TILE_ROCK_CENTER;
                    c->flags = TILE_FLAG_PLATEAU;
                }
            }
            c->overlay_id = 255;
        }
    }
}

// Autotiler for 24x24 Water / Pond / Shoreline tileset rules
static void Tilemap_AutotileWater(TileMap* map) {
    for (int y = 2; y < MAP_TILES_Y - 2; y++) {
        for (int x = 2; x < MAP_TILES_X - 2; x++) {
            MapCell* c = &map->cells[y][x];
            if (!(c->flags & TILE_FLAG_WATER)) continue;

            bool n = (y > 1 && (map->cells[y - 1][x].flags & TILE_FLAG_WATER));
            bool s = (y < MAP_TILES_Y - 2 && (map->cells[y + 1][x].flags & TILE_FLAG_WATER));
            bool w = (x > 1 && (map->cells[y][x - 1].flags & TILE_FLAG_WATER));
            bool e = (x < MAP_TILES_X - 2 && (map->cells[y][x + 1].flags & TILE_FLAG_WATER));

            c->flags = TILE_FLAG_WATER | TILE_FLAG_SOLID;
            c->elevation = 0;
            c->overlay_id = 255;

            if (!n && !w) {
                c->tile_id = TILE_WATER_TL;
            } else if (!n && !e) {
                c->tile_id = TILE_WATER_TR;
            } else if (!s && !w) {
                c->tile_id = TILE_WATER_BL;
            } else if (!s && !e) {
                c->tile_id = TILE_WATER_BR;
            } else if (!n) {
                c->tile_id = TILE_WATER_T;
            } else if (!s) {
                c->tile_id = TILE_WATER_B;
            } else if (!w) {
                c->tile_id = TILE_WATER_L;
            } else if (!e) {
                c->tile_id = TILE_WATER_R;
            } else {
                bool nw = (map->cells[y - 1][x - 1].flags & TILE_FLAG_WATER);
                bool ne = (map->cells[y - 1][x + 1].flags & TILE_FLAG_WATER);
                bool sw = (map->cells[y + 1][x - 1].flags & TILE_FLAG_WATER);
                bool se = (map->cells[y + 1][x + 1].flags & TILE_FLAG_WATER);

                if (!se) {
                    c->tile_id = TILE_WATER_INNER_BL;
                } else if (!sw) {
                    c->tile_id = TILE_WATER_INNER_BR;
                } else if (!ne) {
                    c->tile_id = TILE_WATER_INNER_TL;
                } else if (!nw) {
                    c->tile_id = TILE_WATER_INNER_TR;
                } else {
                    if (map->current_biome == BIOME_VOLCANIC) {
                        c->tile_id = TILE_LAVA_CENTER;
                    } else {
                        int ripple = (x * 13 + y * 23) % 9;
                        if (ripple == 0) c->tile_id = TILE_WATER_SHALLOW_1;
                        else if (ripple == 1) c->tile_id = TILE_WATER_SHALLOW_2;
                        else c->tile_id = TILE_WATER_CENTER;
                    }
                }
            }
        }
    }
}

// Procedural organic plateau carving via multi-lobe discs and cellular smoothing
static void Tilemap_CarveOrganicPlateau(TileMap* map, int cx, int cy, int rx, int ry, int lobes) {
    for (int l = 0; l < lobes; l++) {
        int off_x = (l == 0) ? 0 : (int)(Route_Rand() % (rx > 3 ? rx : 4)) - (rx / 2);
        int off_y = (l == 0) ? 0 : (int)(Route_Rand() % (ry > 3 ? ry : 4)) - (ry / 2);
        int l_cx = cx + off_x;
        int l_cy = cy + off_y;
        int l_rx = rx - (int)(Route_Rand() % 3);
        int l_ry = ry - (int)(Route_Rand() % 3);
        if (l_rx < 4) l_rx = 4;
        if (l_ry < 4) l_ry = 4;

        int min_x = l_cx - l_rx;
        int max_x = l_cx + l_rx;
        int min_y = l_cy - l_ry;
        int max_y = l_cy + l_ry;

        if (min_x < 4) min_x = 4;
        if (max_x > MAP_TILES_X - 5) max_x = MAP_TILES_X - 5;
        if (min_y < 4) min_y = 4;
        if (max_y > MAP_TILES_Y - 5) max_y = MAP_TILES_Y - 5;

        for (int y = min_y; y <= max_y; y++) {
            for (int x = min_x; x <= max_x; x++) {
                // Keep player clearing plaza (55..73) strictly open!
                if (x >= 55 && x <= 73 && y >= 55 && y <= 73) continue;
                // Keep arterial roads clear
                if ((x >= 62 && x <= 66) || (y >= 62 && y <= 66)) continue;
                // Keep Exit gate area clear
                if (x >= 60 && x <= 66 && y <= 12) continue;

                float nx = (float)(x - l_cx) / (float)l_rx;
                float ny = (float)(y - l_cy) / (float)l_ry;
                if (nx * nx + ny * ny <= 1.0f) {
                    map->cells[y][x].elevation = 1;
                    map->cells[y][x].flags = TILE_FLAG_PLATEAU;
                }
            }
        }
    }
}

// Procedural organic pond/lake carving via multi-lobe discs
static void Tilemap_CarveOrganicPond(TileMap* map, int cx, int cy, int rx, int ry, int lobes) {
    for (int l = 0; l < lobes; l++) {
        int off_x = (l == 0) ? 0 : (int)(Route_Rand() % (rx > 3 ? rx : 4)) - (rx / 2);
        int off_y = (l == 0) ? 0 : (int)(Route_Rand() % (ry > 3 ? ry : 4)) - (ry / 2);
        int l_cx = cx + off_x;
        int l_cy = cy + off_y;
        int l_rx = rx - (int)(Route_Rand() % 3);
        int l_ry = ry - (int)(Route_Rand() % 3);
        if (l_rx < 3) l_rx = 3;
        if (l_ry < 3) l_ry = 3;

        int min_x = l_cx - l_rx;
        int max_x = l_cx + l_rx;
        int min_y = l_cy - l_ry;
        int max_y = l_cy + l_ry;

        if (min_x < 4) min_x = 4;
        if (max_x > MAP_TILES_X - 5) max_x = MAP_TILES_X - 5;
        if (min_y < 4) min_y = 4;
        if (max_y > MAP_TILES_Y - 5) max_y = MAP_TILES_Y - 5;

        for (int y = min_y; y <= max_y; y++) {
            for (int x = min_x; x <= max_x; x++) {
                if (x >= 55 && x <= 73 && y >= 55 && y <= 73) continue;
                if ((x >= 62 && x <= 66) || (y >= 62 && y <= 66)) continue;
                if (x >= 60 && x <= 66 && y <= 12) continue;
                // Never overwrite elevated rock with water
                if (map->cells[y][x].elevation == 1) continue;

                float nx = (float)(x - l_cx) / (float)l_rx;
                float ny = (float)(y - l_cy) / (float)l_ry;
                if (nx * nx + ny * ny <= 1.0f) {
                    map->cells[y][x].flags |= (TILE_FLAG_WATER | TILE_FLAG_SOLID);
                    map->cells[y][x].elevation = 0;
                }
            }
        }
    }
}

// Automatic Ramp Solver: Guarantees every plateau component has accessible, correctly-oriented ramps
void Tilemap_SolvePlateauRamps(TileMap* map) {
    if (!map) return;

    static uint8_t visited[MAP_TILES_Y][MAP_TILES_X];
    memset(visited, 0, sizeof(visited));

    for (int start_y = 3; start_y < MAP_TILES_Y - 3; start_y++) {
        for (int start_x = 3; start_x < MAP_TILES_X - 3; start_x++) {
            if (visited[start_y][start_x]) continue;
            if (map->cells[start_y][start_x].elevation != 1) continue;

            // BFS flood-fill this plateau component
            bool has_ramp = false;
            static uint16_t comp_queue[MAP_TILES_Y * MAP_TILES_X];
            int q_head = 0, q_tail = 0;

            visited[start_y][start_x] = 1;
            comp_queue[q_tail++] = (uint16_t)((start_y << 7) | start_x);

            int best_rx = -1, best_ry = -1, best_edge = 0; // 1: S, 2: N, 3: W, 4: E
            int best_score = -9999;

            while (q_head < q_tail) {
                uint16_t curr = comp_queue[q_head++];
                int cx = curr & 0x7F;
                int cy = (curr >> 7) & 0x7F;

                if (map->cells[cy][cx].flags & TILE_FLAG_RAMP) {
                    has_ramp = true;
                }

                if (!has_ramp) {
                    // Test South edge: ramp cut through South cliff
                    if (cy + 1 < MAP_TILES_Y - 2 && cy - 1 >= 2) {
                        MapCell* ground = &map->cells[cy + 1][cx];
                        MapCell* plateau = &map->cells[cy - 1][cx];
                        if (ground->elevation == 0 && !(ground->flags & (TILE_FLAG_SOLID | TILE_FLAG_WATER)) &&
                            plateau->elevation == 1) {
                            int score = 100 - abs(cx - 64) - abs((cy + 1) - 64);
                            if (cx > 2 && cx < MAP_TILES_X - 2 &&
                                map->cells[cy][cx - 1].elevation == 1 &&
                                map->cells[cy][cx + 1].elevation == 1) {
                                score += 50; // Clean cliff notch!
                            }
                            if (score > best_score) {
                                best_score = score;
                                best_rx = cx; best_ry = cy; best_edge = 1;
                            }
                        }
                    }
                    // Test North edge: ramp cut through North cliff
                    if (cy - 1 > 1 && cy + 1 < MAP_TILES_Y - 2) {
                        MapCell* ground = &map->cells[cy - 1][cx];
                        MapCell* plateau = &map->cells[cy + 1][cx];
                        if (ground->elevation == 0 && !(ground->flags & (TILE_FLAG_SOLID | TILE_FLAG_WATER)) &&
                            plateau->elevation == 1) {
                            int score = 90 - abs(cx - 64) - abs((cy - 1) - 64);
                            if (cx > 2 && cx < MAP_TILES_X - 2 &&
                                map->cells[cy][cx - 1].elevation == 1 &&
                                map->cells[cy][cx + 1].elevation == 1) {
                                score += 50;
                            }
                            if (score > best_score) {
                                best_score = score;
                                best_rx = cx; best_ry = cy; best_edge = 2;
                            }
                        }
                    }
                    // Test West edge: ramp cut through West cliff
                    if (cx - 1 > 1 && cx + 1 < MAP_TILES_X - 2) {
                        MapCell* ground = &map->cells[cy][cx - 1];
                        MapCell* plateau = &map->cells[cy][cx + 1];
                        if (ground->elevation == 0 && !(ground->flags & (TILE_FLAG_SOLID | TILE_FLAG_WATER)) &&
                            plateau->elevation == 1) {
                            int score = 80 - abs((cx - 1) - 64) - abs(cy - 64);
                            if (cy > 2 && cy < MAP_TILES_Y - 2 &&
                                map->cells[cy - 1][cx].elevation == 1 &&
                                map->cells[cy + 1][cx].elevation == 1) {
                                score += 50;
                            }
                            if (score > best_score) {
                                best_score = score;
                                best_rx = cx; best_ry = cy; best_edge = 3;
                            }
                        }
                    }
                    // Test East edge: ramp cut through East cliff
                    if (cx + 1 < MAP_TILES_X - 2 && cx - 1 >= 2) {
                        MapCell* ground = &map->cells[cy][cx + 1];
                        MapCell* plateau = &map->cells[cy][cx - 1];
                        if (ground->elevation == 0 && !(ground->flags & (TILE_FLAG_SOLID | TILE_FLAG_WATER)) &&
                            plateau->elevation == 1) {
                            int score = 80 - abs((cx + 1) - 64) - abs(cy - 64);
                            if (cy > 2 && cy < MAP_TILES_Y - 2 &&
                                map->cells[cy - 1][cx].elevation == 1 &&
                                map->cells[cy + 1][cx].elevation == 1) {
                                score += 50;
                            }
                            if (score > best_score) {
                                best_score = score;
                                best_rx = cx; best_ry = cy; best_edge = 4;
                            }
                        }
                    }
                }

                static const int dx[4] = { 0, 0, -1, 1 };
                static const int dy[4] = { -1, 1, 0, 0 };
                for (int i = 0; i < 4; i++) {
                    int nx = cx + dx[i];
                    int ny = cy + dy[i];
                    if (nx >= 2 && nx < MAP_TILES_X - 2 && ny >= 2 && ny < MAP_TILES_Y - 2) {
                        if (!visited[ny][nx] && map->cells[ny][nx].elevation == 1) {
                            visited[ny][nx] = 1;
                            comp_queue[q_tail++] = (uint16_t)((ny << 7) | nx);
                        }
                    }
                }
            }

            if (!has_ramp && best_rx > 0 && best_ry > 0 && best_edge > 0) {
                MapCell* rc = &map->cells[best_ry][best_rx];
                if (best_edge == 1)      rc->tile_id = TILE_ROCK_RAMP_S;
                else if (best_edge == 2) rc->tile_id = TILE_ROCK_RAMP_N;
                else if (best_edge == 3) rc->tile_id = TILE_ROCK_RAMP_W;
                else                     rc->tile_id = TILE_ROCK_RAMP_E;
                rc->flags = TILE_FLAG_RAMP;
                rc->elevation = 1;
                rc->overlay_id = 255;

                // Ground side
                int gx = best_rx, gy = best_ry;
                if (best_edge == 1) gy++;
                else if (best_edge == 2) gy--;
                else if (best_edge == 3) gx--;
                else gx++;

                if (gx >= 2 && gx < MAP_TILES_X - 2 && gy >= 2 && gy < MAP_TILES_Y - 2) {
                    map->cells[gy][gx].flags &= ~(TILE_FLAG_SOLID | TILE_FLAG_WATER | TILE_FLAG_BOULDER);
                    map->cells[gy][gx].flags |= TILE_FLAG_PATH;
                    map->cells[gy][gx].elevation = 0;
                    map->cells[gy][gx].tile_id = TILE_PATH_CENTER;
                    map->cells[gy][gx].overlay_id = 255;

                    // Automatically carve connecting pathway to the nearest central arterial road
                    if (best_edge == 1 || best_edge == 2) {
                        Tilemap_AddPathV(map, gx, gy, 63, 1);
                    } else {
                        Tilemap_AddPathH(map, gx, 63, gy, 1);
                    }
                }

                // Plateau side: guarantee landing tile is walkable flat plateau rock!
                int px = best_rx, py = best_ry;
                if (best_edge == 1) py--;
                else if (best_edge == 2) py++;
                else if (best_edge == 3) px++;
                else px--;

                if (px >= 2 && px < MAP_TILES_X - 2 && py >= 2 && py < MAP_TILES_Y - 2) {
                    map->cells[py][px].flags &= ~(TILE_FLAG_SOLID | TILE_FLAG_WATER | TILE_FLAG_BOULDER);
                    map->cells[py][px].flags |= TILE_FLAG_PLATEAU;
                    map->cells[py][px].elevation = 1;
                    map->cells[py][px].tile_id = TILE_ROCK_CENTER;
                    map->cells[py][px].overlay_id = 255;
                }
            }
        }
    }

    // Safety verification: guarantee no ramp directly opens into water
    for (int y = 2; y < MAP_TILES_Y - 2; y++) {
        for (int x = 2; x < MAP_TILES_X - 2; x++) {
            if (map->cells[y][x].flags & TILE_FLAG_RAMP) {
                static const int n_dx[8] = { 0, 0, -1, 1, -1, 1, -1, 1 };
                static const int n_dy[8] = { -1, 1, 0, 0, -1, -1, 1, 1 };
                for (int i = 0; i < 8; i++) {
                    int nx = x + n_dx[i];
                    int ny = y + n_dy[i];
                    if (map->cells[ny][nx].flags & TILE_FLAG_WATER) {
                        map->cells[ny][nx].flags &= ~TILE_FLAG_WATER;
                        map->cells[ny][nx].flags &= ~TILE_FLAG_SOLID;
                        map->cells[ny][nx].flags |= TILE_FLAG_PATH;
                        map->cells[ny][nx].tile_id = TILE_PATH_CENTER;
                    }
                }
            }
        }
    }
}

void Tilemap_GenerateRouteMap(TileMap* map, int section_idx, int route_idx) {
    if (!map) return;
    map->current_section = section_idx;
    map->current_route = route_idx;

    Route_Srand((uint32_t)(section_idx * 100 + route_idx + 1));

    Tilemap_Init(map);
    map->current_biome = Tilemap_GetBiomeForRoute(section_idx, route_idx);
    Tilemap_AddBoundaryWalls(map);

    // Guaranteed open player plaza around center (64, 64)
    // Clearing 56..72 on both axes guarantees room for player, companions, and kiting
    for (int y = 56; y <= 72; y++) {
        for (int x = 56; x <= 72; x++) {
            map->cells[y][x].tile_id = TILE_GROUND_1 + ((x + y) % 3);
            map->cells[y][x].flags = TILE_FLAG_NONE;
            map->cells[y][x].overlay_id = 255;
        }
    }

    // Cross-world central arterial roads
    Tilemap_AddPathH(map, 6, 121, 63, 3);
    Tilemap_AddPathV(map, 63, 6, 121, 3);

    // Setup Exit Gate / Poké-Portal along North arterial road
    map->exit_gate.active = false;
    map->exit_gate.tile_x = 63;
    map->exit_gate.tile_y = 8;
    map->exit_gate.x = (63 + 0.5f) * TILE_WORLD_SIZE;
    map->exit_gate.y = (8 + 0.5f) * TILE_WORLD_SIZE;
    map->exit_gate.radius = 52.0f;
    map->exit_gate.anim_timer = 0.0f;
    map->exit_gate.player_entered = false;

    // Route (0, 0) baseline layout preserves canonical coordinates for Unit Tests 12 & 15
    if (section_idx == 0 && route_idx == 0) {
        Tilemap_AddPlateau(map, 18, 18, 16, 12, 1, 7);  // South ramp at (26, 29)
        Tilemap_AddPlateau(map, 94, 20, 16, 12, 1, 7);  // South ramp at (102, 31)
        Tilemap_AddPlateau(map, 20, 98, 16, 12, 2, 7);  // North ramp at (28, 98)
        Tilemap_AddPlateau(map, 92, 96, 16, 12, 2, 7);  // North ramp at (100, 96)

        Tilemap_AddPond(map, 44, 26, 10, 8);
        Tilemap_AddPond(map, 74, 94, 10, 8);
        Tilemap_AddPond(map, 20, 60, 8, 10);
        Tilemap_AddPond(map, 100, 60, 8, 10);

        Tilemap_AddPathV(map, 26, 30, 63, 1);
        Tilemap_AddPathV(map, 102, 32, 63, 1);
        Tilemap_AddPathV(map, 28, 63, 97, 1);
        Tilemap_AddPathV(map, 100, 63, 95, 1);

        Tilemap_AddTallGrassPatch(map, 36, 46, 3);
        Tilemap_AddTallGrassPatch(map, 92, 46, 3);
        Tilemap_AddTallGrassPatch(map, 36, 82, 3);
        Tilemap_AddTallGrassPatch(map, 92, 82, 3);
        Tilemap_AddTallGrassPatch(map, 48, 38, 3);
        Tilemap_AddTallGrassPatch(map, 80, 38, 3);

        Tilemap_AddBoulder(map, 38, 34, 75.0f);
        Tilemap_AddBoulder(map, 90, 34, 75.0f);
        Tilemap_AddBoulder(map, 38, 94, 75.0f);
        Tilemap_AddBoulder(map, 90, 94, 75.0f);
        Tilemap_AddBoulder(map, 54, 52, 75.0f);
        Tilemap_AddBoulder(map, 74, 52, 75.0f);

        for (int gx = 20; gx <= 108; gx += 22) {
            for (int gy = 20; gy <= 108; gy += 22) {
                if (abs(gx - 64) < 14 && abs(gy - 64) < 14) continue;
                Tilemap_AddPebbleCluster(map, gx + (int)(Route_Rand() % 5), gy + (int)(Route_Rand() % 5), 4);
            }
        }
    } else {
        // DYNAMIC PROCEDURAL GENERATION: organic winding plateaus, lakes, pathways, grass, and boulders
        switch (map->current_biome) {
            case BIOME_FOREST: { // Dense winding forest clearings, safari tall grass fields
                int nw_x = 22 + (int)(Route_Rand() % 8), nw_y = 22 + (int)(Route_Rand() % 8);
                int ne_x = 90 + (int)(Route_Rand() % 8), ne_y = 22 + (int)(Route_Rand() % 8);
                int sw_x = 22 + (int)(Route_Rand() % 8), sw_y = 90 + (int)(Route_Rand() % 8);
                int se_x = 90 + (int)(Route_Rand() % 8), se_y = 90 + (int)(Route_Rand() % 8);

                Tilemap_CarveOrganicPlateau(map, nw_x, nw_y, 10, 8, 4);
                Tilemap_CarveOrganicPlateau(map, ne_x, ne_y, 10, 8, 4);
                Tilemap_CarveOrganicPlateau(map, sw_x, sw_y, 10, 8, 4);
                Tilemap_CarveOrganicPlateau(map, se_x, se_y, 10, 8, 4);

                Tilemap_CarveOrganicPond(map, 44 + (int)(Route_Rand() % 6), 26 + (int)(Route_Rand() % 6), 7, 6, 3);
                Tilemap_CarveOrganicPond(map, 76 + (int)(Route_Rand() % 6), 96 + (int)(Route_Rand() % 6), 7, 6, 3);
                Tilemap_CarveOrganicPond(map, 22 + (int)(Route_Rand() % 6), 60 + (int)(Route_Rand() % 6), 6, 8, 3);
                Tilemap_CarveOrganicPond(map, 98 + (int)(Route_Rand() % 6), 60 + (int)(Route_Rand() % 6), 6, 8, 3);

                // Extensive Safari / Woodland Tall Grass Patches
                Tilemap_AddTallGrassPatch(map, 38, 38, 4);
                Tilemap_AddTallGrassPatch(map, 88, 38, 4);
                Tilemap_AddTallGrassPatch(map, 38, 88, 4);
                Tilemap_AddTallGrassPatch(map, 88, 88, 4);
                Tilemap_AddTallGrassPatch(map, 48, 54, 3);
                Tilemap_AddTallGrassPatch(map, 80, 54, 3);
                Tilemap_AddTallGrassPatch(map, 48, 72, 3);
                Tilemap_AddTallGrassPatch(map, 80, 72, 3);

                // Overgrown forest shortcut boulders
                Tilemap_AddBoulder(map, 34, 42, 60.0f);
                Tilemap_AddBoulder(map, 94, 42, 60.0f);
                Tilemap_AddBoulder(map, 34, 86, 60.0f);
                Tilemap_AddBoulder(map, 94, 86, 60.0f);
                Tilemap_AddBoulder(map, 54, 34, 60.0f);
                Tilemap_AddBoulder(map, 74, 34, 60.0f);

                for (int gx = 16; gx <= 112; gx += 16) {
                    for (int gy = 16; gy <= 112; gy += 16) {
                        if (abs(gx - 64) < 14 && abs(gy - 64) < 14) continue;
                        Tilemap_AddPebbleCluster(map, gx + (int)(Route_Rand() % 5), gy + (int)(Route_Rand() % 5), 5);
                    }
                }
                break;
            }

            case BIOME_CAVERN: { // Mountain & Cavern: multi-tiered canyon ridges and crystal springs
                int nw_x = 18 + (int)(Route_Rand() % 6), nw_y = 18 + (int)(Route_Rand() % 6);
                int ne_x = 92 + (int)(Route_Rand() % 6), ne_y = 18 + (int)(Route_Rand() % 6);
                int sw_x = 18 + (int)(Route_Rand() % 6), sw_y = 92 + (int)(Route_Rand() % 6);
                int se_x = 92 + (int)(Route_Rand() % 6), se_y = 92 + (int)(Route_Rand() % 6);

                Tilemap_CarveOrganicPlateau(map, nw_x, nw_y, 12, 10, 5);
                Tilemap_CarveOrganicPlateau(map, ne_x, ne_y, 12, 10, 5);
                Tilemap_CarveOrganicPlateau(map, sw_x, sw_y, 12, 10, 5);
                Tilemap_CarveOrganicPlateau(map, se_x, se_y, 12, 10, 5);

                // Canyon flank plateaus
                Tilemap_CarveOrganicPlateau(map, 38 + (int)(Route_Rand() % 4), 38 + (int)(Route_Rand() % 4), 8, 6, 3);
                Tilemap_CarveOrganicPlateau(map, 82 + (int)(Route_Rand() % 4), 38 + (int)(Route_Rand() % 4), 8, 6, 3);
                Tilemap_CarveOrganicPlateau(map, 38 + (int)(Route_Rand() % 4), 84 + (int)(Route_Rand() % 4), 8, 6, 3);
                Tilemap_CarveOrganicPlateau(map, 82 + (int)(Route_Rand() % 4), 84 + (int)(Route_Rand() % 4), 8, 6, 3);

                // Crystal cave springs
                Tilemap_CarveOrganicPond(map, 60, 22, 6, 6, 2);
                Tilemap_CarveOrganicPond(map, 60, 102, 6, 6, 2);

                // Rocky chokepoints and shortcut boulders
                Tilemap_AddBoulder(map, 28, 52, 80.0f);
                Tilemap_AddBoulder(map, 100, 52, 80.0f);
                Tilemap_AddBoulder(map, 28, 76, 80.0f);
                Tilemap_AddBoulder(map, 100, 76, 80.0f);
                Tilemap_AddBoulder(map, 50, 24, 80.0f);
                Tilemap_AddBoulder(map, 78, 24, 80.0f);
                Tilemap_AddBoulder(map, 50, 104, 80.0f);
                Tilemap_AddBoulder(map, 78, 104, 80.0f);
                Tilemap_AddBoulder(map, 34, 63, 80.0f);
                Tilemap_AddBoulder(map, 94, 63, 80.0f);

                Tilemap_AddTallGrassPatch(map, 54, 30, 3);
                Tilemap_AddTallGrassPatch(map, 74, 30, 3);
                Tilemap_AddTallGrassPatch(map, 54, 98, 3);
                Tilemap_AddTallGrassPatch(map, 74, 98, 3);

                Tilemap_AddPebbleCluster(map, 32, 50, 4);
                Tilemap_AddPebbleCluster(map, 96, 50, 4);
                Tilemap_AddPebbleCluster(map, 32, 78, 4);
                Tilemap_AddPebbleCluster(map, 96, 78, 4);
                break;
            }

            case BIOME_COASTAL: { // Coastal waterways, island plateaus, shoreline reeds
                Tilemap_CarveOrganicPond(map, 16 + (int)(Route_Rand() % 6), 18 + (int)(Route_Rand() % 6), 14, 12, 4);
                Tilemap_CarveOrganicPond(map, 104 + (int)(Route_Rand() % 6), 18 + (int)(Route_Rand() % 6), 14, 12, 4);
                Tilemap_CarveOrganicPond(map, 16 + (int)(Route_Rand() % 6), 98 + (int)(Route_Rand() % 6), 14, 12, 4);
                Tilemap_CarveOrganicPond(map, 104 + (int)(Route_Rand() % 6), 98 + (int)(Route_Rand() % 6), 14, 12, 4);

                Tilemap_CarveOrganicPond(map, 46, 48, 8, 7, 3);
                Tilemap_CarveOrganicPond(map, 78, 48, 8, 7, 3);
                Tilemap_CarveOrganicPond(map, 46, 76, 8, 7, 3);
                Tilemap_CarveOrganicPond(map, 78, 76, 8, 7, 3);

                // Island plateaus on dry land between coastal waters
                Tilemap_CarveOrganicPlateau(map, 34, 20, 8, 6, 3);
                Tilemap_CarveOrganicPlateau(map, 88, 20, 8, 6, 3);
                Tilemap_CarveOrganicPlateau(map, 34, 104, 8, 6, 3);
                Tilemap_CarveOrganicPlateau(map, 88, 104, 8, 6, 3);

                Tilemap_AddTallGrassPatch(map, 28, 36, 3);
                Tilemap_AddTallGrassPatch(map, 98, 36, 3);
                Tilemap_AddTallGrassPatch(map, 28, 90, 3);
                Tilemap_AddTallGrassPatch(map, 98, 90, 3);

                Tilemap_AddBoulder(map, 26, 44, 70.0f);
                Tilemap_AddBoulder(map, 102, 44, 70.0f);
                Tilemap_AddBoulder(map, 26, 84, 70.0f);
                Tilemap_AddBoulder(map, 102, 84, 70.0f);

                Tilemap_AddPebbleCluster(map, 40, 26, 4);
                Tilemap_AddPebbleCluster(map, 88, 26, 4);
                Tilemap_AddPebbleCluster(map, 40, 102, 4);
                Tilemap_AddPebbleCluster(map, 88, 102, 4);
                break;
            }

            case BIOME_VOLCANIC: { // Volcanic basalt plateaus & glowing molten lava hazard pools
                int nw_x = 20 + (int)(Route_Rand() % 6), nw_y = 20 + (int)(Route_Rand() % 6);
                int ne_x = 92 + (int)(Route_Rand() % 6), ne_y = 22 + (int)(Route_Rand() % 6);
                int sw_x = 22 + (int)(Route_Rand() % 6), sw_y = 96 + (int)(Route_Rand() % 6);
                int se_x = 90 + (int)(Route_Rand() % 6), se_y = 94 + (int)(Route_Rand() % 6);

                Tilemap_CarveOrganicPlateau(map, nw_x, nw_y, 11, 9, 4);
                Tilemap_CarveOrganicPlateau(map, ne_x, ne_y, 11, 9, 4);
                Tilemap_CarveOrganicPlateau(map, sw_x, sw_y, 11, 9, 4);
                Tilemap_CarveOrganicPlateau(map, se_x, se_y, 11, 9, 4);

                // Glowing molten lava pools dealing ground damage
                Tilemap_CarveOrganicPond(map, 44 + (int)(Route_Rand() % 6), 26 + (int)(Route_Rand() % 6), 8, 6, 3);
                Tilemap_CarveOrganicPond(map, 76 + (int)(Route_Rand() % 6), 94 + (int)(Route_Rand() % 6), 8, 6, 3);
                Tilemap_CarveOrganicPond(map, 20 + (int)(Route_Rand() % 6), 60 + (int)(Route_Rand() % 6), 6, 8, 3);
                Tilemap_CarveOrganicPond(map, 100 + (int)(Route_Rand() % 6), 60 + (int)(Route_Rand() % 6), 6, 8, 3);

                // Volcanic destructible boulders
                Tilemap_AddBoulder(map, 34, 40, 85.0f);
                Tilemap_AddBoulder(map, 94, 40, 85.0f);
                Tilemap_AddBoulder(map, 34, 88, 85.0f);
                Tilemap_AddBoulder(map, 94, 88, 85.0f);
                Tilemap_AddBoulder(map, 48, 63, 85.0f);
                Tilemap_AddBoulder(map, 80, 63, 85.0f);
                Tilemap_AddBoulder(map, 63, 38, 85.0f);
                Tilemap_AddBoulder(map, 63, 90, 85.0f);

                for (int gx = 20; gx <= 108; gx += 22) {
                    for (int gy = 20; gy <= 108; gy += 22) {
                        if (abs(gx - 64) < 14 && abs(gy - 64) < 14) continue;
                        Tilemap_AddPebbleCluster(map, gx + (int)(Route_Rand() % 5), gy + (int)(Route_Rand() % 5), 4);
                    }
                }
                break;
            }

            case BIOME_GYM_ARENA: { // Grand arena with championship viewing plateaus & reflecting pools
                Tilemap_CarveOrganicPlateau(map, 34, 34, 12, 10, 3);
                Tilemap_CarveOrganicPlateau(map, 86, 34, 12, 10, 3);
                Tilemap_CarveOrganicPlateau(map, 34, 88, 12, 10, 3);
                Tilemap_CarveOrganicPlateau(map, 86, 88, 12, 10, 3);

                // Ornamental reflecting pools
                Tilemap_CarveOrganicPond(map, 52, 28, 6, 6, 2);
                Tilemap_CarveOrganicPond(map, 68, 28, 6, 6, 2);
                Tilemap_CarveOrganicPond(map, 52, 92, 6, 6, 2);
                Tilemap_CarveOrganicPond(map, 68, 92, 6, 6, 2);

                // Ceremonial perimeter pathways
                Tilemap_AddPathH(map, 20, 108, 48, 2);
                Tilemap_AddPathH(map, 20, 108, 80, 2);
                Tilemap_AddPathV(map, 48, 20, 108, 2);
                Tilemap_AddPathV(map, 80, 20, 108, 2);

                Tilemap_AddTallGrassPatch(map, 38, 22, 3);
                Tilemap_AddTallGrassPatch(map, 90, 22, 3);
                Tilemap_AddTallGrassPatch(map, 38, 106, 3);
                Tilemap_AddTallGrassPatch(map, 90, 106, 3);

                Tilemap_AddBoulder(map, 22, 48, 90.0f);
                Tilemap_AddBoulder(map, 106, 48, 90.0f);
                Tilemap_AddBoulder(map, 22, 80, 90.0f);
                Tilemap_AddBoulder(map, 106, 80, 90.0f);

                Tilemap_AddPebbleCluster(map, 24, 24, 4);
                Tilemap_AddPebbleCluster(map, 104, 24, 4);
                Tilemap_AddPebbleCluster(map, 24, 104, 4);
                Tilemap_AddPebbleCluster(map, 104, 104, 4);
                break;
            }

            default: { // BIOME_PLAINS: rolling organic plateaus, ponds, meadow grass, boulders
                int nw_x = 20 + (int)(Route_Rand() % 8), nw_y = 20 + (int)(Route_Rand() % 8);
                int ne_x = 92 + (int)(Route_Rand() % 8), ne_y = 22 + (int)(Route_Rand() % 8);
                int sw_x = 22 + (int)(Route_Rand() % 8), sw_y = 96 + (int)(Route_Rand() % 8);
                int se_x = 90 + (int)(Route_Rand() % 8), se_y = 94 + (int)(Route_Rand() % 8);

                Tilemap_CarveOrganicPlateau(map, nw_x, nw_y, 11, 9, 4);
                Tilemap_CarveOrganicPlateau(map, ne_x, ne_y, 11, 9, 4);
                Tilemap_CarveOrganicPlateau(map, sw_x, sw_y, 11, 9, 4);
                Tilemap_CarveOrganicPlateau(map, se_x, se_y, 11, 9, 4);

                Tilemap_CarveOrganicPond(map, 44 + (int)(Route_Rand() % 6), 26 + (int)(Route_Rand() % 6), 7, 6, 3);
                Tilemap_CarveOrganicPond(map, 74 + (int)(Route_Rand() % 6), 94 + (int)(Route_Rand() % 6), 7, 6, 3);
                Tilemap_CarveOrganicPond(map, 20 + (int)(Route_Rand() % 6), 60 + (int)(Route_Rand() % 6), 6, 8, 3);
                Tilemap_CarveOrganicPond(map, 100 + (int)(Route_Rand() % 6), 60 + (int)(Route_Rand() % 6), 6, 8, 3);

                Tilemap_AddTallGrassPatch(map, 36, 46, 3);
                Tilemap_AddTallGrassPatch(map, 92, 46, 3);
                Tilemap_AddTallGrassPatch(map, 36, 82, 3);
                Tilemap_AddTallGrassPatch(map, 92, 82, 3);
                Tilemap_AddTallGrassPatch(map, 48, 38, 3);
                Tilemap_AddTallGrassPatch(map, 80, 38, 3);

                Tilemap_AddBoulder(map, 38, 34, 75.0f);
                Tilemap_AddBoulder(map, 90, 34, 75.0f);
                Tilemap_AddBoulder(map, 38, 94, 75.0f);
                Tilemap_AddBoulder(map, 90, 94, 75.0f);
                Tilemap_AddBoulder(map, 54, 52, 75.0f);
                Tilemap_AddBoulder(map, 74, 52, 75.0f);

                for (int gx = 20; gx <= 108; gx += 22) {
                    for (int gy = 20; gy <= 108; gy += 22) {
                        if (abs(gx - 64) < 14 && abs(gy - 64) < 14) continue;
                        Tilemap_AddPebbleCluster(map, gx + (int)(Route_Rand() % 5), gy + (int)(Route_Rand() % 5), 4);
                    }
                }
                break;
            }
        }
    }

    // Run autotilers on procedural routes to enforce 24x24 tileset edge/corner/inner-corner autotiling rules
    if (section_idx != 0 || route_idx != 0) {
        Tilemap_AutotilePlateaus(map);
        Tilemap_AutotileWater(map);
    }

    // Automatic ramp solver validation pass: guarantees every plateau has an accessible ramp
    Tilemap_SolvePlateauRamps(map);

    // Final guarantee: Exit Gate at (63, 8) along North road is completely clear of obstacles
    int gx = map->exit_gate.tile_x;
    int gy = map->exit_gate.tile_y;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int tx = gx + dx;
            int ty = gy + dy;
            if (tx >= 2 && tx < MAP_TILES_X - 2 && ty >= 2 && ty < MAP_TILES_Y - 2) {
                map->cells[ty][tx].flags &= ~(TILE_FLAG_SOLID | TILE_FLAG_WATER | TILE_FLAG_BOULDER);
                map->cells[ty][tx].flags |= TILE_FLAG_PATH;
                map->cells[ty][tx].elevation = 0;
                map->cells[ty][tx].tile_id = TILE_PATH_CENTER;
                map->cells[ty][tx].overlay_id = 255;
            }
        }
    }
}

static void Tilemap_ApplyBiomeTint(SDL_Texture* tileset_tex, BiomeTheme biome) {
    if (!tileset_tex) return;
    switch (biome) {
        case BIOME_FOREST:
            SDL_SetTextureColorMod(tileset_tex, 220, 255, 215); // Lush green tint
            break;
        case BIOME_CAVERN:
            SDL_SetTextureColorMod(tileset_tex, 180, 195, 220); // Dark slate cave tint
            break;
        case BIOME_COASTAL:
            SDL_SetTextureColorMod(tileset_tex, 255, 250, 230); // Warm sunny coastal tint
            break;
        case BIOME_VOLCANIC:
            SDL_SetTextureColorMod(tileset_tex, 255, 185, 150); // Dark basalt amber tint
            break;
        case BIOME_GYM_ARENA:
            SDL_SetTextureColorMod(tileset_tex, 245, 235, 255); // Ceremonial arena tint
            break;
        default: // BIOME_PLAINS
            SDL_SetTextureColorMod(tileset_tex, 255, 255, 255);
            break;
    }
}

void Tilemap_Render(const TileMap* map, SDL_Renderer* renderer, SDL_Texture* tileset_tex, float camera_x, float camera_y) {
    if (!map || !renderer) return;

    // Viewport frustum culling: calculate visible tile range with 1-tile safety margin
    int start_tx = (int)floorf(camera_x / TILE_WORLD_SIZE) - 1;
    int end_tx   = (int)ceilf((camera_x + WINDOW_WIDTH) / TILE_WORLD_SIZE) + 1;
    int start_ty = (int)floorf(camera_y / TILE_WORLD_SIZE) - 1;
    int end_ty   = (int)ceilf((camera_y + WINDOW_HEIGHT) / TILE_WORLD_SIZE) + 1;

    if (start_tx < 0) start_tx = 0;
    if (end_tx >= MAP_TILES_X) end_tx = MAP_TILES_X - 1;
    if (start_ty < 0) start_ty = 0;
    if (end_ty >= MAP_TILES_Y) end_ty = MAP_TILES_Y - 1;

    // Thematic Biome Visual Styling / Palette Tinting
    Tilemap_ApplyBiomeTint(tileset_tex, map->current_biome);

    for (int ty = start_ty; ty <= end_ty; ty++) {
        for (int tx = start_tx; tx <= end_tx; tx++) {
            const MapCell* cell = &map->cells[ty][tx];
            SDL_Rect dst = {
                (int)(tx * TILE_WORLD_SIZE - camera_x),
                (int)(ty * TILE_WORLD_SIZE - camera_y),
                (int)TILE_WORLD_SIZE,
                (int)TILE_WORLD_SIZE
            };

            if (tileset_tex && cell->tile_id < TILE_TOTAL_COUNT) {
                SDL_Rect src = s_tile_src_rects[cell->tile_id];
                
                // Volcanic lava hazard visual effect
                if (map->current_biome == BIOME_VOLCANIC && (cell->flags & TILE_FLAG_WATER)) {
                    SDL_SetTextureColorMod(tileset_tex, 255, 110, 40); // Molten lava glow
                    SDL_RenderCopy(renderer, tileset_tex, &src, &dst);
                    Tilemap_ApplyBiomeTint(tileset_tex, map->current_biome); // Restore volcanic tint
                } else {
                    SDL_RenderCopy(renderer, tileset_tex, &src, &dst);
                }

                // Render decorative overlay (pebbles, flora, tall grass)
                if (cell->overlay_id < TILE_TOTAL_COUNT) {
                    SDL_Rect osrc = s_tile_src_rects[cell->overlay_id];
                    SDL_RenderCopy(renderer, tileset_tex, &osrc, &dst);
                }
            } else {
                // High-performance fallback colors when headless or without texture
                if (cell->flags & TILE_FLAG_WATER) {
                    if (map->current_biome == BIOME_VOLCANIC) {
                        SDL_SetRenderDrawColor(renderer, 220, 65, 20, 255); // Molten lava
                    } else {
                        SDL_SetRenderDrawColor(renderer, 45, 95, 160, 255);
                    }
                } else if (cell->flags & TILE_FLAG_RAMP) {
                    SDL_SetRenderDrawColor(renderer, 185, 175, 150, 255);
                } else if (cell->flags & TILE_FLAG_SOLID) {
                    SDL_SetRenderDrawColor(renderer, 110, 115, 120, 255);
                } else if (cell->flags & TILE_FLAG_PLATEAU) {
                    SDL_SetRenderDrawColor(renderer, 150, 155, 160, 255);
                } else if (cell->flags & TILE_FLAG_PATH) {
                    SDL_SetRenderDrawColor(renderer, 175, 155, 125, 255);
                } else {
                    if (cell->flags & TILE_FLAG_TALL_GRASS) {
                        SDL_SetRenderDrawColor(renderer, 35, 110, 45, 255);
                    } else if ((tx + ty) % 2 == 0) {
                        SDL_SetRenderDrawColor(renderer, 42, 48, 44, 255);
                    } else {
                        SDL_SetRenderDrawColor(renderer, 36, 42, 38, 255);
                    }
                }
                SDL_RenderFillRect(renderer, &dst);
            }
        }
    }

    // Render Destructible Boulders
    for (int b = 0; b < map->boulder_count; b++) {
        const DestructibleBoulder* bld = &map->boulders[b];
        if (!bld->active) continue;
        if (bld->tile_x < start_tx || bld->tile_x > end_tx || bld->tile_y < start_ty || bld->tile_y > end_ty) continue;

        SDL_Rect b_dst = {
            (int)(bld->tile_x * TILE_WORLD_SIZE - camera_x),
            (int)(bld->tile_y * TILE_WORLD_SIZE - camera_y),
            (int)TILE_WORLD_SIZE,
            (int)TILE_WORLD_SIZE
        };

        if (tileset_tex) {
            SDL_Rect b_src = s_tile_src_rects[TILE_ROCK_BOULDER];
            if (bld->flash_timer > 0.0f) {
                SDL_SetTextureColorMod(tileset_tex, 255, 255, 255);
                SDL_RenderCopy(renderer, tileset_tex, &b_src, &b_dst);
                Tilemap_ApplyBiomeTint(tileset_tex, map->current_biome);
            } else {
                SDL_RenderCopy(renderer, tileset_tex, &b_src, &b_dst);
            }
        } else {
            SDL_SetRenderDrawColor(renderer, 140, 135, 130, 255);
            SDL_RenderFillRect(renderer, &b_dst);
        }

        // Mini HP Bar for damaged boulders
        if (bld->hp < bld->max_hp) {
            int hp_bar_w = 40;
            int hp_bar_h = 4;
            int hp_bar_x = b_dst.x + 12;
            int hp_bar_y = b_dst.y - 6;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 200);
            SDL_Rect hp_bg = { hp_bar_x, hp_bar_y, hp_bar_w, hp_bar_h };
            SDL_RenderFillRect(renderer, &hp_bg);

            float hp_ratio = bld->hp / bld->max_hp;
            if (hp_ratio < 0.0f) hp_ratio = 0.0f;
            SDL_SetRenderDrawColor(renderer, 220, 180, 50, 255);
            SDL_Rect hp_fill = { hp_bar_x, hp_bar_y, (int)(hp_bar_w * hp_ratio), hp_bar_h };
            SDL_RenderFillRect(renderer, &hp_fill);
        }
    }

    if (tileset_tex) {
        SDL_SetTextureColorMod(tileset_tex, 255, 255, 255); // Reset texture tint
    }

    // Render Route Exit Gate / Poké-Portal
    if (map->exit_gate.active) {
        int gx = (int)(map->exit_gate.x - camera_x);
        int gy = (int)(map->exit_gate.y - camera_y);

        if (gx >= -100 && gx <= WINDOW_WIDTH + 100 && gy >= -100 && gy <= WINDOW_HEIGHT + 100) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

            float time_anim = ((TileMap*)map)->exit_gate.anim_timer;
            float pulse = 0.5f + 0.5f * sinf(time_anim * 5.0f);

            // Outer swirling cyan energy ring
            int r_outer = (int)(map->exit_gate.radius + 6.0f * pulse);
            SDL_SetRenderDrawColor(renderer, 40, 200, 255, (Uint8)(160 + 60 * pulse));
            for (int w = -r_outer; w <= r_outer; w++) {
                int h_span = (int)sqrtf((float)(r_outer * r_outer - w * w));
                SDL_RenderDrawLine(renderer, gx + w, gy - h_span / 4, gx + w, gy + h_span / 4);
            }

            // Middle golden aura ring
            int r_mid = (int)(map->exit_gate.radius * 0.75f);
            SDL_SetRenderDrawColor(renderer, 255, 220, 60, 220);
            for (int w = -r_mid; w <= r_mid; w++) {
                int h_span = (int)sqrtf((float)(r_mid * r_mid - w * w));
                SDL_RenderDrawLine(renderer, gx + w, gy - h_span / 3, gx + w, gy + h_span / 3);
            }

            // Glowing white core
            int r_core = 16;
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            for (int w = -r_core; w <= r_core; w++) {
                int h_span = (int)sqrtf((float)(r_core * r_core - w * w));
                SDL_RenderDrawLine(renderer, gx + w, gy - h_span / 2, gx + w, gy + h_span / 2);
            }

            // Render Portal Gate icon if texture loaded
            if (tileset_tex) {
                SDL_Rect g_src = s_tile_src_rects[TILE_PORTAL_GATE];
                SDL_Rect g_dst = { gx - 24, gy - 24, 48, 48 };
                SDL_RenderCopy(renderer, tileset_tex, &g_src, &g_dst);
            }
        }
    }
}

bool Tilemap_IsSolid(const TileMap* map, int tile_x, int tile_y, bool can_fly) {
    if (!map) return false;
    if (tile_x < 0 || tile_x >= MAP_TILES_X || tile_y < 0 || tile_y >= MAP_TILES_Y) {
        return true;
    }
    if (can_fly) {
        // Flying entities only blocked by outer world perimeter
        return (tile_x <= 1 || tile_x >= MAP_TILES_X - 2 || tile_y <= 1 || tile_y >= MAP_TILES_Y - 2);
    }
    return (map->cells[tile_y][tile_x].flags & TILE_FLAG_SOLID) != 0;
}

bool Tilemap_CheckCollisionCircle(const TileMap* map, float* pos_x, float* pos_y, float radius, bool can_fly) {
    if (!map || !pos_x || !pos_y) return false;

    // Flying entities fly over obstacles and are only bounded by outer world perimeter
    if (can_fly) {
        bool collided = false;
        float min_bound = 2.0f * TILE_WORLD_SIZE + radius;
        float max_bound_x = (MAP_TILES_X - 2.0f) * TILE_WORLD_SIZE - radius;
        float max_bound_y = (MAP_TILES_Y - 2.0f) * TILE_WORLD_SIZE - radius;
        if (*pos_x < min_bound)   { *pos_x = min_bound;   collided = true; }
        if (*pos_x > max_bound_x) { *pos_x = max_bound_x; collided = true; }
        if (*pos_y < min_bound)   { *pos_y = min_bound;   collided = true; }
        if (*pos_y > max_bound_y) { *pos_y = max_bound_y; collided = true; }
        return collided;
    }

    bool collided = false;

    // Large entities (e.g. Bosses like Onix with radius 43.2) need to be able to fit
    // through 1-tile wide (64px) ramps. Check if entity is currently in or adjacent to a ramp tile.
    float col_radius = radius;
    if (radius > 26.0f) {
        int ctx = (int)floorf(*pos_x / TILE_WORLD_SIZE);
        int cty = (int)floorf(*pos_y / TILE_WORLD_SIZE);
        bool near_ramp = false;
        for (int dy = -1; dy <= 1 && !near_ramp; dy++) {
            for (int dx = -1; dx <= 1 && !near_ramp; dx++) {
                int nx = ctx + dx;
                int ny = cty + dy;
                if (nx >= 0 && nx < MAP_TILES_X && ny >= 0 && ny < MAP_TILES_Y) {
                    if (map->cells[ny][nx].flags & TILE_FLAG_RAMP) {
                        near_ramp = true;
                    }
                }
            }
        }
        if (near_ramp) {
            col_radius = 26.0f;
        }
    }

    // Resolve collision against solid cliff and water AABBs with 2 relaxation iterations
    for (int iter = 0; iter < 2; iter++) {
        int min_tx = (int)floorf((*pos_x - col_radius) / TILE_WORLD_SIZE);
        int max_tx = (int)floorf((*pos_x + col_radius) / TILE_WORLD_SIZE);
        int min_ty = (int)floorf((*pos_y - col_radius) / TILE_WORLD_SIZE);
        int max_ty = (int)floorf((*pos_y + col_radius) / TILE_WORLD_SIZE);

        if (min_tx < 0) min_tx = 0;
        if (max_tx >= MAP_TILES_X) max_tx = MAP_TILES_X - 1;
        if (min_ty < 0) min_ty = 0;
        if (max_ty >= MAP_TILES_Y) max_ty = MAP_TILES_Y - 1;

        for (int ty = min_ty; ty <= max_ty; ty++) {
            for (int tx = min_tx; tx <= max_tx; tx++) {
                if (!(map->cells[ty][tx].flags & TILE_FLAG_SOLID)) continue;

                float box_min_x = tx * TILE_WORLD_SIZE;
                float box_max_x = (tx + 1) * TILE_WORLD_SIZE;
                float box_min_y = ty * TILE_WORLD_SIZE;
                float box_max_y = (ty + 1) * TILE_WORLD_SIZE;

                // Clamped closest point on AABB
                float cx = fmaxf(box_min_x, fminf(*pos_x, box_max_x));
                float cy = fmaxf(box_min_y, fminf(*pos_y, box_max_y));

                float dx = *pos_x - cx;
                float dy = *pos_y - cy;
                float dist_sq = dx * dx + dy * dy;

                if (dist_sq < col_radius * col_radius) {
                    collided = true;
                    float dist = sqrtf(dist_sq);
                    if (dist > 0.0001f) {
                        float push = col_radius - dist;
                        *pos_x += (dx / dist) * push;
                        *pos_y += (dy / dist) * push;
                    } else {
                        // Center is inside solid box: push along minimum penetration axis
                        float pen_l = *pos_x - box_min_x;
                        float pen_r = box_max_x - *pos_x;
                        float pen_t = *pos_y - box_min_y;
                        float pen_b = box_max_y - *pos_y;
                        float min_pen = fminf(fminf(pen_l, pen_r), fminf(pen_t, pen_b));
                        if (min_pen == pen_l) *pos_x = box_min_x - col_radius;
                        else if (min_pen == pen_r) *pos_x = box_max_x + col_radius;
                        else if (min_pen == pen_t) *pos_y = box_min_y - col_radius;
                        else *pos_y = box_max_y + col_radius;
                    }
                }
            }
        }
    }

    return collided;
}

bool Tilemap_HasLineOfSight(const TileMap* map, float x1, float y1, float x2, float y2) {
    if (!map) return false;

    int tx1 = (int)floorf(x1 / TILE_WORLD_SIZE);
    int ty1 = (int)floorf(y1 / TILE_WORLD_SIZE);
    int tx2 = (int)floorf(x2 / TILE_WORLD_SIZE);
    int ty2 = (int)floorf(y2 / TILE_WORLD_SIZE);

    if (tx1 < 0 || tx1 >= MAP_TILES_X || ty1 < 0 || ty1 >= MAP_TILES_Y) return false;
    if (tx2 < 0 || tx2 >= MAP_TILES_X || ty2 < 0 || ty2 >= MAP_TILES_Y) return false;

    // Both endpoints must not be inside solid obstacles
    if (map->cells[ty1][tx1].flags & TILE_FLAG_SOLID) return false;
    if (map->cells[ty2][tx2].flags & TILE_FLAG_SOLID) return false;

    if (tx1 == tx2 && ty1 == ty2) return true;

    // Fast integer Bresenham line traversal
    int dx = abs(tx2 - tx1);
    int dy = abs(ty2 - ty1);
    int sx = (tx1 < tx2) ? 1 : -1;
    int sy = (ty1 < ty2) ? 1 : -1;
    int err = dx - dy;

    int cx = tx1;
    int cy = ty1;
    uint8_t curr_elev = map->cells[ty1][tx1].elevation;
    uint8_t target_elev = map->cells[ty2][tx2].elevation;

    while (cx != tx2 || cy != ty2) {
        int prev_x = cx;
        int prev_y = cy;
        int e2 = 2 * err;
        int step_x = 0;
        int step_y = 0;

        if (e2 > -dy) {
            err -= dy;
            cx += sx;
            step_x = sx;
        }
        if (e2 < dx) {
            err += dx;
            cy += sy;
            step_y = sy;
        }

        if (cx < 0 || cx >= MAP_TILES_X || cy < 0 || cy >= MAP_TILES_Y) {
            return false;
        }

        // Check corner cutting on diagonal step
        if (step_x != 0 && step_y != 0) {
            if (Tilemap_IsSolid(map, cx - step_x, cy, false) ||
                Tilemap_IsSolid(map, cx, cy - step_y, false)) {
                return false;
            }
        }

        if (map->cells[cy][cx].flags & TILE_FLAG_SOLID) {
            return false;
        }

        // Elevation transition handling:
        // Transition between differing elevations is only permissible via a ramp tile
        uint8_t next_elev = map->cells[cy][cx].elevation;
        if (next_elev != curr_elev) {
            bool has_ramp = ((map->cells[cy][cx].flags & TILE_FLAG_RAMP) != 0) ||
                            ((map->cells[prev_y][prev_x].flags & TILE_FLAG_RAMP) != 0);
            if (!has_ramp) {
                return false;
            }
            curr_elev = next_elev;
        }
    }

    return (curr_elev == target_elev || (map->cells[ty2][tx2].flags & TILE_FLAG_RAMP) != 0);
}

static uint16_t s_bfs_queue[MAP_TILES_X * MAP_TILES_Y];

void Tilemap_UpdateFlowField(TileMap* map, float target_x, float target_y) {
    if (!map) return;

    int ptx = (int)floorf(target_x / TILE_WORLD_SIZE);
    int pty = (int)floorf(target_y / TILE_WORLD_SIZE);

    if (ptx < 0) ptx = 0;
    if (ptx >= MAP_TILES_X) ptx = MAP_TILES_X - 1;
    if (pty < 0) pty = 0;
    if (pty >= MAP_TILES_Y) pty = MAP_TILES_Y - 1;

    int orig_ptx = ptx;
    int orig_pty = pty;

    // If target tile is solid, search outward for nearest passable tile
    if (Tilemap_IsSolid(map, ptx, pty, false)) {
        int best_nx = ptx, best_ny = pty;
        int found = 0;
        for (int r = 1; r <= 4 && !found; r++) {
            for (int dy = -r; dy <= r && !found; dy++) {
                for (int dx = -r; dx <= r && !found; dx++) {
                    int nx = ptx + dx;
                    int ny = pty + dy;
                    if (nx >= 0 && nx < MAP_TILES_X && ny >= 0 && ny < MAP_TILES_Y) {
                        if (!Tilemap_IsSolid(map, nx, ny, false)) {
                            best_nx = nx;
                            best_ny = ny;
                            found = 1;
                        }
                    }
                }
            }
        }
        ptx = best_nx;
        pty = best_ny;
    }

    // Skip recomputation if target tile has not changed and field is valid
    if (map->path_field.valid &&
        map->path_field.target_tile_x == orig_ptx &&
        map->path_field.target_tile_y == orig_pty) {
        return;
    }

    memset(map->path_field.dist, 0xFF, sizeof(map->path_field.dist));

    int head = 0, tail = 0;
    map->path_field.dist[pty][ptx] = 0;
    s_bfs_queue[tail++] = (uint16_t)((pty << 7) | ptx);

    static const int dx[4] = { 0, 0, -1, 1 };
    static const int dy[4] = { -1, 1, 0, 0 };

    while (head < tail) {
        uint16_t curr = s_bfs_queue[head++];
        int cx = curr & 0x7F;
        int cy = (curr >> 7) & 0x7F;
        uint16_t nd = map->path_field.dist[cy][cx] + 1;
        uint8_t c_elev = map->cells[cy][cx].elevation;

        for (int i = 0; i < 4; i++) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (nx < 0 || nx >= MAP_TILES_X || ny < 0 || ny >= MAP_TILES_Y) continue;
            if (Tilemap_IsSolid(map, nx, ny, false)) continue;

            // Enforce ramp requirement for elevation transitions
            uint8_t n_elev = map->cells[ny][nx].elevation;
            if (c_elev != n_elev) {
                bool has_ramp = ((map->cells[cy][cx].flags & TILE_FLAG_RAMP) != 0) ||
                                ((map->cells[ny][nx].flags & TILE_FLAG_RAMP) != 0);
                if (!has_ramp) continue;
            }

            if (map->path_field.dist[ny][nx] == 0xFFFF) {
                map->path_field.dist[ny][nx] = nd;
                s_bfs_queue[tail++] = (uint16_t)((ny << 7) | nx);
            }
        }
    }

    map->path_field.target_tile_x = orig_ptx;
    map->path_field.target_tile_y = orig_pty;
    map->path_field.valid = true;
}

bool Tilemap_GetPathDirection(TileMap* map, float from_x, float from_y, float target_x, float target_y, float* out_dir_x, float* out_dir_y) {
    if (!map || !out_dir_x || !out_dir_y) return false;

    // Default direction directly towards target
    float dx = target_x - from_x;
    float dy = target_y - from_y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > 0.001f) {
        *out_dir_x = dx / dist;
        *out_dir_y = dy / dist;
    } else {
        *out_dir_x = 0.0f;
        *out_dir_y = 0.0f;
        return true;
    }

    int tx = (int)floorf(from_x / TILE_WORLD_SIZE);
    int ty = (int)floorf(from_y / TILE_WORLD_SIZE);
    int ptx = (int)floorf(target_x / TILE_WORLD_SIZE);
    int pty = (int)floorf(target_y / TILE_WORLD_SIZE);

    if (tx < 0 || tx >= MAP_TILES_X || ty < 0 || ty >= MAP_TILES_Y) {
        return false;
    }

    // If within same tile as target, direct movement is optimal
    if (tx == ptx && ty == pty) {
        return true;
    }

    // Direct line-of-sight check
    if (Tilemap_HasLineOfSight(map, from_x, from_y, target_x, target_y)) {
        return true;
    }

    // Ensure flow field is valid
    if (!map->path_field.valid || map->path_field.target_tile_x != ptx || map->path_field.target_tile_y != pty) {
        Tilemap_UpdateFlowField(map, target_x, target_y);
    }

    // If still unreachable or invalid, check nearby non-solid tiles
    if (!map->path_field.valid || map->path_field.dist[ty][tx] == 0xFFFF) {
        uint16_t min_d = 0xFFFF;
        int escape_nx = tx, escape_ny = ty;
        for (int ey = ty - 1; ey <= ty + 1; ey++) {
            for (int ex = tx - 1; ex <= tx + 1; ex++) {
                if (ex >= 0 && ex < MAP_TILES_X && ey >= 0 && ey < MAP_TILES_Y) {
                    if (!Tilemap_IsSolid(map, ex, ey, false) && map->path_field.dist[ey][ex] < min_d) {
                        min_d = map->path_field.dist[ey][ex];
                        escape_nx = ex;
                        escape_ny = ey;
                    }
                }
            }
        }
        if (min_d < 0xFFFF) {
            float wx = (escape_nx + 0.5f) * TILE_WORLD_SIZE;
            float wy = (escape_ny + 0.5f) * TILE_WORLD_SIZE;
            float edx = wx - from_x;
            float edy = wy - from_y;
            float elen = sqrtf(edx * edx + edy * edy);
            if (elen > 0.001f) {
                *out_dir_x = edx / elen;
                *out_dir_y = edy / elen;
            }
        }
        return false;
    }

    uint16_t cur_dist = map->path_field.dist[ty][tx];

    // Find the neighboring tile with maximum progress towards target
    int best_nx = tx;
    int best_ny = ty;
    float max_progress = 0.0f;

    static const int n_dx[8] = { 0, 0, -1, 1, -1, 1, -1, 1 };
    static const int n_dy[8] = { -1, 1, 0, 0, -1, -1, 1, 1 };

    for (int i = 0; i < 8; i++) {
        int nx = tx + n_dx[i];
        int ny = ty + n_dy[i];

        if (nx < 0 || nx >= MAP_TILES_X || ny < 0 || ny >= MAP_TILES_Y) continue;
        if (Tilemap_IsSolid(map, nx, ny, false)) continue;

        // Diagonal corner-cutting check: neither orthogonal tile can be solid
        if (n_dx[i] != 0 && n_dy[i] != 0) {
            if (Tilemap_IsSolid(map, tx + n_dx[i], ty, false) ||
                Tilemap_IsSolid(map, tx, ty + n_dy[i], false)) {
                continue;
            }
        }

        // Elevation transition check: stepping between elevations requires a ramp
        uint8_t c_elev = map->cells[ty][tx].elevation;
        uint8_t n_elev = map->cells[ny][nx].elevation;
        if (c_elev != n_elev) {
            bool has_ramp = ((map->cells[ty][tx].flags & TILE_FLAG_RAMP) != 0) ||
                            ((map->cells[ny][nx].flags & TILE_FLAG_RAMP) != 0);
            if (!has_ramp) continue;
        }

        uint16_t nd = map->path_field.dist[ny][nx];
        if (nd >= cur_dist) continue;

        float edge_len = (n_dx[i] != 0 && n_dy[i] != 0) ? 1.4142f : 1.0f;
        float progress = (float)(cur_dist - nd) / edge_len;

        if (progress > max_progress) {
            max_progress = progress;
            best_nx = nx;
            best_ny = ny;
        }
    }

    if (best_nx == tx && best_ny == ty) {
        return false;
    }

    // Primary waypoint: aim directly at target if adjacent to target tile, else center of best neighbor tile
    float wx = (best_nx == ptx && best_ny == pty) ? target_x : (best_nx + 0.5f) * TILE_WORLD_SIZE;
    float wy = (best_nx == ptx && best_ny == pty) ? target_y : (best_ny + 0.5f) * TILE_WORLD_SIZE;

    // Lookahead: smooth turns by aiming at 2-step waypoint if unobstructed
    int next_nx = best_nx;
    int next_ny = best_ny;
    float next_max_progress = 0.0f;
    uint16_t best_dist = map->path_field.dist[best_ny][best_nx];

    for (int i = 0; i < 8; i++) {
        int nx = best_nx + n_dx[i];
        int ny = best_ny + n_dy[i];

        if (nx < 0 || nx >= MAP_TILES_X || ny < 0 || ny >= MAP_TILES_Y) continue;
        if (Tilemap_IsSolid(map, nx, ny, false)) continue;

        if (n_dx[i] != 0 && n_dy[i] != 0) {
            if (Tilemap_IsSolid(map, best_nx + n_dx[i], best_ny, false) ||
                Tilemap_IsSolid(map, best_nx, best_ny + n_dy[i], false)) {
                continue;
            }
        }

        uint8_t bn_elev = map->cells[best_ny][best_nx].elevation;
        uint8_t nn_elev = map->cells[ny][nx].elevation;
        if (bn_elev != nn_elev) {
            bool has_ramp = ((map->cells[best_ny][best_nx].flags & TILE_FLAG_RAMP) != 0) ||
                            ((map->cells[ny][nx].flags & TILE_FLAG_RAMP) != 0);
            if (!has_ramp) continue;
        }

        uint16_t nd = map->path_field.dist[ny][nx];
        if (nd >= best_dist) continue;

        float edge_len = (n_dx[i] != 0 && n_dy[i] != 0) ? 1.4142f : 1.0f;
        float progress = (float)(best_dist - nd) / edge_len;

        if (progress > next_max_progress) {
            next_max_progress = progress;
            next_nx = nx;
            next_ny = ny;
        }
    }

    if (next_nx != best_nx || next_ny != best_ny) {
        float n_wx = (next_nx == ptx && next_ny == pty) ? target_x : (next_nx + 0.5f) * TILE_WORLD_SIZE;
        float n_wy = (next_nx == ptx && next_ny == pty) ? target_y : (next_ny + 0.5f) * TILE_WORLD_SIZE;
        if (Tilemap_HasLineOfSight(map, from_x, from_y, n_wx, n_wy)) {
            wx = n_wx;
            wy = n_wy;
        }
    }

    float to_wx = wx - from_x;
    float to_wy = wy - from_y;
    float wlen = sqrtf(to_wx * to_wx + to_wy * to_wy);
    if (wlen > 0.001f) {
        *out_dir_x = to_wx / wlen;
        *out_dir_y = to_wy / wlen;
    }

    return false;
}
