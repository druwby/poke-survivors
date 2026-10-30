#include "game.h"
#include "pokedex.h"
#include "kanto_routes.h"
#include "image_loader.h"
#include "sprite.h"
#include "weapons.h"
#include "dev_menu.h"
#include "profiler.h"
#include "font.h"
#include "balance_sim.h"
#include "telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static SpatialGrid g_grid;

static const char* GetAssetPath(const char* subpath) {
    static char path[512];
    if (subpath[0] == '/' || strncmp(subpath, "assets/", 7) == 0) {
        return subpath;
    }
    snprintf(path, sizeof(path), "assets/%s", subpath);
    FILE* f = fopen(path, "r");
    if (f) { fclose(f); return path; }
    snprintf(path, sizeof(path), "assets/sprites/%s", subpath);
    f = fopen(path, "r");
    if (f) { fclose(f); return path; }
    return path;
}

static SDL_Texture* s_gem_textures[3] = { NULL, NULL, NULL };

static void EnsureGemTextures(SDL_Renderer* renderer) {
    if (s_gem_textures[0]) return;

    const int w = 24, h = 24;
    SDL_Color tier_colors[3] = {
        { 40, 255, 130, 255 },  // Tier 1: Radiant Emerald Green
        { 20, 220, 255, 255 },  // Tier 2: Electric Sapphire Cyan
        { 255, 215, 30, 255 }   // Tier 3: Golden Crystal
    };

    for (int t = 0; t < 3; t++) {
        SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
        if (!surf) continue;

        SDL_Color base = tier_colors[t];
        float cx = 11.5f, cy = 11.5f;
        float rx = 8.5f, ry = 10.0f;

        for (int y = 0; y < h; y++) {
            Uint32* row = (Uint32*)((Uint8*)surf->pixels + y * surf->pitch);
            for (int x = 0; x < w; x++) {
                float dx = fabsf((float)x - cx);
                float dy = (float)y - cy;

                // Diamond geometry: broad top apex, pointed bottom apex
                float norm_top = dx / rx + (dy < 0 ? (-dy / (ry * 0.85f)) : 0.0f);
                float norm_bot = dx / (rx * 0.95f) + (dy >= 0 ? (dy / ry) : 0.0f);
                float diamond_dist = (dy < 0) ? norm_top : norm_bot;

                if (diamond_dist > 1.05f) {
                    row[x] = SDL_MapRGBA(surf->format, 0, 0, 0, 0);
                } else if (diamond_dist >= 0.82f) {
                    // Dark crisp border for 100% contrast against any background
                    row[x] = SDL_MapRGBA(surf->format, 10, 15, 25, 250);
                } else {
                    // Faceted crystal body
                    Uint8 r, g, b;
                    if (dy < -2.0f && dx < 4.0f) {
                        // Top-left reflective facet gleam
                        r = (Uint8)(base.r * 0.35f + 255 * 0.65f);
                        g = (Uint8)(base.g * 0.35f + 255 * 0.65f);
                        b = (Uint8)(base.b * 0.35f + 255 * 0.65f);
                    } else if (dy < 1.0f) {
                        // Upper facet: brilliant saturated tier color
                        r = base.r;
                        g = base.g;
                        b = base.b;
                    } else {
                        // Lower shadow facet
                        r = (Uint8)(base.r * 0.60f);
                        g = (Uint8)(base.g * 0.60f);
                        b = (Uint8)(base.b * 0.60f);
                    }

                    // Bright center glint
                    if (fabsf(dx) <= 1.2f && fabsf(dy + 0.5f) <= 1.2f) {
                        r = 255; g = 255; b = 255;
                    }
                    row[x] = SDL_MapRGBA(surf->format, r, g, b, 255);
                }
            }
        }

        s_gem_textures[t] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
        if (s_gem_textures[t]) {
            SDL_SetTextureBlendMode(s_gem_textures[t], SDL_BLENDMODE_BLEND);
        }
    }
}

static void DrawFilledCircle(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)sqrtf((float)(r2 - dy * dy));
        SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void Game_ActivateEnemy(GameState* game, int e_idx) {
    if (game->enemies[e_idx].active) return;
    game->enemies[e_idx].active = true;
    game->enemies[e_idx].is_trapped = false;
    game->enemies[e_idx].trap_timer = 0.0f;
    game->enemies[e_idx].orbiter_hit_cooldown = 0.0f;
    game->active_enemy_indices[game->active_enemy_count] = e_idx;
    game->enemy_index_in_active[e_idx] = game->active_enemy_count;
    game->active_enemy_count++;
}

void Game_DeactivateEnemy(GameState* game, int e_idx) {
    if (!game->enemies[e_idx].active) return;
    game->enemies[e_idx].active = false;
    game->enemies[e_idx].is_trapped = false;
    int pos = game->enemy_index_in_active[e_idx];
    int last = game->active_enemy_count - 1;
    if (pos >= 0 && pos < game->active_enemy_count) {
        if (pos != last) {
            int last_idx = game->active_enemy_indices[last];
            game->active_enemy_indices[pos] = last_idx;
            game->enemy_index_in_active[last_idx] = pos;
        }
        game->active_enemy_count--;
    }
}

static void SpawnGem(GameState* game, Vec2 pos, int value) {
    for (int i = 0; i < MAX_GEMS; i++) {
        if (!game->gems[i].active) {
            game->gems[i].active = true;
            game->gems[i].pos = pos;
            game->gems[i].vel = (Vec2){ 0.0f, 0.0f };
            game->gems[i].value = value;
            game->gems[i].radius = 7.0f;
            game->gems[i].being_pulled = false;
            break;
        }
    }
}

float Game_ApplyEnemyDamage(GameState* game, int enemy_idx, float raw_dmg, const char* source_name) {
    if (enemy_idx < 0 || enemy_idx >= MAX_ENEMIES) return 0.0f;
    Enemy* e = &game->enemies[enemy_idx];
    if (!e->active) return 0.0f;
    if (raw_dmg <= 0.0f) return 0.0f;

    // Track combat engagement timestamp (first hit)
    if (!e->was_hit) {
        e->was_hit = true;
        e->first_hit_time = game->spawner.game_time;
    }

    float dmg = raw_dmg;
    if (game->enemy_types[e->type].move_pattern == ENEMY_MOVE_STATIONARY) {
        dmg *= 0.35f; // Harden: 65% damage reduction!
    }
    if (game->enemy_types[e->type].is_boss) {
        dmg *= 0.65f; // Boss thick armor
    }
    // Dynamic enemy defense reduction
    dmg *= (1.0f - e->defense);
    if (dmg < 1.0f) dmg = 1.0f;

    float old_hp = e->hp;
    float overkill = (dmg > old_hp) ? (dmg - old_hp) : 0.0f;
    e->hp -= dmg;
    e->flash_timer = 0.12f;

    game->total_damage_dealt += (dmg - overkill);
    game->total_overkill_damage += overkill;

    Telemetry_LogHit(game->spawner.game_time, e->uid, e->pokedex_id, dmg, (e->hp > 0.0f ? e->hp : 0.0f), source_name);

    if (e->hp <= 0.0f) {
        float combat_ttk = e->was_hit ? (game->spawner.game_time - e->first_hit_time) : 0.0f;
        if (combat_ttk < 0.0f) combat_ttk = 0.0f;
        float lifetime = game->spawner.game_time - e->spawn_time;
        if (lifetime < 0.0f) lifetime = 0.0f;

        const PokedexEntry* entry = Pokedex_GetEntry(e->pokedex_id);
        int stage = entry ? entry->stage : 1;
        if (stage == 1) {
            game->kills_stage1++;
            game->sum_ttk_stage1 += combat_ttk;
        } else if (stage == 2) {
            game->kills_stage2++;
            game->sum_ttk_stage2 += combat_ttk;
        } else {
            game->kills_stage3++;
            game->sum_ttk_stage3 += combat_ttk;
        }
        if (combat_ttk < 1.0f && !game->enemy_types[e->type].is_boss) {
            game->instant_kills++;
        }

        const RouteConfig* route = KantoRoutes_GetCurrentRoute(game);
        Telemetry_LogDeath(game->spawner.game_time, e->uid, e->pokedex_id,
                           game->enemy_types[e->type].name, stage, combat_ttk, lifetime, overkill,
                           source_name, route ? route->name : "UNKNOWN");

        game->player.kills++;
        game->route_kills++;
        SpawnGem(game, e->pos, game->enemy_types[e->type].xp_value);
        Game_DeactivateEnemy(game, enemy_idx);
    }
    return dmg;
}

bool Party_HasType(const GameState* game, PokeType type) {
    if (type == POKE_TYPE_NONE || type == POKE_TYPE_NORMAL) return true;
    for (int i = 0; i < game->party_count; i++) {
        if (!game->party[i].active) continue;
        if (game->party[i].type1 == type || game->party[i].type2 == type) {
            return true;
        }
    }
    return false;
}

void Game_GenerateLevelUpCards(GameState* game) {
    Player* p = &game->player;
    StarterID s = p->starter;

    // Collect all valid card candidates
    LevelUpCard candidates[32];
    int cand_count = 0;

    // 1. Check for Evolution Fusions (Requires both ingredient weapons in inventory at Lv 3+)
    for (int r = 0; r < g_evolution_recipe_count && cand_count < 32; r++) {
        const EvolutionRecipe* recipe = &g_evolution_recipes[r];
        int slot_a = -1;
        int slot_b = -1;
        for (int i = 0; i < p->weapon_count; i++) {
            if (p->weapons[i].type == recipe->ingredient_a && p->weapons[i].level >= 3) slot_a = i;
            if (p->weapons[i].type == recipe->ingredient_b && p->weapons[i].level >= 3) slot_b = i;
        }
        if (slot_a != -1 && slot_b != -1 && slot_a != slot_b) {
            LevelUpCard* c = &candidates[cand_count++];
            c->type = CHOICE_EVOLVE_WEAPON;
            c->weapon_type = recipe->result;
            c->slot_index = slot_a;
            c->slot_ingredient_b = slot_b;
            strncpy(c->title, recipe->name, 31);
            strncpy(c->desc, recipe->desc, 95);
            strncpy(c->tag, "FUSION", 15);
            c->color = Weapons_GetColor(recipe->result);
        }
    }

    // 2. Upgrades for currently equipped weapons (if < Lv 5 and not already evolved)
    for (int i = 0; i < p->weapon_count && cand_count < 32; i++) {
        ActiveWeaponSlot* w = &p->weapons[i];
        if (w->type == WEAPON_NONE || Weapons_IsEvolved(w->type) || w->level >= 5) continue;
        
        LevelUpCard* c = &candidates[cand_count++];
        c->type = CHOICE_UPGRADE_WEAPON;
        c->weapon_type = w->type;
        c->slot_index = i;
        c->slot_ingredient_b = -1;
        snprintf(c->title, 31, "%s Lv %d", Weapons_GetName(w->type), w->level + 1);
        snprintf(c->desc, 95, "+25%% Damage, enhanced velocity, reach, and reduced cooldown!");
        strncpy(c->tag, "UPGRADE", 15);
        c->color = Weapons_GetColor(w->type);
    }

    // 3. New Weapons from pool (Restricted strictly by elemental types present in the Player's Party!)
    if (p->weapon_count < MAX_ACTIVE_WEAPONS) {
        for (int t = WEAPON_VINE_WHIP; t <= WEAPON_ROCK_SLIDE && cand_count < 32; t++) {
            WeaponType wtype = (WeaponType)t;
            PokeType elem = Weapons_GetType(wtype);
            // CRITICAL ELEMENTAL RESTRICTION: Bulbasaur cannot roll water, Charmander cannot roll grass,
            // unless a Pokemon of that type has been caught and added to the player's team!
            if (elem != POKE_TYPE_NONE && elem != POKE_TYPE_NORMAL && !Party_HasType(game, elem)) {
                continue;
            }

            bool already_owned = false;
            for (int i = 0; i < p->weapon_count; i++) {
                if (p->weapons[i].type == wtype) { already_owned = true; break; }
            }
            if (already_owned) continue;

            LevelUpCard* c = &candidates[cand_count++];
            c->type = CHOICE_LEARN_WEAPON;
            c->weapon_type = wtype;
            c->slot_index = -1;
            c->slot_ingredient_b = -1;
            snprintf(c->title, 31, "%s", Weapons_GetName(wtype));
            strncpy(c->desc, Weapons_GetDescription(wtype), 95);
            snprintf(c->tag, 15, "[%s]", Pokedex_GetTypeName(elem));
            c->color = Weapons_GetColor(wtype);
        }
    }

    // 4. Special [Q] Unlock (if locked)
    if (!p->special_q_unlocked && cand_count < 32) {
        LevelUpCard* c = &candidates[cand_count++];
        c->type = CHOICE_LEARN_ULT_Q;
        c->weapon_type = WEAPON_NONE;
        c->slot_index = -1;
        c->slot_ingredient_b = -1;
        if (s == STARTER_BULBASAUR) {
            strncpy(c->title, "Solar Beam [Q]", 31);
            strncpy(c->desc, "UNLOCK ACTIVE [Q]: Devastating continuous solar ray vaporizing hordes!", 95);
            c->color = (SDL_Color){ 245, 225, 60, 255 };
        } else if (s == STARTER_CHARMANDER) {
            strncpy(c->title, "Fire Blast [Q]", 31);
            strncpy(c->desc, "UNLOCK ACTIVE [Q]: Iconic 5-pointed starburst blast repelling foes with fire!", 95);
            c->color = (SDL_Color){ 255, 60, 30, 255 };
        } else {
            strncpy(c->title, "Hydro Pump [Q]", 31);
            strncpy(c->desc, "UNLOCK ACTIVE [Q]: 4-jet spinning shell panic button repelling swarms!", 95);
            c->color = (SDL_Color){ 50, 220, 255, 255 };
        }
        strncpy(c->tag, "ACTIVE [Q]", 15);
    }

    // 5. Stat Boosts (Passives)
    if (cand_count < 32) {
        LevelUpCard* c = &candidates[cand_count++];
        c->type = CHOICE_STAT_HP;
        c->weapon_type = WEAPON_NONE;
        strncpy(c->title, "Primal Vitality", 31);
        strncpy(c->desc, "+25 Max HP, +0.8 HP/s Passive Regen, Full Heal, +10 Move Speed!", 95);
        strncpy(c->tag, "STAT BOOST", 15);
        c->color = (SDL_Color){ 255, 215, 70, 255 };
    }
    if (cand_count < 32) {
        LevelUpCard* c = &candidates[cand_count++];
        c->type = CHOICE_STAT_DAMAGE;
        c->weapon_type = WEAPON_NONE;
        strncpy(c->title, "Power Band", 31);
        strncpy(c->desc, "+20% Global Attack Damage across all equipped moves!", 95);
        strncpy(c->tag, "STAT BOOST", 15);
        c->color = (SDL_Color){ 255, 100, 50, 255 };
    }
    if (cand_count < 32) {
        LevelUpCard* c = &candidates[cand_count++];
        c->type = CHOICE_STAT_SPEED;
        c->weapon_type = WEAPON_NONE;
        strncpy(c->title, "Quick Claw", 31);
        strncpy(c->desc, "+15 Move Speed, +25 Pickup Radius, -12% Dash Cooldown!", 95);
        strncpy(c->tag, "STAT BOOST", 15);
        c->color = (SDL_Color){ 100, 230, 255, 255 };
    }

    // Pick 3 distinct choices:
    // If there is an evolution candidate, guarantee it in slot 0!
    int selected = 0;
    bool used[32];
    memset(used, 0, sizeof(used));

    for (int i = 0; i < cand_count; i++) {
        if (candidates[i].type == CHOICE_EVOLVE_WEAPON) {
            game->level_up_cards[selected++] = candidates[i];
            used[i] = true;
            break;
        }
    }

    // Fill remaining cards with random candidates
    while (selected < 3 && cand_count > 0) {
        int r_idx = rand() % cand_count;
        if (!used[r_idx]) {
            used[r_idx] = true;
            game->level_up_cards[selected++] = candidates[r_idx];
        } else {
            bool any_left = false;
            for (int k = 0; k < cand_count; k++) {
                if (!used[k]) { any_left = true; break; }
            }
            if (!any_left) break;
        }
    }
}

void Game_ApplyLevelUpChoice(GameState* game, int card_index) {
    if (card_index < 0 || card_index >= 3) return;
    Player* p = &game->player;
    LevelUpCard* card = &game->level_up_cards[card_index];

    switch (card->type) {
        case CHOICE_EVOLVE_WEAPON:
            Weapons_EvolveWeapon(game, card->slot_index, card->slot_ingredient_b, card->weapon_type);
            snprintf(game->route_banner_text, sizeof(game->route_banner_text), "FUSED! LEARNED %s!", Weapons_GetName(card->weapon_type));
            game->route_banner_timer = 3.5f;
            break;

        case CHOICE_UPGRADE_WEAPON:
            Weapons_UpgradeWeapon(game, card->slot_index);
            snprintf(game->route_banner_text, sizeof(game->route_banner_text), "UPGRADED: %s!", card->title);
            game->route_banner_timer = 2.5f;
            break;

        case CHOICE_LEARN_WEAPON:
            Weapons_AddWeapon(game, card->weapon_type);
            snprintf(game->route_banner_text, sizeof(game->route_banner_text), "NEW WEAPON: %s!", Weapons_GetName(card->weapon_type));
            game->route_banner_timer = 2.5f;
            break;

        case CHOICE_UPGRADE_BASIC:
            p->move_basic_level++;
            if (p->weapon_count > 0) Weapons_UpgradeWeapon(game, 0);
            break;

        case CHOICE_LEARN_MOVE_A:
            Weapons_AddWeapon(game, (p->starter == STARTER_BULBASAUR) ? WEAPON_PETAL_DANCE : ((p->starter == STARTER_CHARMANDER) ? WEAPON_FIRE_SPIN : WEAPON_AQUA_ORBS));
            break;

        case CHOICE_LEARN_MOVE_B:
            Weapons_AddWeapon(game, (p->starter == STARTER_CHARMANDER) ? WEAPON_FLAMETHROWER : WEAPON_BUBBLE_BEAM);
            break;

        case CHOICE_LEARN_MOVE_C:
            Weapons_AddWeapon(game, WEAPON_TOXIC_SPORE);
            break;

        case CHOICE_LEARN_ULT_Q:
            p->special_q_unlocked = true;
            p->special_q_cooldown = 7.0f;
            p->special_q_cooldown_timer = 0.0f;
            p->special_q_duration = 1.6f;
            p->special_q_damage = 45.0f;
            p->special_q_range = 145.0f;
            p->special_q_knockback = 520.0f;
            p->special_q_spin_speed = 14.0f;
            snprintf(game->route_banner_text, sizeof(game->route_banner_text), "UNLOCKED ACTIVE [Q] ABILITY!");
            game->route_banner_timer = 3.0f;
            break;

        case CHOICE_STAT_HP:
            p->max_hp += 25.0f;
            p->hp = p->max_hp;
            p->hp_regen += 0.8f;
            p->speed += 10.0f;
            break;

        case CHOICE_STAT_SPEED:
            p->speed += 15.0f;
            p->pickup_radius += 25.0f;
            p->dash_cooldown *= 0.88f;
            break;

        case CHOICE_STAT_DAMAGE:
            p->damage_mult += 0.20f;
            break;
    }

    game->app_state = APP_STATE_PLAYING;
    game->paused = false;
}

void Game_ThrowPokeball(GameState* game) {
    if (game->app_state != APP_STATE_PLAYING) return;
    Player* p = &game->player;
    if (p->pokeball_count <= 0) {
        snprintf(game->route_banner_text, sizeof(game->route_banner_text), "NO POKÉ BALLS! EARN MORE FROM ROUTE/GYM CLEARS!");
        game->route_banner_timer = 2.5f;
        return;
    }
    if (game->party_count >= MAX_PARTY_MEMBERS) {
        snprintf(game->route_banner_text, sizeof(game->route_banner_text), "PARTY FULL (MAX 3)! CANNOT CATCH MORE POKÉMON!");
        game->route_banner_timer = 2.5f;
        return;
    }

    // Determine throw direction towards mouse cursor or aim direction
    Vec2 aim = p->aim_dir;
    float throw_dist = 220.0f;
    if (game->mouse_active) {
        float mdx = game->mouse_pos.x - p->pos.x;
        float mdy = game->mouse_pos.y - p->pos.y;
        float mdist = sqrtf(mdx * mdx + mdy * mdy);
        if (mdist > 10.0f) {
            aim = (Vec2){ mdx / mdist, mdy / mdist };
            throw_dist = (mdist > 300.0f) ? 300.0f : mdist;
        }
    }

    // Spawn Poké Ball projectile
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* proj = &game->projectiles[i];
        if (!proj->active) {
            proj->active = true;
            proj->type = PROJ_POKEBALL;
            proj->pos = p->pos;
            proj->vel = (Vec2){ aim.x * 420.0f, aim.y * 420.0f };
            proj->radius = 9.0f;
            proj->damage = 0.0f;
            proj->lifetime = 0.0f;
            proj->max_lifetime = throw_dist / 420.0f;
            proj->pierce_left = 1;
            proj->knockback = 0.0f;
            proj->is_enemy = false;
            proj->color = (SDL_Color){ 255, 45, 45, 255 };

            p->pokeball_count--;
            snprintf(game->route_banner_text, sizeof(game->route_banner_text), "THREW POKÉ BALL!");
            game->route_banner_timer = 1.2f;
            break;
        }
    }
}

void Party_Update(GameState* game, float dt) {
    Player* p = &game->player;
    if (game->party_count <= 1) return;

    for (int i = 1; i < game->party_count; i++) {
        PartyMember* m = &game->party[i];
        if (!m->active) continue;

        // Smooth formation trailing behind player facing
        float follow_dist = 42.0f;
        float base_angle = atan2f(p->aim_dir.y, p->aim_dir.x) + (float)M_PI;
        float anchor_angle = base_angle + m->follow_angle_offset;
        Vec2 target_pos = {
            p->pos.x + cosf(anchor_angle) * follow_dist,
            p->pos.y + sinf(anchor_angle) * follow_dist
        };

        float dx = target_pos.x - m->pos.x;
        float dy = target_pos.y - m->pos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 350.0f) {
            m->pos = target_pos;
            m->vel = (Vec2){ 0.0f, 0.0f };
        } else {
            float speed_factor = 7.5f;
            m->vel.x = dx * speed_factor;
            m->vel.y = dy * speed_factor;
            float vlen = sqrtf(m->vel.x * m->vel.x + m->vel.y * m->vel.y);
            float max_v = p->speed * 1.35f;
            if (vlen > max_v && vlen > 0.001f) {
                m->vel.x = (m->vel.x / vlen) * max_v;
                m->vel.y = (m->vel.y / vlen) * max_v;
            }
            m->pos.x += m->vel.x * dt;
            m->pos.y += m->vel.y * dt;
            Tilemap_CheckCollisionCircle(&game->tilemap, &m->pos.x, &m->pos.y, p->radius * 0.85f, false);
        }

        // Animation and facing direction update
        float move_spd = sqrtf(m->vel.x * m->vel.x + m->vel.y * m->vel.y);
        m->anim.is_moving = (move_spd > 15.0f);

        // Check for nearby hostile threats
        float closest_dist = 280.0f;
        Vec2 foe_dir = { 0.0f, 0.0f };
        bool found_foe = false;
        for (int k = 0; k < game->active_enemy_count; k++) {
            int e_idx = game->active_enemy_indices[k];
            Enemy* e = &game->enemies[e_idx];
            if (!e->active || e->is_underground) continue;
            float edx = e->pos.x - m->pos.x;
            float edy = e->pos.y - m->pos.y;
            float edist = sqrtf(edx * edx + edy * edy);
            if (edist < closest_dist && edist > 0.001f) {
                closest_dist = edist;
                foe_dir = (Vec2){ edx / edist, edy / edist };
                found_foe = true;
            }
        }

        // Facing direction calculation:
        // Priority 1: If recently attacked, keep facing towards target
        bool is_attacking = (m->attack_timer > m->attack_cooldown - 0.30f);
        if (is_attacking && found_foe) {
            m->anim.direction = Sprite_VecToDirection(foe_dir.x, foe_dir.y, m->anim.direction);
        } else if (m->anim.is_moving && (fabsf(m->vel.x) > 5.0f || fabsf(m->vel.y) > 5.0f)) {
            // Priority 2: Moving companions face movement velocity
            m->anim.direction = Sprite_VecToDirection(m->vel.x, m->vel.y, m->anim.direction);
        } else if (found_foe) {
            // Priority 3: Stationary companions face closest threat
            m->anim.direction = Sprite_VecToDirection(foe_dir.x, foe_dir.y, m->anim.direction);
        } else {
            // Priority 4: Idle companions: face toward player if far, or match player facing in formation
            float to_px = p->pos.x - m->pos.x;
            float to_py = p->pos.y - m->pos.y;
            float p_dist = sqrtf(to_px * to_px + to_py * to_py);
            if (p_dist > 55.0f) {
                m->anim.direction = Sprite_VecToDirection(to_px, to_py, m->anim.direction);
            } else {
                m->anim.direction = p->anim.direction;
            }
        }

        // Ensure safe 0..7 direction
        m->anim.direction = ((m->anim.direction % 8) + 8) % 8;

        // Advance animation frame using Pokemon's actual sprite sheet
        Sprite_UpdateAnim(&m->anim, &m->sprite, dt, m->anim.is_moving);

        // Autonomous companion basic attacks every ~2.5s
        m->attack_timer -= dt;
        if (m->attack_timer <= 0.0f) {
            if (found_foe) {
                m->attack_timer = m->attack_cooldown;
                m->anim.direction = Sprite_VecToDirection(foe_dir.x, foe_dir.y, m->anim.direction);
                ProjectileType ptype = PROJ_SWIFT_STAR;
                SDL_Color pcol = { 255, 230, 80, 255 };
                float pspd = 280.0f;
                float pdmg = 15.0f * p->damage_mult;
                int ppierce = 1;

                if (m->type1 == POKE_TYPE_WATER || m->type2 == POKE_TYPE_WATER) {
                    ptype = PROJ_BUBBLE;
                    pcol = (SDL_Color){ 80, 210, 255, 255 };
                    pspd = 220.0f;
                } else if (m->type1 == POKE_TYPE_FIRE || m->type2 == POKE_TYPE_FIRE) {
                    ptype = PROJ_EMBER;
                    pcol = (SDL_Color){ 255, 120, 30, 255 };
                    pspd = 300.0f;
                } else if (m->type1 == POKE_TYPE_GRASS || m->type2 == POKE_TYPE_GRASS) {
                    ptype = PROJ_RAZOR_LEAF;
                    pcol = (SDL_Color){ 100, 235, 70, 255 };
                    pspd = 320.0f;
                    ppierce = 2;
                } else if (m->type1 == POKE_TYPE_ELECTRIC || m->type2 == POKE_TYPE_ELECTRIC) {
                    ptype = PROJ_LIGHTNING_BOLT;
                    pcol = (SDL_Color){ 255, 240, 80, 255 };
                    pdmg = 22.0f * p->damage_mult;
                } else if (m->type1 == POKE_TYPE_ROCK || m->type2 == POKE_TYPE_ROCK ||
                           m->type1 == POKE_TYPE_GROUND || m->type2 == POKE_TYPE_GROUND) {
                    ptype = PROJ_ROCK_SLIDE;
                    pcol = (SDL_Color){ 175, 145, 95, 255 };
                    pdmg = 20.0f * p->damage_mult;
                }

                for (int j = 0; j < MAX_PROJECTILES; j++) {
                    Projectile* proj = &game->projectiles[j];
                    if (!proj->active) {
                        proj->active = true;
                        proj->type = ptype;
                        proj->pos = m->pos;
                        proj->vel = (Vec2){ foe_dir.x * pspd, foe_dir.y * pspd };
                        proj->radius = 12.0f;
                        proj->damage = pdmg;
                        proj->lifetime = 0.0f;
                        proj->max_lifetime = 1.2f;
                        proj->pierce_left = ppierce;
                        proj->knockback = 70.0f;
                        proj->is_enemy = false;
                        proj->color = pcol;
                        break;
                    }
                }
            } else {
                m->attack_timer = 0.4f;
            }
        }
    }
}

void Party_Draw(GameState* game) {
    SDL_Renderer* renderer = game->renderer;
    Vec2 camera = game->camera;
    float time_sec = game->spawner.game_time;
    Player* p = &game->player;

    for (int i = 1; i < game->party_count; i++) {
        PartyMember* m = &game->party[i];
        if (!m->active) continue;

        // Ensure companions calculate facing angle/direction based on movement velocity or towards nearby targets/player
        if (m->anim.is_moving && (fabsf(m->vel.x) > 5.0f || fabsf(m->vel.y) > 5.0f)) {
            m->anim.direction = Sprite_VecToDirection(m->vel.x, m->vel.y, m->anim.direction);
        } else {
            float closest_dist = 280.0f;
            Vec2 foe_dir = { 0.0f, 0.0f };
            bool found_foe = false;
            for (int k = 0; k < game->active_enemy_count; k++) {
                int e_idx = game->active_enemy_indices[k];
                Enemy* e = &game->enemies[e_idx];
                if (!e->active || e->is_underground) continue;
                float edx = e->pos.x - m->pos.x;
                float edy = e->pos.y - m->pos.y;
                float edist = sqrtf(edx * edx + edy * edy);
                if (edist < closest_dist && edist > 0.001f) {
                    closest_dist = edist;
                    foe_dir = (Vec2){ edx / edist, edy / edist };
                    found_foe = true;
                }
            }
            if (found_foe) {
                m->anim.direction = Sprite_VecToDirection(foe_dir.x, foe_dir.y, m->anim.direction);
            } else {
                float to_px = p->pos.x - m->pos.x;
                float to_py = p->pos.y - m->pos.y;
                float p_dist = sqrtf(to_px * to_px + to_py * to_py);
                if (p_dist > 55.0f) {
                    m->anim.direction = Sprite_VecToDirection(to_px, to_py, m->anim.direction);
                } else {
                    m->anim.direction = p->anim.direction;
                }
            }
        }
        m->anim.direction = ((m->anim.direction % 8) + 8) % 8;

        int screen_x = (int)(m->pos.x - camera.x);
        int screen_y = (int)(m->pos.y - camera.y);

        // 1. Friendly pulsating green aura beneath feet
        float pulse = 0.8f + 0.2f * sinf(time_sec * 5.0f + (float)i * 1.5f);
        int aura_r = (int)(20.0f * pulse);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 50, 240, 110, (Uint8)(100 * pulse));
        for (int r = aura_r - 2; r <= aura_r; r++) {
            for (int a = 0; a < 24; a++) {
                float a1 = (2.0f * (float)M_PI * a) / 24.0f;
                float a2 = (2.0f * (float)M_PI * (a + 1)) / 24.0f;
                SDL_RenderDrawLine(renderer, screen_x + (int)(cosf(a1)*r), screen_y + (int)(sinf(a1)*r),
                                             screen_x + (int)(cosf(a2)*r), screen_y + (int)(sinf(a2)*r));
            }
        }

        // 2. Green Outline Silhouette: draw shifted by 1.5px with green color mod!
        if (m->sprite.loaded && m->sprite.texture) {
            SDL_SetTextureColorMod(m->sprite.texture, 40, 255, 90);
            Vec2 offsets[4] = {
                { m->pos.x - 1.5f, m->pos.y },
                { m->pos.x + 1.5f, m->pos.y },
                { m->pos.x, m->pos.y - 1.5f },
                { m->pos.x, m->pos.y + 1.5f }
            };
            for (int o = 0; o < 4; o++) {
                Sprite_Draw(renderer, &m->sprite, &m->anim, offsets[o], camera, (SDL_Color){ 40, 255, 90, 255 }, 16.0f, false);
            }
            // Draw regular sprite on top
            SDL_SetTextureColorMod(m->sprite.texture, 255, 255, 255);
            Sprite_Draw(renderer, &m->sprite, &m->anim, m->pos, camera, (SDL_Color){ 255, 255, 255, 255 }, 16.0f, false);
        } else {
            Sprite_DrawProceduralFallback(renderer, m->pos, camera, 16.0f, m->anim.direction, (SDL_Color){ 50, 240, 110, 255 }, false);
        }

        // 3. Ally Name Tag Above Head
        char tag_str[48];
        snprintf(tag_str, sizeof(tag_str), "%s [%s]", m->name, Pokedex_GetTypeName(m->type1));
        int tag_w = (int)strlen(tag_str) * 6;
        Font_DrawTextShadow(renderer, game->font_texture, tag_str, screen_x - tag_w / 2, screen_y - 28, 1, (SDL_Color){ 100, 255, 150, 255 });
    }
}

void Game_SelectStarter(GameState* game, StarterID starter) {
    Player* p = &game->player;
    p->starter = starter;
    p->pos.x = WORLD_SIZE_X / 2.0f;
    p->pos.y = WORLD_SIZE_Y / 2.0f;
    p->invuln_timer = 0.0f;
    p->invuln_duration = 0.40f;
    p->damage_flash_timer = 0.0f;
    p->damage_mult = 1.0f * g_balance_config.weapon_damage_mult;
    p->god_mode = false;
    p->level = 1;
    p->xp = 0;
    p->xp_to_next_level = 10;
    p->kills = 0;
    p->state = PLAYER_STATE_NORMAL;
    p->anim.direction = 0;
    p->anim.current_frame = 0;
    p->anim.anim_timer = 0.0f;
    p->anim.is_moving = false;

    // Dash Ability is available by default (tactical dodge roll)
    p->dash_unlocked = true;
    p->dash_cooldown = 1.0f;
    p->dash_cooldown_timer = 0.0f;
    p->dash_duration = 0.22f;
    p->dash_timer = 0.0f;
    p->dash_speed = 380.0f;
    p->dash_dir = (Vec2){ 0.0f, 1.0f };

    // Active Q Ability starts locked (learned upon progression!)
    p->special_q_unlocked = false;
    p->special_q_cooldown = 7.0f;
    p->special_q_cooldown_timer = 0.0f;
    p->special_q_duration = 1.6f;
    p->special_q_damage = 45.0f;
    p->special_q_range = 145.0f;
    p->special_q_knockback = 520.0f;
    p->special_q_spin_speed = 14.0f;

    // Load Starter-Specific Sprites & Base Attributes (Tuned for deliberate, readable movement)
    if (starter == STARTER_BULBASAUR) {
        p->speed = 135.0f;
        p->max_hp = 125.0f;
        p->hp = 125.0f;
        p->hp_regen = 1.2f;
        p->radius = 16.0f;
        p->pickup_radius = 120.0f;
        p->sprite = Pokedex_LoadSprite(game->renderer, 1, 1.8f);
        p->sprite_charge = Sprite_Load(game->renderer, "assets/sprites/sprite/0001/Charge-Anim.png", 32, 40, 10, 8, 0.035f, 1.8f);
        p->sprite_special = Sprite_Load(game->renderer, "assets/sprites/sprite/0001/Attack-Anim.png", 64, 72, 11, 8, 0.045f, 1.8f);
    } else if (starter == STARTER_CHARMANDER) {
        p->speed = 145.0f;
        p->max_hp = 115.0f;
        p->hp = 115.0f;
        p->hp_regen = 0.8f;
        p->radius = 15.0f;
        p->pickup_radius = 120.0f;
        p->sprite = Pokedex_LoadSprite(game->renderer, 4, 1.8f);
        p->sprite_charge = Sprite_Load(game->renderer, "assets/sprites/sprite/0004/Charge-Anim.png", 32, 40, 10, 8, 0.035f, 1.8f);
        p->sprite_special = Sprite_Load(game->renderer, "assets/sprites/sprite/0004/Attack-Anim.png", 64, 64, 11, 8, 0.045f, 1.8f);
    } else { // STARTER_SQUIRTLE
        p->speed = 138.0f;
        p->max_hp = 130.0f;
        p->hp = 130.0f;
        p->hp_regen = 1.0f;
        p->radius = 16.0f;
        p->pickup_radius = 120.0f;
        p->sprite = Pokedex_LoadSprite(game->renderer, 7, 1.8f);
        p->sprite_charge = Sprite_Load(game->renderer, "assets/sprites/sprite/0007/Charge-Anim.png", 32, 32, 10, 8, 0.035f, 1.8f);
        p->sprite_special = Sprite_Load(game->renderer, "assets/sprites/sprite/0007/Withdraw-Anim.png", 32, 16, 7, 8, 0.06f, 1.8f);
    }

    // Reset all combat arrays
    for (int i = 0; i < MAX_PROJECTILES; i++) game->projectiles[i].active = false;
    for (int i = 0; i < MAX_GROUND_HAZARDS; i++) game->ground_hazards[i].active = false;
    for (int i = 0; i < MAX_VINE_STRIKES; i++) game->vine_strikes[i].active = false;
    for (int i = 0; i < MAX_GEMS; i++) game->gems[i].active = false;
    // Reset telemetry metrics
    game->total_damage_dealt = 0.0f;
    game->total_overkill_damage = 0.0f;
    game->danger_time_sec = 0.0f;
    game->prev_player_hp = p->hp;
    game->hp_volatility_accum = 0.0f;
    game->hp_samples_count = 0;
    game->instant_kills = 0;
    game->kills_stage1 = 0;
    game->kills_stage2 = 0;
    game->kills_stage3 = 0;
    game->sum_ttk_stage1 = 0.0f;
    game->sum_ttk_stage2 = 0.0f;
    game->sum_ttk_stage3 = 0.0f;
    game->telemetry_timer = 0.0f;

    // Poké Ball Inventory: Starts with 1 Poké Ball to make smart team-building decisions
    p->pokeball_count = 1;

    // Initialize Party Member 0 (Starter)
    game->party_count = 1;
    memset(game->party, 0, sizeof(game->party));
    PartyMember* sm = &game->party[0];
    sm->active = true;
    int pid = (starter == STARTER_BULBASAUR) ? 1 : ((starter == STARTER_CHARMANDER) ? 4 : 7);
    sm->pokedex_id = pid;
    const PokedexEntry* s_entry = Pokedex_GetEntry(pid);
    if (s_entry) {
        strncpy(sm->name, s_entry->name, sizeof(sm->name) - 1);
        sm->type1 = s_entry->type1;
        sm->type2 = s_entry->type2;
        sm->sprite_scale = s_entry->scale * 1.1f;
    }
    sm->pos = p->pos;
    sm->vel = (Vec2){ 0.0f, 0.0f };
    sm->sprite = p->sprite;
    sm->anim = p->anim;

    // Configure strictly ONE basic attack!
    Weapons_InitForStarter(game, starter);

    // Initialize Route 1 (Section 0, Route 0)
    KantoRoutes_Init(game);

    game->app_state = APP_STATE_PLAYING;
    game->paused = false;
    if (!g_headless_sim) printf("[Game] Selected starter: %d. Route 1 initialized!\n", starter);
}

void Game_Init(GameState* game, SDL_Window* window, SDL_Renderer* renderer) {
    memset(game, 0, sizeof(GameState));
    game->window = window;
    game->renderer = renderer;
    game->running = true;
    game->paused = false;
    
    // Create Font Atlas
    game->font_texture = Font_CreateAtlas(renderer);

    // Initialize Pokédex system
    Pokedex_Init(renderer);
    
    // Camera center
    game->camera.x = (WORLD_SIZE_X - WINDOW_WIDTH) / 2.0f;
    game->camera.y = (WORLD_SIZE_Y - WINDOW_HEIGHT) / 2.0f;
    
    // Pre-load starter portraits & walk sprites for selection menu
    game->starter_portraits[0] = Pokedex_LoadPortrait(renderer, 1);
    game->starter_portraits[1] = Pokedex_LoadPortrait(renderer, 4);
    game->starter_portraits[2] = Pokedex_LoadPortrait(renderer, 7);

    game->starter_sprites[0] = Pokedex_LoadSprite(renderer, 1, 2.2f);
    game->starter_sprites[1] = Pokedex_LoadSprite(renderer, 4, 2.2f);
    game->starter_sprites[2] = Pokedex_LoadSprite(renderer, 7, 2.2f);

    // Start in Starter Select state
    game->app_state = APP_STATE_STARTER_SELECT;
    game->selected_starter_hover = STARTER_NONE;
    game->selected_card_hover = -1;

    // Initialize Rich Environment Tilemap & Load Tileset
    Tilemap_Init(&game->tilemap);
    Tilemap_GenerateRouteMap(&game->tilemap, 0, 0);
    game->tileset_texture = Image_LoadTexture(renderer, GetAssetPath("tilesets/tileset1.png"));
    game->floor_cache_texture = NULL;

    // Initialize Weapons & Spawner
    Weapons_Init(&game->weapons);

    game->spawner.spawn_interval = 1.4f;
    game->spawner.spawn_timer = 0.0f;
    game->spawner.max_enemies = 35;
    game->spawner.batch_size = 1;
    game->spawner.game_time = 0.0f;
    game->spawner.difficulty_mult = 1.0f;
    game->spawner.separation_enabled = true;
    game->spawner.separation_strength = 85.0f;

    // Initialize Spatial Grid
    Grid_Init(&g_grid);

    // Initialize Dev Menu & Profiler
    DevMenu_Init(&game->dev_menu);
    Profiler_Init(&game->profiler);
    
    game->mouse_active = false;
    game->mouse_pos = (Vec2){ 0.0f, 0.0f };

    if (!g_headless_sim) printf("[Game] Initialized Kanto Roguelike Engine.\n");
}

void Game_HandleEvent(GameState* game, const SDL_Event* event) {
    if (game->app_state == APP_STATE_PLAYING) {
        if (event->type == SDL_MOUSEMOTION) {
            game->mouse_active = true;
            game->mouse_pos = (Vec2){ (float)event->motion.x + game->camera.x,
                                      (float)event->motion.y + game->camera.y };
        } else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_RIGHT) {
            Game_ThrowPokeball(game);
        } else if (event->type == SDL_KEYDOWN) {
            if (event->key.keysym.sym == SDLK_e || event->key.keysym.sym == SDLK_c) {
                Game_ThrowPokeball(game);
            }
        }
    } else if (game->app_state == APP_STATE_STARTER_SELECT) {
        if (event->type == SDL_MOUSEMOTION) {
            int mx = event->motion.x;
            int my = event->motion.y;
            if (my >= 130 && my <= 620) {
                if (mx >= 100 && mx <= 420) game->selected_starter_hover = STARTER_BULBASAUR;
                else if (mx >= 460 && mx <= 780) game->selected_starter_hover = STARTER_CHARMANDER;
                else if (mx >= 820 && mx <= 1140) game->selected_starter_hover = STARTER_SQUIRTLE;
                else game->selected_starter_hover = STARTER_NONE;
            } else {
                game->selected_starter_hover = STARTER_NONE;
            }
        } else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
            if (game->selected_starter_hover != STARTER_NONE) {
                Game_SelectStarter(game, game->selected_starter_hover);
            }
        } else if (event->type == SDL_KEYDOWN) {
            if (event->key.keysym.sym == SDLK_1) Game_SelectStarter(game, STARTER_BULBASAUR);
            else if (event->key.keysym.sym == SDLK_2) Game_SelectStarter(game, STARTER_CHARMANDER);
            else if (event->key.keysym.sym == SDLK_3) Game_SelectStarter(game, STARTER_SQUIRTLE);
        }
    } else if (game->app_state == APP_STATE_LEVEL_UP) {
        if (event->type == SDL_MOUSEMOTION) {
            int mx = event->motion.x;
            int my = event->motion.y;
            game->selected_card_hover = -1;
            if (my >= 220 && my <= 520) {
                if (mx >= 160 && mx <= 460) game->selected_card_hover = 0;
                else if (mx >= 490 && mx <= 790) game->selected_card_hover = 1;
                else if (mx >= 820 && mx <= 1120) game->selected_card_hover = 2;
            }
        } else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
            if (game->selected_card_hover >= 0) {
                Game_ApplyLevelUpChoice(game, game->selected_card_hover);
            }
        } else if (event->type == SDL_KEYDOWN) {
            if (event->key.keysym.sym == SDLK_1) Game_ApplyLevelUpChoice(game, 0);
            else if (event->key.keysym.sym == SDLK_2) Game_ApplyLevelUpChoice(game, 1);
            else if (event->key.keysym.sym == SDLK_3) Game_ApplyLevelUpChoice(game, 2);
        }
    } else if (game->app_state == APP_STATE_GAME_OVER || game->app_state == APP_STATE_GAME_VICTORY) {
        if (event->type == SDL_KEYDOWN) {
            if (event->key.keysym.sym == SDLK_r || event->key.keysym.sym == SDLK_SPACE) {
                game->app_state = APP_STATE_STARTER_SELECT;
                game->selected_starter_hover = STARTER_NONE;
            }
        }
    }
}

void Game_HandleInput(GameState* game, const Uint8* keystate, float dt) {
    if (game->app_state != APP_STATE_PLAYING) return;
    
    uint64_t t0 = Profiler_GetTimestamp();
    Player* p = &game->player;
    
    float vx = 0.0f;
    float vy = 0.0f;
    
    if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP])    vy -= 1.0f;
    if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN])  vy += 1.0f;
    if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT])  vx -= 1.0f;
    if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) vx += 1.0f;
    
    float len = sqrtf(vx * vx + vy * vy);
    bool is_moving = (len > 0.01f);
    
    if (is_moving) {
        vx = (vx / len) * p->speed;
        vy = (vy / len) * p->speed;
        if (p->state == PLAYER_STATE_NORMAL) {
            p->anim.direction = Sprite_VecToDirection(vx, vy, p->anim.direction);
        }
    }
    
    // Check Ability 1: DASH (SHIFT)
    bool shift_pressed = keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT];
    if (p->dash_unlocked && shift_pressed && p->dash_cooldown_timer <= 0.0f && p->state == PLAYER_STATE_NORMAL) {
        p->state = PLAYER_STATE_DASHING;
        p->dash_timer = p->dash_duration;
        p->dash_cooldown_timer = p->dash_cooldown;
        if (is_moving) {
            p->dash_dir.x = vx / p->speed;
            p->dash_dir.y = vy / p->speed;
        } else {
            p->dash_dir = Sprite_DirectionToVec(p->anim.direction);
        }
        p->anim.direction = Sprite_VecToDirection(p->dash_dir.x, p->dash_dir.y, p->anim.direction);
        p->anim.current_frame = 0;
        p->anim.anim_timer = 0.0f;
    }
    
    // Check Ability 2: SPECIAL [Q]
    bool q_pressed = keystate[SDL_SCANCODE_Q];
    if (p->special_q_unlocked && q_pressed && p->special_q_cooldown_timer <= 0.0f && p->state == PLAYER_STATE_NORMAL) {
        if (p->starter == STARTER_BULBASAUR) p->state = PLAYER_STATE_SOLAR_BEAM;
        else if (p->starter == STARTER_CHARMANDER) p->state = PLAYER_STATE_FLAMETHROWER;
        else p->state = PLAYER_STATE_HYDRO_PUMP;

        p->special_q_timer = p->special_q_duration;
        p->special_q_cooldown_timer = p->special_q_cooldown;
        p->special_q_current_angle = 0.0f;
        p->special_q_tick_timer = 0.0f;
        p->anim.current_frame = 0;
        p->anim.anim_timer = 0.0f;
        
        // Immediate Defensive Blastwave on activation
        int q_buf[256];
        int found = Grid_QueryCircle(&g_grid, p->pos.x, p->pos.y, p->special_q_range + 20.0f, q_buf, 256);
        for (int k = 0; k < found; k++) {
            int e_idx = q_buf[k];
            Enemy* e = &game->enemies[e_idx];
            if (!e->active || e->is_underground) continue;
            
            float edx = e->pos.x - p->pos.x;
            float edy = e->pos.y - p->pos.y;
            float dist = sqrtf(edx * edx + edy * edy);
            if (dist <= p->special_q_range + 20.0f) {
                float dir_x = (dist > 0.001f) ? (edx / dist) : 1.0f;
                float dir_y = (dist > 0.001f) ? (edy / dist) : 0.0f;
                
                float resist = game->enemy_types[e->type].knockback_resist;
                float blast_force = p->special_q_knockback * 1.5f * (1.0f - resist * 0.40f);
                
                e->knockback_x = dir_x * blast_force;
                e->knockback_y = dir_y * blast_force;
                
                float shove = 45.0f * (1.0f - resist * 0.50f);
                e->pos.x += dir_x * shove;
                e->pos.y += dir_y * shove;
                
                float burst_dmg = (p->special_q_damage * p->damage_mult) * 0.35f;
                float actual = Game_ApplyEnemyDamage(game, e_idx, burst_dmg, "SPECIAL_Q_BURST");
                Weapons_SpawnDamageText(game, e->pos, actual, (SDL_Color){ 80, 240, 255, 255 });
            }
        }
    }
    
    if (p->state == PLAYER_STATE_DASHING) {
        p->vel.x = p->dash_dir.x * p->dash_speed;
        p->vel.y = p->dash_dir.y * p->dash_speed;
        p->anim.direction = Sprite_VecToDirection(p->dash_dir.x, p->dash_dir.y, p->anim.direction);
        if (p->sprite_charge.num_frames > 0 && p->dash_duration > 0.0001f) {
            float progress = 1.0f - (p->dash_timer / p->dash_duration);
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 0.999f) progress = 0.999f;
            p->anim.current_frame = (int)(progress * (float)p->sprite_charge.num_frames);
        }
    } else if (p->state == PLAYER_STATE_HYDRO_PUMP || p->state == PLAYER_STATE_SOLAR_BEAM || p->state == PLAYER_STATE_FLAMETHROWER) {
        p->vel.x = vx * 0.40f;
        p->vel.y = vy * 0.40f;
        Sprite_UpdateAnim(&p->anim, &p->sprite_special, dt, true);
    } else {
        p->vel.x = vx;
        p->vel.y = vy;
        Sprite_UpdateAnim(&p->anim, &p->sprite, dt, is_moving);
    }
    
    game->profiler.t_input = Profiler_ElapsedMs(t0);
}

void Game_Update(GameState* game, float dt) {
    if (game->paused) return;

    if (game->app_state == APP_STATE_STARTER_SELECT) {
        // Animate starter showcase walk sprites
        for (int i = 0; i < 3; i++) {
            Sprite_UpdateAnim(&game->player.anim, &game->starter_sprites[i], dt, true);
        }
        return;
    }

    if (game->app_state != APP_STATE_PLAYING) return;
    
    game->spawner.game_time += dt;
    
    // -------------------------------------------------------------
    // PHASE 1: ROUTE PROGRESSION & SPAWNING
    // -------------------------------------------------------------
    KantoRoutes_Update(game, dt);
    KantoRoutes_SpawnEnemies(game, dt);

    // -------------------------------------------------------------
    // PHASE 2: PLAYER UPDATE & ACTIVE ABILITIES
    // -------------------------------------------------------------
    uint64_t t_p_start = Profiler_GetTimestamp();
    Player* p = &game->player;
    
    p->pos.x += p->vel.x * dt;
    p->pos.y += p->vel.y * dt;
    
    // Resolve collision against solid rock cliffs and water bodies
    Tilemap_CheckCollisionCircle(&game->tilemap, &p->pos.x, &p->pos.y, p->radius, false);
    
    if (p->pos.x < p->radius) p->pos.x = p->radius;
    if (p->pos.x > WORLD_SIZE_X - p->radius) p->pos.x = WORLD_SIZE_X - p->radius;
    if (p->pos.y < p->radius) p->pos.y = p->radius;
    if (p->pos.y > WORLD_SIZE_Y - p->radius) p->pos.y = WORLD_SIZE_Y - p->radius;
    
    if (p->hp < p->max_hp) {
        p->hp += p->hp_regen * dt;
        if (p->hp > p->max_hp) p->hp = p->max_hp;
    }
    
    // Track Big Data Player Telemetry: Danger time (< 40% HP) and HP volatility
    if (p->hp > 0.0f) {
        if (p->hp < p->max_hp * 0.40f) {
            game->danger_time_sec += dt;
        }
        float dhp = fabsf(p->hp - game->prev_player_hp);
        game->hp_volatility_accum += dhp;
        game->hp_samples_count++;
        game->prev_player_hp = p->hp;

        game->telemetry_timer += dt;
        if (game->telemetry_timer >= 5.0f) {
            game->telemetry_timer = 0.0f;
            float d_pct = (game->danger_time_sec / (game->spawner.game_time > 0.001f ? game->spawner.game_time : 1.0f)) * 100.0f;
            Telemetry_LogPlayerStat(game->spawner.game_time, p->hp, p->max_hp, d_pct, p->level, p->kills, "player");
        }
    }
    
    if (p->invuln_timer > 0.0f) p->invuln_timer -= dt;
    if (p->damage_flash_timer > 0.0f) p->damage_flash_timer -= dt;
    if (p->dash_cooldown_timer > 0.0f) p->dash_cooldown_timer -= dt;
    if (p->special_q_cooldown_timer > 0.0f) p->special_q_cooldown_timer -= dt;
    
    if (p->state == PLAYER_STATE_DASHING) {
        p->dash_timer -= dt;
        if (p->dash_timer <= 0.0f) {
            p->state = PLAYER_STATE_NORMAL;
            p->anim.current_frame = 0;
            p->anim.anim_timer = 0.0f;
        } else {
            p->anim.direction = Sprite_VecToDirection(p->dash_dir.x, p->dash_dir.y, p->anim.direction);
            if (p->sprite_charge.num_frames > 0 && p->dash_duration > 0.0001f) {
                float progress = 1.0f - (p->dash_timer / p->dash_duration);
                if (progress < 0.0f) progress = 0.0f;
                if (progress > 0.999f) progress = 0.999f;
                p->anim.current_frame = (int)(progress * (float)p->sprite_charge.num_frames);
            }
        }
    } else if (p->state == PLAYER_STATE_HYDRO_PUMP || p->state == PLAYER_STATE_SOLAR_BEAM || p->state == PLAYER_STATE_FLAMETHROWER) {
        p->special_q_timer -= dt;
        p->special_q_current_angle += p->special_q_spin_speed * dt;
        p->special_q_tick_timer += dt;
        
        if (p->special_q_tick_timer >= 0.07f) {
            p->special_q_tick_timer = 0.0f;
            
            for (int k = 0; k < game->active_enemy_count; k++) {
                int e_idx = game->active_enemy_indices[k];
                Enemy* e = &game->enemies[e_idx];
                if (!e->active || e->is_underground) continue;
                
                float dx = e->pos.x - p->pos.x;
                float dy = e->pos.y - p->pos.y;
                float dist_sq = dx * dx + dy * dy;
                if (dist_sq <= p->special_q_range * p->special_q_range && dist_sq > 0.001f) {
                    float dist = sqrtf(dist_sq);
                    float dmg = (p->special_q_damage * p->damage_mult) * (0.07f / 0.5f);
                    float actual = Game_ApplyEnemyDamage(game, e_idx, dmg, "SPECIAL_Q");
                    
                    float resist = game->enemy_types[e->type].knockback_resist;
                    float push = p->special_q_knockback * (1.0f - resist * 0.45f);
                    float dir_x = dx / dist;
                    float dir_y = dy / dist;
                    
                    e->knockback_x = dir_x * push;
                    e->knockback_y = dir_y * push;
                    
                    float safe_dist = p->radius + e->radius + 18.0f;
                    if (dist < safe_dist) {
                        float pushout = safe_dist - dist;
                        e->pos.x += dir_x * pushout;
                        e->pos.y += dir_y * pushout;
                    }
                    
                    Weapons_SpawnDamageText(game, e->pos, actual, (SDL_Color){ 0, 220, 255, 255 });
                    if (!e->active) {
                        k--;
                    }
                }
            }
        }
        
        if (p->special_q_timer <= 0.0f) {
            p->state = PLAYER_STATE_NORMAL;
        }
    }
    
    game->profiler.t_player = Profiler_ElapsedMs(t_p_start);

    // Update Party Companions (follower physics & autonomous attacks)
    Party_Update(game, dt);

    // -------------------------------------------------------------
    // PHASE 3: WEAPONS UPDATE
    // -------------------------------------------------------------
    uint64_t t_w_start = Profiler_GetTimestamp();
    Weapons_Update(game, dt);
    game->profiler.t_weapons = Profiler_ElapsedMs(t_w_start);

    // -------------------------------------------------------------
    // PHASE 4: ENEMY AI & BULLET-HELL FIRING
    // -------------------------------------------------------------
    uint64_t t_e_start = Profiler_GetTimestamp();
    Tilemap_UpdateFlowField(&game->tilemap, p->pos.x, p->pos.y);
    
    for (int k = 0; k < game->active_enemy_count; k++) {
        int i = game->active_enemy_indices[k];
        Enemy* e = &game->enemies[i];
        EnemyTypeConfig* cfg = &game->enemy_types[e->type];
        
        if (e->attack_cooldown_timer > 0.0f) {
            e->attack_cooldown_timer -= dt;
        }

        if (e->is_trapped) {
            e->trap_timer += dt;
            e->pos.x += e->trap_drift_vel.x * dt;
            e->pos.y += e->trap_drift_vel.y * dt + sinf(e->trap_timer * 8.0f) * 0.5f;
            e->trap_drift_vel.x *= (1.0f - 2.5f * dt);
            e->trap_drift_vel.y *= (1.0f - 1.5f * dt);

            if (e->trap_timer >= e->trap_duration) {
                e->is_trapped = false;
                // Pop burst damage + radial splash (tuned for balanced combat pacing)
                float pop_dmg = (16.0f + (float)p->move_basic_level * 4.0f) * p->damage_mult;

                // Radial splash damage to nearby enemies
                float splash_r = 75.0f;
                for (int sn = 0; sn < game->active_enemy_count; sn++) {
                    int se_idx = game->active_enemy_indices[sn];
                    if (se_idx == i) continue;
                    Enemy* se = &game->enemies[se_idx];
                    if (!se->active || se->is_underground) continue;
                    float sdx = se->pos.x - e->pos.x;
                    float sdy = se->pos.y - e->pos.y;
                    float sdist_sq = sdx * sdx + sdy * sdy;
                    if (sdist_sq <= splash_r * splash_r) {
                        float sdist = sqrtf(sdist_sq);
                        if (sdist > 0.001f) {
                            se->knockback_x += (sdx / sdist) * 160.0f;
                            se->knockback_y += (sdy / sdist) * 160.0f;
                        }
                        int prev_active = game->active_enemy_count;
                        int se_pos = game->enemy_index_in_active[se_idx];
                        float actual_splash = Game_ApplyEnemyDamage(game, se_idx, pop_dmg * 0.45f, "SCALD_BUBBLE_SPLASH");
                        Weapons_SpawnDamageText(game, se->pos, actual_splash, (SDL_Color){ 120, 210, 255, 255 });
                        if (game->active_enemy_count < prev_active) {
                            if (se_pos <= k) k--;
                            if (se_pos < sn) sn--;
                        }
                    }
                }

                float actual_pop = Game_ApplyEnemyDamage(game, i, pop_dmg, "SCALD_BUBBLE_POP");
                Weapons_SpawnDamageText(game, e->pos, actual_pop, (SDL_Color){ 90, 230, 255, 255 });
                if (!e->active) {
                    k--;
                    continue;
                }
            }

            // Knockback decay
            e->pos.x += e->knockback_x * dt;
            e->pos.y += e->knockback_y * dt;
            float decay = 10.0f * dt;
            if (decay > 1.0f) decay = 1.0f;
            e->knockback_x -= e->knockback_x * decay;
            e->knockback_y -= e->knockback_y * decay;
            if (e->flash_timer > 0.0f) e->flash_timer -= dt;
            if (e->orbiter_hit_cooldown > 0.0f) e->orbiter_hit_cooldown -= dt;
            continue;
        }
        
        float dx = p->pos.x - e->pos.x;
        float dy = p->pos.y - e->pos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        
        float dir_x = 0.0f;
        float dir_y = 0.0f;
        if (dist > 0.001f) {
            dir_x = dx / dist;
            dir_y = dy / dist;
        }

        bool can_fly = (cfg->move_pattern == ENEMY_MOVE_FLANK_ORBIT || cfg->attack_type == ENEMY_ATTACK_RANGED_GUST);
        float move_dir_x = dir_x;
        float move_dir_y = dir_y;
        bool has_los = true;
        if (!can_fly) {
            has_los = Tilemap_GetPathDirection(&game->tilemap, e->pos.x, e->pos.y, p->pos.x, p->pos.y, &move_dir_x, &move_dir_y);
        }
        
        switch (e->ai_state) {
            case ENEMY_AI_CHASE: {
                if (cfg->move_pattern == ENEMY_MOVE_BURROW) {
                    e->is_underground = true;
                } else {
                    e->is_underground = false;
                }

                if (cfg->move_pattern == ENEMY_MOVE_CHARGER) {
                    // Charger lines up, halts and winds up when within 230px and has line-of-sight
                    if (e->attack_cooldown_timer <= 0.0f && dist <= 230.0f && (can_fly || has_los)) {
                        e->ai_state = ENEMY_AI_WINDUP;
                        e->state_timer = 0.85f; // Readable telegraph window
                        e->charge_dir = (Vec2){ dir_x, dir_y };
                        e->charge_speed = cfg->speed * 2.3f;
                        e->vel.x = 0.0f;
                        e->vel.y = 0.0f;
                        e->attack_has_hit = false;
                        e->anim.direction = Sprite_VecToDirection(dir_x, dir_y, e->anim.direction);
                        break;
                    }
                } else if (cfg->move_pattern == ENEMY_MOVE_MORTAR) {
                    // Mortar roots in place when within 360px and lobs artillery
                    if (dist <= 360.0f) {
                        e->vel.x = 0.0f;
                        e->vel.y = 0.0f;
                        if (e->attack_cooldown_timer <= 0.0f) {
                            e->ai_state = ENEMY_AI_WINDUP;
                            e->state_timer = 0.65f;
                            e->attack_has_hit = false;
                            e->anim.direction = Sprite_VecToDirection(dir_x, dir_y, e->anim.direction);
                            break;
                        }
                    }
                } else if (cfg->attack_type != ENEMY_ATTACK_TOUCH_ONLY &&
                    e->attack_cooldown_timer <= 0.0f &&
                    dist <= cfg->attack_range &&
                    (can_fly || has_los)) {
                    
                    e->ai_state = ENEMY_AI_WINDUP;
                    e->state_timer = cfg->attack_windup;
                    e->is_underground = false; // Pops out, vulnerable!
                    e->attack_has_hit = false;
                    e->attack_anim.current_frame = 0;
                    e->attack_anim.anim_timer = 0.0f;
                    e->attack_anim.direction = Sprite_VecToDirection(dir_x, dir_y, e->anim.direction);
                    e->vel.x = 0.0f;
                    e->vel.y = 0.0f;
                    break;
                }

                // Movement calculation based on pattern
                if (cfg->move_pattern == ENEMY_MOVE_STATIONARY) {
                    // Metapod / Kakuna: barely moves (immovable bunker)
                    e->vel.x = move_dir_x * (cfg->speed * 0.12f);
                    e->vel.y = move_dir_y * (cfg->speed * 0.12f);
                    e->anim.direction = Sprite_VecToDirection(move_dir_x, move_dir_y, e->anim.direction);
                    Sprite_UpdateAnim(&e->anim, &cfg->sprite, dt, true);
                } else if (cfg->move_pattern == ENEMY_MOVE_FLANK_ORBIT) {
                    // Aerial circling: orbits around player, swooping in and out
                    if (dist > 280.0f) {
                        e->vel.x = dir_x * cfg->speed;
                        e->vel.y = dir_y * cfg->speed;
                    } else {
                        float orbit_dir = (i % 2 == 0) ? 1.0f : -1.0f;
                        e->orbit_angle += orbit_dir * 1.1f * dt;
                        float target_x = p->pos.x + cosf(e->orbit_angle) * e->orbit_dist;
                        float target_y = p->pos.y + sinf(e->orbit_angle) * e->orbit_dist;
                        float ox = target_x - e->pos.x;
                        float oy = target_y - e->pos.y;
                        float olen = sqrtf(ox * ox + oy * oy);
                        if (olen > 0.001f) {
                            e->vel.x = (ox / olen) * cfg->speed * 1.05f;
                            e->vel.y = (oy / olen) * cfg->speed * 1.05f;
                        }
                    }
                    e->anim.direction = Sprite_VecToDirection(e->vel.x, e->vel.y, e->anim.direction);
                    Sprite_UpdateAnim(&e->anim, &cfg->sprite, dt, true);
                } else if (cfg->move_pattern == ENEMY_MOVE_MORTAR) {
                    if (dist > 360.0f) {
                        e->vel.x = move_dir_x * (cfg->speed * 0.8f);
                        e->vel.y = move_dir_y * (cfg->speed * 0.8f);
                        e->anim.direction = Sprite_VecToDirection(move_dir_x, move_dir_y, e->anim.direction);
                        Sprite_UpdateAnim(&e->anim, &cfg->sprite, dt, true);
                    } else {
                        e->vel.x = 0.0f;
                        e->vel.y = 0.0f;
                        e->anim.direction = Sprite_VecToDirection(dir_x, dir_y, e->anim.direction);
                        Sprite_UpdateAnim(&e->anim, &cfg->sprite, dt, false);
                    }
                } else {
                    // Standard swarm & burrow movement
                    e->vel.x = move_dir_x * cfg->speed;
                    e->vel.y = move_dir_y * cfg->speed;
                    e->anim.direction = Sprite_VecToDirection(move_dir_x, move_dir_y, e->anim.direction);
                    Sprite_UpdateAnim(&e->anim, &cfg->sprite, dt, true);
                }
                break;
            }
            
            case ENEMY_AI_WINDUP: {
                e->is_underground = false;
                e->vel.x = 0.0f;
                e->vel.y = 0.0f;
                e->state_timer -= dt;

                if (cfg->move_pattern == ENEMY_MOVE_CHARGER) {
                    // Charger keeps laser locked in windup
                    e->anim.direction = Sprite_VecToDirection(e->charge_dir.x, e->charge_dir.y, e->anim.direction);
                } else {
                    e->anim.direction = Sprite_VecToDirection(dir_x, dir_y, e->anim.direction);
                    e->attack_anim.direction = e->anim.direction;
                }
                
                if (e->state_timer <= 0.0f) {
                    e->ai_state = ENEMY_AI_ATTACKING;
                    e->state_timer = (cfg->move_pattern == ENEMY_MOVE_CHARGER) ? 0.45f : cfg->attack_duration;
                    e->attack_has_hit = false;
                }
                break;
            }
            
            case ENEMY_AI_ATTACKING: {
                e->is_underground = false;
                e->state_timer -= dt;

                if (cfg->move_pattern == ENEMY_MOVE_CHARGER) {
                    // Charger rockets forward along charge_dir at extreme speed
                    e->vel.x = e->charge_dir.x * e->charge_speed;
                    e->vel.y = e->charge_dir.y * e->charge_speed;
                    e->anim.direction = Sprite_VecToDirection(e->charge_dir.x, e->charge_dir.y, e->anim.direction);
                    Sprite_UpdateAnim(&e->anim, &cfg->sprite, dt, true);

                    // Touch collision during charge deals massive damage
                    if (!e->attack_has_hit && dist <= e->radius + p->radius + 14.0f) {
                        e->attack_has_hit = true;
                        if (p->invuln_timer <= 0.0f && !p->god_mode) {
                            float dmg = cfg->touch_damage * 1.8f;
                            p->hp -= dmg;
                            p->invuln_timer = p->invuln_duration;
                            p->damage_flash_timer = 0.20f;
                            p->pos.x += e->charge_dir.x * 55.0f;
                            p->pos.y += e->charge_dir.y * 55.0f;
                            Weapons_SpawnDamageText(game, p->pos, dmg, (SDL_Color){ 255, 50, 50, 255 });
                        }
                    }
                } else {
                    e->vel.x = 0.0f;
                    e->vel.y = 0.0f;
                    if (cfg->has_attack_sprite) {
                        Sprite_UpdateAnim(&e->attack_anim, &cfg->sprite_attack, dt, true);
                    }
                }
                
                if (!e->attack_has_hit && e->state_timer <= cfg->attack_duration * 0.65f) {
                    e->attack_has_hit = true;
                    
                    if (cfg->move_pattern == ENEMY_MOVE_MORTAR) {
                        // Lob artillery onto player position
                        Weapons_SpawnGroundHazard(game, HAZARD_MORTAR_TARGET, p->pos, 42.0f, 1.25f, cfg->attack_damage, true, (SDL_Color){ 255, 60, 40, 255 });
                    } else if (cfg->attack_type == ENEMY_ATTACK_BURROW_ASSASSIN) {
                        if (dist <= cfg->attack_radius + p->radius) {
                            float dmg = cfg->attack_damage;
                            p->hp -= dmg;
                            p->invuln_timer = p->invuln_duration;
                            p->damage_flash_timer = 0.15f;
                            Weapons_SpawnDamageText(game, p->pos, dmg, (SDL_Color){ 255, 140, 40, 255 });
                            p->pos.x += dir_x * 24.0f;
                            p->pos.y += dir_y * 24.0f;
                        }
                    } else if (cfg->attack_type == ENEMY_ATTACK_SLAM_AOE || cfg->attack_type == ENEMY_ATTACK_BOSS_HYBRID) {
                        if (dist <= cfg->attack_radius + p->radius) {
                            float dmg = cfg->attack_damage;
                            p->hp -= dmg;
                            p->invuln_timer = p->invuln_duration;
                            p->damage_flash_timer = 0.15f;
                            Weapons_SpawnDamageText(game, p->pos, dmg, (SDL_Color){ 255, 70, 70, 255 });
                            p->pos.x += dir_x * 45.0f;
                            p->pos.y += dir_y * 45.0f;
                        }
                    } else if (cfg->attack_type == ENEMY_ATTACK_RANGED_EMBER) {
                        Vec2 bvel = { dir_x * 170.0f, dir_y * 170.0f };
                        Weapons_SpawnEnemyProjectile(game, e->pos, bvel, 11.0f, cfg->attack_damage, 2.5f, (SDL_Color){ 255, 110, 20, 255 });
                    } else if (cfg->attack_type == ENEMY_ATTACK_RANGED_WATER) {
                        Vec2 bvel = { dir_x * 160.0f, dir_y * 160.0f };
                        Weapons_SpawnEnemyProjectile(game, e->pos, bvel, 12.0f, cfg->attack_damage, 2.5f, (SDL_Color){ 50, 190, 255, 255 });
                    } else if (cfg->attack_type == ENEMY_ATTACK_RANGED_SPARK) {
                        float base_a = atan2f(dir_y, dir_x);
                        for (int sa = -1; sa <= 1; sa++) {
                            float fa = base_a + (float)sa * 0.28f;
                            Vec2 bvel = { cosf(fa) * 180.0f, sinf(fa) * 180.0f };
                            Weapons_SpawnEnemyProjectile(game, e->pos, bvel, 10.0f, cfg->attack_damage * 0.8f, 2.2f, (SDL_Color){ 255, 235, 40, 255 });
                        }
                    } else if (cfg->attack_type == ENEMY_ATTACK_RANGED_SPORE) {
                        Vec2 bvel = { dir_x * 90.0f, dir_y * 90.0f };
                        Weapons_SpawnEnemyProjectile(game, e->pos, bvel, 14.0f, cfg->attack_damage, 3.2f, (SDL_Color){ 180, 70, 220, 255 });
                    } else if (cfg->attack_type == ENEMY_ATTACK_RANGED_ROCK) {
                        Vec2 bvel = { dir_x * 150.0f, dir_y * 150.0f };
                        Weapons_SpawnEnemyProjectile(game, e->pos, bvel, 13.0f, cfg->attack_damage, 2.5f, (SDL_Color){ 190, 150, 80, 255 });
                    } else if (cfg->attack_type == ENEMY_ATTACK_RANGED_GUST) {
                        Vec2 bvel = { dir_x * 200.0f, dir_y * 200.0f };
                        Weapons_SpawnEnemyProjectile(game, e->pos, bvel, 11.0f, cfg->attack_damage, 2.2f, (SDL_Color){ 180, 230, 255, 255 });
                    } else if (cfg->attack_type == ENEMY_ATTACK_RANGED_PSYCHIC) {
                        Vec2 bvel = { dir_x * 140.0f, dir_y * 140.0f };
                        Weapons_SpawnEnemyProjectile(game, e->pos, bvel, 12.0f, cfg->attack_damage, 2.6f, (SDL_Color){ 240, 70, 160, 255 });
                    }
                }
                
                if (e->state_timer <= 0.0f) {
                    if (cfg->move_pattern == ENEMY_MOVE_BURROW) {
                        e->ai_state = ENEMY_AI_RETREAT;
                        e->state_timer = 1.8f;
                        e->is_underground = true;
                    } else if (cfg->move_pattern == ENEMY_MOVE_CHARGER) {
                        e->ai_state = ENEMY_AI_RECOVERY;
                        e->state_timer = 0.70f;
                        e->vel.x = 0.0f;
                        e->vel.y = 0.0f;
                    } else {
                        e->ai_state = ENEMY_AI_RECOVERY;
                        e->state_timer = 0.40f;
                    }
                    e->attack_cooldown_timer = cfg->attack_cooldown;
                }
                break;
            }
            
            case ENEMY_AI_RETREAT: {
                e->is_underground = true;
                e->state_timer -= dt;
                e->vel.x = -move_dir_x * (cfg->speed * 1.15f);
                e->vel.y = -move_dir_y * (cfg->speed * 1.15f);
                e->anim.direction = Sprite_VecToDirection(-move_dir_x, -move_dir_y, e->anim.direction);
                Sprite_UpdateAnim(&e->anim, &cfg->sprite, dt, true);
                if (e->state_timer <= 0.0f) {
                    e->ai_state = ENEMY_AI_CHASE;
                }
                break;
            }
            
            case ENEMY_AI_RECOVERY: {
                e->vel.x = 0.0f;
                e->vel.y = 0.0f;
                e->state_timer -= dt;
                if (e->state_timer <= 0.0f) {
                    e->ai_state = ENEMY_AI_CHASE;
                }
                break;
            }
        }
        
        // Knockback decay
        e->pos.x += e->knockback_x * dt;
        e->pos.y += e->knockback_y * dt;
        float decay = 10.0f * dt;
        if (decay > 1.0f) decay = 1.0f;
        e->knockback_x -= e->knockback_x * decay;
        e->knockback_y -= e->knockback_y * decay;
        
        e->pos.x += e->vel.x * dt;
        e->pos.y += e->vel.y * dt;
        
        Tilemap_CheckCollisionCircle(&game->tilemap, &e->pos.x, &e->pos.y, e->radius, can_fly);
        
        if (e->flash_timer > 0.0f) e->flash_timer -= dt;
        if (e->orbiter_hit_cooldown > 0.0f) e->orbiter_hit_cooldown -= dt;
    }
    
    game->profiler.t_enemies = Profiler_ElapsedMs(t_e_start);

    // -------------------------------------------------------------
    // PHASE 5: SPATIAL GRID UPDATE
    // -------------------------------------------------------------
    uint64_t t_g_start = Profiler_GetTimestamp();
    Grid_Clear(&g_grid);
    for (int k = 0; k < game->active_enemy_count; k++) {
        int i = game->active_enemy_indices[k];
        Grid_Insert(&g_grid, i, game->enemies[i].pos.x, game->enemies[i].pos.y);
    }
    Grid_ComputeMetrics(&g_grid);
    game->profiler.occupied_cells = g_grid.occupied_cells;
    game->profiler.max_entities_in_cell = g_grid.max_cell_count;
    game->profiler.t_spatial_grid = Profiler_ElapsedMs(t_g_start);

    // -------------------------------------------------------------
    // PHASE 6: WEAPON COLLISION RESOLUTION (Using Spatial Grid)
    // -------------------------------------------------------------
    uint64_t t_c_start = Profiler_GetTimestamp();
    int query_buffer[256];
    
    for (int p_idx = 0; p_idx < MAX_PROJECTILES; p_idx++) {
        Projectile* proj = &game->projectiles[p_idx];
        if (!proj->active || proj->is_enemy || (proj->damage <= 0.0f && proj->type != PROJ_POKEBALL)) continue; // Enemy bullets don't hurt enemies, and visual-only projectiles don't collide
        
        float search_rad = proj->radius + 32.0f;
        int found_count = Grid_QueryCircle(&g_grid, proj->pos.x, proj->pos.y, search_rad, query_buffer, 256);
        
        for (int k = 0; k < found_count; k++) {
            int e_idx = query_buffer[k];
            Enemy* e = &game->enemies[e_idx];
            if (!e->active || e->is_underground) continue; // Untargetable underground!
            
            if (proj->type == PROJ_ORBITER) {
                if (e->orbiter_hit_cooldown > 0.0f) continue;
            } else {
                bool already_hit = false;
                for (int h = 0; h < (int)proj->hit_uids_count; h++) {
                    if (proj->hit_uids[h] == e->uid) {
                        already_hit = true;
                        break;
                    }
                }
                if (already_hit) continue;
            }

            float dx = e->pos.x - proj->pos.x;
            float dy = e->pos.y - proj->pos.y;
            float hit_dist = proj->radius + e->radius;
            
            if (dx * dx + dy * dy <= hit_dist * hit_dist) {
                if (proj->type == PROJ_ORBITER) {
                    e->orbiter_hit_cooldown = 0.35f;
                } else {
                    if (proj->hit_uids_count < 8) {
                        proj->hit_uids[proj->hit_uids_count++] = e->uid;
                    }
                }

                // SPECIAL PROJECTILE: POKÉ BALL CAPTURE MECHANIC
                if (proj->type == PROJ_POKEBALL) {
                    if (game->enemy_types[e->type].is_boss) {
                        snprintf(game->route_banner_text, sizeof(game->route_banner_text), "GYM LEADER POKEMON CANNOT BE CAPTURED!");
                        game->route_banner_timer = 2.5f;
                        p->pokeball_count++; // Refund ball
                        proj->active = false;
                        break;
                    }
                    if (e->hp > e->max_hp * 0.40f && !e->is_trapped) {
                        char escape_msg[64];
                        snprintf(escape_msg, sizeof(escape_msg), "WILD %s BROKE FREE! (WEAKEN TO <40%% HP)", game->enemy_types[e->type].name);
                        snprintf(game->route_banner_text, sizeof(game->route_banner_text), "%s", escape_msg);
                        game->route_banner_timer = 3.0f;
                        Weapons_SpawnDamageText(game, e->pos, 0.0f, (SDL_Color){ 220, 220, 220, 255 });
                        proj->active = false;
                        break;
                    }

                    // CAPTURE SUCCESS! Add to party
                    int pid = game->enemy_types[e->type].pokedex_id;
                    const PokedexEntry* entry = Pokedex_GetEntry(pid);
                    if (entry && game->party_count < MAX_PARTY_MEMBERS) {
                        int slot = game->party_count;
                        PartyMember* m = &game->party[slot];
                        m->active = true;
                        m->pokedex_id = pid;
                        strncpy(m->name, entry->name, sizeof(m->name) - 1);
                        m->name[sizeof(m->name) - 1] = '\0';
                        m->type1 = entry->type1;
                        m->type2 = entry->type2;
                        m->pos = e->pos;
                        m->vel = (Vec2){ 0.0f, 0.0f };
                        m->sprite_scale = entry->scale * 1.05f;
                        m->sprite = Pokedex_LoadSprite(game->renderer, pid, m->sprite_scale);
                        float to_px = p->pos.x - m->pos.x;
                        float to_py = p->pos.y - m->pos.y;
                        int init_dir = Sprite_VecToDirection(to_px, to_py, 0);
                        m->anim = (AnimState){ 0, init_dir, 0.0f, false };
                        m->attack_cooldown = 2.5f;
                        m->attack_timer = 0.8f;
                        m->follow_angle_offset = (slot == 1) ? 2.3f : -2.3f;
                        game->party_count++;

                        char caught_msg[64];
                        const char* t1_str = Pokedex_GetTypeName(entry->type1);
                        snprintf(caught_msg, sizeof(caught_msg), "GOTCHA! CAUGHT %s! [%s] ATTACKS UNLOCKED!", entry->name, t1_str);
                        snprintf(game->route_banner_text, sizeof(game->route_banner_text), "%s", caught_msg);
                        game->route_banner_timer = 4.0f;

                        // Despawn wild enemy
                        Game_DeactivateEnemy(game, e_idx);
                    }
                    proj->active = false;
                    break;
                }

                // Apply Damage via centralized pipeline
                const char* proj_src = "PROJECTILE";
                if (proj->type == PROJ_WATER_BOLT) proj_src = "WATER_GUN";
                else if (proj->type == PROJ_EMBER) proj_src = "EMBER";
                else if (proj->type == PROJ_RAZOR_LEAF) proj_src = "RAZOR_LEAF";
                else if (proj->type == PROJ_ORBITER) proj_src = "ORBITER";
                else if (proj->type == PROJ_BUBBLE) proj_src = "BUBBLE";
                else if (proj->type == PROJ_SWIFT_STAR) proj_src = "SWIFT_STAR";
                else if (proj->type == PROJ_FIRE_BLAST) proj_src = "FIRE_BLAST";
                else if (proj->type == PROJ_ROCK_SLIDE) proj_src = "ROCK_SLIDE";
                else if (proj->type == PROJ_LIGHTNING_BOLT) proj_src = "LIGHTNING_BOLT";
                else if (proj->type == PROJ_SCALD_BUBBLE) proj_src = "SCALD_BUBBLE";

                float final_dmg = Game_ApplyEnemyDamage(game, e_idx, proj->damage, proj_src);
                if (proj->type == PROJ_LIGHTNING_BOLT) {
                    proj->damage = 0.0f; // Visual effect remains alive, but damage is dealt once
                }
                
                // Knockback
                float d_len = sqrtf(dx * dx + dy * dy);
                if (d_len > 0.001f && e->active) {
                    float resist = game->enemy_types[e->type].knockback_resist;
                    float push = proj->knockback * (1.0f - resist);
                    e->knockback_x += (dx / d_len) * push;
                    e->knockback_y += (dy / d_len) * push;
                }
                
                Weapons_SpawnDamageText(game, e->pos, final_dmg, (SDL_Color){ 255, 255, 255, 255 });
                
                // If Ember strikes an enemy, only occasionally spawn fire scorch hazard
                if (proj->type == PROJ_EMBER && (rand() % 100) < 25) {
                    Weapons_SpawnGroundHazard(game, HAZARD_FIRE_SCORCH, e->pos, 26.0f, 1.2f, 4.5f * p->damage_mult, false, (SDL_Color){ 255, 110, 20, 230 });
                }

                // If Fire Blast strikes, spawn molten lava pool!
                if (proj->type == PROJ_FIRE_BLAST) {
                    Weapons_SpawnGroundHazard(game, HAZARD_LAVA_POOL, e->pos, 85.0f, 3.2f, 18.0f * p->damage_mult, false, (SDL_Color){ 255, 60, 20, 255 });
                }

                // If Scald Bubble strikes, trap enemy in boiling mist and spawn scald geyser!
                if (proj->type == PROJ_SCALD_BUBBLE) {
                    if (!e->is_trapped) {
                        e->is_trapped = true;
                        e->trap_timer = 0.0f;
                        e->trap_duration = 1.4f;
                        e->trap_drift_vel = (Vec2){ proj->vel.x * 0.15f, -20.0f };
                    }
                    Weapons_SpawnGroundHazard(game, HAZARD_SCALD_MIST, e->pos, 42.0f, 2.5f, 15.0f * p->damage_mult, false, (SDL_Color){ 140, 230, 255, 180 });
                }

                // If Swift Star with Plasma Tempest strikes, trigger chain lightning
                if (proj->type == PROJ_SWIFT_STAR) {
                    for (int w = 0; w < p->weapon_count; w++) {
                        if (p->weapons[w].type == WEAPON_PLASMA_TEMPEST) {
                            int chains = 0;
                            for (int nk = 0; nk < game->active_enemy_count && chains < 3; nk++) {
                                int ne_idx = game->active_enemy_indices[nk];
                                if (ne_idx == e_idx) continue;
                                Enemy* ne = &game->enemies[ne_idx];
                                if (!ne->active || ne->is_underground) continue;
                                float cdx = ne->pos.x - e->pos.x;
                                float cdy = ne->pos.y - e->pos.y;
                                if (cdx * cdx + cdy * cdy <= 140.0f * 140.0f) {
                                    float actual_cdmg = Game_ApplyEnemyDamage(game, ne_idx, 45.0f * p->damage_mult, "PLASMA_TEMPEST");
                                    Weapons_SpawnDamageText(game, ne->pos, actual_cdmg, (SDL_Color){ 255, 255, 80, 255 });
                                    chains++;
                                    if (!ne->active) {
                                        nk--;
                                    }
                                }
                            }
                            break;
                        }
                    }
                }

                // If Razor Leaf strikes: check for Venom Cataclysm (acid) or Wildfire (scorch)
                if (proj->type == PROJ_RAZOR_LEAF) {
                    for (int w = 0; w < p->weapon_count; w++) {
                        if (p->weapons[w].type == WEAPON_VENOM_CATACLYSM) {
                            Weapons_SpawnGroundHazard(game, HAZARD_ACID_MIRE, e->pos, 45.0f, 2.5f, 10.0f * p->damage_mult, false, (SDL_Color){ 180, 40, 240, 220 });
                            break;
                        } else if (p->weapons[w].type == WEAPON_WILDFIRE) {
                            Weapons_SpawnGroundHazard(game, HAZARD_FIRE_SCORCH, e->pos, 35.0f, 2.0f, 12.0f * p->damage_mult, false, (SDL_Color){ 255, 120, 20, 220 });
                            break;
                        }
                    }
                }

                // If Bubble strikes: check for Storm Surge (lightning) or normal trap
                if (proj->type == PROJ_BUBBLE) {
                    if (!e->is_trapped) {
                        e->is_trapped = true;
                        e->trap_timer = 0.0f;
                        e->trap_duration = 1.15f;
                        e->trap_drift_vel = (Vec2){ proj->vel.x * 0.15f, proj->vel.y * 0.15f - 24.0f };
                    }
                    for (int w = 0; w < p->weapon_count; w++) {
                        if (p->weapons[w].type == WEAPON_STORM_SURGE) {
                            // Call down vertical lightning bolt on target!
                            for (int j = 0; j < MAX_PROJECTILES; j++) {
                                Projectile* lp = &game->projectiles[j];
                                if (!lp->active) {
                                    lp->active = true;
                                    lp->type = PROJ_LIGHTNING_BOLT;
                                    lp->pos = e->pos;
                                    lp->damage = 36.0f * p->damage_mult;
                                    lp->radius = 30.0f;
                                    lp->pierce_left = 3;
                                    lp->knockback = 120.0f;
                                    lp->lifetime = 0.0f;
                                    lp->max_lifetime = 0.35f;
                                    lp->is_enemy = false;
                                    lp->hit_uids_count = 0;
                                    lp->color = (SDL_Color){ 255, 240, 80, 255 };
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }

                // If Rock Slide with Magma Eruption strikes:
                if (proj->type == PROJ_ROCK_SLIDE) {
                    for (int w = 0; w < p->weapon_count; w++) {
                        if (p->weapons[w].type == WEAPON_MAGMA_ERUPTION) {
                            Weapons_SpawnGroundHazard(game, HAZARD_LAVA_POOL, e->pos, 50.0f, 2.8f, 16.0f * p->damage_mult, false, (SDL_Color){ 255, 60, 20, 220 });
                            break;
                        }
                    }
                }

                // If Bulbasaur Petal Dance or Verdant Surge strikes an enemy, siphon life
                if (proj->type == PROJ_ORBITER) {
                    float heal = (p->starter == STARTER_BULBASAUR) ? 1.0f : 0.0f;
                    for (int w = 0; w < p->weapon_count; w++) {
                        if (p->weapons[w].type == WEAPON_VERDANT_SURGE) { heal += 2.0f; break; }
                    }
                    if (heal > 0.0f && p->hp < p->max_hp) {
                        p->hp += heal;
                        if (p->hp > p->max_hp) p->hp = p->max_hp;
                    }
                }
                
                proj->pierce_left--;
                if (proj->pierce_left <= 0) {
                    proj->active = false;
                    break;
                }
            }
        }
    }
    
    // Player Touch Damage Collision
    if (p->invuln_timer <= 0.0f && !p->god_mode) {
        int touch_buf[64];
        int touched = Grid_QueryCircle(&g_grid, p->pos.x, p->pos.y, p->radius + 20.0f, touch_buf, 64);
        int p_tx = (int)floorf(p->pos.x / TILE_WORLD_SIZE);
        int p_ty = (int)floorf(p->pos.y / TILE_WORLD_SIZE);
        uint8_t p_elev = (p_tx >= 0 && p_tx < MAP_TILES_X && p_ty >= 0 && p_ty < MAP_TILES_Y) ? game->tilemap.cells[p_ty][p_tx].elevation : 0;

        for (int k = 0; k < touched; k++) {
            int e_idx = touch_buf[k];
            Enemy* e = &game->enemies[e_idx];
            if (!e->active || e->is_underground || e->is_trapped) continue;

            EnemyTypeConfig* ecfg = &game->enemy_types[e->type];
            bool e_can_fly = (ecfg->move_pattern == ENEMY_MOVE_FLANK_ORBIT || ecfg->attack_type == ENEMY_ATTACK_RANGED_GUST);
            if (!e_can_fly) {
                int e_tx = (int)floorf(e->pos.x / TILE_WORLD_SIZE);
                int e_ty = (int)floorf(e->pos.y / TILE_WORLD_SIZE);
                uint8_t e_elev = (e_tx >= 0 && e_tx < MAP_TILES_X && e_ty >= 0 && e_ty < MAP_TILES_Y) ? game->tilemap.cells[e_ty][e_tx].elevation : 0;
                bool on_ramp = ((game->tilemap.cells[p_ty][p_tx].flags & TILE_FLAG_RAMP) != 0) ||
                               (e_tx >= 0 && e_tx < MAP_TILES_X && e_ty >= 0 && e_ty < MAP_TILES_Y && (game->tilemap.cells[e_ty][e_tx].flags & TILE_FLAG_RAMP) != 0);
                if (!on_ramp && e_elev != p_elev) continue; // Ground entities cannot touch across elevation cliffs unless traversing a ramp
            }
            
            float dx = e->pos.x - p->pos.x;
            float dy = e->pos.y - p->pos.y;
            float hit_dist = p->radius + e->radius;
            if (dx * dx + dy * dy <= hit_dist * hit_dist) {
                p->hp -= e->touch_damage;
                p->damage_flash_timer = 0.18f;
                p->invuln_timer = p->invuln_duration;
                Weapons_SpawnDamageText(game, p->pos, e->touch_damage, (SDL_Color){ 255, 50, 50, 255 });
                break;
            }
        }
    }
    
    game->profiler.t_collision = Profiler_ElapsedMs(t_c_start);

    // -------------------------------------------------------------
    // PHASE 7: GEM MAGNET & XP LEVEL-UP
    // -------------------------------------------------------------
    for (int i = 0; i < MAX_GEMS; i++) {
        Gem* g = &game->gems[i];
        if (!g->active) continue;
        
        float dx = p->pos.x - g->pos.x;
        float dy = p->pos.y - g->pos.y;
        float dist_sq = dx * dx + dy * dy;
        
        if (dist_sq <= p->pickup_radius * p->pickup_radius) {
            g->being_pulled = true;
        }
        
        if (g->being_pulled) {
            float dist = sqrtf(dist_sq);
            if (dist > 1.0f) {
                float pull_spd = 550.0f;
                g->pos.x += (dx / dist) * pull_spd * dt;
                g->pos.y += (dy / dist) * pull_spd * dt;
            }
            
            if (dist <= p->radius + g->radius) {
                p->xp += g->value;
                g->active = false;
                
                while (p->xp >= p->xp_to_next_level) {
                    p->xp -= p->xp_to_next_level;
                    p->level++;
                    p->xp_to_next_level = (int)(p->xp_to_next_level * 1.30f);
                    
                    // Roguelike 3-card draft on every level up!
                    Game_GenerateLevelUpCards(game);
                    game->app_state = APP_STATE_LEVEL_UP;
                    game->paused = true;
                    break;
                }
            }
        }
    }
    
    // Check Death
    if (p->hp <= 0.0f) {
        p->hp = 0.0f;
        game->app_state = APP_STATE_GAME_OVER;
    }

    // Camera follow player smoothly
    game->camera.x = p->pos.x - WINDOW_WIDTH / 2.0f;
    game->camera.y = p->pos.y - WINDOW_HEIGHT / 2.0f;
    if (game->camera.x < 0.0f) game->camera.x = 0.0f;
    if (game->camera.x > WORLD_SIZE_X - WINDOW_WIDTH) game->camera.x = WORLD_SIZE_X - WINDOW_WIDTH;
    if (game->camera.y < 0.0f) game->camera.y = 0.0f;
    if (game->camera.y > WORLD_SIZE_Y - WINDOW_HEIGHT) game->camera.y = WORLD_SIZE_Y - WINDOW_HEIGHT;

    // Profiler entity counters
    game->profiler.active_enemies = game->active_enemy_count;
    int a_projs = 0;
    for (int i = 0; i < MAX_PROJECTILES; i++) if (game->projectiles[i].active) a_projs++;
    game->profiler.active_projectiles = a_projs;
    int a_gems = 0;
    for (int i = 0; i < MAX_GEMS; i++) if (game->gems[i].active) a_gems++;
    game->profiler.active_gems = a_gems;
}

void Game_Draw(GameState* game) {
    SDL_Renderer* renderer = game->renderer;
    Vec2 camera = game->camera;
    uint64_t t_r_start = Profiler_GetTimestamp();

    // -------------------------------------------------------------
    // STATE 1: STARTER SELECTION SCREEN
    // -------------------------------------------------------------
    if (game->app_state == APP_STATE_STARTER_SELECT) {
        SDL_SetRenderDrawColor(renderer, 14, 18, 28, 255);
        SDL_RenderClear(renderer);

        // Header Title
        Font_DrawTextShadow(renderer, game->font_texture, "POKEMON SURVIVORS: KANTO JOURNEY", 320, 42, 3, (SDL_Color){ 255, 215, 60, 255 });
        Font_DrawText(renderer, game->font_texture, "Select your Starter Partner Pokemon to begin your Route & Gym conquest!", 360, 85, 1, (SDL_Color){ 180, 205, 230, 255 });
        Font_DrawText(renderer, game->font_texture, "Click a card or press [1], [2], or [3] on your keyboard", 420, 105, 1, (SDL_Color){ 140, 160, 180, 255 });

        // 3 Starter Cards
        int card_w = 320;
        int card_h = 490;
        int card_y = 140;
        int card_xs[3] = { 100, 480, 860 };
        StarterID starters[3] = { STARTER_BULBASAUR, STARTER_CHARMANDER, STARTER_SQUIRTLE };
        const char* s_names[3] = { "BULBASAUR (#0001)", "CHARMANDER (#0004)", "SQUIRTLE (#0007)" };
        const char* s_types[3] = { "TYPE: GRASS / POISON", "TYPE: FIRE", "TYPE: WATER" };
        const char* s_moves[3] = { "Basic Move: VINE WHIP (Cleave)", "Basic Move: EMBER (Fire Bolt)", "Basic Move: WATER GUN (Water Stream)" };
        const char* s_descs[3] = {
            "Regenerative vitality, crowd sweep attacks. Learns Razor Leaf, Petal Dance & Solar Beam!",
            "High movement speed and offensive ranged fire. Learns Fire Spin, Dragon Breath & Flamethrower!",
            "Balanced defender with defensive shell mechanics. Learns Aqua Orbiters, Aqua Jet & Hydro Pump!"
        };
        SDL_Color s_colors[3] = {
            { 70, 205, 80, 255 },
            { 245, 110, 40, 255 },
            { 60, 170, 245, 255 }
        };

        for (int i = 0; i < 3; i++) {
            int cx = card_xs[i];
            bool hovered = (game->selected_starter_hover == starters[i]);
            
            // Card Background
            SDL_Rect c_rect = { cx, card_y, card_w, card_h };
            if (hovered) {
                SDL_SetRenderDrawColor(renderer, 24, 32, 48, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 18, 22, 34, 255);
            }
            SDL_RenderFillRect(renderer, &c_rect);
            
            // Card Border
            SDL_Color border_col = hovered ? s_colors[i] : (SDL_Color){ 50, 65, 90, 255 };
            SDL_SetRenderDrawColor(renderer, border_col.r, border_col.g, border_col.b, 255);
            SDL_RenderDrawRect(renderer, &c_rect);
            if (hovered) {
                SDL_Rect inner = { cx + 1, card_y + 1, card_w - 2, card_h - 2 };
                SDL_RenderDrawRect(renderer, &inner);
            }

            // Title
            Font_DrawTextShadow(renderer, game->font_texture, s_names[i], cx + 24, card_y + 20, 2, s_colors[i]);
            Font_DrawText(renderer, game->font_texture, s_types[i], cx + 24, card_y + 50, 1, (SDL_Color){ 200, 220, 240, 255 });

            // Portrait Showcase Box
            int pbox_x = cx + (card_w - 140) / 2;
            int pbox_y = card_y + 80;
            SDL_Rect pbox = { pbox_x, pbox_y, 140, 140 };
            SDL_SetRenderDrawColor(renderer, 12, 16, 24, 255);
            SDL_RenderFillRect(renderer, &pbox);
            SDL_SetRenderDrawColor(renderer, border_col.r, border_col.g, border_col.b, 180);
            SDL_RenderDrawRect(renderer, &pbox);

            // Draw Portrait Texture (40x40 scaled up to 120x120)
            if (game->starter_portraits[i]) {
                SDL_Rect dst_port = { pbox_x + 10, pbox_y + 10, 120, 120 };
                SDL_RenderCopy(renderer, game->starter_portraits[i], NULL, &dst_port);
            }

            // Starting Move details
            Font_DrawText(renderer, game->font_texture, s_moves[i], cx + 18, card_y + 240, 1, (SDL_Color){ 255, 230, 80, 255 });

            // Description lines (wrap manually)
            char desc_line1[48] = {0};
            char desc_line2[48] = {0};
            const char* full_desc = s_descs[i];
            int break_pt = 36;
            while (break_pt > 0 && full_desc[break_pt] != ' ') break_pt--;
            strncpy(desc_line1, full_desc, break_pt);
            strncpy(desc_line2, full_desc + break_pt + 1, sizeof(desc_line2) - 1);

            Font_DrawText(renderer, game->font_texture, desc_line1, cx + 18, card_y + 275, 1, (SDL_Color){ 170, 185, 205, 255 });
            Font_DrawText(renderer, game->font_texture, desc_line2, cx + 18, card_y + 295, 1, (SDL_Color){ 170, 185, 205, 255 });

            // Select Button
            int btn_h = 42;
            int btn_y = card_y + card_h - 60;
            SDL_Rect btn_rect = { cx + 20, btn_y, card_w - 40, btn_h };
            if (hovered) {
                SDL_SetRenderDrawColor(renderer, s_colors[i].r, s_colors[i].g, s_colors[i].b, 255);
                SDL_RenderFillRect(renderer, &btn_rect);
                char btn_text[32];
                snprintf(btn_text, sizeof(btn_text), "[%d] CHOOSE PARTNER!", i + 1);
                Font_DrawText(renderer, game->font_texture, btn_text, cx + 45, btn_y + 14, 2, (SDL_Color){ 10, 15, 25, 255 });
            } else {
                SDL_SetRenderDrawColor(renderer, 30, 40, 58, 255);
                SDL_RenderFillRect(renderer, &btn_rect);
                SDL_SetRenderDrawColor(renderer, 60, 80, 110, 255);
                SDL_RenderDrawRect(renderer, &btn_rect);
                char btn_text[32];
                snprintf(btn_text, sizeof(btn_text), "[%d] SELECT", i + 1);
                Font_DrawText(renderer, game->font_texture, btn_text, cx + 80, btn_y + 14, 2, (SDL_Color){ 200, 220, 240, 255 });
            }
        }

        return;
    }

    // -------------------------------------------------------------
    // PLAYING / LEVEL-UP / GAME-OVER WORLD RENDERING
    // -------------------------------------------------------------

    // 1. Render World Tileset (Rich Route Tilemap)
    Tilemap_Render(&game->tilemap, renderer, game->tileset_texture, camera.x, camera.y);
    
    // 2. Spatial Grid Visualizer
    if (game->dev_menu.show_spatial_grid) {
        SDL_SetRenderDrawColor(renderer, 45, 60, 80, 75);
        int sx = ((int)camera.x / GRID_CELL_SIZE) * GRID_CELL_SIZE;
        int sy = ((int)camera.y / GRID_CELL_SIZE) * GRID_CELL_SIZE;
        for (int x = sx; x <= sx + WINDOW_WIDTH + GRID_CELL_SIZE; x += GRID_CELL_SIZE) {
            int line_x = (int)(x - camera.x);
            SDL_RenderDrawLine(renderer, line_x, 0, line_x, WINDOW_HEIGHT);
        }
        for (int y = sy; y <= sy + WINDOW_HEIGHT + GRID_CELL_SIZE; y += GRID_CELL_SIZE) {
            int line_y = (int)(y - camera.y);
            SDL_RenderDrawLine(renderer, 0, line_y, WINDOW_WIDTH, line_y);
        }
    }
    
    // 3. Draw Gems (Radiant Faceted Crystals)
    EnsureGemTextures(renderer);
    for (int i = 0; i < MAX_GEMS; i++) {
        Gem* g = &game->gems[i];
        if (!g->active) continue;
        int gx = (int)(g->pos.x - camera.x);
        int gy = (int)(g->pos.y - camera.y);
        if (gx < -24 || gx > WINDOW_WIDTH + 24 || gy < -24 || gy > WINDOW_HEIGHT + 24) continue;

        // Gentle animated vertical bobbing
        float bob = sinf(game->spawner.game_time * 5.5f + (float)i * 0.75f) * 2.2f;
        int dy = gy + (int)bob;

        int tier = (g->value >= 10 ? 2 : (g->value >= 3 ? 1 : 0));
        int size = (tier == 2 ? 22 : (tier == 1 ? 18 : 15));

        // When vacuumed by magnet, draw an energetic glowing tail
        if (g->being_pulled) {
            float dx = game->player.pos.x - g->pos.x;
            float pdy = game->player.pos.y - g->pos.y;
            float len = sqrtf(dx * dx + pdy * pdy);
            if (len > 0.001f) {
                float nx = dx / len;
                float ny = pdy / len;
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 190);
                for (int t = 1; t <= 3; t++) {
                    int tx = gx - (int)(nx * t * 4.5f);
                    int ty = dy - (int)(ny * t * 4.5f);
                    SDL_Rect spark = { tx - 1, ty - 1, 3, 3 };
                    SDL_RenderFillRect(renderer, &spark);
                }
            }
        }

        if (s_gem_textures[tier]) {
            SDL_Rect r = { gx - size / 2, dy - size / 2, size, size };
            SDL_RenderCopy(renderer, s_gem_textures[tier], NULL, &r);
        } else {
            SDL_Rect r = { gx - size / 2, dy - size / 2, size, size };
            SDL_SetRenderDrawColor(renderer, 40, 255, 120, 255);
            SDL_RenderFillRect(renderer, &r);
        }
    }
    
    // 3.5. Draw Ground Hazards (Fire Scorch, Toxic Spores, Mortar Reticles on Floor)
    Weapons_DrawGroundHazards(game);

    // 4. Draw Enemies
    for (int k = 0; k < game->active_enemy_count; k++) {
        int i = game->active_enemy_indices[k];
        Enemy* e = &game->enemies[i];
        EnemyTypeConfig* cfg = &game->enemy_types[e->type];
        
        int ex = (int)(e->pos.x - camera.x);
        int ey = (int)(e->pos.y - camera.y);
        if (ex < -80 || ex > WINDOW_WIDTH + 80 || ey < -80 || ey > WINDOW_HEIGHT + 80) continue;
        
        bool flash = (e->flash_timer > 0.0f);
        
        // Movement Pattern Visual Telegraphs (Charger Laser, Speed Lines, Harden Shield, Mortar Ring)
        if (cfg->move_pattern == ENEMY_MOVE_CHARGER) {
            if (e->ai_state == ENEMY_AI_WINDUP) {
                // Pulsing bright red telegraph laser
                int alpha = 160 + (int)(sinf(e->state_timer * 20.0f) * 75.0f);
                if (alpha < 60) alpha = 60;
                if (alpha > 255) alpha = 255;
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 40, 40, (Uint8)alpha);
                int lx2 = ex + (int)(e->charge_dir.x * 240.0f);
                int ly2 = ey + (int)(e->charge_dir.y * 240.0f);
                SDL_RenderDrawLine(renderer, ex, ey, lx2, ly2);
                SDL_RenderDrawLine(renderer, ex + 1, ey, lx2 + 1, ly2);
                SDL_RenderDrawLine(renderer, ex, ey + 1, lx2, ly2 + 1);
                
                SDL_SetRenderDrawColor(renderer, 255, 220, 90, (Uint8)alpha);
                SDL_Rect dot = { lx2 - 3, ly2 - 3, 6, 6 };
                SDL_RenderFillRect(renderer, &dot);
            } else if (e->ai_state == ENEMY_AI_ATTACKING) {
                // Speed dust streaks trailing behind rush
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 230, 140, 190);
                for (int s = 1; s <= 3; s++) {
                    int sx = ex - (int)(e->charge_dir.x * (float)(s * 14));
                    int sy = ey - (int)(e->charge_dir.y * (float)(s * 14));
                    SDL_Rect streak = { sx - 2, sy - 2, 4, 4 };
                    SDL_RenderFillRect(renderer, &streak);
                }
            }
        } else if (cfg->move_pattern == ENEMY_MOVE_STATIONARY) {
            // Shimmering silver Harden hexagonal barrier (65% damage resistance!)
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 210, 235, 255, 160);
            int hr = (int)(cfg->radius + 5.0f);
            for (int s = 0; s < 6; s++) {
                float a1 = (float)s * ((float)M_PI / 3.0f);
                float a2 = (float)(s + 1) * ((float)M_PI / 3.0f);
                SDL_RenderDrawLine(renderer, ex + (int)(cosf(a1)*hr), ey + (int)(sinf(a1)*hr),
                                             ex + (int)(cosf(a2)*hr), ey + (int)(sinf(a2)*hr));
            }
        } else if (cfg->move_pattern == ENEMY_MOVE_MORTAR && e->mortar_cooldown_timer < 0.6f) {
            // Charging purple spore warning ring
            int m_rad = (int)(cfg->radius + 3.0f + (0.6f - e->mortar_cooldown_timer) * 16.0f);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 190, 80, 250, 160);
            for (int a = 0; a < 16; a++) {
                float a1 = (float)a * ((float)M_PI / 8.0f);
                float a2 = (float)(a + 1) * ((float)M_PI / 8.0f);
                SDL_RenderDrawLine(renderer, ex + (int)(cosf(a1)*m_rad), ey + (int)(sinf(a1)*m_rad),
                                             ex + (int)(cosf(a2)*m_rad), ey + (int)(sinf(a2)*m_rad));
            }
        }
        
        if (e->is_underground) {
            // Diglett Underground Burrowing Visual
            DrawFilledCircle(renderer, ex, ey, (int)cfg->radius, (SDL_Color){ 35, 20, 15, 180 });
            SDL_SetRenderDrawColor(renderer, 150, 95, 45, 220);
            for (int p = 0; p < 8; p++) {
                int px = ex + ((p * 17) % 21) - 10;
                int py = ey + ((p * 23) % 15) - 7;
                SDL_Rect dirt = { px, py, 3, 3 };
                SDL_RenderFillRect(renderer, &dirt);
            }
        } else if (e->ai_state == ENEMY_AI_ATTACKING || e->ai_state == ENEMY_AI_WINDUP) {
            if (cfg->has_attack_sprite && cfg->sprite_attack.loaded) {
                Sprite_Draw(renderer, &cfg->sprite_attack, &e->attack_anim, e->pos, camera, cfg->debug_color, cfg->radius, flash);
            } else {
                Sprite_Draw(renderer, &cfg->sprite, &e->anim, e->pos, camera, cfg->debug_color, cfg->radius, flash);
            }

            if (e->ai_state == ENEMY_AI_ATTACKING) {
                if (cfg->attack_type == ENEMY_ATTACK_BURROW_ASSASSIN) {
                    // Thick Diglett claw swipe arcs with white tip and red slash
                    float s_ang = Sprite_DirectionToAngle(e->attack_anim.direction);
                    for (int sa = -3; sa <= 3; sa++) {
                        float fa = s_ang + (float)sa * 0.18f;
                        int x1 = ex + (int)(cosf(fa) * 12.0f);
                        int y1 = ey + (int)(sinf(fa) * 12.0f);
                        int x2 = ex + (int)(cosf(fa) * 44.0f);
                        int y2 = ey + (int)(sinf(fa) * 44.0f);
                        // Thick multi-pass slash line
                        SDL_SetRenderDrawColor(renderer, 255, 60, 40, 240);
                        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
                        SDL_RenderDrawLine(renderer, x1 + 1, y1, x2 + 1, y2);
                        SDL_RenderDrawLine(renderer, x1, y1 + 1, x2, y2 + 1);
                        // Sharp white tip
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        SDL_Rect tip = { x2 - 2, y2 - 2, 4, 4 };
                        SDL_RenderFillRect(renderer, &tip);
                    }
                } else if (cfg->attack_type == ENEMY_ATTACK_SLAM_AOE || cfg->attack_type == ENEMY_ATTACK_BOSS_HYBRID) {
                    // Stomp shockwave dust ring - thick multi-band wave
                    int sw_r = (int)(cfg->attack_radius * (1.0f - e->state_timer / cfg->attack_duration));
                    for (int dr = -3; dr <= 3; dr++) {
                        int r_band = sw_r + dr;
                        if (r_band < 2) continue;
                        Uint8 a = (Uint8)(210 - abs(dr) * 45);
                        SDL_SetRenderDrawColor(renderer, 230, 180, 110, a);
                        for (int a_step = 0; a_step < 32; a_step++) {
                            float a1 = (2.0f * (float)M_PI * a_step) / 32.0f;
                            float a2 = (2.0f * (float)M_PI * (a_step + 1)) / 32.0f;
                            SDL_RenderDrawLine(renderer, ex + (int)(cosf(a1)*r_band), ey + (int)(sinf(a1)*r_band),
                                                         ex + (int)(cosf(a2)*r_band), ey + (int)(sinf(a2)*r_band));
                        }
                    }
                }
            }
        } else {
            Sprite_Draw(renderer, &cfg->sprite, &e->anim, e->pos, camera, cfg->debug_color, cfg->radius, flash);
        }

        if (e->is_trapped) {
            float bx = e->pos.x - camera.x;
            float by = e->pos.y - camera.y;
            float bubble_r = cfg->radius + 12.0f + sinf(e->trap_timer * 10.0f) * 2.0f;
            DrawFilledCircle(renderer, (int)bx, (int)by, (int)bubble_r, (SDL_Color){ 80, 210, 255, 95 });
            SDL_SetRenderDrawColor(renderer, 140, 240, 255, 220);
            for (int a_step = 0; a_step < 24; a_step++) {
                float a1 = (2.0f * (float)M_PI * a_step) / 24.0f;
                float a2 = (2.0f * (float)M_PI * (a_step + 1)) / 24.0f;
                SDL_RenderDrawLine(renderer, (int)(bx + cosf(a1)*bubble_r), (int)(by + sinf(a1)*bubble_r),
                                             (int)(bx + cosf(a2)*bubble_r), (int)(by + sinf(a2)*bubble_r));
            }
            int hx = (int)(bx - bubble_r * 0.42f);
            int hy = (int)(by - bubble_r * 0.42f);
            DrawFilledCircle(renderer, hx, hy, (int)(bubble_r * 0.26f), (SDL_Color){ 255, 255, 255, 220 });
        }

        // Catchable indicator if enemy is weakened under 40% HP or trapped in bubble!
        if ((e->hp <= e->max_hp * 0.40f || e->is_trapped) && !cfg->is_boss) {
            float pulse = 0.8f + 0.2f * sinf(game->spawner.game_time * 8.0f);
            int ey_tag = (int)(e->pos.y - camera.y - cfg->radius - 14.0f);
            int ex_tag = (int)(e->pos.x - camera.x);
            Font_DrawTextShadow(renderer, game->font_texture, "[E] CATCH", ex_tag - 26, ey_tag, 1, 
                (SDL_Color){ 255, (Uint8)(220 * pulse), 60, 255 });
        }
    }

    // 4.5. Draw Party Companions (Followers with friendly green glow & silhouettes)
    Party_Draw(game);
    
    // 5. Draw Player
    Player* p = &game->player;
    bool player_flash = (p->damage_flash_timer > 0.0f);
    
    if (p->state == PLAYER_STATE_HYDRO_PUMP) {
        float px = p->pos.x - camera.x;
        float py = p->pos.y - camera.y;
        float theta = p->special_q_current_angle;
        double rot_deg = (double)(theta * (180.0f / (float)M_PI));
        
        float hw = 22.0f;
        float hh = 12.0f;
        float corners[4][2] = {
            {  hw, -hh },
            {  hw,  hh },
            { -hw,  hh },
            { -hw, -hh }
        };
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        // 4 rounded spinning water thruster jets
        for (int k = 0; k < 4; k++) {
            float lx = corners[k][0];
            float ly = corners[k][1];
            float cos_t = cosf(theta);
            float sin_t = sinf(theta);
            float cx = px + (lx * cos_t - ly * sin_t);
            float cy = py + (lx * sin_t + ly * cos_t);
            
            float tx = -(cy - py);
            float ty = +(cx - px);
            float tlen = sqrtf(tx * tx + ty * ty);
            if (tlen > 0.001f) { tx /= tlen; ty /= tlen; }
            float nx = (cx - px);
            float ny = (cy - py);
            float nlen = sqrtf(nx * nx + ny * ny);
            if (nlen > 0.001f) { nx /= nlen; ny /= nlen; }
            
            float ex_x = -0.75f * tx + 0.85f * nx;
            float ex_y = -0.75f * ty + 0.85f * ny;
            float elen = sqrtf(ex_x * ex_x + ex_y * ex_y);
            if (elen > 0.001f) { ex_x /= elen; ex_y /= elen; }
            float phi_0 = atan2f(ex_y, ex_x);
            
            int num_nodes = 14;
            float max_range = p->special_q_range;
            for (int s = 1; s <= num_nodes; s++) {
                float u = (float)s / (float)num_nodes;
                float dist = 6.0f + u * (max_range - 6.0f);
                float curl = 0.65f * powf(u, 1.25f);
                float phi = phi_0 - curl;
                float wx = cx + dist * cosf(phi);
                float wy = cy + dist * sinf(phi);
                float wave = sinf(u * 14.0f - game->spawner.game_time * 26.0f);
                float node_radius = 4.5f + 8.5f * u + 2.0f * wave;
                
                DrawFilledCircle(renderer, (int)wx, (int)wy, (int)(node_radius * 1.35f), (SDL_Color){ 20, 100, 220, 75 });
                DrawFilledCircle(renderer, (int)wx, (int)wy, (int)node_radius, (SDL_Color){ 60, 210, 255, 190 });
                DrawFilledCircle(renderer, (int)wx, (int)wy, (int)(node_radius * 0.45f), (SDL_Color){ 230, 250, 255, 245 });
            }
        }
        
        Sprite_DrawRotated(renderer, &p->sprite_special, &p->anim, p->pos, camera,
                           rot_deg, (SDL_Color){ 80, 180, 240, 255 }, p->radius, player_flash);
    } else if (p->state == PLAYER_STATE_SOLAR_BEAM) {
        // Solar Beam Continuous Light Laser
        float px = p->pos.x - camera.x;
        float py = p->pos.y - camera.y;
        float ang = Sprite_DirectionToAngle(p->anim.direction);
        float bx = px + cosf(ang) * p->special_q_range;
        float by = py + sinf(ang) * p->special_q_range;
        DrawFilledCircle(renderer, (int)px, (int)py, 22, (SDL_Color){ 255, 240, 100, 200 });
        SDL_SetRenderDrawColor(renderer, 255, 245, 120, 220);
        for (int lw = -8; lw <= 8; lw++) {
            SDL_RenderDrawLine(renderer, (int)px - lw, (int)py + lw, (int)bx - lw, (int)by + lw);
        }
        Sprite_Draw(renderer, &p->sprite_special, &p->anim, p->pos, camera, (SDL_Color){ 80, 200, 80, 255 }, p->radius, player_flash);
    } else if (p->state == PLAYER_STATE_FLAMETHROWER) {
        // CHARMANDER ACTIVE [Q]: Iconic FIRE BLAST (大字爆 / Daimonji) Starburst!
        float px = p->pos.x - camera.x;
        float py = p->pos.y - camera.y;
        float ang = Sprite_DirectionToAngle(p->anim.direction);

        float progress = 1.0f - (p->special_q_timer / p->special_q_duration);
        float burst_r = p->special_q_range * (0.35f + progress * 0.75f);
        float center_x = px + cosf(ang) * (burst_r * 0.55f);
        float center_y = py + sinf(ang) * (burst_r * 0.55f);

        // Central blast core
        DrawFilledCircle(renderer, (int)center_x, (int)center_y, (int)(burst_r * 0.35f), (SDL_Color){ 255, 80, 20, 160 });
        DrawFilledCircle(renderer, (int)center_x, (int)center_y, (int)(burst_r * 0.20f), (SDL_Color){ 255, 230, 80, 230 });

        // 5 arms of 大 relative to facing angle
        float arm_angles[5] = {
            ang,                       // Forward head
            ang - (float)M_PI * 0.45f, // Left crossbar
            ang + (float)M_PI * 0.45f, // Right crossbar
            ang - (float)M_PI * 0.78f, // Lower-left leg
            ang + (float)M_PI * 0.78f  // Lower-right leg
        };

        for (int a = 0; a < 5; a++) {
            float tip_x = center_x + cosf(arm_angles[a]) * burst_r;
            float tip_y = center_y + sinf(arm_angles[a]) * burst_r;

            // Blazing flame line along arm
            SDL_SetRenderDrawColor(renderer, 255, 90, 20, 230);
            for (int w = -3; w <= 3; w++) {
                SDL_RenderDrawLine(renderer, (int)center_x + w, (int)center_y, (int)tip_x + w, (int)tip_y);
                SDL_RenderDrawLine(renderer, (int)center_x, (int)center_y + w, (int)tip_x, (int)tip_y + w);
            }

            // Burning fireball at tip of each arm
            DrawFilledCircle(renderer, (int)tip_x, (int)tip_y, (int)(burst_r * 0.22f), (SDL_Color){ 255, 70, 10, 190 });
            DrawFilledCircle(renderer, (int)tip_x, (int)tip_y, (int)(burst_r * 0.12f), (SDL_Color){ 255, 240, 120, 255 });
        }

        Sprite_Draw(renderer, &p->sprite_special, &p->anim, p->pos, camera, (SDL_Color){ 240, 100, 30, 255 }, p->radius, player_flash);
    } else if (p->state == PLAYER_STATE_DASHING) {
        // Ghost afterimages
        float ghost_dists[3] = { 18.0f, 36.0f, 54.0f };
        Uint8 ghost_alphas[3] = { 150, 95, 45 };
        
        if (p->sprite_charge.texture) {
            SDL_SetTextureBlendMode(p->sprite_charge.texture, SDL_BLENDMODE_BLEND);
        }
        for (int g = 2; g >= 0; g--) {
            Vec2 gpos = {
                p->pos.x - p->dash_dir.x * ghost_dists[g],
                p->pos.y - p->dash_dir.y * ghost_dists[g]
            };
            if (p->sprite_charge.texture) {
                SDL_SetTextureAlphaMod(p->sprite_charge.texture, ghost_alphas[g]);
                SDL_SetTextureColorMod(p->sprite_charge.texture, 100, 210, 255);
            }
            Sprite_Draw(renderer, &p->sprite_charge, &p->anim, gpos, camera,
                        (SDL_Color){ 100, 200, 255, ghost_alphas[g] }, p->radius, false);
        }
        if (p->sprite_charge.texture) {
            SDL_SetTextureAlphaMod(p->sprite_charge.texture, 255);
            SDL_SetTextureColorMod(p->sprite_charge.texture, 255, 255, 255);
        }
        Sprite_Draw(renderer, &p->sprite_charge, &p->anim, p->pos, camera,
                    (SDL_Color){ 80, 180, 240, 255 }, p->radius, player_flash);
    } else {
        Sprite_Draw(renderer, &p->sprite, &p->anim, p->pos, camera, 
                    (SDL_Color){ 80, 180, 240, 255 }, p->radius, player_flash);
    }
    
    // 6. Draw Weapons & Bullet-Hell Projectiles
    Weapons_Draw(game);
    
    // 7. Visual Debugs (Hitboxes & Pickup Range)
    if (game->dev_menu.show_hitboxes) {
        int px = (int)(p->pos.x - camera.x);
        int py = (int)(p->pos.y - camera.y);
        int pr = (int)p->radius;
        SDL_SetRenderDrawColor(renderer, 0, 255, 255, 180);
        for (int a = 0; a < 24; a++) {
            float a1 = (2.0f * (float)M_PI * a) / 24.0f;
            float a2 = (2.0f * (float)M_PI * (a + 1)) / 24.0f;
            SDL_RenderDrawLine(renderer, px + (int)(cosf(a1)*pr), py + (int)(sinf(a1)*pr),
                                         px + (int)(cosf(a2)*pr), py + (int)(sinf(a2)*pr));
        }
    }
    if (game->dev_menu.show_pickup_range) {
        int px = (int)(p->pos.x - camera.x);
        int py = (int)(p->pos.y - camera.y);
        int pr = (int)p->pickup_radius;
        SDL_SetRenderDrawColor(renderer, 50, 255, 100, 120);
        for (int a = 0; a < 48; a++) {
            float a1 = (2.0f * (float)M_PI * a) / 48.0f;
            float a2 = (2.0f * (float)M_PI * (a + 0.6f)) / 48.0f;
            SDL_RenderDrawLine(renderer, px + (int)(cosf(a1)*pr), py + (int)(sinf(a1)*pr),
                                         px + (int)(cosf(a2)*pr), py + (int)(sinf(a2)*pr));
        }
    }
    
    // 8. Player HUD & Route Banner
    // Top XP Bar
    int xp_h = 10;
    SDL_SetRenderDrawColor(renderer, 15, 20, 30, 230);
    SDL_Rect xp_bg = { 0, 0, WINDOW_WIDTH, xp_h };
    SDL_RenderFillRect(renderer, &xp_bg);
    float xp_frac = (float)p->xp / (float)p->xp_to_next_level;
    if (xp_frac > 1.0f) xp_frac = 1.0f;
    SDL_SetRenderDrawColor(renderer, 50, 210, 255, 255);
    SDL_Rect xp_fill = { 0, 0, (int)(WINDOW_WIDTH * xp_frac), xp_h };
    SDL_RenderFillRect(renderer, &xp_fill);

    // Top-Center Route Banner Card
    const RouteConfig* route = KantoRoutes_GetCurrentRoute(game);
    if (route) {
        int rcard_w = 420;
        int rcard_h = 56;
        int rcard_x = (WINDOW_WIDTH - rcard_w) / 2;
        int rcard_y = 16;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 15, 22, 34, 230);
        SDL_Rect r_bg = { rcard_x, rcard_y, rcard_w, rcard_h };
        SDL_RenderFillRect(renderer, &r_bg);
        SDL_SetRenderDrawColor(renderer, route->is_gym ? 230 : 60, route->is_gym ? 50 : 120, route->is_gym ? 50 : 200, 255);
        SDL_RenderDrawRect(renderer, &r_bg);

        // Section & Route title
        Font_DrawText(renderer, game->font_texture, route->section_name, rcard_x + 14, rcard_y + 8, 1, (SDL_Color){ 180, 200, 225, 255 });
        Font_DrawTextShadow(renderer, game->font_texture, route->name, rcard_x + 14, rcard_y + 22, 2, route->is_gym ? (SDL_Color){ 255, 80, 80, 255 } : (SDL_Color){ 255, 220, 70, 255 });

        // Timer & Kill Counter
        char r_stat[48];
        snprintf(r_stat, sizeof(r_stat), "TIME: %02d:%02d  KILLS: %d/%d", 
                 (int)game->route_timer / 60, (int)game->route_timer % 60,
                 game->route_kills, game->route_kill_quota);
        Font_DrawText(renderer, game->font_texture, r_stat, rcard_x + 14, rcard_y + 40, 1, (SDL_Color){ 200, 240, 255, 255 });

        // Route countdown bar
        int bar_w = 120;
        int bar_h = 8;
        int bar_x = rcard_x + rcard_w - bar_w - 14;
        int bar_y = rcard_y + 36;
        SDL_SetRenderDrawColor(renderer, 30, 40, 55, 255);
        SDL_Rect bar_bg = { bar_x, bar_y, bar_w, bar_h };
        SDL_RenderFillRect(renderer, &bar_bg);
        float time_frac = game->route_timer / (route->duration_sec > 0.0f ? route->duration_sec : 60.0f);
        if (time_frac < 0.0f) time_frac = 0.0f;
        SDL_SetRenderDrawColor(renderer, 80, 220, 255, 255);
        SDL_Rect bar_fill = { bar_x, bar_y, (int)(bar_w * time_frac), bar_h };
        SDL_RenderFillRect(renderer, &bar_fill);
    }

    // Floating Announcement Banner
    if (game->route_banner_timer > 0.0f) {
        int b_w = 460;
        int b_h = 36;
        int b_x = (WINDOW_WIDTH - b_w) / 2;
        int b_y = 78;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 15, 25, 230);
        SDL_Rect b_rect = { b_x, b_y, b_w, b_h };
        SDL_RenderFillRect(renderer, &b_rect);
        SDL_SetRenderDrawColor(renderer, 255, 215, 60, 255);
        SDL_RenderDrawRect(renderer, &b_rect);
        Font_DrawTextShadow(renderer, game->font_texture, game->route_banner_text, b_x + 20, b_y + 10, 2, (SDL_Color){ 255, 235, 90, 255 });
    }

    // Top-Left Player Status Bar Card
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 15, 20, 30, 225);
    SDL_Rect status_bg = { 14, 18, 300, 108 };
    SDL_RenderFillRect(renderer, &status_bg);
    SDL_SetRenderDrawColor(renderer, 50, 80, 120, 255);
    SDL_RenderDrawRect(renderer, &status_bg);
    
    char hbuf[64];
    snprintf(hbuf, sizeof(hbuf), "LVL %d   KILLS: %d   TIME: %02d:%02d", 
             p->level, p->kills, 
             (int)game->spawner.game_time / 60, (int)game->spawner.game_time % 60);
    Font_DrawText(renderer, game->font_texture, hbuf, 24, 26, 1, (SDL_Color){ 255, 220, 80, 255 });
    
    // HP Bar
    int hp_w = 278;
    int hp_h = 14;
    int hp_x = 24;
    int hp_y = 42;
    SDL_SetRenderDrawColor(renderer, 30, 15, 15, 255);
    SDL_Rect hp_bg_rect = { hp_x, hp_y, hp_w, hp_h };
    SDL_RenderFillRect(renderer, &hp_bg_rect);
    float hp_frac = p->hp / p->max_hp;
    if (hp_frac < 0.0f) hp_frac = 0.0f;
    SDL_SetRenderDrawColor(renderer, 230, 45, 45, 255);
    SDL_Rect hp_fill_rect = { hp_x, hp_y, (int)(hp_w * hp_frac), hp_h };
    SDL_RenderFillRect(renderer, &hp_fill_rect);
    
    snprintf(hbuf, sizeof(hbuf), "HP: %d / %d", (int)p->hp, (int)p->max_hp);
    Font_DrawText(renderer, game->font_texture, hbuf, hp_x + 6, hp_y + 3, 1, (SDL_Color){ 255, 255, 255, 255 });
    
    snprintf(hbuf, sizeof(hbuf), "XP: %d / %d   [F1] Dev Menu", p->xp, p->xp_to_next_level);
    Font_DrawText(renderer, game->font_texture, hbuf, 24, 61, 1, (SDL_Color){ 180, 200, 220, 255 });

    // Active Ability Badges: Dash [SHIFT] & Special [Q]
    int btn_y = 78;
    int dash_w = 135;
    int dash_x = 24;
    SDL_Rect dash_r = { dash_x, btn_y, dash_w, 20 };
    if (p->state == PLAYER_STATE_DASHING) {
        SDL_SetRenderDrawColor(renderer, 30, 110, 200, 240);
        SDL_RenderFillRect(renderer, &dash_r);
        Font_DrawText(renderer, game->font_texture, "[SHIFT] DASHING!", dash_x + 6, btn_y + 6, 1, (SDL_Color){ 255, 255, 255, 255 });
    } else if (p->dash_cooldown_timer > 0.0f) {
        SDL_SetRenderDrawColor(renderer, 24, 28, 38, 240);
        SDL_RenderFillRect(renderer, &dash_r);
        float cd_frac = 1.0f - (p->dash_cooldown_timer / p->dash_cooldown);
        if (cd_frac < 0.0f) cd_frac = 0.0f;
        SDL_SetRenderDrawColor(renderer, 40, 80, 120, 200);
        SDL_Rect bar_r = { dash_x, btn_y, (int)(dash_w * cd_frac), 20 };
        SDL_RenderFillRect(renderer, &bar_r);
        char cdt[32];
        snprintf(cdt, sizeof(cdt), "DASH: %.1fs", p->dash_cooldown_timer);
        Font_DrawText(renderer, game->font_texture, cdt, dash_x + 18, btn_y + 6, 1, (SDL_Color){ 160, 175, 195, 255 });
    } else {
        SDL_SetRenderDrawColor(renderer, 20, 60, 75, 240);
        SDL_RenderFillRect(renderer, &dash_r);
        Font_DrawText(renderer, game->font_texture, "[SHIFT] DASH", dash_x + 16, btn_y + 6, 1, (SDL_Color){ 100, 240, 255, 255 });
    }
    
    int q_w = 135;
    int q_x = 167;
    SDL_Rect q_r = { q_x, btn_y, q_w, 20 };
    const char* q_name = (p->starter == STARTER_BULBASAUR) ? "SOLAR BEAM" : ((p->starter == STARTER_CHARMANDER) ? "FLAME" : "HYDRO PUMP");
    if (!p->special_q_unlocked) {
        SDL_SetRenderDrawColor(renderer, 20, 22, 30, 200);
        SDL_RenderFillRect(renderer, &q_r);
        Font_DrawText(renderer, game->font_texture, "[Q] LOCKED", q_x + 24, btn_y + 6, 1, (SDL_Color){ 110, 125, 145, 255 });
    } else if (p->state == PLAYER_STATE_HYDRO_PUMP || p->state == PLAYER_STATE_SOLAR_BEAM || p->state == PLAYER_STATE_FLAMETHROWER) {
        SDL_SetRenderDrawColor(renderer, 20, 90, 180, 240);
        SDL_RenderFillRect(renderer, &q_r);
        char active_str[32];
        snprintf(active_str, sizeof(active_str), "* %s *", q_name);
        Font_DrawText(renderer, game->font_texture, active_str, q_x + 8, btn_y + 6, 1, (SDL_Color){ 255, 245, 140, 255 });
    } else if (p->special_q_cooldown_timer > 0.0f) {
        SDL_SetRenderDrawColor(renderer, 24, 28, 38, 240);
        SDL_RenderFillRect(renderer, &q_r);
        float cd_frac = 1.0f - (p->special_q_cooldown_timer / p->special_q_cooldown);
        if (cd_frac < 0.0f) cd_frac = 0.0f;
        SDL_SetRenderDrawColor(renderer, 30, 75, 130, 200);
        SDL_Rect bar_r = { q_x, btn_y, (int)(q_w * cd_frac), 20 };
        SDL_RenderFillRect(renderer, &bar_r);
        char cdt[32];
        snprintf(cdt, sizeof(cdt), "CD: %.1fs", p->special_q_cooldown_timer);
        Font_DrawText(renderer, game->font_texture, cdt, q_x + 24, btn_y + 6, 1, (SDL_Color){ 160, 175, 195, 255 });
    } else {
        SDL_SetRenderDrawColor(renderer, 15, 50, 95, 240);
        SDL_RenderFillRect(renderer, &q_r);
        char ready_str[32];
        snprintf(ready_str, sizeof(ready_str), "[Q] %s", q_name);
        Font_DrawText(renderer, game->font_texture, ready_str, q_x + 8, btn_y + 6, 1, (SDL_Color){ 120, 230, 255, 255 });
    }

    // Active Weapon Inventory Badges (bottom HUD)
    int wep_start_x = 316;
    for (int w = 0; w < MAX_ACTIVE_WEAPONS; w++) {
        int wx = wep_start_x + w * 120;
        SDL_Rect wr = { wx, btn_y, 114, 20 };
        if (w < p->weapon_count && p->weapons[w].type != WEAPON_NONE) {
            ActiveWeaponSlot* slot = &p->weapons[w];
            bool evolved = Weapons_IsEvolved(slot->type);
            SDL_Color wc = Weapons_GetColor(slot->type);
            SDL_SetRenderDrawColor(renderer, evolved ? 45 : 20, evolved ? 35 : 25, evolved ? 10 : 35, 230);
            SDL_RenderFillRect(renderer, &wr);
            SDL_SetRenderDrawColor(renderer, wc.r, wc.g, wc.b, 200);
            SDL_RenderDrawRect(renderer, &wr);
            char wtxt[32];
            if (evolved) {
                snprintf(wtxt, sizeof(wtxt), "* %s *", Weapons_GetName(slot->type));
            } else {
                snprintf(wtxt, sizeof(wtxt), "%s L%d", Weapons_GetName(slot->type), slot->level);
            }
            Font_DrawText(renderer, game->font_texture, wtxt, wx + 6, btn_y + 6, 1, wc);
        } else {
            SDL_SetRenderDrawColor(renderer, 15, 18, 24, 180);
            SDL_RenderFillRect(renderer, &wr);
            Font_DrawText(renderer, game->font_texture, "[EMPTY]", wx + 28, btn_y + 6, 1, (SDL_Color){ 90, 105, 120, 200 });
        }
    }

    // -------------------------------------------------------------
    // HUD: PARTY ROSTER (UP TO 3 POKEMON) & POKÉ BALL INVENTORY
    // -------------------------------------------------------------
    int party_x = 810;
    int party_y = 16;
    int party_slot_w = 145;
    int party_slot_h = 36;

    for (int i = 0; i < MAX_PARTY_MEMBERS; i++) {
        int sx = party_x + i * (party_slot_w + 8);
        SDL_Rect sr = { sx, party_y, party_slot_w, party_slot_h };
        if (i < game->party_count && game->party[i].active) {
            PartyMember* m = &game->party[i];
            bool is_starter = (i == 0);
            SDL_Color tcol = Pokedex_GetTypeColor(m->type1);
            SDL_SetRenderDrawColor(renderer, is_starter ? 20 : 16, is_starter ? 30 : 36, is_starter ? 45 : 24, 235);
            SDL_RenderFillRect(renderer, &sr);
            SDL_SetRenderDrawColor(renderer, is_starter ? 80 : 50, is_starter ? 170 : 230, is_starter ? 255 : 120, 220);
            SDL_RenderDrawRect(renderer, &sr);

            char p_title[32];
            snprintf(p_title, sizeof(p_title), "%s %s", is_starter ? "*" : "+", m->name);
            Font_DrawText(renderer, game->font_texture, p_title, sx + 6, party_y + 4, 1, (SDL_Color){ 255, 255, 255, 255 });

            char p_type[32];
            if (m->type2 != POKE_TYPE_NONE) {
                snprintf(p_type, sizeof(p_type), "%s / %s", Pokedex_GetTypeName(m->type1), Pokedex_GetTypeName(m->type2));
            } else {
                snprintf(p_type, sizeof(p_type), "TYPE: %s", Pokedex_GetTypeName(m->type1));
            }
            Font_DrawText(renderer, game->font_texture, p_type, sx + 6, party_y + 20, 1, tcol);
        } else {
            SDL_SetRenderDrawColor(renderer, 15, 18, 24, 180);
            SDL_RenderFillRect(renderer, &sr);
            SDL_SetRenderDrawColor(renderer, 45, 55, 70, 180);
            SDL_RenderDrawRect(renderer, &sr);
            Font_DrawText(renderer, game->font_texture, "[EMPTY PARTY]", sx + 12, party_y + 13, 1, (SDL_Color){ 95, 110, 125, 200 });
        }
    }

    // Poké Ball Inventory Box & Capture Prompt
    int pb_x = 810;
    int pb_y = 58;
    int pb_w = 451;
    int pb_h = 24;
    SDL_Rect pb_r = { pb_x, pb_y, pb_w, pb_h };
    SDL_SetRenderDrawColor(renderer, 24, 20, 28, 235);
    SDL_RenderFillRect(renderer, &pb_r);
    SDL_SetRenderDrawColor(renderer, 220, 70, 70, 220);
    SDL_RenderDrawRect(renderer, &pb_r);

    char pb_str[96];
    snprintf(pb_str, sizeof(pb_str), "POKÉ BALLS: %d/2   [E / R-Click] Throw Ball   (Target HP < 35%%)", p->pokeball_count);
    Font_DrawText(renderer, game->font_texture, pb_str, pb_x + 10, pb_y + 7, 1, 
        (p->pokeball_count > 0) ? (SDL_Color){ 255, 225, 90, 255 } : (SDL_Color){ 160, 160, 160, 255 });

    // Boss HP Bar (if active gym boss)
    for (int k = 0; k < game->active_enemy_count; k++) {
        int e_idx = game->active_enemy_indices[k];
        Enemy* e = &game->enemies[e_idx];
        if (game->enemy_types[e->type].is_boss) {
            int bbar_w = 480;
            int bbar_h = 20;
            int bbar_x = (WINDOW_WIDTH - bbar_w) / 2;
            int bbar_y = 118;
            SDL_SetRenderDrawColor(renderer, 20, 15, 20, 235);
            SDL_Rect b_bg = { bbar_x, bbar_y, bbar_w, bbar_h };
            SDL_RenderFillRect(renderer, &b_bg);
            float b_frac = e->hp / e->max_hp;
            if (b_frac < 0.0f) b_frac = 0.0f;
            SDL_SetRenderDrawColor(renderer, 240, 40, 40, 255);
            SDL_Rect b_fill = { bbar_x, bbar_y, (int)(bbar_w * b_frac), bbar_h };
            SDL_RenderFillRect(renderer, &b_fill);
            SDL_SetRenderDrawColor(renderer, 255, 215, 60, 255);
            SDL_RenderDrawRect(renderer, &b_bg);

            char boss_hp_str[64];
            snprintf(boss_hp_str, sizeof(boss_hp_str), "%s : %d / %d HP", game->enemy_types[e->type].name, (int)e->hp, (int)e->max_hp);
            Font_DrawTextShadow(renderer, game->font_texture, boss_hp_str, bbar_x + 12, bbar_y + 4, 1, (SDL_Color){ 255, 255, 255, 255 });
            break;
        }
    }

    // -------------------------------------------------------------
    // MODAL: LEVEL UP CHOICES MODAL (PAUSES GAME)
    // -------------------------------------------------------------
    if (game->app_state == APP_STATE_LEVEL_UP) {
        // Dark translucent overlay
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 8, 12, 18, 220);
        SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(renderer, &full);

        // Header Title
        Font_DrawTextShadow(renderer, game->font_texture, "LEVEL UP! CHOOSE A NEW MOVE / UPGRADE", 340, 140, 3, (SDL_Color){ 255, 220, 60, 255 });
        Font_DrawText(renderer, game->font_texture, "Click a card or press [1], [2], or [3] to select your progression", 400, 185, 1, (SDL_Color){ 200, 220, 240, 255 });

        int card_w = 300;
        int card_h = 320;
        int card_y = 220;
        int card_xs[3] = { 160, 490, 820 };

        for (int i = 0; i < 3; i++) {
            LevelUpCard* card = &game->level_up_cards[i];
            int cx = card_xs[i];
            bool hovered = (game->selected_card_hover == i);

            SDL_Rect c_rect = { cx, card_y, card_w, card_h };
            SDL_SetRenderDrawColor(renderer, hovered ? 28 : 18, hovered ? 38 : 24, hovered ? 55 : 34, 255);
            SDL_RenderFillRect(renderer, &c_rect);

            SDL_Color border_col = hovered ? card->color : (SDL_Color){ 60, 80, 110, 255 };
            SDL_SetRenderDrawColor(renderer, border_col.r, border_col.g, border_col.b, 255);
            SDL_RenderDrawRect(renderer, &c_rect);
            if (hovered) {
                SDL_Rect inner = { cx + 1, card_y + 1, card_w - 2, card_h - 2 };
                SDL_RenderDrawRect(renderer, &inner);
            }

            // Tag badge
            int tag_w = 90;
            int tag_h = 22;
            SDL_Rect tag_r = { cx + 16, card_y + 18, tag_w, tag_h };
            SDL_SetRenderDrawColor(renderer, card->color.r, card->color.g, card->color.b, 200);
            SDL_RenderFillRect(renderer, &tag_r);
            Font_DrawText(renderer, game->font_texture, card->tag, cx + 22, card_y + 24, 1, (SDL_Color){ 10, 15, 25, 255 });

            // Title
            Font_DrawTextShadow(renderer, game->font_texture, card->title, cx + 16, card_y + 54, 2, card->color);

            // Description (wrap manually)
            char line1[48] = {0};
            char line2[48] = {0};
            int break_pt = 32;
            int dlen = (int)strlen(card->desc);
            if (dlen > break_pt) {
                while (break_pt > 0 && card->desc[break_pt] != ' ') break_pt--;
                strncpy(line1, card->desc, break_pt);
                strncpy(line2, card->desc + break_pt + 1, sizeof(line2) - 1);
            } else {
                strncpy(line1, card->desc, sizeof(line1) - 1);
            }
            Font_DrawText(renderer, game->font_texture, line1, cx + 16, card_y + 105, 1, (SDL_Color){ 200, 215, 235, 255 });
            Font_DrawText(renderer, game->font_texture, line2, cx + 16, card_y + 125, 1, (SDL_Color){ 200, 215, 235, 255 });

            // Bottom Select Button
            int btn_h = 36;
            int btn_y = card_y + card_h - 52;
            SDL_Rect btn_r = { cx + 16, btn_y, card_w - 32, btn_h };
            if (hovered) {
                SDL_SetRenderDrawColor(renderer, card->color.r, card->color.g, card->color.b, 255);
                SDL_RenderFillRect(renderer, &btn_r);
                char btxt[32];
                snprintf(btxt, sizeof(btxt), "[%d] LEARN MOVE", i + 1);
                Font_DrawText(renderer, game->font_texture, btxt, cx + 65, btn_y + 11, 2, (SDL_Color){ 10, 15, 25, 255 });
            } else {
                SDL_SetRenderDrawColor(renderer, 30, 42, 60, 255);
                SDL_RenderFillRect(renderer, &btn_r);
                SDL_SetRenderDrawColor(renderer, 60, 80, 110, 255);
                SDL_RenderDrawRect(renderer, &btn_r);
                char btxt[32];
                snprintf(btxt, sizeof(btxt), "[%d] SELECT", i + 1);
                Font_DrawText(renderer, game->font_texture, btxt, cx + 85, btn_y + 11, 2, (SDL_Color){ 190, 210, 230, 255 });
            }
        }
    }

    // -------------------------------------------------------------
    // MODAL: GAME OVER / VICTORY OVERLAY
    // -------------------------------------------------------------
    if (game->app_state == APP_STATE_GAME_OVER) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 25, 8, 8, 230);
        SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(renderer, &full);

        Font_DrawTextShadow(renderer, game->font_texture, "YOUR POKEMON FAINTED!", 410, 220, 3, (SDL_Color){ 255, 60, 60, 255 });
        char stats[64];
        snprintf(stats, sizeof(stats), "Level: %d  |  Total Kills: %d  |  Route: %s", p->level, p->kills, route ? route->name : "Kanto");
        Font_DrawText(renderer, game->font_texture, stats, 430, 280, 1, (SDL_Color){ 240, 200, 200, 255 });
        Font_DrawTextShadow(renderer, game->font_texture, "Press [R] or [SPACE] to return to Starter Selection", 370, 340, 2, (SDL_Color){ 255, 230, 80, 255 });
    } else if (game->app_state == APP_STATE_GAME_VICTORY) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 15, 20, 10, 230);
        SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(renderer, &full);

        Font_DrawTextShadow(renderer, game->font_texture, "KANTO CHAMPION! HALL OF FAME!", 350, 200, 3, (SDL_Color){ 255, 215, 50, 255 });
        Font_DrawText(renderer, game->font_texture, "You conquered all 8 Gym Badges and defeated the legendary Mewtwo!", 350, 260, 1, (SDL_Color){ 220, 245, 200, 255 });
        char stats[64];
        snprintf(stats, sizeof(stats), "Final Level: %d  |  Total Kills: %d", p->level, p->kills);
        Font_DrawText(renderer, game->font_texture, stats, 480, 290, 1, (SDL_Color){ 255, 255, 255, 255 });
        Font_DrawTextShadow(renderer, game->font_texture, "Press [R] or [SPACE] to Play Again", 460, 350, 2, (SDL_Color){ 255, 230, 80, 255 });
    }

    // 9. Draw Profiler Overlay
    Profiler_DrawOverlay(game);
    
    // 10. Draw Dev Menu (on top of everything)
    DevMenu_Draw(game);
    
    game->profiler.t_render = Profiler_ElapsedMs(t_r_start);
}

void Game_Cleanup(GameState* game) {
    if (game->font_texture) { SDL_DestroyTexture(game->font_texture); game->font_texture = NULL; }
    if (game->tileset_texture) { SDL_DestroyTexture(game->tileset_texture); game->tileset_texture = NULL; }
    if (game->floor_cache_texture) { SDL_DestroyTexture(game->floor_cache_texture); game->floor_cache_texture = NULL; }
    if (game->player.sprite_charge.texture) { SDL_DestroyTexture(game->player.sprite_charge.texture); game->player.sprite_charge.texture = NULL; }
    if (game->player.sprite_special.texture) { SDL_DestroyTexture(game->player.sprite_special.texture); game->player.sprite_special.texture = NULL; }
    game->player.sprite.texture = NULL; // Texture is owned and managed by Pokedex cache
}
