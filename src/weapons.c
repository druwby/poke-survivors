#include "weapons.h"
#include "game.h"
#include "font.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void Weapons_Init(WeaponsManager* wm) {
    // 1. Water Wand (Default primary)
    wm->water_wand.enabled = true;
    wm->water_wand.cooldown = 0.42f;
    wm->water_wand.cooldown_timer = 0.0f;
    wm->water_wand.damage = 25.0f;
    wm->water_wand.proj_speed = 440.0f;
    wm->water_wand.proj_radius = 13.0f;
    wm->water_wand.pierce = 2;
    wm->water_wand.knockback = 120.0f;
    
    // 2. Orbiters (Aqua Orbs / Fire Spin / Petal Dance)
    wm->orbiters.enabled = false;
    wm->orbiters.cooldown = 0.0f; // Continuous
    wm->orbiters.cooldown_timer = 0.0f;
    wm->orbiters.damage = 22.0f;
    wm->orbiters.proj_speed = 3.2f; // Angular speed (rad/sec)
    wm->orbiters.proj_radius = 17.0f;
    wm->orbiters.pierce = 999;
    wm->orbiters.knockback = 160.0f;
    wm->orbiters.orbiter_count = 3;
    wm->orbiters.orbiter_radius = 120.0f;
    wm->orbiters.orbiter_speed = 3.6f;
    wm->orbiters.orbiter_cycle_timer = 0.0f;
    wm->orbiters.orbiter_active_duration = 3.6f;
    wm->orbiters.orbiter_cooldown_duration = 2.4f;
    wm->orbiters.orbiter_current_radius = 120.0f;
    wm->orbiters.orbiter_alpha = 1.0f;

    // 3. Cleave (Whip / Slash / Breath)
    wm->cleave.enabled = false;
    wm->cleave.cooldown = 1.15f;
    wm->cleave.cooldown_timer = 0.0f;
    wm->cleave.damage = 50.0f;
    wm->cleave.slash_range = 145.0f;
    wm->cleave.slash_angle = 1.6f; // Radians arc
    wm->cleave.knockback = 300.0f;

    // 4. Radial Pulse (Bubble Beam / Flame Burst / Toxic Spore)
    wm->pulse.enabled = false;
    wm->pulse.cooldown = 2.0f;
    wm->pulse.cooldown_timer = 0.0f;
    wm->pulse.damage = 42.0f;
    wm->pulse.pulse_max_radius = 160.0f;
    wm->pulse.proj_speed = 220.0f; // Expansion rate
    wm->pulse.knockback = 180.0f;
}

void Weapons_InitForStarter(GameState* game, StarterID starter) {
    WeaponsManager* wm = &game->weapons;
    Weapons_Init(wm); // Set default radius, speeds, knockbacks
    wm->water_wand.enabled = false;
    wm->orbiters.enabled = false;
    wm->cleave.enabled = false;
    wm->pulse.enabled = false;

    // Each starter begins with strictly ONE basic attack!
    if (starter == STARTER_BULBASAUR) {
        wm->cleave.enabled = true; // Vine Whip (Heavy hitting multi-vine lash)
        wm->cleave.damage = 85.0f; // Buffed to hit hard!
        wm->cleave.cooldown = 0.88f;
        wm->cleave.slash_range = 160.0f;
        wm->cleave.knockback = 340.0f;

        // Petal Dance config when learned
        wm->orbiters.orbiter_active_duration = 3.6f;
        wm->orbiters.orbiter_cooldown_duration = 2.4f;
        wm->orbiters.orbiter_radius = 130.0f;
        wm->orbiters.damage = 38.0f;
    } else if (starter == STARTER_CHARMANDER) {
        wm->water_wand.enabled = true; // Directional Flamethrower (Only points wherever player points)
        wm->water_wand.damage = 18.0f;
        wm->water_wand.cooldown = 0.08f; // ~12.5 flames/sec high-speed channeled stream
        wm->water_wand.proj_speed = 440.0f;
        wm->water_wand.proj_radius = 16.0f;
        wm->water_wand.pierce = 3;
        wm->water_wand.knockback = 75.0f;
    } else if (starter == STARTER_SQUIRTLE) {
        wm->water_wand.enabled = true; // Bubble Beam (Slow moving bubbles that trap and pop enemies)
        wm->water_wand.damage = 22.0f;
        wm->water_wand.cooldown = 0.48f;
        wm->water_wand.proj_speed = 210.0f; // Slow floating buoyant bubbles
        wm->water_wand.proj_radius = 20.0f; // Large iridescent soap bubbles
        wm->water_wand.pierce = 1;
        wm->water_wand.knockback = 45.0f;
    }
}

void Weapons_SpawnDamageText(GameState* game, Vec2 pos, float damage, SDL_Color color) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!game->damage_texts[i].active) {
            game->damage_texts[i].active = true;
            game->damage_texts[i].pos = pos;
            game->damage_texts[i].pos.x += (float)((rand() % 16) - 8);
            game->damage_texts[i].pos.y += (float)((rand() % 16) - 8);
            game->damage_texts[i].damage = damage;
            game->damage_texts[i].lifetime = 0.0f;
            game->damage_texts[i].max_lifetime = 0.65f;
            game->damage_texts[i].color = color;
            break;
        }
    }
}

static Projectile* AllocProjectile(GameState* game) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!game->projectiles[i].active) {
            game->projectiles[i].active = true;
            game->projectiles[i].is_enemy = false;
            game->projectiles[i].color = (SDL_Color){ 255, 255, 255, 255 };
            return &game->projectiles[i];
        }
    }
    return NULL;
}

void Weapons_SpawnEnemyProjectile(GameState* game, Vec2 pos, Vec2 vel, float radius, float damage, float lifetime, SDL_Color color) {
    Projectile* p = AllocProjectile(game);
    if (!p) return;
    p->type = PROJ_ENEMY_BULLET;
    p->pos = pos;
    p->vel = vel;
    p->radius = (radius < 11.0f) ? 11.0f : radius;
    p->damage = damage;
    p->lifetime = 0.0f;
    p->max_lifetime = lifetime;
    p->pierce_left = 1;
    p->is_enemy = true;
    p->color = color;
    p->knockback = 50.0f;
}

void Weapons_SpawnGroundHazard(GameState* game, HazardType type, Vec2 pos, float radius, float duration, float damage, bool is_enemy, SDL_Color color) {
    for (int i = 0; i < MAX_GROUND_HAZARDS; i++) {
        if (!game->ground_hazards[i].active) {
            GroundHazard* gh = &game->ground_hazards[i];
            gh->active = true;
            gh->type = type;
            gh->pos = pos;
            gh->radius = radius;
            gh->timer = 0.0f;
            gh->max_duration = duration;
            gh->tick_timer = 0.0f;
            gh->damage = damage;
            gh->is_enemy = is_enemy;
            gh->color = color;
            break;
        }
    }
}

void Weapons_FireVineWhip(GameState* game) {
    Player* p = &game->player;
    int max_vines = p->move_basic_level;
    if (max_vines < 1) max_vines = 1;
    if (max_vines > 4) max_vines = 4;

    float reach = game->weapons.cleave.slash_range;
    if (reach < 160.0f) reach = 160.0f;

    // Find up to max_vines closest enemies within reach
    int targets[4] = { -1, -1, -1, -1 };
    float target_dists[4] = { 999999.0f, 999999.0f, 999999.0f, 999999.0f };

    for (int k = 0; k < game->active_enemy_count; k++) {
        int e_idx = game->active_enemy_indices[k];
        Enemy* e = &game->enemies[e_idx];
        if (!e->active || e->is_underground) continue;

        float dx = e->pos.x - p->pos.x;
        float dy = e->pos.y - p->pos.y;
        float d_sq = dx * dx + dy * dy;
        if (d_sq > reach * reach) continue;

        for (int slot = 0; slot < max_vines; slot++) {
            if (d_sq < target_dists[slot]) {
                for (int s = max_vines - 1; s > slot; s--) {
                    targets[s] = targets[s - 1];
                    target_dists[s] = target_dists[s - 1];
                }
                targets[slot] = e_idx;
                target_dists[slot] = d_sq;
                break;
            }
        }
    }

    float base_angle = 0.0f;
    switch (p->anim.direction) {
        case 0: base_angle = (float)M_PI * 0.5f; break;  // Down
        case 1: base_angle = (float)M_PI * 0.25f; break; // Down-Right
        case 2: base_angle = 0.0f; break;                // Right
        case 3: base_angle = -(float)M_PI * 0.25f; break;// Up-Right
        case 4: base_angle = -(float)M_PI * 0.5f; break; // Up
        case 5: base_angle = -(float)M_PI * 0.75f; break;// Up-Left
        case 6: base_angle = (float)M_PI; break;         // Left
        case 7: base_angle = (float)M_PI * 0.75f; break; // Down-Left
    }

    // Launch each vine towards its target or fan out
    for (int v = 0; v < max_vines; v++) {
        Vec2 dest;
        if (targets[v] >= 0) {
            dest = game->enemies[targets[v]].pos;
        } else {
            float fan = ((float)v - (float)(max_vines - 1) * 0.5f) * 0.35f;
            float fa = base_angle + fan;
            dest.x = p->pos.x + cosf(fa) * reach;
            dest.y = p->pos.y + sinf(fa) * reach;
        }

        for (int i = 0; i < MAX_VINE_STRIKES; i++) {
            if (!game->vine_strikes[i].active) {
                VineWhipStrike* vs = &game->vine_strikes[i];
                vs->active = true;
                vs->start_pos = p->pos;
                vs->target_pos = dest;
                float vdx = dest.x - p->pos.x;
                float vdy = dest.y - p->pos.y;
                float vlen = sqrtf(vdx * vdx + vdy * vdy);
                if (vlen > 0.001f) {
                    vs->dir.x = vdx / vlen;
                    vs->dir.y = vdy / vlen;
                } else {
                    vs->dir.x = cosf(base_angle);
                    vs->dir.y = sinf(base_angle);
                }
                vs->length = vlen;
                vs->max_length = vlen + 25.0f;
                vs->timer = 0.0f;
                vs->max_duration = 0.22f;
                vs->damage = game->weapons.cleave.damage * p->damage_mult;
                vs->knockback = game->weapons.cleave.knockback;
                vs->wave_seed = (float)v * 1.57f + (float)(rand() % 10) * 0.2f;
                vs->struck = false;
                break;
            }
        }
    }
}

void Weapons_FireWaterWand(GameState* game) {
    Player* p = &game->player;

    // Find closest enemy within 600px range
    float min_dist_sq = 600.0f * 600.0f;
    int target_idx = -1;
    
    for (int k = 0; k < game->active_enemy_count; k++) {
        int i = game->active_enemy_indices[k];
        if (game->enemies[i].is_underground) continue; // Diglett is untargetable underground!
        float dx = game->enemies[i].pos.x - p->pos.x;
        if (fabsf(dx) > 600.0f) continue;
        float dy = game->enemies[i].pos.y - p->pos.y;
        if (fabsf(dy) > 600.0f) continue;
        float dist_sq = dx * dx + dy * dy;
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            target_idx = i;
        }
    }
    
    Vec2 dir = { 1.0f, 0.0f };
    if (target_idx != -1) {
        float dx = game->enemies[target_idx].pos.x - p->pos.x;
        float dy = game->enemies[target_idx].pos.y - p->pos.y;
        float len = sqrtf(dx * dx + dy * dy);
        if (len > 0.0001f) {
            dir.x = dx / len;
            dir.y = dy / len;
        }
    } else {
        // Fire in facing direction
        switch (p->anim.direction) {
            case 0: dir = (Vec2){ 0.0f, 1.0f }; break;
            case 1: dir = (Vec2){ 0.707f, 0.707f }; break;
            case 2: dir = (Vec2){ 1.0f, 0.0f }; break;
            case 3: dir = (Vec2){ 0.707f, -0.707f }; break;
            case 4: dir = (Vec2){ 0.0f, -1.0f }; break;
            case 5: dir = (Vec2){ -0.707f, -0.707f }; break;
            case 6: dir = (Vec2){ -1.0f, 0.0f }; break;
            case 7: dir = (Vec2){ -0.707f, 0.707f }; break;
        }
    }
    
    float base_a = atan2f(dir.y, dir.x);

    if (p->starter == STARTER_CHARMANDER) {
        // CHARMANDER: Directional Flamethrower (STRICTLY wherever player is pointing, sweeps with WASD)
        Vec2 flame_dir = { 1.0f, 0.0f };
        switch (p->anim.direction) {
            case 0: flame_dir = (Vec2){  0.0f,  1.0f }; break; // Down
            case 1: flame_dir = (Vec2){  0.707f,  0.707f }; break; // Down-Right
            case 2: flame_dir = (Vec2){  1.0f,  0.0f }; break; // Right
            case 3: flame_dir = (Vec2){  0.707f, -0.707f }; break; // Up-Right
            case 4: flame_dir = (Vec2){  0.0f, -1.0f }; break; // Up
            case 5: flame_dir = (Vec2){ -0.707f, -0.707f }; break; // Up-Left
            case 6: flame_dir = (Vec2){ -1.0f,  0.0f }; break; // Left
            case 7: flame_dir = (Vec2){ -0.707f,  0.707f }; break; // Down-Left
        }
        float base_flame_a = atan2f(flame_dir.y, flame_dir.x);
        float cone_spread = 0.28f + (float)(p->move_basic_level - 1) * 0.06f;
        float rnd_offset = (((float)(rand() % 100) / 100.0f) - 0.5f) * cone_spread;
        float fire_a = base_flame_a + rnd_offset;

        Projectile* proj = AllocProjectile(game);
        if (proj) {
            proj->type = PROJ_EMBER;
            proj->color = (SDL_Color){ 255, 120, 20, 255 };
            // Fire stream emerges from player's snout along facing vector
            proj->pos.x = p->pos.x + flame_dir.x * 14.0f;
            proj->pos.y = p->pos.y + flame_dir.y * 14.0f;
            float spd = game->weapons.water_wand.proj_speed * (0.85f + (float)(rand() % 30) * 0.01f);
            proj->vel.x = cosf(fire_a) * spd;
            proj->vel.y = sinf(fire_a) * spd;
            proj->radius = game->weapons.water_wand.proj_radius + (float)(p->move_basic_level - 1) * 2.5f;
            proj->damage = game->weapons.water_wand.damage * p->damage_mult;
            proj->pierce_left = 3 + (p->move_basic_level - 1);
            proj->knockback = game->weapons.water_wand.knockback;
            proj->lifetime = 0.0f;
            proj->max_lifetime = 0.40f + (float)(p->move_basic_level - 1) * 0.05f; // Travels ~175px
            proj->is_enemy = false;
        }
        return;
    } else if (p->starter == STARTER_SQUIRTLE) {
        // SQUIRTLE: Bubble Beam (Slow moving buoyant bubbles that trap enemies and pop)
        int bubble_count = p->move_basic_level;
        if (bubble_count < 1) bubble_count = 1;
        if (bubble_count > 3) bubble_count = 3;

        for (int b = 0; b < bubble_count; b++) {
            float angle_offset = ((float)b - (float)(bubble_count - 1) * 0.5f) * 0.22f;
            float bubble_a = base_a + angle_offset;
            Projectile* proj = AllocProjectile(game);
            if (!proj) break;
            proj->type = PROJ_BUBBLE;
            proj->color = (SDL_Color){ 80, 220, 255, 230 };
            proj->pos = p->pos;
            proj->pos.x += -dir.y * ((float)b - (float)(bubble_count - 1) * 0.5f) * 10.0f;
            proj->pos.y +=  dir.x * ((float)b - (float)(bubble_count - 1) * 0.5f) * 10.0f;
            proj->vel.x = cosf(bubble_a) * game->weapons.water_wand.proj_speed;
            proj->vel.y = sinf(bubble_a) * game->weapons.water_wand.proj_speed;
            proj->radius = game->weapons.water_wand.proj_radius + (float)(p->move_basic_level - 1) * 2.0f;
            proj->damage = game->weapons.water_wand.damage * p->damage_mult;
            proj->pierce_left = 1; // 1 enemy encased per bubble
            proj->knockback = game->weapons.water_wand.knockback;
            proj->lifetime = 0.0f;
            proj->max_lifetime = 2.4f; // Floats for up to 2.4s
            proj->is_enemy = false;
        }
        return;
    } else {
        // BULBASAUR RAZOR LEAF (Fast spinning aerodynamic leaf blades)
        Projectile* proj = AllocProjectile(game);
        if (!proj) return;
        proj->type = PROJ_RAZOR_LEAF;
        proj->color = (SDL_Color){ 90, 240, 80, 255 };
        proj->pos = p->pos;
        proj->vel.x = dir.x * 520.0f;
        proj->vel.y = dir.y * 520.0f;
        proj->radius = 14.0f;
        proj->damage = game->weapons.water_wand.damage * p->damage_mult * 1.25f;
        proj->pierce_left = 4;
        proj->knockback = 120.0f;
        proj->lifetime = 0.0f;
        proj->max_lifetime = 1.4f;
        proj->is_enemy = false;
    }
}

void Weapons_FireCleave(GameState* game) {
    float base_angle = 0.0f;
    switch (game->player.anim.direction) {
        case 0: base_angle = (float)M_PI * 0.5f; break;  // Down
        case 1: base_angle = (float)M_PI * 0.25f; break; // Down-Right
        case 2: base_angle = 0.0f; break;                // Right
        case 3: base_angle = -(float)M_PI * 0.25f; break;// Up-Right
        case 4: base_angle = -(float)M_PI * 0.5f; break; // Up
        case 5: base_angle = -(float)M_PI * 0.75f; break;// Up-Left
        case 6: base_angle = (float)M_PI; break;         // Left
        case 7: base_angle = (float)M_PI * 0.75f; break; // Down-Left
    }
    
    Projectile* p = AllocProjectile(game);
    if (!p) return;
    
    p->type = PROJ_SLASH;
    p->pos = game->player.pos;
    p->vel.x = cosf(base_angle);
    p->vel.y = sinf(base_angle);
    p->radius = game->weapons.cleave.slash_range;
    p->damage = game->weapons.cleave.damage * game->player.damage_mult;
    p->pierce_left = 999;
    p->knockback = game->weapons.cleave.knockback;
    p->orbit_angle = base_angle;
    p->orbit_dist = game->weapons.cleave.slash_angle;
    p->lifetime = 0.0f;
    p->max_lifetime = 0.22f;
    p->is_enemy = false;

    if (game->player.starter == STARTER_BULBASAUR) {
        p->color = (SDL_Color){ 70, 220, 70, 255 }; // Green Vine Whip
    } else if (game->player.starter == STARTER_CHARMANDER) {
        p->color = (SDL_Color){ 255, 110, 30, 255 }; // Fiery Dragon Breath
    } else {
        p->color = (SDL_Color){ 50, 190, 255, 255 }; // Aqua Jet
    }
}

void Weapons_FirePulse(GameState* game) {
    Projectile* p = AllocProjectile(game);
    if (!p) return;
    
    p->type = PROJ_PULSE_RING;
    p->pos = game->player.pos;
    p->vel = (Vec2){ 0.0f, 0.0f };
    p->radius = 10.0f;
    p->damage = game->weapons.pulse.damage * game->player.damage_mult;
    p->pierce_left = 999;
    p->knockback = game->weapons.pulse.knockback;
    p->lifetime = 0.0f;
    p->max_lifetime = game->weapons.pulse.pulse_max_radius / game->weapons.pulse.proj_speed;
    p->is_enemy = false;

    if (game->player.starter == STARTER_BULBASAUR) {
        p->color = (SDL_Color){ 160, 70, 210, 255 }; // Toxic Spore Ring
    } else if (game->player.starter == STARTER_CHARMANDER) {
        p->color = (SDL_Color){ 255, 140, 20, 255 }; // Flame Burst
    } else {
        p->color = (SDL_Color){ 60, 210, 240, 255 }; // Bubble Beam
    }
}

void Weapons_Update(GameState* game, float dt) {
    // 1. Water Wand / Ember / Razor Leaf
    if (game->weapons.water_wand.enabled) {
        game->weapons.water_wand.cooldown_timer -= dt;
        if (game->weapons.water_wand.cooldown_timer <= 0.0f) {
            Weapons_FireWaterWand(game);
            game->weapons.water_wand.cooldown_timer = game->weapons.water_wand.cooldown;
        }
    }
    
    // 2. Cleave / Vine Whip / Dragon Breath
    if (game->weapons.cleave.enabled) {
        game->weapons.cleave.cooldown_timer -= dt;
        if (game->weapons.cleave.cooldown_timer <= 0.0f) {
            if (game->player.starter == STARTER_BULBASAUR) {
                Weapons_FireVineWhip(game);
            } else {
                Weapons_FireCleave(game);
            }
            game->weapons.cleave.cooldown_timer = game->weapons.cleave.cooldown;
        }
    }

    // 3. Radial Pulse / Toxic Spore / Bubble Beam
    if (game->weapons.pulse.enabled) {
        game->weapons.pulse.cooldown_timer -= dt;
        if (game->weapons.pulse.cooldown_timer <= 0.0f) {
            Weapons_FirePulse(game);
            game->weapons.pulse.cooldown_timer = game->weapons.pulse.cooldown;
        }
    }
    
    // 4. Update Orbiters / Petal Dance
    static float orbiter_global_angle = 0.0f;
    if (game->weapons.orbiters.enabled) {
        WeaponConfig* orb = &game->weapons.orbiters;
        bool is_bulbasaur = (game->player.starter == STARTER_BULBASAUR);

        if (is_bulbasaur) {
            orb->orbiter_cycle_timer += dt;
            float total_cycle = orb->orbiter_active_duration + orb->orbiter_cooldown_duration;
            if (total_cycle <= 0.001f) total_cycle = 6.0f;
            if (orb->orbiter_cycle_timer >= total_cycle) {
                orb->orbiter_cycle_timer -= total_cycle;
            }

            float t = orb->orbiter_cycle_timer;
            if (t < orb->orbiter_active_duration) {
                // Active bloom phase
                orbiter_global_angle += (orb->orbiter_speed * 1.35f) * dt;
                if (t < 0.65f) {
                    // Extend out from player
                    float prog = t / 0.65f;
                    orb->orbiter_current_radius = 16.0f + (orb->orbiter_radius - 16.0f) * prog;
                    orb->orbiter_alpha = prog;
                } else if (t > orb->orbiter_active_duration - 0.65f) {
                    // Dissipate outwards and fade
                    float prog = (t - (orb->orbiter_active_duration - 0.65f)) / 0.65f;
                    orb->orbiter_current_radius = orb->orbiter_radius + 45.0f * prog;
                    orb->orbiter_alpha = 1.0f - prog;
                } else {
                    // Full spin
                    orb->orbiter_current_radius = orb->orbiter_radius;
                    orb->orbiter_alpha = 1.0f;
                }
            } else {
                // Cooldown / dissipated
                orb->orbiter_current_radius = 0.0f;
                orb->orbiter_alpha = 0.0f;
            }
        } else {
            orb->orbiter_current_radius = orb->orbiter_radius;
            orb->orbiter_alpha = 1.0f;
            orbiter_global_angle += orb->orbiter_speed * dt;
        }

        if (orbiter_global_angle > 2.0f * (float)M_PI) {
            orbiter_global_angle -= 2.0f * (float)M_PI;
        }
    }
    
    int orb_count = (game->weapons.orbiters.enabled && game->weapons.orbiters.orbiter_alpha > 0.01f) ? game->weapons.orbiters.orbiter_count : 0;
    int current_active_orbs = 0;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (game->projectiles[i].active && game->projectiles[i].type == PROJ_ORBITER) {
            if (current_active_orbs < orb_count) {
                float angle = orbiter_global_angle + ((2.0f * (float)M_PI * current_active_orbs) / orb_count);
                game->projectiles[i].pos.x = game->player.pos.x + cosf(angle) * game->weapons.orbiters.orbiter_current_radius;
                game->projectiles[i].pos.y = game->player.pos.y + sinf(angle) * game->weapons.orbiters.orbiter_current_radius;
                game->projectiles[i].radius = game->weapons.orbiters.proj_radius;
                game->projectiles[i].damage = game->weapons.orbiters.damage * game->player.damage_mult * game->weapons.orbiters.orbiter_alpha;
                game->projectiles[i].knockback = game->weapons.orbiters.knockback;
                game->projectiles[i].orbit_angle = angle;
                if (game->player.starter == STARTER_BULBASAUR) {
                    game->projectiles[i].color = (SDL_Color){ 255, 120, 190, (Uint8)(game->weapons.orbiters.orbiter_alpha * 255.0f) };
                } else if (game->player.starter == STARTER_CHARMANDER) {
                    game->projectiles[i].color = (SDL_Color){ 255, 90, 20, 255 };  // Fire Spin orange
                } else {
                    game->projectiles[i].color = (SDL_Color){ 60, 150, 255, 255 }; // Aqua Orbs cyan
                }
                current_active_orbs++;
            } else {
                game->projectiles[i].active = false;
            }
        }
    }
    while (current_active_orbs < orb_count) {
        Projectile* p = AllocProjectile(game);
        if (!p) break;
        p->type = PROJ_ORBITER;
        p->active = true;
        p->pierce_left = 999;
        p->max_lifetime = 9999.0f;
        p->lifetime = 0.0f;
        p->is_enemy = false;
        float angle = orbiter_global_angle + ((2.0f * (float)M_PI * current_active_orbs) / orb_count);
        p->pos.x = game->player.pos.x + cosf(angle) * game->weapons.orbiters.orbiter_current_radius;
        p->pos.y = game->player.pos.y + sinf(angle) * game->weapons.orbiters.orbiter_current_radius;
        p->radius = game->weapons.orbiters.proj_radius;
        p->damage = game->weapons.orbiters.damage * game->player.damage_mult * game->weapons.orbiters.orbiter_alpha;
        p->knockback = game->weapons.orbiters.knockback;
        p->orbit_angle = angle;
        if (game->player.starter == STARTER_BULBASAUR) {
            p->color = (SDL_Color){ 255, 120, 190, (Uint8)(game->weapons.orbiters.orbiter_alpha * 255.0f) };
        } else if (game->player.starter == STARTER_CHARMANDER) {
            p->color = (SDL_Color){ 255, 90, 20, 255 };
        } else {
            p->color = (SDL_Color){ 60, 150, 255, 255 };
        }
        current_active_orbs++;
    }

    // 5. Update All Projectiles
    Player* player = &game->player;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* p = &game->projectiles[i];
        if (!p->active) continue;
        
        p->lifetime += dt;
        if (p->lifetime >= p->max_lifetime || p->pierce_left <= 0) {
            if (p->type == PROJ_EMBER) {
                Weapons_SpawnGroundHazard(game, HAZARD_FIRE_SCORCH, p->pos, 28.0f, 1.4f, 14.0f * player->damage_mult, false, (SDL_Color){ 255, 120, 20, 255 });
            }
            p->active = false;
            continue;
        }
        
        if (p->is_enemy) {
            // Move enemy projectile
            p->pos.x += p->vel.x * dt;
            p->pos.y += p->vel.y * dt;

            // Check collision with Player!
            float dx = p->pos.x - player->pos.x;
            float dy = p->pos.y - player->pos.y;
            float hit_dist = p->radius + player->radius;
            if (dx * dx + dy * dy <= hit_dist * hit_dist) {
                if (player->invuln_timer <= 0.0f && !player->god_mode) {
                    player->hp -= p->damage;
                    player->damage_flash_timer = 0.20f;
                    player->invuln_timer = player->invuln_duration;
                    Weapons_SpawnDamageText(game, player->pos, p->damage, (SDL_Color){ 255, 60, 60, 255 });
                }
                p->active = false;
            }
        } else {
            // Player projectiles movement
            if (p->type == PROJ_WATER_BOLT || p->type == PROJ_EMBER || p->type == PROJ_RAZOR_LEAF) {
                p->pos.x += p->vel.x * dt;
                p->pos.y += p->vel.y * dt;
            } else if (p->type == PROJ_BUBBLE) {
                // SQUIRTLE BUBBLE BEAM: Slow floating buoyant drift with sinusoidal wobble
                p->pos.x += p->vel.x * dt;
                p->pos.y += p->vel.y * dt + sinf(p->lifetime * 7.5f) * 1.6f;
            } else if (p->type == PROJ_PULSE_RING) {
                p->radius += game->weapons.pulse.proj_speed * dt;
            }
        }
    }

    // 5b. Update Vine Whip Strikes
    for (int i = 0; i < MAX_VINE_STRIKES; i++) {
        VineWhipStrike* vs = &game->vine_strikes[i];
        if (!vs->active) continue;
        vs->timer += dt;
        if (vs->timer >= vs->max_duration) {
            vs->active = false;
            continue;
        }

        // Vine whip lash occurs at 38% duration (apex of extension)
        if (!vs->struck && vs->timer >= vs->max_duration * 0.38f) {
            vs->struck = true;
            Vec2 p1 = vs->start_pos;
            Vec2 p2 = { vs->start_pos.x + vs->dir.x * vs->max_length,
                        vs->start_pos.y + vs->dir.y * vs->max_length };
            float seg_dx = p2.x - p1.x;
            float seg_dy = p2.y - p1.y;
            float seg_len_sq = seg_dx * seg_dx + seg_dy * seg_dy;

            for (int k = 0; k < game->active_enemy_count; k++) {
                int e_idx = game->active_enemy_indices[k];
                Enemy* e = &game->enemies[e_idx];
                if (!e->active || e->is_underground) continue;

                // Projection parameter t in [0, 1] along whip segment
                float t = 0.0f;
                if (seg_len_sq > 0.001f) {
                    t = ((e->pos.x - p1.x) * seg_dx + (e->pos.y - p1.y) * seg_dy) / seg_len_sq;
                    if (t < 0.0f) t = 0.0f;
                    if (t > 1.0f) t = 1.0f;
                }
                float close_x = p1.x + t * seg_dx;
                float close_y = p1.y + t * seg_dy;
                float cdx = e->pos.x - close_x;
                float cdy = e->pos.y - close_y;
                float dist_sq = cdx * cdx + cdy * cdy;
                float hit_r = 34.0f + e->radius; // Generous 34px lash width!

                if (dist_sq <= hit_r * hit_r) {
                    // Critical whip snap at the tip (t >= 0.70 deals 1.30x critical crack!)
                    float dmg_mult = (t >= 0.70f) ? 1.30f : 1.0f;
                    float dmg = vs->damage * dmg_mult;
                    if (game->enemy_types[e->type].move_pattern == ENEMY_MOVE_STATIONARY) dmg *= 0.35f;
                    e->hp -= dmg;
                    e->flash_timer = 0.14f;
                    float resist = game->enemy_types[e->type].knockback_resist;
                    float kb = vs->knockback * (1.0f - resist * 0.4f) * (t >= 0.70f ? 1.25f : 1.0f);
                    e->knockback_x = vs->dir.x * kb;
                    e->knockback_y = vs->dir.y * kb;
                    SDL_Color dmg_col = (t >= 0.70f) ? (SDL_Color){ 255, 255, 120, 255 } : (SDL_Color){ 80, 245, 80, 255 };
                    Weapons_SpawnDamageText(game, e->pos, dmg, dmg_col);

                    if (e->hp <= 0.0f) {
                        player->kills++;
                        game->route_kills++;
                        for (int g = 0; g < MAX_GEMS; g++) {
                            if (!game->gems[g].active) {
                                game->gems[g].active = true;
                                game->gems[g].pos = e->pos;
                                game->gems[g].value = game->enemy_types[e->type].xp_value;
                                break;
                            }
                        }
                        Game_DeactivateEnemy(game, e_idx);
                    }
                }
            }
        }
    }

    // 5c. Update Ground Hazards (Fire Scorch, Poison Spores, Mortar Reticles)
    for (int i = 0; i < MAX_GROUND_HAZARDS; i++) {
        GroundHazard* gh = &game->ground_hazards[i];
        if (!gh->active) continue;
        gh->timer += dt;
        if (gh->timer >= gh->max_duration) {
            if (gh->type == HAZARD_MORTAR_TARGET) {
                float pdx = player->pos.x - gh->pos.x;
                float pdy = player->pos.y - gh->pos.y;
                float pr = gh->radius + player->radius;
                if (pdx * pdx + pdy * pdy <= pr * pr) {
                    if (player->invuln_timer <= 0.0f && !player->god_mode) {
                        player->hp -= gh->damage;
                        player->damage_flash_timer = 0.20f;
                        player->invuln_timer = player->invuln_duration;
                        Weapons_SpawnDamageText(game, player->pos, gh->damage, (SDL_Color){ 255, 60, 60, 255 });
                    }
                }
                Weapons_SpawnGroundHazard(game, HAZARD_TOXIC_SPORE, gh->pos, gh->radius * 0.85f, 2.5f, gh->damage * 0.25f, true, (SDL_Color){ 160, 60, 220, 255 });
            }
            gh->active = false;
            continue;
        }

        gh->tick_timer += dt;
        if (gh->tick_timer >= 0.28f) {
            gh->tick_timer = 0.0f;
            if (gh->type == HAZARD_FIRE_SCORCH) {
                for (int k = 0; k < game->active_enemy_count; k++) {
                    int e_idx = game->active_enemy_indices[k];
                    Enemy* e = &game->enemies[e_idx];
                    if (!e->active || e->is_underground) continue;
                    float dx = e->pos.x - gh->pos.x;
                    float dy = e->pos.y - gh->pos.y;
                    float r_tot = gh->radius + e->radius;
                    if (dx * dx + dy * dy <= r_tot * r_tot) {
                        float dmg = gh->damage;
                        if (game->enemy_types[e->type].move_pattern == ENEMY_MOVE_STATIONARY) dmg *= 0.35f;
                        e->hp -= dmg;
                        e->flash_timer = 0.08f;
                        Weapons_SpawnDamageText(game, e->pos, dmg, (SDL_Color){ 255, 140, 30, 255 });
                        if (e->hp <= 0.0f) {
                            player->kills++;
                            game->route_kills++;
                            for (int g = 0; g < MAX_GEMS; g++) {
                                if (!game->gems[g].active) {
                                    game->gems[g].active = true;
                                    game->gems[g].pos = e->pos;
                                    game->gems[g].value = game->enemy_types[e->type].xp_value;
                                    break;
                                }
                            }
                            Game_DeactivateEnemy(game, e_idx);
                        }
                    }
                }
            } else if (gh->type == HAZARD_TOXIC_SPORE) {
                if (gh->is_enemy) {
                    float pdx = player->pos.x - gh->pos.x;
                    float pdy = player->pos.y - gh->pos.y;
                    float pr = gh->radius + player->radius;
                    if (pdx * pdx + pdy * pdy <= pr * pr) {
                        if (player->invuln_timer <= 0.0f && !player->god_mode) {
                            player->hp -= gh->damage;
                            player->damage_flash_timer = 0.15f;
                            player->invuln_timer = 0.25f;
                            Weapons_SpawnDamageText(game, player->pos, gh->damage, (SDL_Color){ 180, 50, 220, 255 });
                        }
                    }
                } else {
                    for (int k = 0; k < game->active_enemy_count; k++) {
                        int e_idx = game->active_enemy_indices[k];
                        Enemy* e = &game->enemies[e_idx];
                        if (!e->active || e->is_underground) continue;
                        float dx = e->pos.x - gh->pos.x;
                        float dy = e->pos.y - gh->pos.y;
                        float r_tot = gh->radius + e->radius;
                        if (dx * dx + dy * dy <= r_tot * r_tot) {
                            float dmg = gh->damage;
                            if (game->enemy_types[e->type].move_pattern == ENEMY_MOVE_STATIONARY) dmg *= 0.35f;
                            e->hp -= dmg;
                            e->flash_timer = 0.08f;
                            e->vel.x *= 0.50f;
                            e->vel.y *= 0.50f;
                            Weapons_SpawnDamageText(game, e->pos, dmg, (SDL_Color){ 140, 230, 80, 255 });
                            if (e->hp <= 0.0f) {
                                player->kills++;
                                game->route_kills++;
                                for (int g = 0; g < MAX_GEMS; g++) {
                                    if (!game->gems[g].active) {
                                        game->gems[g].active = true;
                                        game->gems[g].pos = e->pos;
                                        game->gems[g].value = game->enemy_types[e->type].xp_value;
                                        break;
                                    }
                                }
                                Game_DeactivateEnemy(game, e_idx);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 6. Update Damage Text Particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        DamageText* t = &game->damage_texts[i];
        if (!t->active) continue;
        
        t->lifetime += dt;
        if (t->lifetime >= t->max_lifetime) {
            t->active = false;
            continue;
        }
        
        t->pos.y -= 45.0f * dt;
    }
}

static SDL_Texture* s_orb_texture = NULL;
static SDL_Texture* s_glow_texture = NULL;

static void EnsureOrbTexture(SDL_Renderer* renderer) {
    if (s_orb_texture) return;
    const int size = 64;
    float center = 31.5f;
    float max_r = 30.0f;

    // 1. Crisp, High-Opacity Anti-Aliased Bullet Orb
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, size, size, 32, SDL_PIXELFORMAT_RGBA32);
    if (surf) {
        for (int y = 0; y < size; y++) {
            Uint32* row = (Uint32*)((Uint8*)surf->pixels + y * surf->pitch);
            for (int x = 0; x < size; x++) {
                float dx = (float)x - center;
                float dy = (float)y - center;
                float dist = sqrtf(dx * dx + dy * dy);
                float norm = dist / max_r;

                if (norm >= 1.0f) {
                    row[x] = SDL_MapRGBA(surf->format, 0, 0, 0, 0);
                } else if (norm > 0.82f) {
                    // Smooth anti-aliased outer rim
                    float edge = (1.0f - norm) / 0.18f;
                    Uint8 a = (Uint8)(edge * 255.0f);
                    row[x] = SDL_MapRGBA(surf->format, 255, 255, 255, a);
                } else {
                    // Solid, 100% opaque core body
                    row[x] = SDL_MapRGBA(surf->format, 255, 255, 255, 255);
                }
            }
        }
        s_orb_texture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
        if (s_orb_texture) {
            SDL_SetTextureBlendMode(s_orb_texture, SDL_BLENDMODE_BLEND);
        }
    }

    // 2. Soft Luminous Glow Halo
    surf = SDL_CreateRGBSurfaceWithFormat(0, size, size, 32, SDL_PIXELFORMAT_RGBA32);
    if (surf) {
        for (int y = 0; y < size; y++) {
            Uint32* row = (Uint32*)((Uint8*)surf->pixels + y * surf->pitch);
            for (int x = 0; x < size; x++) {
                float dx = (float)x - center;
                float dy = (float)y - center;
                float dist = sqrtf(dx * dx + dy * dy);
                float norm = dist / max_r;

                if (norm >= 1.0f) {
                    row[x] = SDL_MapRGBA(surf->format, 0, 0, 0, 0);
                } else {
                    float a = cosf(norm * (float)M_PI * 0.5f);
                    a = a * a; // Smooth quadratic falloff
                    Uint8 alpha = (Uint8)(a * 235.0f);
                    row[x] = SDL_MapRGBA(surf->format, 255, 255, 255, alpha);
                }
            }
        }
        s_glow_texture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
        if (s_glow_texture) {
            SDL_SetTextureBlendMode(s_glow_texture, SDL_BLENDMODE_BLEND);
        }
    }
}

void Weapons_DrawGroundHazards(GameState* game) {
    SDL_Renderer* renderer = game->renderer;
    Vec2 camera = game->camera;
    EnsureOrbTexture(renderer);

    // 0. Draw Ground Hazards (Fire Scorch, Toxic Spores, Mortar Reticles)
    for (int i = 0; i < MAX_GROUND_HAZARDS; i++) {
        GroundHazard* gh = &game->ground_hazards[i];
        if (!gh->active) continue;
        int hx = (int)(gh->pos.x - camera.x);
        int hy = (int)(gh->pos.y - camera.y);
        int hr = (int)gh->radius;
        if (hx < -60 || hx > WINDOW_WIDTH + 60 || hy < -60 || hy > WINDOW_HEIGHT + 60) continue;

        float progress = gh->timer / gh->max_duration;
        float alpha = 1.0f - progress * 0.45f;

        if (gh->type == HAZARD_FIRE_SCORCH) {
            // Flickering crackling fire patch on floor
            if (s_glow_texture) {
                SDL_Rect halo = { hx - hr, hy - hr, hr * 2, hr * 2 };
                SDL_SetTextureColorMod(s_glow_texture, 255, 90, 10);
                SDL_SetTextureAlphaMod(s_glow_texture, (Uint8)(alpha * 170.0f));
                SDL_RenderCopy(renderer, s_glow_texture, NULL, &halo);

                int ir = (int)(hr * 0.65f);
                SDL_Rect inner = { hx - ir, hy - ir, ir * 2, ir * 2 };
                SDL_SetTextureColorMod(s_glow_texture, 255, 210, 30);
                SDL_SetTextureAlphaMod(s_glow_texture, (Uint8)(alpha * 220.0f));
                SDL_RenderCopy(renderer, s_glow_texture, NULL, &inner);
            }
            // Dancing fire sparks
            for (int f = 0; f < 3; f++) {
                float fa = (float)f * 2.09f + gh->timer * 8.0f;
                int fx = hx + (int)(cosf(fa) * hr * 0.45f);
                int fy = hy + (int)(sinf(fa) * hr * 0.45f);
                SDL_Rect spark = { fx - 2, fy - 2, 4, 4 };
                SDL_SetRenderDrawColor(renderer, 255, 240, 100, 240);
                SDL_RenderFillRect(renderer, &spark);
            }
        } else if (gh->type == HAZARD_TOXIC_SPORE) {
            // Toxic poison puddle with bubbling spores
            if (s_glow_texture) {
                SDL_Rect halo = { hx - hr, hy - hr, hr * 2, hr * 2 };
                SDL_SetTextureColorMod(s_glow_texture, 150, 40, 220);
                SDL_SetTextureAlphaMod(s_glow_texture, (Uint8)(alpha * 160.0f));
                SDL_RenderCopy(renderer, s_glow_texture, NULL, &halo);
            }
            for (int b = 0; b < 4; b++) {
                float ba = (float)b * 1.57f + gh->timer * 5.0f;
                int bx = hx + (int)(cosf(ba) * hr * 0.5f);
                int by = hy + (int)(sinf(ba) * hr * 0.5f);
                SDL_Rect spore = { bx - 2, by - 2, 4, 4 };
                SDL_SetRenderDrawColor(renderer, 100, 240, 80, 220);
                SDL_RenderFillRect(renderer, &spore);
            }
        } else if (gh->type == HAZARD_MORTAR_TARGET) {
            // Enemy artillery warning reticle with contracting ring
            SDL_SetRenderDrawColor(renderer, 255, 40, 30, 220);
            for (int a = 0; a < 24; a++) {
                float a1 = (2.0f * (float)M_PI * a) / 24.0f;
                float a2 = (2.0f * (float)M_PI * (a + 1)) / 24.0f;
                SDL_RenderDrawLine(renderer, hx + (int)(cosf(a1)*hr), hy + (int)(sinf(a1)*hr),
                                             hx + (int)(cosf(a2)*hr), hy + (int)(sinf(a2)*hr));
            }
            SDL_RenderDrawLine(renderer, hx - 12, hy, hx + 12, hy);
            SDL_RenderDrawLine(renderer, hx, hy - 12, hx, hy + 12);
            // Contracting countdown circle
            float cr = hr * (1.0f - gh->timer / gh->max_duration);
            if (cr > 2.0f) {
                SDL_SetRenderDrawColor(renderer, 255, 220, 50, 240);
                for (int a = 0; a < 20; a++) {
                    float a1 = (2.0f * (float)M_PI * a) / 20.0f;
                    float a2 = (2.0f * (float)M_PI * (a + 1)) / 20.0f;
                    SDL_RenderDrawLine(renderer, hx + (int)(cosf(a1)*cr), hy + (int)(sinf(a1)*cr),
                                                 hx + (int)(cosf(a2)*cr), hy + (int)(sinf(a2)*cr));
                }
            }
        }
    }
}

static void DrawFlowerPetal(SDL_Renderer* renderer, float cx, float cy, float radius, float angle, SDL_Color color, Uint8 alpha) {
    if (alpha == 0) return;
    
    float L = radius * 2.3f;
    float W = radius * 1.1f;
    
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    float ux = cos_a, uy = sin_a;
    float vx = -sin_a, vy = cos_a;
    
    SDL_Vertex verts[8];
    // Base
    verts[0].position = (SDL_FPoint){ cx, cy };
    verts[0].color = (SDL_Color){ (Uint8)(color.r * 0.85f), (Uint8)(color.g * 0.75f), (Uint8)(color.b * 0.90f), alpha };
    verts[0].tex_coord = (SDL_FPoint){ 0, 0 };
    
    // Left lower
    verts[1].position = (SDL_FPoint){ cx + ux * (L * 0.35f) - vx * (W * 0.70f),
                                      cy + uy * (L * 0.35f) - vy * (W * 0.70f) };
    verts[1].color = (SDL_Color){ color.r, color.g, color.b, alpha };
    verts[1].tex_coord = (SDL_FPoint){ 0, 0 };
    
    // Left upper
    verts[2].position = (SDL_FPoint){ cx + ux * (L * 0.70f) - vx * W,
                                      cy + uy * (L * 0.70f) - vy * W };
    verts[2].color = (SDL_Color){ color.r, (Uint8)(color.g + 25 > 255 ? 255 : color.g + 25), color.b, alpha };
    verts[2].tex_coord = (SDL_FPoint){ 0, 0 };
    
    // Tip
    verts[3].position = (SDL_FPoint){ cx + ux * L, cy + uy * L };
    verts[3].color = (SDL_Color){ 255, 235, 245, alpha };
    verts[3].tex_coord = (SDL_FPoint){ 0, 0 };
    
    // Right upper
    verts[4].position = (SDL_FPoint){ cx + ux * (L * 0.70f) + vx * W,
                                      cy + uy * (L * 0.70f) + vy * W };
    verts[4].color = (SDL_Color){ color.r, (Uint8)(color.g + 25 > 255 ? 255 : color.g + 25), color.b, alpha };
    verts[4].tex_coord = (SDL_FPoint){ 0, 0 };
    
    // Right lower
    verts[5].position = (SDL_FPoint){ cx + ux * (L * 0.35f) + vx * (W * 0.70f),
                                      cy + uy * (L * 0.35f) + vy * (W * 0.70f) };
    verts[5].color = (SDL_Color){ color.r, color.g, color.b, alpha };
    verts[5].tex_coord = (SDL_FPoint){ 0, 0 };
    
    // Spine center 1
    verts[6].position = (SDL_FPoint){ cx + ux * (L * 0.35f), cy + uy * (L * 0.35f) };
    verts[6].color = (SDL_Color){ 255, 210, 235, alpha };
    verts[6].tex_coord = (SDL_FPoint){ 0, 0 };
    
    // Spine center 2
    verts[7].position = (SDL_FPoint){ cx + ux * (L * 0.70f), cy + uy * (L * 0.70f) };
    verts[7].color = (SDL_Color){ 255, 225, 240, alpha };
    verts[7].tex_coord = (SDL_FPoint){ 0, 0 };
    
    int indices[24] = {
        0, 1, 6,
        6, 1, 2,
        6, 2, 7,
        7, 2, 3,
        0, 6, 5,
        6, 4, 5,
        6, 7, 4,
        7, 3, 4
    };
    
    SDL_RenderGeometry(renderer, NULL, verts, 8, indices, 24);
    
    // Crisp pale vein line
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, (Uint8)(alpha * 0.85f));
    SDL_RenderDrawLine(renderer, (int)verts[0].position.x, (int)verts[0].position.y,
                                 (int)verts[6].position.x, (int)verts[6].position.y);
    SDL_RenderDrawLine(renderer, (int)verts[6].position.x, (int)verts[6].position.y,
                                 (int)verts[7].position.x, (int)verts[7].position.y);
    SDL_RenderDrawLine(renderer, (int)verts[7].position.x, (int)verts[7].position.y,
                                 (int)verts[3].position.x, (int)verts[3].position.y);
}

void Weapons_Draw(GameState* game) {
    SDL_Renderer* renderer = game->renderer;
    Vec2 camera = game->camera;
    EnsureOrbTexture(renderer);

    // Draw Projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* p = &game->projectiles[i];
        if (!p->active) continue;

        int sx = (int)(p->pos.x - camera.x);
        int sy = (int)(p->pos.y - camera.y);
        int r = (int)p->radius;
        if (r < 5) r = 5;

        if (sx < -60 || sx > WINDOW_WIDTH + 60 || sy < -60 || sy > WINDOW_HEIGHT + 60) continue;

        if (p->is_enemy) {
            // Vibrant Bullet-Hell Plasma Orbs with High-Contrast Border
            if (s_orb_texture) {
                // 1. Outer luminous energy halo
                if (s_glow_texture) {
                    int hr = (int)(r * 1.55f);
                    SDL_Rect halo = { sx - hr, sy - hr, hr * 2, hr * 2 };
                    SDL_SetTextureColorMod(s_glow_texture, p->color.r, p->color.g, p->color.b);
                    SDL_SetTextureAlphaMod(s_glow_texture, 160);
                    SDL_RenderCopy(renderer, s_glow_texture, NULL, &halo);
                }

                // 2. Dark contrast outline (makes bullets pop against ANY background!)
                SDL_Rect outline = { sx - r - 2, sy - r - 2, (r + 2) * 2, (r + 2) * 2 };
                SDL_SetTextureColorMod(s_orb_texture, 10, 15, 25);
                SDL_SetTextureAlphaMod(s_orb_texture, 230);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &outline);

                // 3. Solid saturated colored orb
                SDL_Rect body = { sx - r, sy - r, r * 2, r * 2 };
                SDL_SetTextureColorMod(s_orb_texture, p->color.r, p->color.g, p->color.b);
                SDL_SetTextureAlphaMod(s_orb_texture, 255);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &body);

                // 4. Brilliant hot white center
                int cr = (r * 4) / 9;
                if (cr < 2) cr = 2;
                SDL_Rect core = { sx - cr, sy - cr, cr * 2, cr * 2 };
                SDL_SetTextureColorMod(s_orb_texture, 255, 255, 255);
                SDL_SetTextureAlphaMod(s_orb_texture, 255);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &core);
            } else {
                SDL_SetRenderDrawColor(renderer, p->color.r, p->color.g, p->color.b, 255);
                SDL_Rect bullet = { sx - r, sy - r, r * 2, r * 2 };
                SDL_RenderFillRect(renderer, &bullet);
            }
        } else if (p->type == PROJ_BUBBLE) {
            // SQUIRTLE BUBBLE BEAM: Iridescent floating soap bubble!
            if (s_glow_texture) {
                int hr = (int)(r * 1.55f);
                SDL_Rect halo = { sx - hr, sy - hr, hr * 2, hr * 2 };
                SDL_SetTextureColorMod(s_glow_texture, 80, 220, 255);
                SDL_SetTextureAlphaMod(s_glow_texture, 110);
                SDL_RenderCopy(renderer, s_glow_texture, NULL, &halo);
            }
            if (s_orb_texture) {
                SDL_Rect body = { sx - r, sy - r, r * 2, r * 2 };
                SDL_SetTextureColorMod(s_orb_texture, 50, 190, 255);
                SDL_SetTextureAlphaMod(s_orb_texture, 85);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &body);
            }
            SDL_SetRenderDrawColor(renderer, 150, 240, 255, 220);
            for (int a = 0; a < 24; a++) {
                float a1 = (2.0f * (float)M_PI * a) / 24.0f;
                float a2 = (2.0f * (float)M_PI * (a + 1)) / 24.0f;
                SDL_RenderDrawLine(renderer, sx + (int)(cosf(a1)*r), sy + (int)(sinf(a1)*r),
                                             sx + (int)(cosf(a2)*r), sy + (int)(sinf(a2)*r));
            }
            int hx = sx - (int)(r * 0.42f);
            int hy = sy - (int)(r * 0.42f);
            int gr = (r * 3) / 8;
            if (gr < 2) gr = 2;
            if (s_orb_texture) {
                SDL_Rect gleam = { hx - gr, hy - gr, gr * 2, gr * 2 };
                SDL_SetTextureColorMod(s_orb_texture, 255, 255, 255);
                SDL_SetTextureAlphaMod(s_orb_texture, 240);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &gleam);
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 240);
                SDL_Rect gleam = { hx - gr, hy - gr, gr * 2, gr * 2 };
                SDL_RenderFillRect(renderer, &gleam);
            }
            int bx = sx + (int)(r * 0.38f);
            int by = sy + (int)(r * 0.38f);
            int br = gr / 2;
            if (br < 1) br = 1;
            SDL_SetRenderDrawColor(renderer, 210, 245, 255, 160);
            SDL_Rect b_gleam = { bx - br, by - br, br * 2, br * 2 };
            SDL_RenderFillRect(renderer, &b_gleam);
        } else if (p->type == PROJ_EMBER) {
            // CHARMANDER DIRECTIONAL FLAMETHROWER: Billowing fire puffs with heat aura!
            float prog = p->lifetime / p->max_lifetime;
            float expand_r = (float)r * (0.75f + prog * 1.15f);
            int er = (int)expand_r;
            if (er < 4) er = 4;
            Uint8 fire_alpha = (Uint8)((1.0f - prog * prog) * 240.0f);

            if (s_glow_texture) {
                int hr = (int)(er * 1.6f);
                SDL_Rect halo = { sx - hr, sy - hr, hr * 2, hr * 2 };
                SDL_SetTextureColorMod(s_glow_texture, 255, 70, 10);
                SDL_SetTextureAlphaMod(s_glow_texture, (Uint8)(fire_alpha * 0.75f));
                SDL_RenderCopy(renderer, s_glow_texture, NULL, &halo);
            }
            if (s_orb_texture) {
                SDL_Rect body = { sx - er, sy - er, er * 2, er * 2 };
                SDL_SetTextureColorMod(s_orb_texture, 255, (Uint8)(140 - prog * 80.0f), 20);
                SDL_SetTextureAlphaMod(s_orb_texture, fire_alpha);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &body);

                int cr = (int)(er * 0.45f * (1.0f - prog * 0.6f));
                if (cr >= 2) {
                    SDL_Rect core = { sx - cr, sy - cr, cr * 2, cr * 2 };
                    SDL_SetTextureColorMod(s_orb_texture, 255, 245, 160);
                    SDL_SetTextureAlphaMod(s_orb_texture, fire_alpha);
                    SDL_RenderCopy(renderer, s_orb_texture, NULL, &core);
                }
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 120, 20, fire_alpha);
                SDL_Rect puff = { sx - er, sy - er, er * 2, er * 2 };
                SDL_RenderFillRect(renderer, &puff);
            }
        } else if (p->type == PROJ_WATER_BOLT || p->type == PROJ_RAZOR_LEAF) {
            // Player Elemental Energy Bolts
            if (s_orb_texture) {
                if (s_glow_texture) {
                    int hr = (int)(r * 1.6f);
                    SDL_Rect halo = { sx - hr, sy - hr, hr * 2, hr * 2 };
                    SDL_SetTextureColorMod(s_glow_texture, p->color.r, p->color.g, p->color.b);
                    SDL_SetTextureAlphaMod(s_glow_texture, 175);
                    SDL_RenderCopy(renderer, s_glow_texture, NULL, &halo);
                }

                SDL_Rect outline = { sx - r - 2, sy - r - 2, (r + 2) * 2, (r + 2) * 2 };
                SDL_SetTextureColorMod(s_orb_texture, 10, 15, 25);
                SDL_SetTextureAlphaMod(s_orb_texture, 220);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &outline);

                SDL_Rect body = { sx - r, sy - r, r * 2, r * 2 };
                SDL_SetTextureColorMod(s_orb_texture, p->color.r, p->color.g, p->color.b);
                SDL_SetTextureAlphaMod(s_orb_texture, 255);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &body);

                int cr = (r * 4) / 9;
                if (cr < 2) cr = 2;
                SDL_Rect core = { sx - cr, sy - cr, cr * 2, cr * 2 };
                SDL_SetTextureColorMod(s_orb_texture, 255, 255, 255);
                SDL_SetTextureAlphaMod(s_orb_texture, 255);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &core);
            } else {
                SDL_SetRenderDrawColor(renderer, p->color.r, p->color.g, p->color.b, 255);
                SDL_Rect bolt = { sx - r, sy - r, r * 2, r * 2 };
                SDL_RenderFillRect(renderer, &bolt);
            }
        } else if (p->type == PROJ_ORBITER) {
            // Orbiting Shields (Aqua Orbs / Fire Spin / Petal Dance)
            if (game->player.starter == STARTER_BULBASAUR) {
                // BULBASAUR PETAL DANCE: Botanical flower petals blooming and spinning
                float spin_a = p->orbit_angle + 1.25f + p->lifetime * 3.5f;
                DrawFlowerPetal(renderer, (float)sx, (float)sy, (float)r, spin_a, p->color, (Uint8)(game->weapons.orbiters.orbiter_alpha * 255.0f));
            } else if (s_orb_texture) {
                if (s_glow_texture) {
                    int hr = (int)(r * 1.65f);
                    SDL_Rect halo = { sx - hr, sy - hr, hr * 2, hr * 2 };
                    SDL_SetTextureColorMod(s_glow_texture, p->color.r, p->color.g, p->color.b);
                    SDL_SetTextureAlphaMod(s_glow_texture, 185);
                    SDL_RenderCopy(renderer, s_glow_texture, NULL, &halo);
                }

                SDL_Rect outline = { sx - r - 2, sy - r - 2, (r + 2) * 2, (r + 2) * 2 };
                SDL_SetTextureColorMod(s_orb_texture, 10, 15, 25);
                SDL_SetTextureAlphaMod(s_orb_texture, 220);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &outline);

                SDL_Rect body = { sx - r, sy - r, r * 2, r * 2 };
                SDL_SetTextureColorMod(s_orb_texture, p->color.r, p->color.g, p->color.b);
                SDL_SetTextureAlphaMod(s_orb_texture, 255);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &body);

                int cr = (r * 4) / 8;
                if (cr < 3) cr = 3;
                SDL_Rect core = { sx - cr, sy - cr, cr * 2, cr * 2 };
                SDL_SetTextureColorMod(s_orb_texture, 255, 255, 255);
                SDL_SetTextureAlphaMod(s_orb_texture, 255);
                SDL_RenderCopy(renderer, s_orb_texture, NULL, &core);
            } else {
                SDL_SetRenderDrawColor(renderer, p->color.r, p->color.g, p->color.b, 220);
                SDL_Rect orb = { sx - r, sy - r, r * 2, r * 2 };
                SDL_RenderFillRect(renderer, &orb);
            }
        } else if (p->type == PROJ_SLASH) {
            // Thick, Sweeping Energy Crescent Blade (Vine Whip / Dragon Breath / Aqua Jet)
            float progress = p->lifetime / p->max_lifetime;
            float alpha = 1.0f - progress * progress;
            if (alpha < 0.05f) continue;

            float start_a = p->orbit_angle - p->orbit_dist * 0.5f;
            float end_a = p->orbit_angle + p->orbit_dist * 0.5f;
            const int arc_steps = 24;

            SDL_Vertex verts[80];
            int indices[300];
            int v_idx = 0;
            int i_idx = 0;

            float base_r = p->radius;
            float max_thick = 32.0f; // 32px thick curved blade ribbon!

            for (int s = 0; s <= arc_steps; s++) {
                float u = (float)s / (float)arc_steps;
                float a = start_a + (end_a - start_a) * u;
                float cos_a = cosf(a);
                float sin_a = sinf(a);

                // Taper blade at both ends
                float taper = sinf(u * (float)M_PI);
                float thick = max_thick * taper;

                float r_out = base_r;
                float r_mid = base_r - thick * 0.38f;
                float r_in  = base_r - thick;

                // Outer edge: pure white razor cutting gleam
                Uint8 a_out = (Uint8)(alpha * 255.0f);
                verts[v_idx + 0].position = (SDL_FPoint){ (float)sx + cos_a * r_out, (float)sy + sin_a * r_out };
                verts[v_idx + 0].color = (SDL_Color){ 255, 255, 255, a_out };
                verts[v_idx + 0].tex_coord = (SDL_FPoint){ 0, 0 };

                // Middle: rich saturated elemental color
                Uint8 a_mid = (Uint8)(alpha * 245.0f);
                verts[v_idx + 1].position = (SDL_FPoint){ (float)sx + cos_a * r_mid, (float)sy + sin_a * r_mid };
                verts[v_idx + 1].color = (SDL_Color){ p->color.r, p->color.g, p->color.b, a_mid };
                verts[v_idx + 1].tex_coord = (SDL_FPoint){ 0, 0 };

                // Inner trailing wake
                Uint8 a_in = (Uint8)(alpha * 55.0f);
                verts[v_idx + 2].position = (SDL_FPoint){ (float)sx + cos_a * r_in, (float)sy + sin_a * r_in };
                verts[v_idx + 2].color = (SDL_Color){ p->color.r, p->color.g, p->color.b, a_in };
                verts[v_idx + 2].tex_coord = (SDL_FPoint){ 0, 0 };

                if (s < arc_steps) {
                    int curr = s * 3;
                    int next = (s + 1) * 3;

                    // Outer quad
                    indices[i_idx++] = curr + 0;
                    indices[i_idx++] = next + 0;
                    indices[i_idx++] = next + 1;
                    indices[i_idx++] = curr + 0;
                    indices[i_idx++] = next + 1;
                    indices[i_idx++] = curr + 1;

                    // Inner quad
                    indices[i_idx++] = curr + 1;
                    indices[i_idx++] = next + 1;
                    indices[i_idx++] = next + 2;
                    indices[i_idx++] = curr + 1;
                    indices[i_idx++] = next + 2;
                    indices[i_idx++] = curr + 2;
                }
                v_idx += 3;
            }

            SDL_RenderGeometry(renderer, NULL, verts, v_idx, indices, i_idx);

            // Draw thick 3px sharp cutting edge along the outer arc perimeter
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, (Uint8)(alpha * 255.0f));
            for (int s = 0; s < arc_steps; s++) {
                float u1 = (float)s / (float)arc_steps;
                float u2 = (float)(s + 1) / (float)arc_steps;
                float a1 = start_a + (end_a - start_a) * u1;
                float a2 = start_a + (end_a - start_a) * u2;
                int x1 = sx + (int)(cosf(a1) * base_r);
                int y1 = sy + (int)(sinf(a1) * base_r);
                int x2 = sx + (int)(cosf(a2) * base_r);
                int y2 = sy + (int)(sinf(a2) * base_r);
                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
                SDL_RenderDrawLine(renderer, x1 + 1, y1, x2 + 1, y2);
                SDL_RenderDrawLine(renderer, x1, y1 + 1, x2, y2 + 1);
            }
        } else if (p->type == PROJ_PULSE_RING) {
            // Thick, Radiant Expanding Shockwave Wave (Bubble Beam / Flame Burst / Toxic Spore)
            float progress = p->lifetime / p->max_lifetime;
            float alpha = 1.0f - progress * progress;
            if (alpha < 0.05f) continue;

            const int steps = 40;
            SDL_Vertex verts[82];
            int indices[246];
            int v_idx = 0;
            int i_idx = 0;

            float r_out = p->radius;
            float thick = 18.0f; // 18px thick expanding shockwave band!
            float r_in = (r_out > thick) ? (r_out - thick) : 0.0f;

            for (int s = 0; s <= steps; s++) {
                float a = (2.0f * (float)M_PI * (float)s) / (float)steps;
                float cos_a = cosf(a);
                float sin_a = sinf(a);

                // Outer shockwave crest: brilliant white
                verts[v_idx + 0].position = (SDL_FPoint){ (float)sx + cos_a * r_out, (float)sy + sin_a * r_out };
                verts[v_idx + 0].color = (SDL_Color){ 255, 255, 255, (Uint8)(alpha * 255.0f) };
                verts[v_idx + 0].tex_coord = (SDL_FPoint){ 0, 0 };

                // Inner wave body: saturated elemental color
                verts[v_idx + 1].position = (SDL_FPoint){ (float)sx + cos_a * r_in, (float)sy + sin_a * r_in };
                verts[v_idx + 1].color = (SDL_Color){ p->color.r, p->color.g, p->color.b, (Uint8)(alpha * 135.0f) };
                verts[v_idx + 1].tex_coord = (SDL_FPoint){ 0, 0 };

                if (s < steps) {
                    int curr = s * 2;
                    int next = (s + 1) * 2;
                    indices[i_idx++] = curr + 0;
                    indices[i_idx++] = next + 0;
                    indices[i_idx++] = next + 1;
                    indices[i_idx++] = curr + 0;
                    indices[i_idx++] = next + 1;
                    indices[i_idx++] = curr + 1;
                }
                v_idx += 2;
            }

            SDL_RenderGeometry(renderer, NULL, verts, v_idx, indices, i_idx);
        }
    }

    // Draw Active Vine Whip Strikes (Procedural Wavy Multi-Segmented Leafy Vines)
    for (int i = 0; i < MAX_VINE_STRIKES; i++) {
        VineWhipStrike* vs = &game->vine_strikes[i];
        if (!vs->active) continue;

        float progress = vs->timer / vs->max_duration;
        float ext = sinf(progress * (float)M_PI);
        float cur_len = vs->max_length * ext;

        const int segs = 14;
        int sx_pts[16];
        int sy_pts[16];

        for (int k = 0; k <= segs; k++) {
            float u = (float)k / (float)segs;
            float bx = vs->start_pos.x + vs->dir.x * (cur_len * u);
            float by = vs->start_pos.y + vs->dir.y * (cur_len * u);
            float px = -vs->dir.y;
            float py = vs->dir.x;

            float wave = sinf(u * (float)M_PI * 1.8f - vs->timer * 34.0f + vs->wave_seed);
            float envelope = sinf(u * (float)M_PI);
            float amp = 24.0f * (1.0f - progress * 0.45f);

            sx_pts[k] = (int)(bx + px * (wave * envelope * amp) - camera.x);
            sy_pts[k] = (int)(by + py * (wave * envelope * amp) - camera.y);
        }

        // 1. Thick dark vine bark underline
        SDL_SetRenderDrawColor(renderer, 20, 65, 25, 255);
        for (int k = 0; k < segs; k++) {
            int x1 = sx_pts[k], y1 = sy_pts[k];
            int x2 = sx_pts[k + 1], y2 = sy_pts[k + 1];
            SDL_RenderDrawLine(renderer, x1 - 1, y1, x2 - 1, y2);
            SDL_RenderDrawLine(renderer, x1 + 1, y1, x2 + 1, y2);
            SDL_RenderDrawLine(renderer, x1, y1 - 1, x2, y2 - 1);
            SDL_RenderDrawLine(renderer, x1, y1 + 1, x2, y2 + 1);
        }

        // 2. Vibrant leafy green core
        SDL_SetRenderDrawColor(renderer, 65, 215, 65, 255);
        for (int k = 0; k < segs; k++) {
            SDL_RenderDrawLine(renderer, sx_pts[k], sy_pts[k], sx_pts[k + 1], sy_pts[k + 1]);
        }

        // 3. Leaf thorn offshoots at segments 4 and 8
        int thorn_segs[2] = { 4, 8 };
        for (int t = 0; t < 2; t++) {
            int k = thorn_segs[t];
            float thorn_sign = (t == 0) ? 1.0f : -1.0f;
            int tx = sx_pts[k] + (int)(-vs->dir.y * thorn_sign * 9.0f + vs->dir.x * 4.0f);
            int ty = sy_pts[k] + (int)(vs->dir.x * thorn_sign * 9.0f + vs->dir.y * 4.0f);
            SDL_SetRenderDrawColor(renderer, 100, 245, 90, 255);
            SDL_RenderDrawLine(renderer, sx_pts[k], sy_pts[k], tx, ty);
            SDL_Rect leaf_dot = { tx - 1, ty - 1, 3, 3 };
            SDL_RenderFillRect(renderer, &leaf_dot);
        }

        // 4. Sharp whip crack tip snap
        int tip_x = sx_pts[segs];
        int tip_y = sy_pts[segs];
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect tip_core = { tip_x - 3, tip_y - 3, 6, 6 };
        SDL_RenderFillRect(renderer, &tip_core);
    }
    
    // Draw Floating Damage Text
    if (game->dev_menu.show_damage_numbers) {
        char buf[16];
        for (int i = 0; i < MAX_PARTICLES; i++) {
            DamageText* t = &game->damage_texts[i];
            if (!t->active) continue;
            
            int sx = (int)(t->pos.x - camera.x);
            int sy = (int)(t->pos.y - camera.y);
            if (sx < -30 || sx > WINDOW_WIDTH + 30 || sy < -30 || sy > WINDOW_HEIGHT + 30) continue;
            
            float alpha = 1.0f - (t->lifetime / t->max_lifetime);
            SDL_Color col = t->color;
            col.a = (Uint8)(alpha * 255.0f);
            
            snprintf(buf, sizeof(buf), "%d", (int)t->damage);
            Font_DrawTextShadow(renderer, game->font_texture, buf, sx, sy, 2, col);
        }
    }
}
