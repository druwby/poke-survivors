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

void Tilemap_GenerateRouteMap(TileMap* map, int section_idx, int route_idx) {
    if (!map) return;
    map->current_section = section_idx;
    map->current_route = route_idx;

    Route_Srand((uint32_t)(section_idx * 100 + route_idx + 1));

    Tilemap_Init(map);
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

    // Determine biome theme
    // 0: Plains/Routes, 1: Forest/Safari, 2: Mountain/Cave, 3: Coastal/Water, 4: Gym Arena
    int biome = 0;
    if (section_idx == 0 && route_idx == 2) biome = 1; // Viridian Forest
    else if (section_idx == 0 && route_idx == 3) biome = 4; // Pewter Gym
    else if (section_idx == 1 && route_idx == 1) biome = 2; // Mt. Moon
    else if (section_idx == 1 && (route_idx == 2 || route_idx == 3)) biome = 3; // Route 24 / Cerulean Gym
    else if (section_idx == 2 && route_idx == 2) biome = 2; // Diglett's Cave
    else if (section_idx == 2 && route_idx == 3) biome = 4; // Vermilion Gym
    else if (section_idx == 3 && route_idx == 1) biome = 2; // Rock Tunnel
    else if (section_idx == 3 && route_idx == 3) biome = 4; // Celadon Gym
    else if (section_idx == 4 && route_idx == 2) biome = 1; // Safari Zone
    else if (section_idx == 4 && (route_idx == 1 || route_idx == 3)) biome = 4; // Fuchsia Gym / Route 12
    else if (section_idx == 5 && (route_idx == 2 || route_idx == 3)) biome = 4; // Dojo / Saffron Gym
    else if (section_idx == 6 && (route_idx == 0 || route_idx == 2)) biome = 3; // Seafoam / Route 20
    else if (section_idx == 6 && route_idx == 3) biome = 4; // Cinnabar Gym
    else if (section_idx == 7 && route_idx == 1) biome = 2; // Victory Road
    else if (section_idx == 7 && (route_idx == 2 || route_idx == 3)) biome = 4; // Viridian Gym / Indigo Plateau

    switch (biome) {
        case 1: { // FOREST / SAFARI (Woodland glades, ponds, lookout plateaus)
            Tilemap_AddPlateau(map, 20, 20, 14, 10, 1, 6);  // South ramp at (27, 29)
            Tilemap_AddPlateau(map, 94, 22, 14, 10, 1, 6);  // South ramp at (101, 31)
            Tilemap_AddPlateau(map, 22, 98, 14, 10, 2, 6);  // North ramp at (29, 98)
            Tilemap_AddPlateau(map, 92, 96, 14, 10, 2, 6);  // North ramp at (99, 96)

            Tilemap_AddPond(map, 44, 24, 10, 8);
            Tilemap_AddPond(map, 74, 94, 10, 8);
            Tilemap_AddPond(map, 24, 60, 8, 10);
            Tilemap_AddPond(map, 96, 60, 8, 10);

            // Connecting paths from central roads directly to ramps
            Tilemap_AddPathV(map, 27, 30, 63, 1);
            Tilemap_AddPathV(map, 101, 32, 63, 1);
            Tilemap_AddPathV(map, 29, 63, 97, 1);
            Tilemap_AddPathV(map, 99, 63, 95, 1);

            // Natural pebble and flora clusters
            for (int gx = 16; gx <= 112; gx += 16) {
                for (int gy = 16; gy <= 112; gy += 16) {
                    if (abs(gx - 64) < 14 && abs(gy - 64) < 14) continue;
                    Tilemap_AddPebbleCluster(map, gx + (int)(Route_Rand() % 5), gy + (int)(Route_Rand() % 5), 5);
                }
            }
            break;
        }

        case 2: { // MOUNTAIN / CAVE (Mt. Moon, Rock Tunnel, Diglett's Cave, Victory Road)
            // Multiple imposing rock plateaus with dual ramps
            Tilemap_AddPlateau(map, 16, 16, 18, 14, 1, 8);  // South ramp at (25, 29)
            Tilemap_AddPlateau(map, 94, 16, 18, 14, 1, 8);  // South ramp at (103, 29)
            Tilemap_AddPlateau(map, 16, 98, 18, 14, 2, 8);  // North ramp at (25, 98)
            Tilemap_AddPlateau(map, 94, 98, 18, 14, 2, 8);  // North ramp at (103, 98)

            // Flanking canyon plateaus
            Tilemap_AddPlateau(map, 38, 36, 12, 10, 1, 5);
            Tilemap_AddPlateau(map, 78, 36, 12, 10, 1, 5);
            Tilemap_AddPlateau(map, 38, 82, 12, 10, 2, 5);
            Tilemap_AddPlateau(map, 78, 82, 12, 10, 2, 5);

            // Small cave springs
            Tilemap_AddPond(map, 60, 20, 8, 8);
            Tilemap_AddPond(map, 60, 100, 8, 8);

            // Mountain paths connecting ramps
            Tilemap_AddPathV(map, 25, 30, 63, 1);
            Tilemap_AddPathV(map, 103, 30, 63, 1);
            Tilemap_AddPathV(map, 25, 63, 97, 1);
            Tilemap_AddPathV(map, 103, 63, 97, 1);

            Tilemap_AddPebbleCluster(map, 32, 50, 4);
            Tilemap_AddPebbleCluster(map, 96, 50, 4);
            Tilemap_AddPebbleCluster(map, 32, 78, 4);
            Tilemap_AddPebbleCluster(map, 96, 78, 4);
            break;
        }

        case 3: { // COASTAL / WATER (Route 24, Cerulean Gym, Seafoam, Route 20)
            // Open water bodies along coastal flanks
            Tilemap_AddPond(map, 10, 14, 18, 18);
            Tilemap_AddPond(map, 100, 14, 18, 18);
            Tilemap_AddPond(map, 10, 96, 18, 18);
            Tilemap_AddPond(map, 100, 96, 18, 18);

            // Flanking waterways along central road
            Tilemap_AddPond(map, 44, 46, 12, 10);
            Tilemap_AddPond(map, 72, 46, 12, 10);
            Tilemap_AddPond(map, 44, 72, 12, 10);
            Tilemap_AddPond(map, 72, 72, 12, 10);

            // Island plateaus positioned on dry land next to water, with ramps facing paths
            Tilemap_AddPlateau(map, 32, 18, 10, 8, 1, 4);  // South ramp at (37, 25)
            Tilemap_AddPlateau(map, 86, 18, 10, 8, 1, 4);  // South ramp at (91, 25)
            Tilemap_AddPlateau(map, 32, 102, 10, 8, 2, 4); // North ramp at (37, 102)
            Tilemap_AddPlateau(map, 86, 102, 10, 8, 2, 4); // North ramp at (91, 102)

            // Direct walkable path connections from central road to ramps
            Tilemap_AddPathV(map, 37, 26, 63, 1);
            Tilemap_AddPathV(map, 91, 26, 63, 1);
            Tilemap_AddPathV(map, 37, 63, 101, 1);
            Tilemap_AddPathV(map, 91, 63, 101, 1);

            Tilemap_AddPebbleCluster(map, 40, 26, 4);
            Tilemap_AddPebbleCluster(map, 88, 26, 4);
            Tilemap_AddPebbleCluster(map, 40, 102, 4);
            Tilemap_AddPebbleCluster(map, 88, 102, 4);
            break;
        }

        case 4: { // GYM ARENA & INDIGO PLATEAU
            // Grand central tournament arena with championship plateau
            Tilemap_AddPlateau(map, 28, 28, 20, 16, 1, 9);  // South ramp at (38, 43)
            Tilemap_AddPlateau(map, 80, 28, 20, 16, 1, 9);  // South ramp at (90, 43)
            Tilemap_AddPlateau(map, 28, 84, 20, 16, 2, 9);  // North ramp at (38, 84)
            Tilemap_AddPlateau(map, 80, 84, 20, 16, 2, 9);  // North ramp at (90, 84)

            // Flanking reflecting pools
            Tilemap_AddPond(map, 52, 28, 10, 10);
            Tilemap_AddPond(map, 66, 28, 10, 10);
            Tilemap_AddPond(map, 52, 90, 10, 10);
            Tilemap_AddPond(map, 66, 90, 10, 10);

            // Ceremonial perimeter pathways
            Tilemap_AddPathH(map, 20, 108, 48, 2);
            Tilemap_AddPathH(map, 20, 108, 80, 2);
            Tilemap_AddPathV(map, 48, 20, 108, 2);
            Tilemap_AddPathV(map, 80, 20, 108, 2);

            // Direct approach paths to all 4 arena plateau ramps
            Tilemap_AddPathV(map, 38, 44, 48, 1);
            Tilemap_AddPathV(map, 90, 44, 48, 1);
            Tilemap_AddPathV(map, 38, 80, 83, 1);
            Tilemap_AddPathV(map, 90, 80, 83, 1);

            Tilemap_AddPebbleCluster(map, 24, 24, 4);
            Tilemap_AddPebbleCluster(map, 104, 24, 4);
            Tilemap_AddPebbleCluster(map, 24, 104, 4);
            Tilemap_AddPebbleCluster(map, 104, 104, 4);
            break;
        }

        default: { // PLAINS & REGULAR ROUTES (Route 1, Route 2, etc.)
            // Balanced rolling plateaus with dual traversable ramps
            Tilemap_AddPlateau(map, 18, 18, 16, 12, 1, 7);  // South ramp at (26, 29)
            Tilemap_AddPlateau(map, 94, 20, 16, 12, 1, 7);  // South ramp at (102, 31)
            Tilemap_AddPlateau(map, 20, 98, 16, 12, 2, 7);  // North ramp at (28, 98)
            Tilemap_AddPlateau(map, 92, 96, 16, 12, 2, 7);  // North ramp at (100, 96)

            // Scenic roadside ponds
            Tilemap_AddPond(map, 44, 26, 10, 8);
            Tilemap_AddPond(map, 74, 94, 10, 8);
            Tilemap_AddPond(map, 20, 60, 8, 10);
            Tilemap_AddPond(map, 100, 60, 8, 10);

            // Connecting paths leading directly to ramps
            Tilemap_AddPathV(map, 26, 30, 63, 1);
            Tilemap_AddPathV(map, 102, 32, 63, 1);
            Tilemap_AddPathV(map, 28, 63, 97, 1);
            Tilemap_AddPathV(map, 100, 63, 95, 1);

            // Natural roadside pebble clusters
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
                SDL_RenderCopy(renderer, tileset_tex, &src, &dst);

                // Render decorative overlay (pebbles, flora)
                if (cell->overlay_id < TILE_TOTAL_COUNT) {
                    SDL_Rect osrc = s_tile_src_rects[cell->overlay_id];
                    SDL_RenderCopy(renderer, tileset_tex, &osrc, &dst);
                }
            } else {
                // High-performance fallback colors when headless or without texture
                if (cell->flags & TILE_FLAG_WATER) {
                    SDL_SetRenderDrawColor(renderer, 45, 95, 160, 255);
                } else if (cell->flags & TILE_FLAG_RAMP) {
                    SDL_SetRenderDrawColor(renderer, 185, 175, 150, 255);
                } else if (cell->flags & TILE_FLAG_SOLID) {
                    SDL_SetRenderDrawColor(renderer, 110, 115, 120, 255);
                } else if (cell->flags & TILE_FLAG_PLATEAU) {
                    SDL_SetRenderDrawColor(renderer, 150, 155, 160, 255);
                } else if (cell->flags & TILE_FLAG_PATH) {
                    SDL_SetRenderDrawColor(renderer, 175, 155, 125, 255);
                } else {
                    if ((tx + ty) % 2 == 0) SDL_SetRenderDrawColor(renderer, 42, 48, 44, 255);
                    else SDL_SetRenderDrawColor(renderer, 36, 42, 38, 255);
                }
                SDL_RenderFillRect(renderer, &dst);
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
