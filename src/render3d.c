#include "render3d.h"
#include "camera3d.h"
#include "sprite.h"
#include "font.h"
#include "kanto_routes.h"
#include "balance_sim.h"
#include "spatial_grid.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PLATEAU_HEIGHT 38.0f
#define WATER_DEPTH   -6.0f
#define MAX_DEPTH_ITEMS 4096

#define TILESET_TEX_W 432.0f
#define TILESET_TEX_H 192.0f

typedef struct {
    Vec3 view_pos;     // View space (x, y, z)
    float u;           // Normalized texture u [0..1]
    float v;           // Normalized texture v [0..1]
    SDL_Color color;   // Vertex tint / lighting
} ViewVertex;

// Clipping against near plane (V_z >= near_z)
static int ClipPolygonNearZ(const ViewVertex in_verts[4], int in_count, ViewVertex out_verts[8], float near_z) {
    int out_count = 0;
    for (int i = 0; i < in_count; i++) {
        const ViewVertex* a = &in_verts[i];
        const ViewVertex* b = &in_verts[(i + 1) % in_count];
        
        bool a_in = (a->view_pos.z >= near_z);
        bool b_in = (b->view_pos.z >= near_z);
        
        if (a_in && b_in) {
            out_verts[out_count++] = *b;
        } else if (!a_in && b_in) {
            float denom = b->view_pos.z - a->view_pos.z;
            float t = (fabsf(denom) > 1e-6f) ? ((near_z - a->view_pos.z) / denom) : 0.0f;
            ViewVertex inter;
            inter.view_pos.x = a->view_pos.x + t * (b->view_pos.x - a->view_pos.x);
            inter.view_pos.y = a->view_pos.y + t * (b->view_pos.y - a->view_pos.y);
            inter.view_pos.z = near_z;
            inter.u = a->u + t * (b->u - a->u);
            inter.v = a->v + t * (b->v - a->v);
            inter.color.r = (Uint8)(a->color.r + t * (b->color.r - a->color.r));
            inter.color.g = (Uint8)(a->color.g + t * (b->color.g - a->color.g));
            inter.color.b = (Uint8)(a->color.b + t * (b->color.b - a->color.b));
            inter.color.a = (Uint8)(a->color.a + t * (b->color.a - a->color.a));
            out_verts[out_count++] = inter;
            out_verts[out_count++] = *b;
        } else if (a_in && !b_in) {
            float denom = b->view_pos.z - a->view_pos.z;
            float t = (fabsf(denom) > 1e-6f) ? ((near_z - a->view_pos.z) / denom) : 0.0f;
            ViewVertex inter;
            inter.view_pos.x = a->view_pos.x + t * (b->view_pos.x - a->view_pos.x);
            inter.view_pos.y = a->view_pos.y + t * (b->view_pos.y - a->view_pos.y);
            inter.view_pos.z = near_z;
            inter.u = a->u + t * (b->u - a->u);
            inter.v = a->v + t * (b->v - a->v);
            inter.color.r = (Uint8)(a->color.r + t * (b->color.r - a->color.r));
            inter.color.g = (Uint8)(a->color.g + t * (b->color.g - a->color.g));
            inter.color.b = (Uint8)(a->color.b + t * (b->color.b - a->color.b));
            inter.color.a = (Uint8)(a->color.a + t * (b->color.a - a->color.a));
            out_verts[out_count++] = inter;
        }
    }
    return out_count;
}

// Low-level 3D quad rasterizer with explicit per-vertex UV mapping
static void DrawTexturedQuad3DUV(SDL_Renderer* renderer, const Camera3D* cam, SDL_Texture* tex,
                                 Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                                 float u0, float v0, float u1, float v1,
                                 float u2, float v2, float u3, float v3,
                                 SDL_Color color) {
    ViewVertex in_v[4];
    Camera3D_WorldToView(cam, p0, &in_v[0].view_pos);
    in_v[0].u = u0; in_v[0].v = v0; in_v[0].color = color;
    
    Camera3D_WorldToView(cam, p1, &in_v[1].view_pos);
    in_v[1].u = u1; in_v[1].v = v1; in_v[1].color = color;
    
    Camera3D_WorldToView(cam, p2, &in_v[2].view_pos);
    in_v[2].u = u2; in_v[2].v = v2; in_v[2].color = color;
    
    Camera3D_WorldToView(cam, p3, &in_v[3].view_pos);
    in_v[3].u = u3; in_v[3].v = v3; in_v[3].color = color;

    // Check if completely behind near clipping plane
    if (in_v[0].view_pos.z < cam->near_z &&
        in_v[1].view_pos.z < cam->near_z &&
        in_v[2].view_pos.z < cam->near_z &&
        in_v[3].view_pos.z < cam->near_z) {
        return;
    }

    ViewVertex clipped[8];
    int count = ClipPolygonNearZ(in_v, 4, clipped, cam->near_z);
    if (count < 3) return;

    SDL_Vertex sdl_v[8];
    for (int i = 0; i < count; i++) {
        Camera3D_ViewToScreen(cam, clipped[i].view_pos, &sdl_v[i].position.x, &sdl_v[i].position.y);
        sdl_v[i].color = clipped[i].color;
        sdl_v[i].tex_coord.x = clipped[i].u;
        sdl_v[i].tex_coord.y = clipped[i].v;
    }

    // Triangle fan from index 0
    int indices[18];
    int num_indices = (count - 2) * 3;
    for (int i = 0; i < count - 2; i++) {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    SDL_RenderGeometry(renderer, tex, sdl_v, count, indices, num_indices);
}

// Convenience wrapper for vertical billboard / cliff faces (p0=BL, p1=BR, p2=TR, p3=TL)
static void DrawTexturedQuad3D(SDL_Renderer* renderer, const Camera3D* cam, SDL_Texture* tex,
                               Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                               float u0, float v0, float u1, float v1,
                               SDL_Color color) {
    DrawTexturedQuad3DUV(renderer, cam, tex, p0, p1, p2, p3,
                         u0, v1,  u1, v1,  u1, v0,  u0, v0, color);
}

// Near-plane clipped 3D line drawing helper
static void DrawClippedLine3D(SDL_Renderer* renderer, const Camera3D* cam, Vec3 p1, Vec3 p2, SDL_Color color, int thickness) {
    Vec3 v1, v2;
    Camera3D_WorldToView(cam, p1, &v1);
    Camera3D_WorldToView(cam, p2, &v2);

    bool in1 = (v1.z >= cam->near_z);
    bool in2 = (v2.z >= cam->near_z);

    if (!in1 && !in2) return;

    if (!in1 && in2) {
        float denom = v2.z - v1.z;
        float t = (fabsf(denom) > 1e-6f) ? ((cam->near_z - v1.z) / denom) : 0.0f;
        v1.x = v1.x + t * (v2.x - v1.x);
        v1.y = v1.y + t * (v2.y - v1.y);
        v1.z = cam->near_z;
    } else if (in1 && !in2) {
        float denom = v2.z - v1.z;
        float t = (fabsf(denom) > 1e-6f) ? ((cam->near_z - v1.z) / denom) : 0.0f;
        v2.x = v1.x + t * (v2.x - v1.x);
        v2.y = v1.y + t * (v2.y - v1.y);
        v2.z = cam->near_z;
    }

    float sx1, sy1, sx2, sy2;
    if (Camera3D_ViewToScreen(cam, v1, &sx1, &sy1) && Camera3D_ViewToScreen(cam, v2, &sx2, &sy2)) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        for (int w = -thickness; w <= thickness; w++) {
            SDL_RenderDrawLine(renderer, (int)sx1 + w, (int)sy1, (int)sx2 + w, (int)sy2);
            SDL_RenderDrawLine(renderer, (int)sx1, (int)sy1 + w, (int)sx2, (int)sy2 + w);
        }
    }
}

float Render3D_GetTerrainHeight(const TileMap* map, float world_x, float world_z) {
    if (!map) return 0.0f;
    int tx = (int)(world_x / TILE_WORLD_SIZE);
    int ty = (int)(world_z / TILE_WORLD_SIZE);
    if (tx < 0 || tx >= MAP_TILES_X || ty < 0 || ty >= MAP_TILES_Y) return 0.0f;

    const MapCell* cell = &map->cells[ty][tx];
    
    if (cell->flags & TILE_FLAG_RAMP) {
        float fx = (world_x - (float)tx * TILE_WORLD_SIZE) / TILE_WORLD_SIZE;
        float fz = (world_z - (float)ty * TILE_WORLD_SIZE) / TILE_WORLD_SIZE;
        if (cell->tile_id == TILE_ROCK_RAMP_S) {
            return (1.0f - fz) * PLATEAU_HEIGHT;
        } else if (cell->tile_id == TILE_ROCK_RAMP_N) {
            return fz * PLATEAU_HEIGHT;
        } else if (cell->tile_id == TILE_ROCK_RAMP_W) {
            return fx * PLATEAU_HEIGHT;
        } else if (cell->tile_id == TILE_ROCK_RAMP_E) {
            return (1.0f - fx) * PLATEAU_HEIGHT;
        }
        return PLATEAU_HEIGHT * 0.5f;
    }
    
    if (cell->elevation == 1 || (cell->flags & TILE_FLAG_PLATEAU)) {
        return PLATEAU_HEIGHT;
    }
    if (cell->flags & TILE_FLAG_WATER) {
        return WATER_DEPTH;
    }
    return 0.0f;
}

// -------------------------------------------------------------
// DEPTH-SORTED ITEMS PIPELINE
// -------------------------------------------------------------

typedef enum {
    ITEM_BILLBOARD_SPRITE,
    ITEM_GROUND_SHADOW,
    ITEM_GROUND_HAZARD,
    ITEM_PROJECTILE,
    ITEM_GEM,
    ITEM_BOULDER,
    ITEM_UNDERGROUND_DIRT,
    ITEM_PORTAL_GATE
} DepthItemType;

typedef struct {
    float depth;
    DepthItemType type;
    int index;
    Vec3 pos;
    float scale;
    float radius;
    SDL_Color color;
    const SpriteSheet* sheet;
    const AnimState* anim;
    int rel_dir;
    bool flash;
} DepthItem;

static DepthItem s_depth_items[MAX_DEPTH_ITEMS];
static int s_depth_item_count = 0;

static int DepthCompare(const void* a, const void* b) {
    const DepthItem* da = (const DepthItem*)a;
    const DepthItem* db = (const DepthItem*)b;
    // Sort back-to-front: larger depth drawn first (Painter's algorithm)
    if (db->depth > da->depth) return 1;
    if (db->depth < da->depth) return -1;
    return 0;
}

void Render3D_Init(GameState* game) {
    if (!game) return;
    Camera3D_Init(&game->camera3d, VIEW_3D_THIRD_PERSON);
    float p_height = Render3D_GetTerrainHeight(&game->tilemap, game->player.pos.x, game->player.pos.y);
    Camera3D_Snap(&game->camera3d, (Vec3){ game->player.pos.x, p_height, game->player.pos.y });
}

static void RenderTerrain3D(GameState* game) {
    TileMap* map = &game->tilemap;
    Camera3D* cam = &game->camera3d;
    SDL_Renderer* renderer = game->renderer;
    SDL_Texture* tex = game->tileset_texture;
    
    int center_tx = (int)(cam->smooth_target.x / TILE_WORLD_SIZE);
    int center_ty = (int)(cam->smooth_target.z / TILE_WORLD_SIZE);
    
    int radius = (cam->mode == VIEW_3D_TOPDOWN) ? 28 : 26;
    int min_tx = center_tx - radius; if (min_tx < 0) min_tx = 0;
    int max_tx = center_tx + radius; if (max_tx >= MAP_TILES_X) max_tx = MAP_TILES_X - 1;
    int min_ty = center_ty - radius; if (min_ty < 0) min_ty = 0;
    int max_ty = center_ty + radius; if (max_ty >= MAP_TILES_Y) max_ty = MAP_TILES_Y - 1;

    // Dynamically choose iteration order based on camera view direction
    // to strictly preserve back-to-front painter's order across all 360-degree orientations
    int step_y = (cam->forward.z < 0.0f) ? 1 : -1;
    int start_y = (step_y > 0) ? min_ty : max_ty;
    int end_y = (step_y > 0) ? (max_ty + 1) : (min_ty - 1);

    int step_x = (cam->forward.x < 0.0f) ? 1 : -1;
    int start_x = (step_x > 0) ? min_tx : max_tx;
    int end_x = (step_x > 0) ? (max_tx + 1) : (min_tx - 1);

    for (int ty = start_y; ty != end_y; ty += step_y) {
        for (int tx = start_x; tx != end_x; tx += step_x) {
            const MapCell* cell = &map->cells[ty][tx];
            
            float x0 = (float)tx * TILE_WORLD_SIZE;
            float x1 = (float)(tx + 1) * TILE_WORLD_SIZE;
            float z0 = (float)ty * TILE_WORLD_SIZE;
            float z1 = (float)(ty + 1) * TILE_WORLD_SIZE;
            
            // Corner heights for this cell
            float y00 = 0.0f, y10 = 0.0f, y11 = 0.0f, y01 = 0.0f;
            
            if (cell->flags & TILE_FLAG_RAMP) {
                if (cell->tile_id == TILE_ROCK_RAMP_S) {
                    y00 = PLATEAU_HEIGHT; y10 = PLATEAU_HEIGHT; y11 = 0.0f; y01 = 0.0f;
                } else if (cell->tile_id == TILE_ROCK_RAMP_N) {
                    y00 = 0.0f; y10 = 0.0f; y11 = PLATEAU_HEIGHT; y01 = PLATEAU_HEIGHT;
                } else if (cell->tile_id == TILE_ROCK_RAMP_W) {
                    y00 = 0.0f; y10 = PLATEAU_HEIGHT; y11 = PLATEAU_HEIGHT; y01 = 0.0f;
                } else if (cell->tile_id == TILE_ROCK_RAMP_E) {
                    y00 = PLATEAU_HEIGHT; y10 = 0.0f; y11 = 0.0f; y01 = PLATEAU_HEIGHT;
                }
            } else if (cell->elevation == 1 || (cell->flags & TILE_FLAG_PLATEAU)) {
                y00 = PLATEAU_HEIGHT; y10 = PLATEAU_HEIGHT; y11 = PLATEAU_HEIGHT; y01 = PLATEAU_HEIGHT;
            } else if (cell->flags & TILE_FLAG_WATER) {
                y00 = WATER_DEPTH; y10 = WATER_DEPTH; y11 = WATER_DEPTH; y01 = WATER_DEPTH;
            }

            // Frustum check on cell center
            Vec3 center = { (x0 + x1) * 0.5f, (y00 + y11) * 0.5f, (z0 + z1) * 0.5f };
            Vec3 center_view;
            Camera3D_WorldToView(cam, center, &center_view);
            if (center_view.z < -TILE_WORLD_SIZE || center_view.z > cam->far_z + TILE_WORLD_SIZE) continue;

            // View-space horizontal and vertical frustum culling
            if (center_view.z > cam->near_z) {
                float half_w = (WINDOW_WIDTH * 0.5f) * (center_view.z / cam->focal_length) + TILE_WORLD_SIZE * 1.5f;
                float half_h = (WINDOW_HEIGHT * 0.5f) * (center_view.z / cam->focal_length) + TILE_WORLD_SIZE * 1.5f;
                if (fabsf(center_view.x) > half_w || fabsf(center_view.y) > half_h) continue;
            }

            // Texture coords
            const SDL_Rect* src_rect = Tilemap_GetSrcRect(cell->tile_id);
            float u0 = src_rect ? (float)src_rect->x / TILESET_TEX_W : 0.0f;
            float v0 = src_rect ? (float)src_rect->y / TILESET_TEX_H : 0.0f;
            float u1 = src_rect ? (float)(src_rect->x + src_rect->w) / TILESET_TEX_W : 1.0f;
            float v1 = src_rect ? (float)(src_rect->y + src_rect->h) / TILESET_TEX_H : 1.0f;

            // Lighting & Palette Tint
            SDL_Color top_color = { 235, 235, 245, 255 };
            if (map->current_biome == BIOME_VOLCANIC && (cell->flags & TILE_FLAG_WATER)) {
                top_color = (SDL_Color){ 255, 120, 50, 255 }; // Glowing lava
            } else if (map->current_biome == BIOME_CAVERN) {
                top_color = (SDL_Color){ 165, 175, 195, 255 };
            } else if (map->current_biome == BIOME_FOREST) {
                top_color = (SDL_Color){ 215, 240, 215, 255 };
            }

            // Top quad: p0=NW, p1=NE, p2=SE, p3=SW
            // Pass explicit UVs so terrain is right-side up (NW=u0,v0; NE=u1,v0; SE=u1,v1; SW=u0,v1)
            Vec3 p0 = { x0, y00, z0 };
            Vec3 p1 = { x1, y10, z0 };
            Vec3 p2 = { x1, y11, z1 };
            Vec3 p3 = { x0, y01, z1 };
            
            DrawTexturedQuad3DUV(renderer, cam, tex, p0, p1, p2, p3,
                                 u0, v0,  u1, v0,  u1, v1,  u0, v1, top_color);

            // Overlay flora / pebbles / tall grass
            if (cell->overlay_id < TILE_TOTAL_COUNT) {
                const SDL_Rect* o_src = Tilemap_GetSrcRect(cell->overlay_id);
                if (o_src) {
                    float ou0 = (float)o_src->x / TILESET_TEX_W;
                    float ov0 = (float)o_src->y / TILESET_TEX_H;
                    float ou1 = (float)(o_src->x + o_src->w) / TILESET_TEX_W;
                    float ov1 = (float)(o_src->y + o_src->h) / TILESET_TEX_H;
                    Vec3 op0 = { x0, y00 + 0.4f, z0 };
                    Vec3 op1 = { x1, y10 + 0.4f, z0 };
                    Vec3 op2 = { x1, y11 + 0.4f, z1 };
                    Vec3 op3 = { x0, y01 + 0.4f, z1 };
                    DrawTexturedQuad3DUV(renderer, cam, tex, op0, op1, op2, op3,
                                         ou0, ov0,  ou1, ov0,  ou1, ov1,  ou0, ov1, top_color);
                }
            }

            // 3D Cliff Faces on lower adjacent neighbors with Backface Culling
            if (cell->elevation == 1 && !(cell->flags & TILE_FLAG_RAMP)) {
                const SDL_Rect* cliff_src = Tilemap_GetSrcRect(TILE_ROCK_B);
                float cu0 = cliff_src ? (float)cliff_src->x / TILESET_TEX_W : 0.0f;
                float cv0 = cliff_src ? (float)cliff_src->y / TILESET_TEX_H : 0.0f;
                float cu1 = cliff_src ? (float)(cliff_src->x + cliff_src->w) / TILESET_TEX_W : 1.0f;
                float cv1 = cliff_src ? (float)(cliff_src->y + cliff_src->h) / TILESET_TEX_H : 1.0f;

                // South face: normal +Z (only visible if camera is South of face: cam->pos.z > z1)
                if (cam->pos.z > z1 && ty + 1 < MAP_TILES_Y && map->cells[ty + 1][tx].elevation == 0 && !(map->cells[ty + 1][tx].flags & TILE_FLAG_RAMP)) {
                    Vec3 c0 = { x0, 0.0f, z1 };
                    Vec3 c1 = { x1, 0.0f, z1 };
                    Vec3 c2 = { x1, PLATEAU_HEIGHT, z1 };
                    Vec3 c3 = { x0, PLATEAU_HEIGHT, z1 };
                    DrawTexturedQuad3D(renderer, cam, tex, c0, c1, c2, c3, cu0, cv0, cu1, cv1, (SDL_Color){ 180, 180, 195, 255 });
                }
                // North face: normal -Z (only visible if camera is North of face: cam->pos.z < z0)
                if (cam->pos.z < z0 && ty - 1 >= 0 && map->cells[ty - 1][tx].elevation == 0 && !(map->cells[ty - 1][tx].flags & TILE_FLAG_RAMP)) {
                    Vec3 c0 = { x1, 0.0f, z0 };
                    Vec3 c1 = { x0, 0.0f, z0 };
                    Vec3 c2 = { x0, PLATEAU_HEIGHT, z0 };
                    Vec3 c3 = { x1, PLATEAU_HEIGHT, z0 };
                    DrawTexturedQuad3D(renderer, cam, tex, c0, c1, c2, c3, cu0, cv0, cu1, cv1, (SDL_Color){ 140, 140, 160, 255 });
                }
                // West face: normal -X (only visible if camera is West of face: cam->pos.x < x0)
                if (cam->pos.x < x0 && tx - 1 >= 0 && map->cells[ty][tx - 1].elevation == 0 && !(map->cells[ty][tx - 1].flags & TILE_FLAG_RAMP)) {
                    Vec3 c0 = { x0, 0.0f, z0 };
                    Vec3 c1 = { x0, 0.0f, z1 };
                    Vec3 c2 = { x0, PLATEAU_HEIGHT, z1 };
                    Vec3 c3 = { x0, PLATEAU_HEIGHT, z0 };
                    DrawTexturedQuad3D(renderer, cam, tex, c0, c1, c2, c3, cu0, cv0, cu1, cv1, (SDL_Color){ 200, 200, 215, 255 });
                }
                // East face: normal +X (only visible if camera is East of face: cam->pos.x > x1)
                if (cam->pos.x > x1 && tx + 1 < MAP_TILES_X && map->cells[ty][tx + 1].elevation == 0 && !(map->cells[ty][tx + 1].flags & TILE_FLAG_RAMP)) {
                    Vec3 c0 = { x1, 0.0f, z1 };
                    Vec3 c1 = { x1, 0.0f, z0 };
                    Vec3 c2 = { x1, PLATEAU_HEIGHT, z0 };
                    Vec3 c3 = { x1, PLATEAU_HEIGHT, z1 };
                    DrawTexturedQuad3D(renderer, cam, tex, c0, c1, c2, c3, cu0, cv0, cu1, cv1, (SDL_Color){ 160, 160, 180, 255 });
                }
            }
        }
    }
}

static void DrawBillboardQuad(SDL_Renderer* renderer, const Camera3D* cam,
                              const SpriteSheet* sheet, int frame, int dir,
                              Vec3 ground_pos, float width, float height,
                              SDL_Color fallback_color, bool hit_flash) {
    float hw = width * 0.5f;
    
    // Cylindrical Y-billboard: spans horizontally along cam->right, vertically along +Y
    Vec3 w_vec = { cam->right.x * hw, 0.0f, cam->right.z * hw };
    
    Vec3 p0 = { ground_pos.x - w_vec.x, ground_pos.y,          ground_pos.z - w_vec.z };
    Vec3 p1 = { ground_pos.x + w_vec.x, ground_pos.y,          ground_pos.z + w_vec.z };
    Vec3 p2 = { ground_pos.x + w_vec.x, ground_pos.y + height, ground_pos.z + w_vec.z };
    Vec3 p3 = { ground_pos.x - w_vec.x, ground_pos.y + height, ground_pos.z - w_vec.z };

    if (sheet && sheet->loaded && sheet->texture) {
        int tw = 0, th = 0;
        SDL_QueryTexture(sheet->texture, NULL, NULL, &tw, &th);
        if (tw <= 0 || th <= 0) return;

        int num_frames = sheet->num_frames > 0 ? sheet->num_frames : 1;
        int num_dirs   = sheet->num_directions > 0 ? sheet->num_directions : 1;
        
        int f = ((frame % num_frames) + num_frames) % num_frames;
        int d = ((dir % num_dirs) + num_dirs) % num_dirs;
        
        float u0 = (float)(f * sheet->frame_w) / (float)tw;
        float v0 = (float)(d * sheet->frame_h) / (float)th;
        float u1 = (float)((f + 1) * sheet->frame_w) / (float)tw;
        float v1 = (float)((d + 1) * sheet->frame_h) / (float)th;
        
        SDL_Color color = hit_flash ? (SDL_Color){ 255, 100, 100, 255 } : fallback_color;
        DrawTexturedQuad3D(renderer, cam, sheet->texture, p0, p1, p2, p3, u0, v0, u1, v1, color);
    } else {
        // Fallback procedural colored billboard
        SDL_Color col = hit_flash ? (SDL_Color){ 255, 255, 255, 255 } : fallback_color;
        DrawTexturedQuad3D(renderer, cam, NULL, p0, p1, p2, p3, 0.0f, 0.0f, 1.0f, 1.0f, col);
    }
}

static void DrawGroundShadow3D(SDL_Renderer* renderer, const Camera3D* cam, Vec3 pos, float radius) {
    int segments = 12;
    SDL_Vertex verts[14];
    int indices[36];
    
    Vec3 center_view;
    Camera3D_WorldToView(cam, (Vec3){ pos.x, pos.y + 0.3f, pos.z }, &center_view);
    if (center_view.z < cam->near_z + 1.0f) return;
    
    SDL_FPoint c_screen;
    if (!Camera3D_ViewToScreen(cam, center_view, &c_screen.x, &c_screen.y)) return;
    
    verts[0].position = c_screen;
    verts[0].color = (SDL_Color){ 10, 15, 25, 140 };
    verts[0].tex_coord = (SDL_FPoint){ 0.5f, 0.5f };
    
    for (int i = 0; i < segments; i++) {
        float ang = (float)i * (2.0f * (float)M_PI / (float)segments);
        Vec3 p = { pos.x + cosf(ang) * radius, pos.y + 0.3f, pos.z + sinf(ang) * radius };
        Vec3 v;
        Camera3D_WorldToView(cam, p, &v);
        SDL_FPoint s;
        if (!Camera3D_ViewToScreen(cam, v, &s.x, &s.y)) return; // Safely abort if rim is behind near plane!
        
        verts[i + 1].position = s;
        verts[i + 1].color = (SDL_Color){ 10, 15, 25, 0 }; // Soft faded rim
        verts[i + 1].tex_coord = (SDL_FPoint){ 0.5f + cosf(ang) * 0.5f, 0.5f + sinf(ang) * 0.5f };
        
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = (i + 1 == segments) ? 1 : (i + 2);
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(renderer, NULL, verts, segments + 1, indices, segments * 3);
}

void Render3D_DrawWorld(GameState* game) {
    if (!game || !game->renderer) return;
    SDL_Renderer* renderer = game->renderer;
    Camera3D* cam = &game->camera3d;
    
    // Clear 3D Sky / Horizon
    if (game->tilemap.current_biome == BIOME_VOLCANIC) {
        SDL_SetRenderDrawColor(renderer, 28, 14, 12, 255);
    } else if (game->tilemap.current_biome == BIOME_CAVERN) {
        SDL_SetRenderDrawColor(renderer, 10, 12, 18, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 24, 32, 48, 255); // Deep retro indigo sky
    }
    SDL_RenderClear(renderer);

    // 1. Render 3D Terrain
    RenderTerrain3D(game);
    
    // 2. Collect 3D Render Items for Depth Sorting
    s_depth_item_count = 0;

    float p_height = Render3D_GetTerrainHeight(&game->tilemap, game->player.pos.x, game->player.pos.y);
    Vec3 p_pos_3d = { game->player.pos.x, p_height, game->player.pos.y };

    // Ground Shadows for entities
    for (int k = 0; k < game->active_enemy_count && s_depth_item_count < MAX_DEPTH_ITEMS; k++) {
        int i = game->active_enemy_indices[k];
        Enemy* e = &game->enemies[i];
        if (!e->active || e->is_underground) continue;
        float eh = Render3D_GetTerrainHeight(&game->tilemap, e->pos.x, e->pos.y);
        Vec3 ep = { e->pos.x, eh, e->pos.y };
        Vec3 ev;
        Camera3D_WorldToView(cam, ep, &ev);
        if (ev.z > cam->near_z && ev.z < cam->far_z) {
            DepthItem* it = &s_depth_items[s_depth_item_count++];
            it->depth = ev.z + 0.5f; // Draw shadow slightly behind billboard
            it->type = ITEM_GROUND_SHADOW;
            it->pos = ep;
            it->radius = game->enemy_types[e->type].radius * 0.9f;
        }
    }
    
    // Player Shadow
    if (s_depth_item_count < MAX_DEPTH_ITEMS) {
        Vec3 pv;
        Camera3D_WorldToView(cam, p_pos_3d, &pv);
        if (pv.z > cam->near_z) {
            DepthItem* it = &s_depth_items[s_depth_item_count++];
            it->depth = pv.z + 0.5f;
            it->type = ITEM_GROUND_SHADOW;
            it->pos = p_pos_3d;
            it->radius = game->player.radius * 0.95f;
        }
    }

    // Destructible Boulders
    for (int b = 0; b < game->tilemap.boulder_count && s_depth_item_count < MAX_DEPTH_ITEMS; b++) {
        DestructibleBoulder* bld = &game->tilemap.boulders[b];
        if (!bld->active) continue;
        float bx = (float)bld->tile_x * TILE_WORLD_SIZE + TILE_WORLD_SIZE * 0.5f;
        float bz = (float)bld->tile_y * TILE_WORLD_SIZE + TILE_WORLD_SIZE * 0.5f;
        float by = Render3D_GetTerrainHeight(&game->tilemap, bx, bz);
        Vec3 bp = { bx, by, bz };
        Vec3 bv;
        Camera3D_WorldToView(cam, bp, &bv);
        if (bv.z <= cam->near_z || bv.z > cam->far_z) continue;

        DepthItem* it = &s_depth_items[s_depth_item_count++];
        it->depth = bv.z;
        it->type = ITEM_BOULDER;
        it->index = b;
        it->pos = bp;
        it->scale = 1.0f;
        it->radius = TILE_WORLD_SIZE * 0.45f;
        it->flash = (bld->flash_timer > 0.0f);
    }

    // Enemies (Y-billboards or underground burrows)
    for (int k = 0; k < game->active_enemy_count && s_depth_item_count < MAX_DEPTH_ITEMS; k++) {
        int i = game->active_enemy_indices[k];
        Enemy* e = &game->enemies[i];
        if (!e->active) continue;
        
        EnemyTypeConfig* cfg = &game->enemy_types[e->type];
        float eh = Render3D_GetTerrainHeight(&game->tilemap, e->pos.x, e->pos.y);
        
        // Underground Diglett burrowing
        if (e->is_underground) {
            Vec3 ep = { e->pos.x, eh, e->pos.y };
            Vec3 ev;
            Camera3D_WorldToView(cam, ep, &ev);
            if (ev.z > cam->near_z && ev.z < cam->far_z) {
                DepthItem* it = &s_depth_items[s_depth_item_count++];
                it->depth = ev.z;
                it->type = ITEM_UNDERGROUND_DIRT;
                it->index = i;
                it->pos = ep;
                it->radius = cfg->radius;
            }
            continue;
        }

        // Aerial hover for flyers
        if (cfg->move_pattern == ENEMY_MOVE_FLANK_ORBIT) {
            eh += 20.0f + sinf(game->spawner.game_time * 4.0f + (float)i) * 3.5f;
        }
        
        Vec3 ep = { e->pos.x, eh, e->pos.y };
        Vec3 ev;
        Camera3D_WorldToView(cam, ep, &ev);
        if (ev.z <= cam->near_z || ev.z > cam->far_z) continue;

        DepthItem* it = &s_depth_items[s_depth_item_count++];
        it->depth = ev.z;
        it->type = ITEM_BILLBOARD_SPRITE;
        it->index = i;
        it->pos = ep;
        it->color = cfg->debug_color;
        it->flash = (e->flash_timer > 0.0f);

        bool is_atk = (e->ai_state == ENEMY_AI_ATTACKING || e->ai_state == ENEMY_AI_WINDUP) && cfg->has_attack_sprite && cfg->sprite_attack.loaded;
        it->sheet = is_atk ? &cfg->sprite_attack : &cfg->sprite;
        it->anim  = is_atk ? &e->attack_anim : &e->anim;
        it->rel_dir = Camera3D_GetRelativeDirection(cam, it->anim->direction);
        it->scale = cfg->sprite_scale > 0.01f ? cfg->sprite_scale : 1.5f;
        it->radius = cfg->radius;
    }

    // Party Companions (Y-billboards)
    for (int m = 0; m < game->party_count && s_depth_item_count < MAX_DEPTH_ITEMS; m++) {
        PartyMember* mem = &game->party[m];
        if (!mem->active) continue;
        float mh = Render3D_GetTerrainHeight(&game->tilemap, mem->pos.x, mem->pos.y);
        Vec3 mp = { mem->pos.x, mh, mem->pos.y };
        Vec3 mv;
        Camera3D_WorldToView(cam, mp, &mv);
        if (mv.z <= cam->near_z || mv.z > cam->far_z) continue;

        DepthItem* it = &s_depth_items[s_depth_item_count++];
        it->depth = mv.z;
        it->type = ITEM_BILLBOARD_SPRITE;
        it->pos = mp;
        it->sheet = &mem->sprite;
        it->anim = &mem->anim;
        it->rel_dir = Camera3D_GetRelativeDirection(cam, mem->anim.direction);
        it->scale = mem->sprite_scale > 0.01f ? mem->sprite_scale : 1.5f;
        it->color = (SDL_Color){ 50, 240, 110, 255 };
        it->flash = false;
        it->radius = 16.0f;
    }

    // Player Dash Ghost Afterimages in 3D
    if (game->player.state == PLAYER_STATE_DASHING) {
        float ghost_dists[3] = { 18.0f, 36.0f, 54.0f };
        for (int g = 0; g < 3 && s_depth_item_count < MAX_DEPTH_ITEMS; g++) {
            float gx = game->player.pos.x - game->player.dash_dir.x * ghost_dists[g];
            float gz = game->player.pos.y - game->player.dash_dir.y * ghost_dists[g];
            float gy = Render3D_GetTerrainHeight(&game->tilemap, gx, gz);
            Vec3 gp = { gx, gy, gz };
            Vec3 gv;
            Camera3D_WorldToView(cam, gp, &gv);
            if (gv.z <= cam->near_z || gv.z > cam->far_z) continue;

            DepthItem* it = &s_depth_items[s_depth_item_count++];
            it->depth = gv.z + 0.1f * (float)(g + 1);
            it->type = ITEM_BILLBOARD_SPRITE;
            it->pos = gp;
            it->sheet = &game->player.sprite_charge;
            it->anim = &game->player.anim;
            it->rel_dir = Camera3D_GetRelativeDirection(cam, game->player.anim.direction);
            it->scale = 1.75f;
            it->flash = false;
            it->color = (SDL_Color){ 100, 200, 255, (Uint8)(140 / (g + 1)) };
            it->radius = game->player.radius;
        }
    }

    // Player (Y-billboard)
    if (s_depth_item_count < MAX_DEPTH_ITEMS) {
        Vec3 pv;
        Camera3D_WorldToView(cam, p_pos_3d, &pv);
        if (pv.z > cam->near_z) {
            DepthItem* it = &s_depth_items[s_depth_item_count++];
            it->depth = pv.z;
            it->type = ITEM_BILLBOARD_SPRITE;
            it->pos = p_pos_3d;
            it->flash = (game->player.damage_flash_timer > 0.0f);
            it->color = (SDL_Color){ 80, 180, 240, 255 };
            
            if (game->player.state == PLAYER_STATE_DASHING) {
                it->sheet = &game->player.sprite_charge;
            } else if (game->player.state == PLAYER_STATE_HYDRO_PUMP || game->player.state == PLAYER_STATE_SOLAR_BEAM || game->player.state == PLAYER_STATE_FLAMETHROWER) {
                it->sheet = &game->player.sprite_special;
            } else {
                it->sheet = &game->player.sprite;
            }
            it->anim = &game->player.anim;
            it->rel_dir = Camera3D_GetRelativeDirection(cam, game->player.anim.direction);
            it->scale = 1.75f;
            it->radius = game->player.radius;
        }
    }

    // Gems (3D floating crystals)
    for (int g = 0; g < MAX_GEMS && s_depth_item_count < MAX_DEPTH_ITEMS; g++) {
        Gem* gem = &game->gems[g];
        if (!gem->active) continue;
        float gh = Render3D_GetTerrainHeight(&game->tilemap, gem->pos.x, gem->pos.y);
        float bob = sinf(game->spawner.game_time * 5.5f + (float)g * 0.75f) * 3.0f;
        Vec3 gp = { gem->pos.x, gh + 10.0f + bob, gem->pos.y };
        Vec3 gv;
        Camera3D_WorldToView(cam, gp, &gv);
        if (gv.z <= cam->near_z || gv.z > cam->far_z) continue;

        DepthItem* it = &s_depth_items[s_depth_item_count++];
        it->depth = gv.z;
        it->type = ITEM_GEM;
        it->index = g;
        it->pos = gp;
        it->radius = gem->radius;
    }

    // Projectiles
    for (int p = 0; p < MAX_PROJECTILES && s_depth_item_count < MAX_DEPTH_ITEMS; p++) {
        Projectile* proj = &game->projectiles[p];
        if (!proj->active) continue;
        float ph = Render3D_GetTerrainHeight(&game->tilemap, proj->pos.x, proj->pos.y);
        
        float proj_y = ph + 16.0f;
        if (proj->type == PROJ_MORTAR_SHELL) {
            float u = proj->lifetime / (proj->max_lifetime > 0.001f ? proj->max_lifetime : 1.0f);
            proj_y = ph + 16.0f + 140.0f * sinf(u * (float)M_PI);
        } else if (proj->type == PROJ_LIGHTNING_BOLT) {
            proj_y = ph + 80.0f;
        } else if (proj->type == PROJ_BUBBLE || proj->type == PROJ_SCALD_BUBBLE) {
            proj_y = ph + 14.0f + sinf(proj->lifetime * 8.0f) * 4.0f;
        }
        
        Vec3 pp = { proj->pos.x, proj_y, proj->pos.y };
        Vec3 pv;
        Camera3D_WorldToView(cam, pp, &pv);
        if (pv.z <= cam->near_z || pv.z > cam->far_z) continue;

        DepthItem* it = &s_depth_items[s_depth_item_count++];
        it->depth = pv.z;
        it->type = ITEM_PROJECTILE;
        it->index = p;
        it->pos = pp;
        it->radius = proj->radius;
        it->color = proj->color;
    }

    // Ground Hazards (Fire scorch, toxic spores, acid mires)
    for (int h = 0; h < MAX_GROUND_HAZARDS && s_depth_item_count < MAX_DEPTH_ITEMS; h++) {
        GroundHazard* haz = &game->ground_hazards[h];
        if (!haz->active) continue;
        float hh = Render3D_GetTerrainHeight(&game->tilemap, haz->pos.x, haz->pos.y);
        Vec3 hp = { haz->pos.x, hh + 0.6f, haz->pos.y };
        Vec3 hv;
        Camera3D_WorldToView(cam, hp, &hv);
        if (hv.z <= cam->near_z || hv.z > cam->far_z) continue;

        DepthItem* it = &s_depth_items[s_depth_item_count++];
        it->depth = hv.z + 0.3f;
        it->type = ITEM_GROUND_HAZARD;
        it->index = h;
        it->pos = hp;
        it->radius = haz->radius;
        it->color = haz->color;
    }

    // Poké-Portal Route Exit Gate
    if (game->tilemap.exit_gate.active && s_depth_item_count < MAX_DEPTH_ITEMS) {
        float gh = Render3D_GetTerrainHeight(&game->tilemap, game->tilemap.exit_gate.x, game->tilemap.exit_gate.y);
        Vec3 gp = { game->tilemap.exit_gate.x, gh, game->tilemap.exit_gate.y };
        Vec3 gv;
        Camera3D_WorldToView(cam, gp, &gv);
        if (gv.z > cam->near_z && gv.z < cam->far_z) {
            DepthItem* it = &s_depth_items[s_depth_item_count++];
            it->depth = gv.z;
            it->type = ITEM_PORTAL_GATE;
            it->pos = gp;
            it->radius = game->tilemap.exit_gate.radius;
        }
    }

    // Sort all depth items back-to-front
    qsort(s_depth_items, s_depth_item_count, sizeof(DepthItem), DepthCompare);

    // 3. Render Depth-Sorted Items
    for (int i = 0; i < s_depth_item_count; i++) {
        DepthItem* it = &s_depth_items[i];
        
        if (it->type == ITEM_GROUND_SHADOW) {
            DrawGroundShadow3D(renderer, cam, it->pos, it->radius);
        } else if (it->type == ITEM_BOULDER) {
            DestructibleBoulder* bld = &game->tilemap.boulders[it->index];
            DrawGroundShadow3D(renderer, cam, it->pos, it->radius);
            
            const SDL_Rect* b_src = Tilemap_GetSrcRect(TILE_ROCK_BOULDER);
            float bw = 48.0f;
            float bh = 48.0f;
            float hw = bw * 0.5f;
            Vec3 w_vec = { cam->right.x * hw, 0.0f, cam->right.z * hw };
            Vec3 p0 = { it->pos.x - w_vec.x, it->pos.y,        it->pos.z - w_vec.z };
            Vec3 p1 = { it->pos.x + w_vec.x, it->pos.y,        it->pos.z + w_vec.z };
            Vec3 p2 = { it->pos.x + w_vec.x, it->pos.y + bh,   it->pos.z + w_vec.z };
            Vec3 p3 = { it->pos.x - w_vec.x, it->pos.y + bh,   it->pos.z - w_vec.z };

            SDL_Color col = it->flash ? (SDL_Color){ 255, 255, 255, 255 } : (SDL_Color){ 220, 220, 225, 255 };
            if (b_src && game->tileset_texture) {
                float u0 = (float)b_src->x / TILESET_TEX_W;
                float v0 = (float)b_src->y / TILESET_TEX_H;
                float u1 = (float)(b_src->x + b_src->w) / TILESET_TEX_W;
                float v1 = (float)(b_src->y + b_src->h) / TILESET_TEX_H;
                DrawTexturedQuad3D(renderer, cam, game->tileset_texture, p0, p1, p2, p3, u0, v0, u1, v1, col);
            } else {
                DrawTexturedQuad3D(renderer, cam, NULL, p0, p1, p2, p3, 0.0f, 0.0f, 1.0f, 1.0f, (SDL_Color){ 130, 125, 120, 255 });
            }

            // Floating Mini HP Bar for damaged boulders
            if (bld->hp < bld->max_hp) {
                float sx, sy;
                if (Camera3D_Project(cam, (Vec3){ it->pos.x, it->pos.y + bh + 4.0f, it->pos.z }, &sx, &sy, NULL)) {
                    int bar_w = 36;
                    int bar_h = 4;
                    SDL_Rect hp_bg = { (int)sx - bar_w / 2, (int)sy, bar_w, bar_h };
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 220);
                    SDL_RenderFillRect(renderer, &hp_bg);

                    float ratio = bld->hp / bld->max_hp;
                    if (ratio < 0.0f) ratio = 0.0f;
                    SDL_SetRenderDrawColor(renderer, 220, 180, 50, 255);
                    SDL_Rect hp_fg = { (int)sx - bar_w / 2, (int)sy, (int)(bar_w * ratio), bar_h };
                    SDL_RenderFillRect(renderer, &hp_fg);
                }
            }
        } else if (it->type == ITEM_UNDERGROUND_DIRT) {
            // Diglett Underground Burrowing Mound
            DrawGroundShadow3D(renderer, cam, it->pos, it->radius);
            float sx, sy;
            if (Camera3D_Project(cam, (Vec3){ it->pos.x, it->pos.y + 2.0f, it->pos.z }, &sx, &sy, NULL)) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 140, 90, 45, 230);
                for (int p = 0; p < 8; p++) {
                    int px = (int)sx + ((p * 17) % 21) - 10;
                    int py = (int)sy + ((p * 23) % 15) - 7;
                    SDL_Rect dirt = { px, py, 3, 3 };
                    SDL_RenderFillRect(renderer, &dirt);
                }
            }
        } else if (it->type == ITEM_BILLBOARD_SPRITE) {
            float spr_w = 48.0f * it->scale;
            float spr_h = 48.0f * it->scale;
            if (it->sheet && it->sheet->frame_w > 0) {
                spr_w = (float)it->sheet->frame_w * it->scale;
                spr_h = (float)it->sheet->frame_h * it->scale;
            }
            int cur_frame = it->anim ? it->anim->current_frame : 0;
            DrawBillboardQuad(renderer, cam, it->sheet, cur_frame, it->rel_dir,
                              it->pos, spr_w, spr_h, it->color, it->flash);

            // Enemy-specific visual telegraphs and overlays
            if (it->index >= 0 && it->index < MAX_ENEMIES) {
                Enemy* e = &game->enemies[it->index];
                if (e->active) {
                    EnemyTypeConfig* cfg = &game->enemy_types[e->type];

                    // Charger telegraph laser in 3D
                    if (cfg->move_pattern == ENEMY_MOVE_CHARGER && e->ai_state == ENEMY_AI_WINDUP) {
                        Vec3 l_start = { it->pos.x, it->pos.y + 12.0f, it->pos.z };
                        Vec3 l_end   = { it->pos.x + e->charge_dir.x * 240.0f, it->pos.y + 12.0f, it->pos.z + e->charge_dir.y * 240.0f };
                        int alpha = 160 + (int)(sinf(e->state_timer * 20.0f) * 75.0f);
                        if (alpha < 60) alpha = 60;
                        if (alpha > 255) alpha = 255;
                        DrawClippedLine3D(renderer, cam, l_start, l_end, (SDL_Color){ 255, 40, 40, (Uint8)alpha }, 1);
                    }

                    // Trapped bubble sphere in 3D
                    if (e->is_trapped) {
                        float sx, sy;
                        if (Camera3D_Project(cam, (Vec3){ it->pos.x, it->pos.y + spr_h * 0.45f, it->pos.z }, &sx, &sy, NULL)) {
                            float br = (cfg->radius + 12.0f) * (cam->focal_length / it->depth);
                            if (br > 4.0f) {
                                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                                SDL_SetRenderDrawColor(renderer, 80, 210, 255, 90);
                                SDL_Rect b_fill = { (int)(sx - br), (int)(sy - br), (int)(br * 2.0f), (int)(br * 2.0f) };
                                SDL_RenderFillRect(renderer, &b_fill);
                                SDL_SetRenderDrawColor(renderer, 150, 240, 255, 220);
                                for (int a = 0; a < 16; a++) {
                                    float a1 = (float)a * (2.0f * (float)M_PI / 16.0f);
                                    float a2 = (float)(a + 1) * (2.0f * (float)M_PI / 16.0f);
                                    SDL_RenderDrawLine(renderer, (int)(sx + cosf(a1)*br), (int)(sy + sinf(a1)*br),
                                                                 (int)(sx + cosf(a2)*br), (int)(sy + sinf(a2)*br));
                                }
                            }
                        }
                    }

                    // Catchable indicator tag above head in 3D
                    if ((e->hp <= e->max_hp * 0.40f || e->is_trapped) && !cfg->is_boss) {
                        float sx, sy;
                        if (Camera3D_Project(cam, (Vec3){ it->pos.x, it->pos.y + spr_h + 8.0f, it->pos.z }, &sx, &sy, NULL)) {
                            float pulse = 0.8f + 0.2f * sinf(game->spawner.game_time * 8.0f);
                            Font_DrawTextShadow(renderer, game->font_texture, "[E] CATCH", (int)sx - 26, (int)sy, 1, 
                                                (SDL_Color){ 255, (Uint8)(220 * pulse), 60, 255 });
                        }
                    }
                }
            }
        } else if (it->type == ITEM_GROUND_HAZARD) {
            DrawGroundShadow3D(renderer, cam, it->pos, it->radius);
            float sx, sy;
            if (Camera3D_Project(cam, it->pos, &sx, &sy, NULL)) {
                float proj_r = it->radius * (cam->focal_length / it->depth);
                if (proj_r > 3.0f) {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, it->color.r, it->color.g, it->color.b, 160);
                    for (int a = 0; a < 16; a++) {
                        float a1 = (float)a * (2.0f * (float)M_PI / 16.0f);
                        float a2 = (float)(a + 1) * (2.0f * (float)M_PI / 16.0f);
                        SDL_RenderDrawLine(renderer, (int)(sx + cosf(a1)*proj_r), (int)(sy + sinf(a1)*proj_r*0.6f),
                                                     (int)(sx + cosf(a2)*proj_r), (int)(sy + sinf(a2)*proj_r*0.6f));
                    }
                }
            }
        } else if (it->type == ITEM_GEM) {
            float sx, sy;
            if (Camera3D_Project(cam, it->pos, &sx, &sy, NULL)) {
                float pr = it->radius * (cam->focal_length / it->depth) * 1.2f;
                if (pr < 3.0f) pr = 3.0f;
                SDL_Rect r = { (int)(sx - pr), (int)(sy - pr), (int)(pr * 2.0f), (int)(pr * 2.0f) };
                SDL_SetRenderDrawColor(renderer, 60, 255, 140, 255);
                SDL_RenderFillRect(renderer, &r);
                SDL_SetRenderDrawColor(renderer, 240, 255, 255, 255);
                SDL_Rect core = { (int)(sx - pr * 0.4f), (int)(sy - pr * 0.4f), (int)(pr * 0.8f), (int)(pr * 0.8f) };
                SDL_RenderFillRect(renderer, &core);
            }
        } else if (it->type == ITEM_PROJECTILE) {
            Projectile* prj = (it->index >= 0 && it->index < MAX_PROJECTILES) ? &game->projectiles[it->index] : NULL;
            
            if (prj && prj->type == PROJ_POKEBALL) {
                // Thrown 3D Poké Ball capsule
                float sx, sy;
                if (Camera3D_Project(cam, it->pos, &sx, &sy, NULL)) {
                    float pr = it->radius * (cam->focal_length / it->depth);
                    if (pr < 3.0f) pr = 3.0f;
                    int ipr = (int)pr;
                    // Red top hemisphere
                    SDL_SetRenderDrawColor(renderer, 240, 35, 35, 255);
                    for (int dy = -ipr; dy <= 0; dy++) {
                        int dx_max = (int)sqrtf((float)(ipr * ipr - dy * dy));
                        SDL_RenderDrawLine(renderer, (int)sx - dx_max, (int)sy + dy, (int)sx + dx_max, (int)sy + dy);
                    }
                    // White bottom hemisphere
                    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
                    for (int dy = 1; dy <= ipr; dy++) {
                        int dx_max = (int)sqrtf((float)(ipr * ipr - dy * dy));
                        SDL_RenderDrawLine(renderer, (int)sx - dx_max, (int)sy + dy, (int)sx + dx_max, (int)sy + dy);
                    }
                    // Dividing black belt
                    SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
                    SDL_RenderDrawLine(renderer, (int)sx - ipr, (int)sy, (int)sx + ipr, (int)sy);
                    // Center button
                    int btn_r = (ipr >= 6) ? 3 : 2;
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_Rect b_core = { (int)sx - btn_r, (int)sy - btn_r, btn_r * 2, btn_r * 2 };
                    SDL_RenderFillRect(renderer, &b_core);
                }
            } else if (prj && (prj->type == PROJ_BUBBLE || prj->type == PROJ_SCALD_BUBBLE)) {
                // Iridescent 3D bubble
                float sx, sy;
                if (Camera3D_Project(cam, it->pos, &sx, &sy, NULL)) {
                    float pr = it->radius * (cam->focal_length / it->depth);
                    if (pr < 3.0f) pr = 3.0f;
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, it->color.r, it->color.g, it->color.b, 100);
                    SDL_Rect body = { (int)(sx - pr), (int)(sy - pr), (int)(pr * 2.0f), (int)(pr * 2.0f) };
                    SDL_RenderFillRect(renderer, &body);
                    SDL_SetRenderDrawColor(renderer, 200, 245, 255, 230);
                    for (int a = 0; a < 16; a++) {
                        float a1 = (float)a * (2.0f * (float)M_PI / 16.0f);
                        float a2 = (float)(a + 1) * (2.0f * (float)M_PI / 16.0f);
                        SDL_RenderDrawLine(renderer, (int)(sx + cosf(a1)*pr), (int)(sy + sinf(a1)*pr),
                                                     (int)(sx + cosf(a2)*pr), (int)(sy + sinf(a2)*pr));
                    }
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 240);
                    SDL_Rect gleam = { (int)(sx - pr * 0.45f), (int)(sy - pr * 0.45f), (int)(pr * 0.45f), (int)(pr * 0.45f) };
                    SDL_RenderFillRect(renderer, &gleam);
                }
            } else if (prj && prj->type == PROJ_EMBER) {
                // Expanding fire puff
                float sx, sy;
                if (Camera3D_Project(cam, it->pos, &sx, &sy, NULL)) {
                    float pr = it->radius * (cam->focal_length / it->depth);
                    if (pr < 3.0f) pr = 3.0f;
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 255, 75, 15, 180);
                    SDL_Rect outer = { (int)(sx - pr * 1.35f), (int)(sy - pr * 1.35f), (int)(pr * 2.7f), (int)(pr * 2.7f) };
                    SDL_RenderFillRect(renderer, &outer);
                    SDL_SetRenderDrawColor(renderer, 255, 160, 30, 240);
                    SDL_Rect mid = { (int)(sx - pr), (int)(sy - pr), (int)(pr * 2.0f), (int)(pr * 2.0f) };
                    SDL_RenderFillRect(renderer, &mid);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 200, 255);
                    SDL_Rect core = { (int)(sx - pr * 0.45f), (int)(sy - pr * 0.45f), (int)(pr * 0.9f), (int)(pr * 0.9f) };
                    SDL_RenderFillRect(renderer, &core);
                }
            } else if (prj && prj->type == PROJ_SWIFT_STAR) {
                // Spinning 5-point star
                float sx, sy;
                if (Camera3D_Project(cam, it->pos, &sx, &sy, NULL)) {
                    float pr = it->radius * (cam->focal_length / it->depth);
                    if (pr < 4.0f) pr = 4.0f;
                    float spin = prj->lifetime * 8.0f;
                    SDL_Point star_pts[11];
                    for (int pt = 0; pt < 10; pt++) {
                        float a = spin + (float)pt * ((float)M_PI / 5.0f);
                        float cur_r = (pt % 2 == 0) ? pr : pr * 0.45f;
                        star_pts[pt].x = (int)(sx + cosf(a) * cur_r);
                        star_pts[pt].y = (int)(sy + sinf(a) * cur_r);
                    }
                    star_pts[10] = star_pts[0];
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 255, 240, 80, 255);
                    SDL_RenderDrawLines(renderer, star_pts, 11);
                }
            } else if (prj && prj->type == PROJ_SLASH) {
                // Curved sweeping 3D crescent blade arc
                float progress = prj->lifetime / (prj->max_lifetime > 0.001f ? prj->max_lifetime : 1.0f);
                float alpha = 1.0f - progress * progress;
                if (alpha > 0.05f) {
                    float start_a = prj->orbit_angle - prj->orbit_dist * 0.5f;
                    float end_a = prj->orbit_angle + prj->orbit_dist * 0.5f;
                    const int steps = 14;
                    float base_r = prj->radius;
                    
                    for (int s = 0; s < steps; s++) {
                        float u1 = (float)s / (float)steps;
                        float u2 = (float)(s + 1) / (float)steps;
                        float a1 = start_a + (end_a - start_a) * u1;
                        float a2 = start_a + (end_a - start_a) * u2;
                        
                        Vec3 p1_out = { it->pos.x + cosf(a1) * base_r, it->pos.y, it->pos.z + sinf(a1) * base_r };
                        Vec3 p2_out = { it->pos.x + cosf(a2) * base_r, it->pos.y, it->pos.z + sinf(a2) * base_r };
                        
                        DrawClippedLine3D(renderer, cam, p1_out, p2_out, 
                            (SDL_Color){ prj->color.r, prj->color.g, prj->color.b, (Uint8)(alpha * 240.0f) }, 2);
                    }
                }
            } else if (prj && prj->type == PROJ_LIGHTNING_BOLT) {
                // Jagged 3D lightning strike from sky to ground
                Vec3 top_b = { it->pos.x, it->pos.y + 120.0f, it->pos.z };
                Vec3 bot_b = { it->pos.x, it->pos.y, it->pos.z };
                DrawClippedLine3D(renderer, cam, top_b, bot_b, (SDL_Color){ 180, 240, 255, 255 }, 2);
            } else {
                // Standard glowing elemental plasma projectile
                float sx, sy;
                if (Camera3D_Project(cam, it->pos, &sx, &sy, NULL)) {
                    float pr = it->radius * (cam->focal_length / it->depth) * 1.35f;
                    if (pr < 3.0f) pr = 3.0f;
                    SDL_Rect r = { (int)(sx - pr), (int)(sy - pr), (int)(pr * 2.0f), (int)(pr * 2.0f) };
                    SDL_SetRenderDrawColor(renderer, it->color.r, it->color.g, it->color.b, 255);
                    SDL_RenderFillRect(renderer, &r);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_Rect core = { (int)(sx - pr * 0.35f), (int)(sy - pr * 0.35f), (int)(pr * 0.7f), (int)(pr * 0.7f) };
                    SDL_RenderFillRect(renderer, &core);
                }
            }
        } else if (it->type == ITEM_PORTAL_GATE) {
            // Giant glowing 3D Poké-Portal arch ring
            float sx, sy;
            if (Camera3D_Project(cam, it->pos, &sx, &sy, NULL)) {
                float pr = it->radius * (cam->focal_length / it->depth);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 80, 220, 255, 220);
                for (int a = 0; a < 24; a++) {
                    float a1 = (float)a * (2.0f * (float)M_PI / 24.0f);
                    float a2 = (float)(a + 1) * (2.0f * (float)M_PI / 24.0f);
                    SDL_RenderDrawLine(renderer, (int)(sx + cosf(a1)*pr), (int)(sy - pr + sinf(a1)*pr),
                                                 (int)(sx + cosf(a2)*pr), (int)(sy - pr + sinf(a2)*pr));
                }
                Font_DrawTextShadow(renderer, game->font_texture, "ROUTE PORTAL", (int)sx - 42, (int)(sy - pr * 2.2f), 1, (SDL_Color){ 255, 220, 60, 255 });
            }
        }
    }

    // 4. Vine Whip Splines in 3D
    for (int v = 0; v < MAX_VINE_STRIKES; v++) {
        VineWhipStrike* whip = &game->vine_strikes[v];
        if (!whip->active) continue;
        float sh = Render3D_GetTerrainHeight(&game->tilemap, whip->start_pos.x, whip->start_pos.y) + 16.0f;
        float th = Render3D_GetTerrainHeight(&game->tilemap, whip->tip_pos.x, whip->tip_pos.y) + 16.0f;
        
        int nodes = 10;
        Vec3 prev_pt = { whip->start_pos.x, sh, whip->start_pos.y };
        
        for (int n = 1; n <= nodes; n++) {
            float u = (float)n / (float)nodes;
            float wave = sinf(u * 12.0f + whip->wave_seed) * 8.0f * (1.0f - u);
            Vec3 cur_pt = {
                whip->start_pos.x + (whip->tip_pos.x - whip->start_pos.x) * u - whip->dir.y * wave,
                sh + (th - sh) * u + sinf(u * (float)M_PI) * 12.0f,
                whip->start_pos.y + (whip->tip_pos.y - whip->start_pos.y) * u + whip->dir.x * wave
            };
            DrawClippedLine3D(renderer, cam, prev_pt, cur_pt, (SDL_Color){ 60, 220, 80, 240 }, 1);
            prev_pt = cur_pt;
        }
    }

    // 5. Player Active Special Abilities in 3D
    Player* pl = &game->player;
    if (pl->state == PLAYER_STATE_SOLAR_BEAM) {
        float px = pl->pos.x;
        float py = p_height + 18.0f;
        float pz = pl->pos.y;
        float ang = Sprite_DirectionToAngle(pl->anim.direction);
        float end_x = px + cosf(ang) * pl->special_q_range;
        float end_z = pz + sinf(ang) * pl->special_q_range;
        
        DrawClippedLine3D(renderer, cam, (Vec3){ px, py, pz }, (Vec3){ end_x, py, end_z }, 
                          (SDL_Color){ 255, 245, 120, 230 }, 5);
    } else if (pl->state == PLAYER_STATE_FLAMETHROWER) {
        // Fire Blast 3D starburst
        float px = pl->pos.x;
        float py = p_height + 16.0f;
        float pz = pl->pos.y;
        float ang = Sprite_DirectionToAngle(pl->anim.direction);
        float progress = 1.0f - (pl->special_q_timer / pl->special_q_duration);
        float burst_r = pl->special_q_range * (0.35f + progress * 0.75f);
        
        float arm_angles[5] = {
            ang,
            ang - (float)M_PI * 0.45f,
            ang + (float)M_PI * 0.45f,
            ang - (float)M_PI * 0.78f,
            ang + (float)M_PI * 0.78f
        };
        for (int a = 0; a < 5; a++) {
            float tx = px + cosf(arm_angles[a]) * burst_r;
            float tz = pz + sinf(arm_angles[a]) * burst_r;
            DrawClippedLine3D(renderer, cam, (Vec3){ px, py, pz }, (Vec3){ tx, py, tz },
                              (SDL_Color){ 255, 80, 20, 220 }, 3);
        }
    } else if (pl->state == PLAYER_STATE_HYDRO_PUMP) {
        // 4 Spinning water thruster jets
        float px = pl->pos.x;
        float py = p_height + 14.0f;
        float pz = pl->pos.y;
        float theta = pl->special_q_current_angle;
        
        for (int k = 0; k < 4; k++) {
            float ang = theta + (float)k * ((float)M_PI * 0.5f);
            float jx = px + cosf(ang) * pl->special_q_range * 0.85f;
            float jz = pz + sinf(ang) * pl->special_q_range * 0.85f;
            DrawClippedLine3D(renderer, cam, (Vec3){ px, py, pz }, (Vec3){ jx, py, jz },
                              (SDL_Color){ 60, 210, 255, 200 }, 2);
        }
    }

    // 6. Floating Damage Numbers in 3D
    if (game->dev_menu.show_damage_numbers) {
        char buf[16];
        for (int i = 0; i < MAX_PARTICLES; i++) {
            DamageText* t = &game->damage_texts[i];
            if (!t->active) continue;
            float th = Render3D_GetTerrainHeight(&game->tilemap, t->pos.x, t->pos.y);
            float rise = (t->lifetime / t->max_lifetime) * 28.0f;
            Vec3 tp = { t->pos.x, th + 28.0f + rise, t->pos.y };
            float sx, sy;
            if (Camera3D_Project(cam, tp, &sx, &sy, NULL)) {
                if (sx < -30 || sx > WINDOW_WIDTH + 30 || sy < -30 || sy > WINDOW_HEIGHT + 30) continue;
                float alpha = 1.0f - (t->lifetime / t->max_lifetime);
                SDL_Color col = t->color;
                col.a = (Uint8)(alpha * 255.0f);
                snprintf(buf, sizeof(buf), "%d", (int)t->damage);
                Font_DrawTextShadow(renderer, game->font_texture, buf, (int)sx, (int)sy, 2, col);
            }
        }
    }

    // 7. Dev Menu Visual Debug Overlays in 3D
    if (game->dev_menu.show_hitboxes) {
        // Player hitbox ring on ground
        for (int a = 0; a < 24; a++) {
            float a1 = (float)a * (2.0f * (float)M_PI / 24.0f);
            float a2 = (float)(a + 1) * (2.0f * (float)M_PI / 24.0f);
            Vec3 p1 = { pl->pos.x + cosf(a1) * pl->radius, p_height + 0.5f, pl->pos.y + sinf(a1) * pl->radius };
            Vec3 p2 = { pl->pos.x + cosf(a2) * pl->radius, p_height + 0.5f, pl->pos.y + sinf(a2) * pl->radius };
            DrawClippedLine3D(renderer, cam, p1, p2, (SDL_Color){ 0, 255, 255, 220 }, 1);
        }
        // Enemy hitbox rings
        for (int k = 0; k < game->active_enemy_count; k++) {
            int ei = game->active_enemy_indices[k];
            Enemy* e = &game->enemies[ei];
            if (!e->active || e->is_underground) continue;
            float eh = Render3D_GetTerrainHeight(&game->tilemap, e->pos.x, e->pos.y);
            float er = game->enemy_types[e->type].radius;
            for (int a = 0; a < 16; a++) {
                float a1 = (float)a * (2.0f * (float)M_PI / 16.0f);
                float a2 = (float)(a + 1) * (2.0f * (float)M_PI / 16.0f);
                Vec3 p1 = { e->pos.x + cosf(a1) * er, eh + 0.5f, e->pos.y + sinf(a1) * er };
                Vec3 p2 = { e->pos.x + cosf(a2) * er, eh + 0.5f, e->pos.y + sinf(a2) * er };
                DrawClippedLine3D(renderer, cam, p1, p2, (SDL_Color){ 255, 60, 60, 220 }, 1);
            }
        }
    }

    if (game->dev_menu.show_pickup_range) {
        for (int a = 0; a < 36; a++) {
            float a1 = (float)a * (2.0f * (float)M_PI / 36.0f);
            float a2 = (float)(a + 0.6f) * (2.0f * (float)M_PI / 36.0f);
            Vec3 p1 = { pl->pos.x + cosf(a1) * pl->pickup_radius, p_height + 0.5f, pl->pos.y + sinf(a1) * pl->pickup_radius };
            Vec3 p2 = { pl->pos.x + cosf(a2) * pl->pickup_radius, p_height + 0.5f, pl->pos.y + sinf(a2) * pl->pickup_radius };
            DrawClippedLine3D(renderer, cam, p1, p2, (SDL_Color){ 50, 255, 100, 160 }, 1);
        }
    }

    if (game->dev_menu.show_spatial_grid) {
        int sx = ((int)pl->pos.x / GRID_CELL_SIZE) * GRID_CELL_SIZE;
        int sz = ((int)pl->pos.y / GRID_CELL_SIZE) * GRID_CELL_SIZE;
        for (int x = sx - 600; x <= sx + 600; x += GRID_CELL_SIZE) {
            Vec3 g1 = { (float)x, p_height + 0.2f, (float)(sz - 600) };
            Vec3 g2 = { (float)x, p_height + 0.2f, (float)(sz + 600) };
            DrawClippedLine3D(renderer, cam, g1, g2, (SDL_Color){ 80, 120, 180, 100 }, 1);
        }
        for (int z = sz - 600; z <= sz + 600; z += GRID_CELL_SIZE) {
            Vec3 g1 = { (float)(sx - 600), p_height + 0.2f, (float)z };
            Vec3 g2 = { (float)(sx + 600), p_height + 0.2f, (float)z };
            DrawClippedLine3D(renderer, cam, g1, g2, (SDL_Color){ 80, 120, 180, 100 }, 1);
        }
    }
}
