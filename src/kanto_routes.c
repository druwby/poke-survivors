#include "kanto_routes.h"
#include "game.h"
#include "pokedex.h"
#include "sprite.h"
#include "image_loader.h"
#include "telemetry.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const RouteConfig s_sections[8][4] = {
    // Section 0: Pewter City (Brock - Boulder Badge)
    {
        { "ROUTE 1", "SECTION 1 - BOULDER BADGE", { 16, 19 }, 2, 0, NULL, 60.0f, 35, false }, // Pidgey, Rattata
        { "ROUTE 2", "SECTION 1 - BOULDER BADGE", { 10, 13, 29, 32 }, 4, 0, NULL, 60.0f, 40, false }, // Caterpie, Weedle, NidoranF, NidoranM
        { "VIRIDIAN FOREST", "SECTION 1 - BOULDER BADGE", { 11, 14, 12, 15, 25 }, 5, 0, NULL, 60.0f, 45, false }, // Metapod, Kakuna, Butterfree, Beedrill, Pikachu
        { "PEWTER GYM - BROCK", "SECTION 1 - BOULDER BADGE", { 74, 27, 41 }, 3, 95, "LEADER BROCK'S ONIX", 90.0f, 50, true }, // Geodude, Sandshrew, Zubat -> Onix
    },
    // Section 1: Cerulean City (Misty - Cascade Badge)
    {
        { "ROUTE 3", "SECTION 2 - CASCADE BADGE", { 21, 23, 56, 39 }, 4, 0, NULL, 60.0f, 40, false }, // Spearow, Ekans, Mankey, Jigglypuff
        { "MT. MOON", "SECTION 2 - CASCADE BADGE", { 41, 74, 46, 35 }, 4, 0, NULL, 60.0f, 45, false }, // Zubat, Geodude, Paras, Clefairy
        { "ROUTE 24 - NUGGET BRIDGE", "SECTION 2 - CASCADE BADGE", { 69, 43, 63, 17 }, 4, 0, NULL, 60.0f, 45, false }, // Bellsprout, Oddish, Abra, Pidgeotto
        { "CERULEAN GYM - MISTY", "SECTION 2 - CASCADE BADGE", { 118, 54, 60, 86 }, 4, 121, "LEADER MISTY'S STARMIE", 90.0f, 50, true }, // Goldeen, Psyduck, Poliwag, Seel -> Starmie
    },
    // Section 2: Vermilion City (Lt. Surge - Thunder Badge)
    {
        { "ROUTE 6", "SECTION 3 - THUNDER BADGE", { 52, 37, 58 }, 3, 0, NULL, 60.0f, 40, false }, // Meowth, Vulpix, Growlithe
        { "ROUTE 11", "SECTION 3 - THUNDER BADGE", { 96, 20, 21 }, 3, 0, NULL, 60.0f, 40, false }, // Drowzee, Raticate, Spearow
        { "DIGLETT'S CAVE", "SECTION 3 - THUNDER BADGE", { 50, 51 }, 2, 0, NULL, 60.0f, 40, false }, // Diglett, Dugtrio
        { "VERMILION GYM - LT. SURGE", "SECTION 3 - THUNDER BADGE", { 100, 81, 25 }, 3, 26, "LT. SURGE'S RAICHU", 90.0f, 50, true }, // Voltorb, Magnemite, Pikachu -> Raichu
    },
    // Section 3: Celadon City (Erika - Rainbow Badge)
    {
        { "ROUTE 9", "SECTION 4 - RAINBOW BADGE", { 22, 66, 81 }, 3, 0, NULL, 60.0f, 40, false }, // Fearow, Machop, Magnemite
        { "ROCK TUNNEL", "SECTION 4 - RAINBOW BADGE", { 74, 75, 66, 95 }, 4, 0, NULL, 60.0f, 45, false }, // Geodude, Graveler, Machop, Onix
        { "ROUTE 7 & 8", "SECTION 4 - RAINBOW BADGE", { 37, 52, 58, 17 }, 4, 0, NULL, 60.0f, 45, false }, // Vulpix, Meowth, Growlithe, Pidgeotto
        { "CELADON GYM - ERIKA", "SECTION 4 - RAINBOW BADGE", { 114, 44, 70 }, 3, 45, "LEADER ERIKA'S VILEPLUME", 90.0f, 50, true }, // Tangela, Gloom, Weepinbell -> Vileplume
    },
    // Section 4: Fuchsia City (Koga - Soul Badge)
    {
        { "POKEMON TOWER", "SECTION 5 - SOUL BADGE", { 92, 93, 104 }, 3, 0, NULL, 60.0f, 40, false }, // Gastly, Haunter, Cubone
        { "ROUTE 12", "SECTION 5 - SOUL BADGE", { 48, 72, 79, 98 }, 4, 0, NULL, 60.0f, 45, false }, // Venonat, Tentacool, Slowpoke, Krabby
        { "SAFARI ZONE", "SECTION 5 - SOUL BADGE", { 102, 111, 115, 123, 127, 128 }, 6, 0, NULL, 60.0f, 50, false }, // Exeggcute, Rhyhorn, Kangaskhan, Scyther, Pinsir, Tauros
        { "FUCHSIA GYM - KOGA", "SECTION 5 - SOUL BADGE", { 109, 89, 49 }, 3, 110, "LEADER KOGA'S WEEZING", 90.0f, 50, true }, // Koffing, Muk, Venomoth -> Weezing
    },
    // Section 5: Saffron City (Sabrina - Marsh Badge)
    {
        { "SILPH CO.", "SECTION 6 - MARSH BADGE", { 88, 101, 82, 137 }, 4, 0, NULL, 60.0f, 45, false }, // Grimer, Electrode, Magneton, Porygon
        { "ROUTE 14", "SECTION 6 - MARSH BADGE", { 18, 49, 24 }, 3, 0, NULL, 60.0f, 40, false }, // Pidgeot, Venomoth, Arbok
        { "FIGHTING DOJO", "SECTION 6 - MARSH BADGE", { 106, 107, 67 }, 3, 0, NULL, 60.0f, 40, false }, // Hitmonlee, Hitmonchan, Machoke
        { "SAFFRON GYM - SABRINA", "SECTION 6 - MARSH BADGE", { 64, 122, 97 }, 3, 65, "LEADER SABRINA'S ALAKAZAM", 90.0f, 50, true }, // Kadabra, Mr. Mime, Hypno -> Alakazam
    },
    // Section 6: Cinnabar Island (Blaine - Volcano Badge)
    {
        { "SEAFOAM ISLANDS", "SECTION 7 - VOLCANO BADGE", { 86, 87, 90, 91, 55, 144 }, 6, 0, NULL, 60.0f, 50, false }, // Seel, Dewgong, Shellder, Cloyster, Golduck, Articuno
        { "POKEMON MANSION", "SECTION 7 - VOLCANO BADGE", { 77, 78, 109, 132 }, 4, 0, NULL, 60.0f, 45, false }, // Ponyta, Rapidash, Koffing, Ditto
        { "ROUTE 20 WATERWAY", "SECTION 7 - VOLCANO BADGE", { 129, 130, 131, 134 }, 4, 0, NULL, 60.0f, 45, false }, // Magikarp, Gyarados, Lapras, Vaporeon
        { "CINNABAR GYM - BLAINE", "SECTION 7 - VOLCANO BADGE", { 58, 59, 77, 78 }, 4, 126, "LEADER BLAINE'S MAGMAR", 90.0f, 50, true }, // Growlithe, Arcanine, Ponyta, Rapidash -> Magmar
    },
    // Section 7: Viridian City & Indigo Plateau (Giovanni & Champion - Earth Badge & Hall of Fame)
    {
        { "ROUTE 21", "SECTION 8 - EARTH BADGE & FINALE", { 73, 99, 119, 130 }, 4, 0, NULL, 60.0f, 45, false }, // Tentacruel, Kingler, Seaking, Gyarados
        { "VICTORY ROAD", "SECTION 8 - EARTH BADGE & FINALE", { 68, 76, 105, 145, 146 }, 5, 0, NULL, 60.0f, 50, false }, // Machamp, Golem, Marowak, Zapdos, Moltres
        { "VIRIDIAN GYM - GIOVANNI", "SECTION 8 - EARTH BADGE & FINALE", { 51, 31, 34 }, 3, 112, "LEADER GIOVANNI'S RHYDON", 90.0f, 50, true }, // Dugtrio, Nidoqueen, Nidoking -> Rhydon
        { "INDIGO PLATEAU - CHAMPION", "FINALE - HALL OF FAME", { 149, 130, 94, 143 }, 4, 150, "LEGENDARY ACE MEWTWO", 120.0f, 60, true }, // Dragonite, Gyarados, Gengar, Snorlax -> MEWTWO
    }
};

static const int s_section_route_counts[8] = { 4, 4, 4, 4, 4, 4, 4, 4 };

int KantoRoutes_GetTotalSections(void) {
    return 8;
}

int KantoRoutes_GetRoutesInSection(int section_idx) {
    if (section_idx < 0 || section_idx >= 8) return 0;
    return s_section_route_counts[section_idx];
}

const RouteConfig* KantoRoutes_GetCurrentRoute(const GameState* game) {
    int s = game->current_section;
    int r = game->current_route_index;
    if (s < 0 || s >= 8 || r < 0 || r >= s_section_route_counts[s]) return NULL;
    return &s_sections[s][r];
}

void KantoRoutes_Init(GameState* game) {
    game->current_section = 0;
    game->current_route_index = 0;
    KantoRoutes_LoadRoute(game, 0, 0);
}

void KantoRoutes_LoadRoute(GameState* game, int section_idx, int route_idx) {
    if (section_idx >= 8) {
        game->app_state = APP_STATE_GAME_VICTORY;
        return;
    }
    if (route_idx >= s_section_route_counts[section_idx]) {
        section_idx++;
        route_idx = 0;
        if (section_idx >= 8) {
            game->app_state = APP_STATE_GAME_VICTORY;
            return;
        }
    }

    game->current_section = section_idx;
    game->current_route_index = route_idx;
    const RouteConfig* route = &s_sections[section_idx][route_idx];

    game->route_timer = route->duration_sec;
    game->route_kills = 0;
    game->route_kill_quota = route->kill_quota;
    game->gym_boss_spawned = false;

    snprintf(game->route_banner_text, sizeof(game->route_banner_text), "%s", route->name);
    game->route_banner_timer = 3.5f;

    // Generate thematic rich tilemap for this route
    Tilemap_GenerateRouteMap(&game->tilemap, section_idx, route_idx);

    // Load active Pokemon for this route into game->enemy_types
    int type_slot = 0;
    for (int i = 0; i < route->pokemon_count && type_slot < MAX_ENEMY_TYPES - 1; i++) {
        int pid = route->pokemon_ids[i];
        const PokedexEntry* entry = Pokedex_GetEntry(pid);
        if (!entry) continue;

        EnemyTypeConfig* cfg = &game->enemy_types[type_slot];
        cfg->pokedex_id = pid;
        strncpy(cfg->name, entry->name, sizeof(cfg->name) - 1);
        cfg->name[sizeof(cfg->name) - 1] = '\0';
        
        // Dynamic route scaling and stage defense
        float route_hp_scale = (1.0f + (float)section_idx * 1.8f + (float)route_idx * 0.55f) * g_balance_config.route_hp_scaling;
        cfg->max_hp = entry->base_hp * g_balance_config.enemy_hp_mult * route_hp_scale;
        cfg->defense = g_balance_config.enemy_defense_base + (float)(entry->stage - 1) * g_balance_config.enemy_defense_per_stage;
        if (cfg->defense < 0.0f) cfg->defense = 0.0f;
        if (cfg->defense > 0.70f) cfg->defense = 0.70f;

        cfg->speed = entry->base_speed * 0.48f; // Paced movement speed (readable kiting)
        cfg->touch_damage = entry->touch_damage;
        cfg->radius = entry->radius;
        cfg->xp_value = entry->xp_value;
        cfg->knockback_resist = entry->knockback_resist;
        cfg->spawn_weight = 10.0f;
        cfg->sprite_scale = entry->scale;
        cfg->debug_color = Pokedex_GetTypeColor(entry->type1);
        cfg->sprite = Pokedex_LoadSprite(game->renderer, pid, entry->scale);
        cfg->attack_type = entry->attack_type;
        cfg->attack_range = entry->attack_range;
        cfg->attack_cooldown = entry->attack_cooldown;
        cfg->attack_windup = entry->attack_windup;
        cfg->attack_duration = 0.35f;
        cfg->attack_damage = entry->attack_damage;
        cfg->attack_radius = entry->attack_radius;
        cfg->is_boss = false;

        // Assign movement archetypes based on Pokemon traits
        if (pid == 50 || pid == 51) {
            cfg->move_pattern = ENEMY_MOVE_BURROW;
        } else if (pid == 11 || pid == 14) {
            cfg->move_pattern = ENEMY_MOVE_STATIONARY;
        } else if (entry->type1 == POKE_TYPE_FLYING || entry->type2 == POKE_TYPE_FLYING) {
            cfg->move_pattern = ENEMY_MOVE_FLANK_ORBIT;
        } else if (pid == 15 || pid == 56 || pid == 66 || pid == 111 || pid == 128 || pid == 127) {
            cfg->move_pattern = ENEMY_MOVE_CHARGER;
        } else if (pid == 43 || pid == 44 || pid == 69 || pid == 70 || pid == 102 || pid == 109) {
            cfg->move_pattern = ENEMY_MOVE_MORTAR;
        } else {
            cfg->move_pattern = ENEMY_MOVE_SWARM;
        }

        // Custom attack sprites for Diglett and Snorlax
        cfg->has_attack_sprite = false;
        if (pid == 50 || pid == 51) {
            char p[256];
            snprintf(p, sizeof(p), "assets/sprites/sprite/%04d/Swing-Anim.png", pid);
            int swing_h = (pid == 51) ? 80 : 72;
            cfg->sprite_attack = Sprite_Load(game->renderer, p, 72, swing_h, 9, 8, 0.042f, cfg->sprite_scale);
            cfg->has_attack_sprite = cfg->sprite_attack.loaded;
        } else if (pid == 143) {
            cfg->sprite_attack = Sprite_Load(game->renderer, "assets/sprites/sprite/0143/Stomp-Anim.png", 80, 104, 14, 8, 0.040f, cfg->sprite_scale);
            cfg->has_attack_sprite = cfg->sprite_attack.loaded;
        }

        type_slot++;
    }

    // If Gym wave, configure boss in the last slot
    if (route->is_gym && route->boss_pokemon_id > 0) {
        int bpid = route->boss_pokemon_id;
        const PokedexEntry* b_entry = Pokedex_GetEntry(bpid);
        if (b_entry) {
            EnemyTypeConfig* boss_cfg = &game->enemy_types[type_slot];
            boss_cfg->pokedex_id = bpid;
            strncpy(boss_cfg->name, route->boss_name ? route->boss_name : b_entry->name, sizeof(boss_cfg->name) - 1);
            boss_cfg->name[sizeof(boss_cfg->name) - 1] = '\0';
            float boss_hp_mult = (22.0f + (float)section_idx * 5.0f) * g_balance_config.boss_hp_mult;
            boss_cfg->max_hp = b_entry->base_hp * boss_hp_mult;
            boss_cfg->defense = 0.35f; // Thick armor for epic gym boss encounter
            boss_cfg->speed = b_entry->base_speed * 0.40f;
            boss_cfg->touch_damage = b_entry->touch_damage * 1.5f;
            boss_cfg->radius = b_entry->radius * 1.35f;
            boss_cfg->xp_value = 120;
            boss_cfg->knockback_resist = 0.96f; // Immovable titan
            boss_cfg->spawn_weight = 0.0f; // Boss is spawned specifically
            boss_cfg->sprite_scale = b_entry->scale * 1.35f;
            boss_cfg->debug_color = (SDL_Color){ 230, 40, 40, 255 };
            boss_cfg->sprite = Pokedex_LoadSprite(game->renderer, bpid, boss_cfg->sprite_scale);
            boss_cfg->attack_type = b_entry->attack_type;
            boss_cfg->attack_range = b_entry->attack_range * 1.25f;
            boss_cfg->attack_cooldown = b_entry->attack_cooldown * 0.90f;
            boss_cfg->attack_windup = b_entry->attack_windup;
            boss_cfg->attack_duration = 0.40f;
            boss_cfg->attack_damage = b_entry->attack_damage * 1.4f;
            boss_cfg->attack_radius = b_entry->attack_radius * 1.3f;
            boss_cfg->is_boss = true;
            boss_cfg->move_pattern = ENEMY_MOVE_SWARM;
            boss_cfg->has_attack_sprite = false;

            if (bpid == 95) { // Onix ground stomp
                boss_cfg->attack_type = ENEMY_ATTACK_SLAM_AOE;
            } else if (bpid == 143) {
                boss_cfg->sprite_attack = Sprite_Load(game->renderer, "assets/sprites/sprite/0143/Stomp-Anim.png", 80, 104, 14, 8, 0.040f, boss_cfg->sprite_scale);
                boss_cfg->has_attack_sprite = boss_cfg->sprite_attack.loaded;
            }

            type_slot++;
        }
    }

    game->active_enemy_type_count = type_slot;

    // Dynamic difficulty curve based on section and route index (Tuned for clean horde pacing)
    if (section_idx == 0 && route_idx == 0) {
        game->spawner.spawn_interval = 1.6f;
        game->spawner.batch_size = 1;
        game->spawner.max_enemies = 18;
    } else if (section_idx == 0 && route_idx == 1) {
        game->spawner.spawn_interval = 1.35f;
        game->spawner.batch_size = 1;
        game->spawner.max_enemies = 26;
    } else if (section_idx == 0 && route_idx == 2) {
        game->spawner.spawn_interval = 1.20f;
        game->spawner.batch_size = 2;
        game->spawner.max_enemies = 34;
    } else if (route->is_gym) {
        game->spawner.spawn_interval = 2.0f;
        game->spawner.batch_size = 1;
        game->spawner.max_enemies = 18;
    } else {
        float sec_factor = 1.0f - (float)section_idx * 0.04f;
        if (sec_factor < 0.65f) sec_factor = 0.65f;
        game->spawner.spawn_interval = 1.15f * sec_factor;
        game->spawner.batch_size = 2;
        game->spawner.max_enemies = 35 + section_idx * 6; // Caps at 35-75 enemies
    }

    // Apply global balance configuration multipliers
    game->spawner.spawn_interval *= g_balance_config.spawn_interval_mult;
    game->spawner.max_enemies = (int)((float)game->spawner.max_enemies * g_balance_config.max_enemies_mult);
    if (game->spawner.max_enemies < 5) game->spawner.max_enemies = 5;
    if (game->spawner.max_enemies > MAX_ENEMIES) game->spawner.max_enemies = MAX_ENEMIES;
}

void KantoRoutes_AdvanceRoute(GameState* game) {
    // Clear out active enemies from the completed route
    for (int i = 0; i < MAX_ENEMIES; i++) {
        game->enemies[i].active = false;
    }
    game->active_enemy_count = 0;

    // Milestone Reward: Award +1 Poké Ball on route/milestone completion (max 2 held)
    if (game->player.pokeball_count < 2) {
        game->player.pokeball_count++;
    }

    int s = game->current_section;
    int r = game->current_route_index;
    r++;
    if (r >= s_section_route_counts[s]) {
        s++;
        r = 0;
    }
    KantoRoutes_LoadRoute(game, s, r);
}

void KantoRoutes_Update(GameState* game, float dt) {
    if (game->route_banner_timer > 0.0f) {
        game->route_banner_timer -= dt;
    }

    const RouteConfig* route = KantoRoutes_GetCurrentRoute(game);
    if (!route) return;

    game->route_timer -= dt;
    if (game->route_timer < 0.0f) game->route_timer = 0.0f;

    // Gym wave mechanics
    if (route->is_gym) {
        // Spawn boss when kill milestone or timer triggers
        if (!game->gym_boss_spawned && (game->route_kills >= 20 || game->route_timer <= 45.0f)) {
            // Find the boss slot in enemy_types
            int boss_slot = -1;
            for (int i = 0; i < game->active_enemy_type_count; i++) {
                if (game->enemy_types[i].is_boss) {
                    boss_slot = i;
                    break;
                }
            }
            if (boss_slot >= 0) {
                // Find a free enemy slot
                for (int e = 0; e < MAX_ENEMIES; e++) {
                    if (!game->enemies[e].active) {
                        Enemy* b = &game->enemies[e];
                        memset(b, 0, sizeof(Enemy));
                        EnemyTypeConfig* cfg = &game->enemy_types[boss_slot];
                        b->type = boss_slot;
                        b->pokedex_id = cfg->pokedex_id;
                        b->hp = cfg->max_hp;
                        b->max_hp = cfg->max_hp;
                        b->defense = cfg->defense;
                        b->uid = ++g_enemy_uid_counter;
                        b->spawn_time = game->spawner.game_time;
                        b->radius = cfg->radius;
                        b->touch_damage = cfg->touch_damage;
                        // Spawn 400px above player
                        b->pos.x = game->player.pos.x;
                        b->pos.y = game->player.pos.y - 400.0f;
                        b->ai_state = ENEMY_AI_CHASE;
                        Game_ActivateEnemy(game, e);
                        Telemetry_LogSpawn(game->spawner.game_time, b->uid, b->pokedex_id, cfg->name, 3, b->max_hp, route->name, game->current_section);
                        game->gym_boss_spawned = true;
                        game->spawner.spawn_interval = 3.2f; // Rare minion reinforcements during boss fight
                        game->spawner.batch_size = 1;
                        game->spawner.max_enemies = 8;
                        snprintf(game->route_banner_text, sizeof(game->route_banner_text), "GYM LEADER BATTLE!");
                        game->route_banner_timer = 3.5f;
                        break;
                    }
                }
            }
        }

        // Check if gym boss was defeated
        if (game->gym_boss_spawned) {
            bool boss_alive = false;
            for (int i = 0; i < game->active_enemy_count; i++) {
                int e_idx = game->active_enemy_indices[i];
                if (game->enemy_types[game->enemies[e_idx].type].is_boss) {
                    boss_alive = true;
                    break;
                }
            }
            if (!boss_alive) {
                // Boss defeated! Heal player + give badge reward + advance
                game->player.hp += 60.0f;
                if (game->player.hp > game->player.max_hp) game->player.hp = game->player.max_hp;
                game->player.xp += 150;
                snprintf(game->route_banner_text, sizeof(game->route_banner_text), "BADGE ACQUIRED! SECTION CLEAR!");
                game->route_banner_timer = 4.0f;
                KantoRoutes_AdvanceRoute(game);
                return;
            }
        }
    } else {
        // Standard route: timed or kill milestone
        if (game->route_timer <= 0.0f || game->route_kills >= game->route_kill_quota) {
            KantoRoutes_AdvanceRoute(game);
            return;
        }
    }
}

void KantoRoutes_SpawnEnemies(GameState* game, float dt) {
    if (game->active_enemy_type_count <= 0) return;
    SpawnerConfig* sp = &game->spawner;
    sp->spawn_timer += dt;
    if (sp->spawn_timer < sp->spawn_interval) return;
    sp->spawn_timer = 0.0f;

    int regular_type_count = game->active_enemy_type_count;
    // Exclude boss slot from random batch spawning
    if (game->enemy_types[regular_type_count - 1].is_boss) {
        regular_type_count--;
    }
    if (regular_type_count <= 0) return;

    for (int b = 0; b < sp->batch_size; b++) {
        if (game->active_enemy_count >= sp->max_enemies) break;

        // Choose random free slot
        int free_idx = -1;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!game->enemies[i].active) {
                free_idx = i;
                break;
            }
        }
        if (free_idx < 0) break;

        int t_idx = rand() % regular_type_count;
        EnemyTypeConfig* cfg = &game->enemy_types[t_idx];
        Enemy* e = &game->enemies[free_idx];
        memset(e, 0, sizeof(Enemy));
        e->type = t_idx;
        e->pokedex_id = cfg->pokedex_id;

        // Dynamic time-based tenacity scaling: (1 + alpha * (t / 60)^gamma)
        float time_scale = 1.0f + g_balance_config.time_hp_alpha * powf(game->spawner.game_time / 60.0f, g_balance_config.time_hp_gamma);
        e->max_hp = cfg->max_hp * time_scale;
        e->hp = e->max_hp;
        e->defense = cfg->defense;
        e->uid = ++g_enemy_uid_counter;
        e->spawn_time = game->spawner.game_time;

        e->radius = cfg->radius;
        e->touch_damage = cfg->touch_damage;

        // Spawn on ring around player outside camera viewport
        float angle = ((float)rand() / (float)RAND_MAX) * 2.0f * (float)M_PI;
        float dist = 680.0f + (float)(rand() % 120);
        e->pos.x = game->player.pos.x + cosf(angle) * dist;
        e->pos.y = game->player.pos.y + sinf(angle) * dist;

        // Clamp to world
        if (e->pos.x < 32.0f) e->pos.x = 32.0f;
        if (e->pos.x > WORLD_SIZE_X - 32.0f) e->pos.x = WORLD_SIZE_X - 32.0f;
        if (e->pos.y < 32.0f) e->pos.y = 32.0f;
        if (e->pos.y > WORLD_SIZE_Y - 32.0f) e->pos.y = WORLD_SIZE_Y - 32.0f;

        // Push out of solid rock/water if spawned on an obstacle
        bool can_fly = (cfg->move_pattern == ENEMY_MOVE_FLANK_ORBIT || cfg->attack_type == ENEMY_ATTACK_RANGED_GUST);
        Tilemap_CheckCollisionCircle(&game->tilemap, &e->pos.x, &e->pos.y, e->radius, can_fly);

        e->ai_state = ENEMY_AI_CHASE;
        e->state_timer = 0.0f;
        e->attack_cooldown_timer = (float)(rand() % 20) * 0.1f;
        e->is_underground = (cfg->attack_type == ENEMY_ATTACK_BURROW_ASSASSIN);
        e->attack_has_hit = false;

        Game_ActivateEnemy(game, free_idx);

        const RouteConfig* cur_route = KantoRoutes_GetCurrentRoute(game);
        Telemetry_LogSpawn(game->spawner.game_time, e->uid, e->pokedex_id, cfg->name,
                           Pokedex_GetEntry(e->pokedex_id) ? Pokedex_GetEntry(e->pokedex_id)->stage : 1,
                           e->max_hp, cur_route ? cur_route->name : "UNKNOWN", game->current_section);
    }
}
