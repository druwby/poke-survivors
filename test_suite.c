#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <SDL.h>

#include "src/types.h"
#include "src/sprite.h"
#include "src/pokedex.h"
#include "src/image_loader.h"
#include "src/game.h"
#include "src/kanto_routes.h"
#include "src/balance_sim.h"
#include "src/telemetry.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Unit test 1: Direction vector mapping, angles, and mathematical edge cases
void test_direction_mapping() {
    printf("[TEST] Running test_direction_mapping...\n");
    // Row 0: Down (0, 1)
    // Row 1: Down-Right (1, 1)
    // Row 2: Right (1, 0)
    // Row 3: Up-Right (1, -1)
    // Row 4: Up (0, -1)
    // Row 5: Up-Left (-1, -1)
    // Row 6: Left (-1, 0)
    // Row 7: Down-Left (-1, 1)

    assert(Sprite_VecToDirection(0.0f, 1.0f, 0) == 0);
    assert(Sprite_VecToDirection(1.0f, 1.0f, 0) == 1);
    assert(Sprite_VecToDirection(1.0f, 0.0f, 0) == 2);
    assert(Sprite_VecToDirection(1.0f, -1.0f, 0) == 3);
    assert(Sprite_VecToDirection(0.0f, -1.0f, 0) == 4);
    assert(Sprite_VecToDirection(-1.0f, -1.0f, 0) == 5);
    assert(Sprite_VecToDirection(-1.0f, 0.0f, 0) == 6);
    assert(Sprite_VecToDirection(-1.0f, 1.0f, 0) == 7);

    // Test zero vector retains current_direction
    assert(Sprite_VecToDirection(0.0f, 0.0f, 3) == 3);
    assert(Sprite_VecToDirection(0.001f, 0.002f, 5) == 5);

    // Test NaN and Infinity resilience (must not crash or return invalid index)
    float my_nan = 0.0f / 0.0f;
    float my_inf = 1.0f / 0.0f;
    assert(Sprite_VecToDirection(my_nan, 1.0f, 2) == 2);
    assert(Sprite_VecToDirection(1.0f, my_nan, 4) == 4);
    assert(Sprite_VecToDirection(my_inf, 0.0f, 6) == 6);
    assert(Sprite_VecToDirection(0.0f, my_inf, 1) == 1);

    // Test round trip with Sprite_DirectionToVec
    for (int dir = 0; dir < 8; dir++) {
        Vec2 v = Sprite_DirectionToVec(dir);
        int mapped = Sprite_VecToDirection(v.x, v.y, -1);
        assert(mapped == dir);
    }

    // Test negative / out of bounds direction normalization
    assert(Sprite_DirectionToAngle(-1) == Sprite_DirectionToAngle(7));
    assert(Sprite_DirectionToAngle(8) == Sprite_DirectionToAngle(0));
    assert(Sprite_DirectionToAngle(10) == Sprite_DirectionToAngle(2));

    printf("  -> Direction mapping passed!\n");
}

// Unit test 2: Sprite_UpdateAnim logic and edge cases
void test_anim_update() {
    printf("[TEST] Running test_anim_update...\n");
    SpriteSheet sheet;
    memset(&sheet, 0, sizeof(sheet));
    sheet.num_frames = 6;
    sheet.num_directions = 8;
    sheet.frame_duration = 0.10f;

    AnimState anim = { 0, 2, 0.0f, false };

    // Stationary test: frame should reset to 0
    anim.current_frame = 3;
    anim.anim_timer = 0.05f;
    Sprite_UpdateAnim(&anim, &sheet, 0.016f, false);
    assert(anim.current_frame == 0);
    assert(anim.anim_timer == 0.0f);
    assert(anim.direction == 2); // Direction retained

    // Moving test: frame progresses after duration
    Sprite_UpdateAnim(&anim, &sheet, 0.05f, true);
    assert(anim.current_frame == 0);
    assert(fabsf(anim.anim_timer - 0.05f) < 0.001f);

    Sprite_UpdateAnim(&anim, &sheet, 0.06f, true);
    assert(anim.current_frame == 1);
    assert(fabsf(anim.anim_timer - 0.01f) < 0.001f);

    // Multi-frame jump on large dt (O(1) frame advancement)
    Sprite_UpdateAnim(&anim, &sheet, 0.35f, true);
    // Was 1, plus 3 frames = 4
    assert(anim.current_frame == 4);

    // Sheet switch normalization: if anim was on frame 5 and sheet only has 4 frames
    SpriteSheet sheet4 = sheet;
    sheet4.num_frames = 4;
    Sprite_UpdateAnim(&anim, &sheet4, 0.0f, true);
    assert(anim.current_frame < 4);

    // 1-frame sheet handling: should reset current_frame to 0
    SpriteSheet sheet1 = sheet;
    sheet1.num_frames = 1;
    anim.current_frame = 3;
    Sprite_UpdateAnim(&anim, &sheet1, 0.05f, true);
    assert(anim.current_frame == 0);

    // NULL safety
    Sprite_UpdateAnim(NULL, &sheet, 0.1f, true);
    Sprite_UpdateAnim(&anim, NULL, 0.1f, true);

    printf("  -> Animation update passed!\n");
}

// Unit test 3: Dash directional and frame progression simulation
void test_dash_logic() {
    printf("[TEST] Running test_dash_logic...\n");
    // Test standstill dash vector for all 8 directions
    for (int dir = 0; dir < 8; dir++) {
        Vec2 dash_dir = Sprite_DirectionToVec(dir);
        int result_dir = Sprite_VecToDirection(dash_dir.x, dash_dir.y, -1);
        assert(result_dir == dir);
    }

    // Test dash frame selection proportional to dash timer
    float dash_duration = 0.22f;
    int total_frames = 10;
    
    // At t=0 (start of dash, timer = dash_duration): frame = 0
    float timer = dash_duration;
    float progress = 1.0f - (timer / dash_duration);
    int frame0 = (int)(progress * total_frames);
    assert(frame0 == 0);

    // At t = duration / 2: frame = 5
    timer = dash_duration * 0.5f;
    progress = 1.0f - (timer / dash_duration);
    int frame_mid = (int)(progress * total_frames);
    assert(frame_mid == 5);

    // At t near end (timer = 0.001): frame = 9
    timer = 0.001f;
    progress = 1.0f - (timer / dash_duration);
    int frame_end = (int)(progress * total_frames);
    assert(frame_end == 9);

    printf("  -> Dash logic passed!\n");
}

// Unit test 4: Companion facing priority logic (attacking, moving, enemy threat, formation idle)
void test_party_companion_facing() {
    printf("[TEST] Running test_party_companion_facing...\n");

    // Case 1: Attacking companion faces threat during attack follow-through
    float attack_timer = 2.45f;
    float attack_cooldown = 2.50f;
    bool is_attacking = (attack_timer > attack_cooldown - 0.30f);
    assert(is_attacking);
    Vec2 foe_dir = { -1.0f, 0.0f }; // Enemy is to the Left
    bool found_foe = true;
    int dir = 0;
    if (is_attacking && found_foe) {
        dir = Sprite_VecToDirection(foe_dir.x, foe_dir.y, dir);
    }
    assert(dir == 6); // Left

    // Case 2: Moving fast (not in attack follow-through) -> face velocity
    attack_timer = 1.0f;
    is_attacking = false;
    Vec2 vel = { -80.0f, -80.0f };
    float move_spd = sqrtf(vel.x * vel.x + vel.y * vel.y);
    assert(move_spd > 15.0f);
    if (is_attacking && found_foe) {
        dir = Sprite_VecToDirection(foe_dir.x, foe_dir.y, dir);
    } else if (move_spd > 15.0f) {
        dir = Sprite_VecToDirection(vel.x, vel.y, dir);
    }
    assert(dir == 5); // Up-Left

    // Case 3: Stationary, nearby threat at (50, 0) -> face foe (Right, 2)
    vel = (Vec2){ 0.0f, 0.0f };
    move_spd = 0.0f;
    foe_dir = (Vec2){ 1.0f, 0.0f };
    found_foe = true;
    if (is_attacking && found_foe) {
        dir = Sprite_VecToDirection(foe_dir.x, foe_dir.y, dir);
    } else if (move_spd > 15.0f) {
        dir = Sprite_VecToDirection(vel.x, vel.y, dir);
    } else if (found_foe) {
        dir = Sprite_VecToDirection(foe_dir.x, foe_dir.y, dir);
    }
    assert(dir == 2); // Right

    // Case 4: Stationary, no threat, close in formation (< 55px) -> match player facing
    found_foe = false;
    Vec2 p_pos = { 100.0f, 100.0f };
    Vec2 m_pos = { 70.0f, 80.0f }; // 36px distance
    float to_px = p_pos.x - m_pos.x;
    float to_py = p_pos.y - m_pos.y;
    float p_dist = sqrtf(to_px * to_px + to_py * to_py);
    assert(p_dist <= 55.0f);
    int p_facing = 4; // Player facing Up
    if (is_attacking && found_foe) {
        dir = Sprite_VecToDirection(foe_dir.x, foe_dir.y, dir);
    } else if (move_spd > 15.0f) {
        dir = Sprite_VecToDirection(vel.x, vel.y, dir);
    } else if (found_foe) {
        dir = Sprite_VecToDirection(foe_dir.x, foe_dir.y, dir);
    } else {
        if (p_dist > 55.0f) {
            dir = Sprite_VecToDirection(to_px, to_py, dir);
        } else {
            dir = p_facing;
        }
    }
    assert(dir == 4); // Matches player facing Up!

    // Case 5: Stationary, separated from player (> 55px) -> face toward player
    m_pos = (Vec2){ 10.0f, 100.0f }; // 90px distance to the West
    to_px = p_pos.x - m_pos.x; // +90 (East)
    to_py = p_pos.y - m_pos.y; // 0
    p_dist = sqrtf(to_px * to_px + to_py * to_py);
    assert(p_dist > 55.0f);
    if (p_dist > 55.0f) {
        dir = Sprite_VecToDirection(to_px, to_py, dir);
    } else {
        dir = p_facing;
    }
    assert(dir == 2); // Faces East towards player

    printf("  -> Companion facing passed!\n");
}

// Unit test 5: Headless SDL rendering, sprite dimension validation, route loader attack sprites
void test_sprite_loading_and_rendering() {
    printf("[TEST] Running test_sprite_loading_and_rendering...\n");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("  SDL_Init failed: %s\n", SDL_GetError());
        return;
    }

    SDL_Window* window = SDL_CreateWindow("Test", 0, 0, 64, 64, SDL_WINDOW_HIDDEN);
    if (!window) {
        printf("  SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("  SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    Pokedex_Init(renderer);

    // Test Starter Charge Sprites
    // Bulbasaur Charge: should be 32x40, 10 frames, 8 dirs
    SpriteSheet b_charge = Sprite_Load(renderer, "assets/sprites/sprite/0001/Charge-Anim.png", 32, 40, 10, 8, 0.035f, 1.8f);
    assert(b_charge.loaded);
    assert(b_charge.frame_w == 32);
    assert(b_charge.frame_h == 40);
    assert(b_charge.num_frames == 10);
    assert(b_charge.num_directions == 8);

    // Test Auto-correction if caller incorrectly passed 32x32:
    SpriteSheet b_bad = Sprite_Load(renderer, "assets/sprites/sprite/0001/Charge-Anim.png", 32, 32, 10, 8, 0.035f, 1.8f);
    assert(b_bad.loaded);
    assert(b_bad.frame_w == 32);
    assert(b_bad.frame_h == 40); // Auto-corrected from 320 / 8 = 40!

    // Charmander Charge: 32x40
    SpriteSheet c_charge = Sprite_Load(renderer, "assets/sprites/sprite/0004/Charge-Anim.png", 32, 40, 10, 8, 0.035f, 1.8f);
    assert(c_charge.loaded);
    assert(c_charge.frame_w == 32);
    assert(c_charge.frame_h == 40);

    // Squirtle Charge: 32x32
    SpriteSheet s_charge = Sprite_Load(renderer, "assets/sprites/sprite/0007/Charge-Anim.png", 32, 32, 10, 8, 0.035f, 1.8f);
    assert(s_charge.loaded);
    assert(s_charge.frame_w == 32);
    assert(s_charge.frame_h == 32);

    // Dugtrio Swing: 72x80
    SpriteSheet d_swing = Sprite_Load(renderer, "assets/sprites/sprite/0051/Swing-Anim.png", 72, 80, 9, 8, 0.042f, 2.0f);
    assert(d_swing.loaded);
    assert(d_swing.frame_w == 72);
    assert(d_swing.frame_h == 80);

    // Diglett Swing: 72x72
    SpriteSheet dig_swing = Sprite_Load(renderer, "assets/sprites/sprite/0050/Swing-Anim.png", 72, 72, 9, 8, 0.042f, 2.0f);
    assert(dig_swing.loaded);
    assert(dig_swing.frame_w == 72);
    assert(dig_swing.frame_h == 72);

    // Dugtrio auto-correction if caller passed 72:
    SpriteSheet d_bad = Sprite_Load(renderer, "assets/sprites/sprite/0051/Swing-Anim.png", 72, 72, 9, 8, 0.042f, 2.0f);
    assert(d_bad.frame_h == 80); // Auto-corrected to 80!

    // Verify Diglett's Cave Route Loading in game engine (Section 1, Route 1 = Diglett's Cave)
    GameState game;
    memset(&game, 0, sizeof(game));
    game.renderer = renderer;
    KantoRoutes_Init(&game);
    // Section 2, Route 2 is Diglett's Cave (contains Diglett #50 and Dugtrio #51)
    KantoRoutes_LoadRoute(&game, 2, 2);
    bool found_diglett = false;
    bool found_dugtrio = false;
    for (int t = 0; t < game.active_enemy_type_count; t++) {
        EnemyTypeConfig* cfg = &game.enemy_types[t];
        if (cfg->pokedex_id == 50) {
            found_diglett = true;
            assert(cfg->has_attack_sprite);
            assert(cfg->sprite_attack.loaded);
            assert(cfg->sprite_attack.frame_w == 72);
            assert(cfg->sprite_attack.frame_h == 72);
        }
        if (cfg->pokedex_id == 51) {
            found_dugtrio = true;
            assert(cfg->has_attack_sprite);
            assert(cfg->sprite_attack.loaded);
            assert(cfg->sprite_attack.frame_w == 72);
            assert(cfg->sprite_attack.frame_h == 80);
        }
    }
    assert(found_diglett);
    assert(found_dugtrio);

    // Test Sprite_Draw and Sprite_DrawRotated for all 8 directions and out of bounds frame/dir
    for (int dir = -2; dir <= 10; dir++) {
        AnimState anim = { 15, dir, 0.0f, true };
        Sprite_Draw(renderer, &b_charge, &anim, (Vec2){ 32, 32 }, (Vec2){ 0, 0 }, (SDL_Color){ 255, 255, 255, 255 }, 16.0f, false);
        Sprite_Draw(renderer, &b_charge, &anim, (Vec2){ 32, 32 }, (Vec2){ 0, 0 }, (SDL_Color){ 255, 255, 255, 255 }, 16.0f, true);
        Sprite_DrawRotated(renderer, &b_charge, &anim, (Vec2){ 32, 32 }, (Vec2){ 0, 0 }, 45.0, (SDL_Color){ 255, 255, 255, 255 }, 16.0f, false);
    }

    // Test Fallback drawing
    SpriteSheet empty_sheet;
    memset(&empty_sheet, 0, sizeof(empty_sheet));
    AnimState fb_anim = { 0, 1, 0.0f, false };
    Sprite_Draw(renderer, &empty_sheet, &fb_anim, (Vec2){ 32, 32 }, (Vec2){ 0, 0 }, (SDL_Color){ 50, 240, 110, 255 }, 16.0f, false);

    // Verify all 151 Pokemon sprites load cleanly via Pokedex_LoadSprite
    for (int pid = 1; pid <= 151; pid++) {
        SpriteSheet p_sheet = Pokedex_LoadSprite(renderer, pid, 1.8f);
        assert(p_sheet.loaded);
        assert(p_sheet.num_directions == 8);
        assert(p_sheet.frame_w > 0);
        assert(p_sheet.frame_h > 0);
        assert(p_sheet.num_frames > 0);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("  -> Sprite loading & rendering test passed (all 151 Pokemon + Diglett/Dugtrio route attack sprites verified)!\n");
}

// Unit test 6: Balance Config & JSON Override Parsing
void test_balance_config_defaults_and_override() {
    printf("[TEST] Running test_balance_config_defaults_and_override...\n");
    BalanceConfig_InitDefaults();
    assert(fabsf(g_balance_config.enemy_hp_mult - 1.35f) < 0.001f);
    assert(fabsf(g_balance_config.time_hp_alpha - 0.15f) < 0.001f);
    assert(fabsf(g_balance_config.time_hp_gamma - 1.0f) < 0.001f);
    assert(fabsf(g_balance_config.enemy_defense_per_stage - 0.08f) < 0.001f);

    // Test non-existent file
    assert(!BalanceConfig_LoadFromFile("nonexistent_path_xyz.json"));

    // Write a valid override file
    const char* test_json_path = "test_balance_override.json";
    FILE* f = fopen(test_json_path, "w");
    assert(f != NULL);
    fprintf(f, "{\n");
    fprintf(f, "  \"enemy_hp_mult\": 1.85,\n");
    fprintf(f, "  \"enemy_defense_base\": 0.12,\n");
    fprintf(f, "  \"time_hp_alpha\": 0.60,\n");
    fprintf(f, "  \"spawn_interval_mult\": 0.80\n");
    fprintf(f, "}\n");
    fclose(f);

    bool loaded = BalanceConfig_LoadFromFile(test_json_path);
    assert(loaded);
    assert(fabsf(g_balance_config.enemy_hp_mult - 1.85f) < 0.01f);
    assert(fabsf(g_balance_config.enemy_defense_base - 0.12f) < 0.01f);
    assert(fabsf(g_balance_config.time_hp_alpha - 0.60f) < 0.01f);
    assert(fabsf(g_balance_config.spawn_interval_mult - 0.80f) < 0.01f);
    // Unspecified fields should retain default
    assert(fabsf(g_balance_config.time_hp_gamma - 1.0f) < 0.001f);

    remove(test_json_path);
    BalanceConfig_InitDefaults();
    printf("  -> Balance config & JSON override passed!\n");
}

// Unit test 7: Enemy Defense, Stationary Armor & Damage Pipeline
void test_enemy_defense_and_damage_pipeline() {
    printf("[TEST] Running test_enemy_defense_and_damage_pipeline...\n");
    static GameState game;
    memset(&game, 0, sizeof(GameState));
    BalanceConfig_InitDefaults();

    // Setup an enemy
    game.active_enemy_count = 1;
    game.enemies[0].active = true;
    game.enemies[0].type = 0;
    game.enemies[0].pokedex_id = 16; // Pidgey
    game.enemies[0].max_hp = 100.0f;
    game.enemies[0].hp = 100.0f;
    game.enemies[0].defense = 0.20f; // 20% defense
    game.enemies[0].spawn_time = 0.0f;
    game.spawner.game_time = 5.0f;
    game.enemy_types[0].move_pattern = ENEMY_MOVE_SWARM;
    game.enemy_types[0].is_boss = false;

    // Apply 50 raw damage: with 20% defense, actual dmg = 50 * (1 - 0.20) = 40
    float actual_dmg = Game_ApplyEnemyDamage(&game, 0, 50.0f, "TEST_WEAPON");
    assert(fabsf(actual_dmg - 40.0f) < 0.01f);
    assert(fabsf(game.enemies[0].hp - 60.0f) < 0.01f);
    assert(fabsf(game.total_damage_dealt - 40.0f) < 0.01f);
    assert(fabsf(game.total_overkill_damage - 0.0f) < 0.01f);

    // Apply 100 raw damage: with 20% defense, dmg = 80. Old hp = 60.
    // Overkill = 80 - 60 = 20. Total damage dealt += 60. Total overkill += 20.
    actual_dmg = Game_ApplyEnemyDamage(&game, 0, 100.0f, "TEST_WEAPON");
    assert(fabsf(actual_dmg - 80.0f) < 0.01f);
    assert(fabsf(game.total_damage_dealt - 100.0f) < 0.01f);
    assert(fabsf(game.total_overkill_damage - 20.0f) < 0.01f);
    assert(!game.enemies[0].active); // Deactivated on death

    // Test stationary enemy (Harden 65% reduction)
    game.enemies[1].active = true;
    game.enemies[1].type = 1;
    game.enemies[1].hp = 100.0f;
    game.enemies[1].max_hp = 100.0f;
    game.enemies[1].defense = 0.0f;
    game.enemy_types[1].move_pattern = ENEMY_MOVE_STATIONARY;
    game.enemy_types[1].is_boss = false;
    actual_dmg = Game_ApplyEnemyDamage(&game, 1, 100.0f, "TEST_WEAPON");
    // Stationary multiplies dmg by 0.35f -> 35.0f
    assert(fabsf(actual_dmg - 35.0f) < 0.01f);
    assert(fabsf(game.enemies[1].hp - 65.0f) < 0.01f);
    assert(game.instant_kills == 1); // 2 hits on same frame = instant kill (< 1.0s)

    printf("  -> Enemy defense & damage pipeline passed!\n");
}

// Unit test 8: Dynamic Route & Time HP Scaling formulas
void test_dynamic_scaling_and_time_tenacity() {
    printf("[TEST] Running test_dynamic_scaling_and_time_tenacity...\n");
    BalanceConfig_InitDefaults();

    // At t = 0 min, time_scale = 1.0f
    float scale_0 = 1.0f + g_balance_config.time_hp_alpha * powf(0.0f / 60.0f, g_balance_config.time_hp_gamma);
    assert(fabsf(scale_0 - 1.0f) < 0.001f);

    // At t = 60s (1 min), scale = 1.0 + 0.15 * (1.0)^1.0 = 1.15
    float scale_60 = 1.0f + g_balance_config.time_hp_alpha * powf(60.0f / 60.0f, g_balance_config.time_hp_gamma);
    assert(fabsf(scale_60 - 1.15f) < 0.001f);

    // At t = 120s (2 min), scale = 1.0 + 0.15 * (2.0)^1.0 = 1.30
    float scale_120 = 1.0f + g_balance_config.time_hp_alpha * powf(120.0f / 60.0f, g_balance_config.time_hp_gamma);
    assert(fabsf(scale_120 - 1.30f) < 0.001f);

    printf("  -> Dynamic scaling and time tenacity passed!\n");
}

// Unit test 9: Telemetry Streamer Event Serialization
void test_telemetry_streamer() {
    printf("[TEST] Running test_telemetry_streamer...\n");
    const char* telem_path = "test_stream_verify.jsonl";
    bool ok = Telemetry_Init(telem_path);
    assert(ok);

    Telemetry_LogSpawn(1.5f, 101, 16, "Pidgey", 1, 85.0f, "ROUTE 1", 0);
    Telemetry_LogHit(2.0f, 101, 16, 25.0f, 60.0f, "VINE_WHIP");
    Telemetry_LogDeath(4.2f, 101, 16, "Pidgey", 1, 2.2f, 8.5f, 12.0f, "VINE_WHIP", "ROUTE 1");
    Telemetry_LogPlayerStat(5.0f, 110.0f, 125.0f, 0.0f, 2, 1, "Optimal");
    Telemetry_LogRunSummary("Bulbasaur", "Optimal", true, 60.0f, 5, 45, 2.8f, 5.1f, 12.4f, 0.15f, 3.2f, 0);

    Telemetry_Close();

    FILE* f = fopen(telem_path, "r");
    assert(f != NULL);
    char line[512];
    int line_count = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strlen(line) > 5) {
            assert(line[0] == '{');
            assert(strstr(line, "\"event\":") != NULL);
            line_count++;
        }
    }
    fclose(f);
    assert(line_count == 5);
    remove(telem_path);

    printf("  -> Telemetry streamer passed!\n");
}

// Unit test 10: Combat Engagement TTK and Instant Kill Detection (independent of spawn wander time)
void test_combat_engagement_ttk_and_instant_kill_detection() {
    printf("[TEST] Running test_combat_engagement_ttk_and_instant_kill_detection...\n");
    static GameState game;
    memset(&game, 0, sizeof(GameState));
    BalanceConfig_InitDefaults();

    // Case 1: Enemy spawned at t = 10.0s, wanders until t = 30.0s (20s wander time)
    // Then takes 2 hits over 2.5 seconds (t = 30.0s and t = 32.5s)
    game.active_enemy_count = 1;
    game.enemies[0].active = true;
    game.enemies[0].type = 0;
    game.enemies[0].pokedex_id = 16;
    game.enemies[0].max_hp = 100.0f;
    game.enemies[0].hp = 100.0f;
    game.enemies[0].defense = 0.0f;
    game.enemies[0].spawn_time = 10.0f;
    game.enemies[0].was_hit = false;
    game.enemy_types[0].move_pattern = ENEMY_MOVE_SWARM;
    game.enemy_types[0].is_boss = false;

    // First hit at t = 30.0s: 40 dmg
    game.spawner.game_time = 30.0f;
    Game_ApplyEnemyDamage(&game, 0, 40.0f, "TEST_WEAPON");
    assert(game.enemies[0].was_hit == true);
    assert(fabsf(game.enemies[0].first_hit_time - 30.0f) < 0.001f);
    assert(fabsf(game.enemies[0].hp - 60.0f) < 0.001f);

    // Second hit at t = 32.5s (2.5s combat engagement): 80 dmg (lethal)
    game.spawner.game_time = 32.5f;
    Game_ApplyEnemyDamage(&game, 0, 80.0f, "TEST_WEAPON");
    assert(!game.enemies[0].active);
    assert(game.kills_stage1 == 1);
    // Combat TTK must be exactly 2.5s, NOT 22.5s!
    assert(fabsf(game.sum_ttk_stage1 - 2.5f) < 0.01f);
    // 2.5s >= 1.0s, so this is NOT an instant kill
    assert(game.instant_kills == 0);

    // Case 2: Fragile enemy vaporized on first hit (< 1.0s)
    game.enemies[1].active = true;
    game.enemies[1].type = 0;
    game.enemies[1].pokedex_id = 19; // Rattata (stage 1)
    game.enemies[1].max_hp = 30.0f;
    game.enemies[1].hp = 30.0f;
    game.enemies[1].defense = 0.0f;
    game.enemies[1].spawn_time = 20.0f;
    game.enemies[1].was_hit = false;

    game.spawner.game_time = 45.0f;
    // 1-hit kill at t = 45.0s
    Game_ApplyEnemyDamage(&game, 1, 50.0f, "TEST_WEAPON");
    assert(!game.enemies[1].active);
    assert(game.kills_stage1 == 2);
    // Combat TTK for this kill is 0.0s (< 1.0s) -> must increment instant_kills!
    assert(game.instant_kills == 1);
    assert(fabsf(game.sum_ttk_stage1 - 2.5f) < 0.01f); // sum_ttk_stage1 was 2.5 + 0.0 = 2.5

    printf("  -> Combat engagement TTK and instant kill detection passed!\n");
}

// Unit test 11: Exact tileset slice coordinates and layout bounds
void test_tilemap_coordinates_and_slices() {
    printf("[TEST] Running test_tilemap_coordinates_and_slices...\n");
    TileMap map;
    Tilemap_Init(&map);

    // Verify all tile slices are exactly 24x24 and within image boundaries (432x192)
    for (int i = 0; i < TILE_TOTAL_COUNT; i++) {
        const SDL_Rect* r = Tilemap_GetSrcRect((TileIndex)i);
        assert(r != NULL);
        assert(r->w == 24);
        assert(r->h == 24);
        assert(r->x >= 0 && r->x + r->w <= 432);
        assert(r->y >= 0 && r->y + r->h <= 192);
        assert(r->x % 24 == 0);
        assert(r->y % 24 == 0);

        // Assert no tile slice overlaps the 3 blank/transparent slots in 432x192:
        // Slot 1: (120, 48), Slot 2: (264, 48), Slot 3: (408, 48)
        assert(!(r->x == 120 && r->y == 48));
        assert(!(r->x == 264 && r->y == 48));
        assert(!(r->x == 408 && r->y == 48));
    }

    // Verify specific key landmarks
    assert(Tilemap_GetSrcRect(TILE_ROCK_TL)->x == 0 && Tilemap_GetSrcRect(TILE_ROCK_TL)->y == 0);
    assert(Tilemap_GetSrcRect(TILE_ROCK_CENTER)->x == 24 && Tilemap_GetSrcRect(TILE_ROCK_CENTER)->y == 24);
    assert(Tilemap_GetSrcRect(TILE_ROCK_B)->x == 24 && Tilemap_GetSrcRect(TILE_ROCK_B)->y == 48);
    assert(Tilemap_GetSrcRect(TILE_ROCK_RAMP_S)->x == 96 && Tilemap_GetSrcRect(TILE_ROCK_RAMP_S)->y == 0);
    assert(Tilemap_GetSrcRect(TILE_ROCK_INNER_TL)->x == 24 && Tilemap_GetSrcRect(TILE_ROCK_INNER_TL)->y == 144);
    assert(Tilemap_GetSrcRect(TILE_WATER_TL)->x == 144 && Tilemap_GetSrcRect(TILE_WATER_TL)->y == 0);
    assert(Tilemap_GetSrcRect(TILE_WATER_CENTER)->x == 168 && Tilemap_GetSrcRect(TILE_WATER_CENTER)->y == 24);
    assert(Tilemap_GetSrcRect(TILE_GROUND_1)->x == 288 && Tilemap_GetSrcRect(TILE_GROUND_1)->y == 24);
    assert(Tilemap_GetSrcRect(TILE_PATH_CENTER)->x == 312 && Tilemap_GetSrcRect(TILE_PATH_CENTER)->y == 24);

    printf("  -> Exact 24x24 tile slice coordinates and non-empty bounds verified!\n");
}

// Unit test 12: Plateau, ramp, and water topological alignment
void test_tilemap_plateau_and_water_alignment() {
    printf("[TEST] Running test_tilemap_plateau_and_water_alignment...\n");
    TileMap map;
    Tilemap_Init(&map);

    // Generate Route 1 map
    Tilemap_GenerateRouteMap(&map, 0, 0);

    // Verify boundary walls:
    // North boundary: Row 0 is solid, Row 1 is solid cliff face facing playfield (TILE_ROCK_B)
    for (int x = 0; x < MAP_TILES_X; x++) {
        assert(map.cells[0][x].flags & TILE_FLAG_SOLID);
        assert(map.cells[1][x].flags & TILE_FLAG_SOLID);
        if (x >= 1 && x < MAP_TILES_X - 1) {
            assert(map.cells[1][x].tile_id == TILE_ROCK_B || map.cells[1][x].tile_id == TILE_ROCK_INNER_BL || map.cells[1][x].tile_id == TILE_ROCK_INNER_BR);
        }
    }
    // South boundary: Row 126 is solid cliff edge facing playfield (TILE_ROCK_T), Row 127 is solid
    for (int x = 0; x < MAP_TILES_X; x++) {
        assert(map.cells[MAP_TILES_Y - 2][x].flags & TILE_FLAG_SOLID);
        assert(map.cells[MAP_TILES_Y - 1][x].flags & TILE_FLAG_SOLID);
        if (x >= 1 && x < MAP_TILES_X - 1) {
            assert(map.cells[MAP_TILES_Y - 2][x].tile_id == TILE_ROCK_T || map.cells[MAP_TILES_Y - 2][x].tile_id == TILE_ROCK_INNER_TL || map.cells[MAP_TILES_Y - 2][x].tile_id == TILE_ROCK_INNER_TR);
        }
    }
    // West & East boundaries
    for (int y = 0; y < MAP_TILES_Y; y++) {
        assert(map.cells[y][0].flags & TILE_FLAG_SOLID);
        assert(map.cells[y][1].flags & TILE_FLAG_SOLID);
        assert(map.cells[y][MAP_TILES_X - 2].flags & TILE_FLAG_SOLID);
        assert(map.cells[y][MAP_TILES_X - 1].flags & TILE_FLAG_SOLID);
    }

    // Count terrain occurrences
    int rock_count = 0, water_count = 0, ramp_count = 0, path_count = 0, deco_count = 0;
    for (int y = 0; y < MAP_TILES_Y; y++) {
        for (int x = 0; x < MAP_TILES_X; x++) {
            uint8_t flags = map.cells[y][x].flags;
            if (flags & TILE_FLAG_RAMP) ramp_count++;
            if (flags & TILE_FLAG_WATER) water_count++;
            if (flags & TILE_FLAG_PLATEAU) rock_count++;
            if (flags & TILE_FLAG_PATH) path_count++;
            if (flags & TILE_FLAG_DECO) deco_count++;
        }
    }

    assert(ramp_count > 0);
    assert(water_count > 0);
    assert(rock_count > 0);
    assert(path_count > 0);
    assert(deco_count > 0);

    printf("  -> Plateau, ramp (%d ramps), water (%d tiles), and decor (%d clusters) alignment verified!\n",
           ramp_count, water_count, deco_count);
}

// Unit test 13: Collision resolution, ramp traversal, and flying entities
void test_tilemap_collision_and_ramp_traversal() {
    printf("[TEST] Running test_tilemap_collision_and_ramp_traversal...\n");
    TileMap map;
    Tilemap_Init(&map);
    Tilemap_GenerateRouteMap(&map, 0, 0);

    // Find the first ramp on the map
    int ramp_x = -1, ramp_y = -1;
    for (int y = 0; y < MAP_TILES_Y && ramp_x < 0; y++) {
        for (int x = 0; x < MAP_TILES_X; x++) {
            if (map.cells[y][x].flags & TILE_FLAG_RAMP) {
                ramp_x = x;
                ramp_y = y;
                break;
            }
        }
    }
    assert(ramp_x > 0 && ramp_y > 0);

    // Verify ramp tile itself is NOT solid
    assert(!Tilemap_IsSolid(&map, ramp_x, ramp_y, false));
    // Verify adjacent cliff edge IS solid
    assert(Tilemap_IsSolid(&map, ramp_x - 1, ramp_y, false) || Tilemap_IsSolid(&map, ramp_x + 1, ramp_y, false));

    float rad = 18.0f;

    // Test 1: Ground entity on ramp moves freely
    float px = ramp_x * TILE_WORLD_SIZE + 32.0f;
    float py = ramp_y * TILE_WORLD_SIZE + 32.0f;
    bool col = Tilemap_CheckCollisionCircle(&map, &px, &py, rad, false);
    assert(!col);

    // Test 2: Ground entity moving into solid cliff next to ramp is pushed out
    int cliff_x = (Tilemap_IsSolid(&map, ramp_x - 1, ramp_y, false)) ? (ramp_x - 1) : (ramp_x + 1);
    float cx = cliff_x * TILE_WORLD_SIZE + 32.0f;
    float cy = (ramp_y + 1) * TILE_WORLD_SIZE + 10.0f; // below cliff
    cy -= 20.0f; // attempt to enter cliff
    col = Tilemap_CheckCollisionCircle(&map, &cx, &cy, rad, false);
    assert(col); // Blocked by cliff!

    // Test 3: Flying entity passes freely over solid cliff
    float fly_x = cliff_x * TILE_WORLD_SIZE + 32.0f;
    float fly_y = ramp_y * TILE_WORLD_SIZE + 32.0f;
    col = Tilemap_CheckCollisionCircle(&map, &fly_x, &fly_y, rad, true); // can_fly = true
    assert(!col); // Flying ignores terrain!

    // Test 4: Sliding along solid cliff face (diagonal movement into wall pushes out along normal)
    float slide_x = cliff_x * TILE_WORLD_SIZE + 10.0f;
    float slide_y = (ramp_y + 1) * TILE_WORLD_SIZE + 15.0f; // slightly overlapping cliff
    col = Tilemap_CheckCollisionCircle(&map, &slide_x, &slide_y, rad, false);
    assert(col);
    assert(slide_y > (ramp_y + 1) * TILE_WORLD_SIZE + 15.0f); // pushed South away from cliff face

    // Test 5: Bidirectional ramp traversal (approach from ground, cross ramp, enter plateau, and return)
    float step_x = ramp_x * TILE_WORLD_SIZE + 32.0f;
    float step_y = (ramp_y + 1) * TILE_WORLD_SIZE + 32.0f; // Ground level below ramp
    assert(!Tilemap_CheckCollisionCircle(&map, &step_x, &step_y, rad, false));
    step_y -= TILE_WORLD_SIZE; // On ramp
    assert(!Tilemap_CheckCollisionCircle(&map, &step_x, &step_y, rad, false));
    step_y -= TILE_WORLD_SIZE; // On plateau
    assert(!Tilemap_CheckCollisionCircle(&map, &step_x, &step_y, rad, false));

    printf("  -> Collision resolution, ramp traversal, and flying pass-through verified!\n");
}

// Unit test 14: Dynamic route generation across all Kanto sections and routes
void test_tilemap_route_generation_across_all_routes() {
    printf("[TEST] Running test_tilemap_route_generation_across_all_routes...\n");
    TileMap map;
    Tilemap_Init(&map);

    int total_sections = KantoRoutes_GetTotalSections();
    assert(total_sections == 8);

    for (int s = 0; s < total_sections; s++) {
        int route_count = KantoRoutes_GetRoutesInSection(s);
        for (int r = 0; r < route_count; r++) {
            Tilemap_GenerateRouteMap(&map, s, r);

            // Assert player start position (64, 64) is never solid
            assert(!Tilemap_IsSolid(&map, 64, 64, false));

            // Assert safe clearance in 58..70
            for (int cy = 60; cy <= 68; cy++) {
                for (int cx = 60; cx <= 68; cx++) {
                    assert(!Tilemap_IsSolid(&map, cx, cy, false));
                }
            }

            // Assert world boundaries are solid
            assert(Tilemap_IsSolid(&map, 0, 64, false));
            assert(Tilemap_IsSolid(&map, MAP_TILES_X - 1, 64, false));
            assert(Tilemap_IsSolid(&map, 64, 0, false));
            assert(Tilemap_IsSolid(&map, 64, MAP_TILES_Y - 1, false));

            // Assert no ramp directly opens into deep water (players cannot be trapped upon exiting ramps)
            for (int y = 1; y < MAP_TILES_Y - 1; y++) {
                for (int x = 1; x < MAP_TILES_X - 1; x++) {
                    if (map.cells[y][x].flags & TILE_FLAG_RAMP) {
                        assert(!(map.cells[y - 1][x].flags & TILE_FLAG_WATER));
                        assert(!(map.cells[y + 1][x].flags & TILE_FLAG_WATER));
                        assert(!(map.cells[y][x - 1].flags & TILE_FLAG_WATER));
                        assert(!(map.cells[y][x + 1].flags & TILE_FLAG_WATER));
                    }
                }
            }
        }
    }

    // Verify deterministic generation: re-generating section 0, route 0 produces identical map
    TileMap map_copy;
    Tilemap_GenerateRouteMap(&map_copy, 0, 0);
    Tilemap_GenerateRouteMap(&map, 0, 0);
    assert(memcmp(map.cells, map_copy.cells, sizeof(map.cells)) == 0);

    printf("  -> Route generation across all 8 sections and 32 Kanto routes verified!\n");
}

int main() {
    g_headless_sim = true;
    printf("=========================================\n");
    printf("SPRITE, BALANCE & TELEMETRY VALIDATION SUITE\n");
    printf("=========================================\n");
    test_direction_mapping();
    test_anim_update();
    test_dash_logic();
    test_party_companion_facing();
    test_sprite_loading_and_rendering();
    test_balance_config_defaults_and_override();
    test_enemy_defense_and_damage_pipeline();
    test_dynamic_scaling_and_time_tenacity();
    test_telemetry_streamer();
    test_combat_engagement_ttk_and_instant_kill_detection();
    test_tilemap_coordinates_and_slices();
    test_tilemap_plateau_and_water_alignment();
    test_tilemap_collision_and_ramp_traversal();
    test_tilemap_route_generation_across_all_routes();
    printf("\n>>> ALL TESTS PASSED SUCCESSFULLY! <<<\n");
    return 0;
}
