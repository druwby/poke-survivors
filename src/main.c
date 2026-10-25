#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include "config.h"
#include "types.h"
#include "game.h"
#include "profiler.h"
#include "dev_menu.h"
#include "balance_sim.h"
#include "pokedex.h"

int main(int argc, char* argv[]) {
    bool benchmark_mode = false;
    bool balance_test_mode = false;
    int sim_trials = 10; // Default to 10 trials per starter for robust multi-trial statistical averaging
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--benchmark") == 0 || strcmp(argv[i], "-b") == 0) {
            benchmark_mode = true;
        } else if (strcmp(argv[i], "--balance-test") == 0 || strcmp(argv[i], "--simulate") == 0 || strcmp(argv[i], "-s") == 0) {
            balance_test_mode = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                int n = atoi(argv[i + 1]);
                if (n > 0) sim_trials = n;
            }
        } else if (strcmp(argv[i], "--trials") == 0 || strcmp(argv[i], "-n") == 0) {
            if (i + 1 < argc) {
                int n = atoi(argv[++i]);
                if (n > 0) sim_trials = n;
            }
        }
    }
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "Failed to initialize SDL2: %s\n", SDL_GetError());
        return 1;
    }
    
    // Pixel-perfect nearest neighbor filtering for retro sprites
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    
    Uint32 window_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
    if (benchmark_mode || balance_test_mode) {
        window_flags |= SDL_WINDOW_HIDDEN;
    } else {
        window_flags |= SDL_WINDOW_SHOWN;
    }
    
    SDL_Window* window = SDL_CreateWindow(
        "Rogue Survivors [C99 High-Performance Engine]",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        window_flags
    );
    
    if (!window) {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;
    if (!benchmark_mode && !balance_test_mode) {
        render_flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, render_flags);
    if (!renderer) {
        fprintf(stderr, "Failed to create SDL renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Scale 1280x720 logical canvas to fill full Retina / HighDPI framebuffer
    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    if (balance_test_mode) {
        BalanceSim_RunTrials(window, renderer, sim_trials, 300.0f);
        Pokedex_Cleanup();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }
    
    static GameState game;
    Game_Init(&game, window, renderer);
    game.vsync_enabled = !benchmark_mode;
    
    if (benchmark_mode) {
        Game_SelectStarter(&game, STARTER_SQUIRTLE);
        printf("\n=======================================================\n");
        printf(" RUNNING HIGH-LOAD PROFILER STRESS BENCHMARK\n");
        printf(" Spawning 1,200 active enemies, 4 weapons, spatial grid\n");
        printf("=======================================================\n");
        
        // Spawn 1,200 enemies for heavy stress test
        for (int i = 0; i < 1200 && i < MAX_ENEMIES; i++) {
            Game_ActivateEnemy(&game, i);
            game.enemies[i].type = (EnemyTypeID)(i % 2);
            game.enemies[i].hp = 50.0f;
            game.enemies[i].max_hp = 50.0f;
            game.enemies[i].radius = 16.0f;
            game.enemies[i].touch_damage = 10.0f;
            game.enemies[i].ai_state = ENEMY_AI_CHASE;
            game.enemies[i].is_underground = (game.enemy_types[game.enemies[i].type].attack_type == ENEMY_ATTACK_BURROW_ASSASSIN);
            game.enemies[i].attack_cooldown_timer = (float)(i % 5) * 0.5f;
            game.enemies[i].attack_has_hit = false;
            game.enemies[i].attack_anim = (AnimState){0};
            float angle = (float)i * 0.1f;
            float dist = 200.0f + (float)(i % 400);
            game.enemies[i].pos.x = game.player.pos.x + cosf(angle) * dist;
            game.enemies[i].pos.y = game.player.pos.y + sinf(angle) * dist;
        }
    }
    
    uint64_t last_counter = SDL_GetPerformanceCounter();
    const double perf_freq = (double)SDL_GetPerformanceFrequency();
    int frame_count = 0;
    const int benchmark_max_frames = 300;
    
    while (game.running) {
        Profiler_StartFrame(&game.profiler);
        
        uint64_t current_counter = SDL_GetPerformanceCounter();
        double delta_seconds = (double)(current_counter - last_counter) / perf_freq;
        last_counter = current_counter;
        
        if (delta_seconds > 0.05) delta_seconds = 0.05;
        float dt = benchmark_mode ? (1.0f / 60.0f) : (float)delta_seconds;
        
        // 1. Event Poll
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                game.running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (game.dev_menu.open) {
                        game.dev_menu.open = false;
                        if (game.dev_menu.pause_when_open) game.paused = false;
                    } else {
                        game.running = false;
                    }
                } else if (event.key.keysym.sym == SDLK_F11) {
                    Uint32 flags = SDL_GetWindowFlags(window);
                    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                        SDL_SetWindowFullscreen(window, 0);
                    } else {
                        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                } else if (event.key.keysym.sym == SDLK_F2) {
                    game.vsync_enabled = !game.vsync_enabled;
                    SDL_RenderSetVSync(renderer, game.vsync_enabled ? 1 : 0);
                    printf("[Display] VSync %s\n", game.vsync_enabled ? "ENABLED (Locked to refresh)" : "DISABLED (Uncapped)");
                }
            }
            Game_HandleEvent(&game, &event);
            DevMenu_HandleEvent(&game, &event);
        }
        
        // --- ACTIVE CPU WORK (Simulation + Draw Call Submission) ---
        uint64_t cpu_start = Profiler_GetTimestamp();
        
        // 2. Keyboard Input
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        Uint8 simulated_keystate[SDL_NUM_SCANCODES];
        if (benchmark_mode) {
            memset(simulated_keystate, 0, sizeof(simulated_keystate));
            simulated_keystate[SDL_SCANCODE_D] = 1;
            if (frame_count >= 20 && frame_count <= 25) {
                simulated_keystate[SDL_SCANCODE_LSHIFT] = 1; // Trigger Dash
            }
            if (frame_count >= 60 && frame_count <= 65) {
                simulated_keystate[SDL_SCANCODE_Q] = 1;      // Trigger Hydro Pump
            }
            keystate = simulated_keystate;
        }
        Game_HandleInput(&game, keystate, dt);
        
        // 3. Game Simulation
        Game_Update(&game, dt);
        
        // 4. Render Frame (draw command submission)
        Game_Draw(&game);
        
        // 5. Dev Menu Tick
        DevMenu_Update(&game);
        
        game.profiler.t_cpu = Profiler_ElapsedMs(cpu_start);
        
        // --- PRESENT (Blocks on macOS Metal VSync) ---
        uint64_t vsync_start = Profiler_GetTimestamp();
        SDL_RenderPresent(renderer);
        game.profiler.t_vsync = Profiler_ElapsedMs(vsync_start);
        
        // 6. Complete Profiler Frame
        Profiler_EndFrame(&game.profiler);
        
        frame_count++;
        if (benchmark_mode && frame_count >= benchmark_max_frames) {
            printf("\n--- BENCHMARK RESULTS (300 frames @ 1,200 entities) ---\n");
            printf(" Real CPU Time        : %6.3f ms (SIMULATION + DRAW CALLS)\n", game.profiler.t_cpu);
            printf(" Potential Throughput : %6.0f FPS (ENGINE MAX CAPACITY)\n", game.profiler.max_potential_fps);
            printf(" Display VSync Sleep  : %6.2f ms (MONITOR IDLE WAIT)\n", game.profiler.t_vsync);
            printf(" Total Frame Interval : %6.2f ms (%.1f Display FPS)\n", game.profiler.frame_time_ms, game.profiler.fps);
            printf("  - Enemy AI / Steering: %6.3f ms\n", game.profiler.t_enemies);
            printf("  - Spatial Hash Grid : %6.3f ms\n", game.profiler.t_spatial_grid);
            printf("  - Collision Check   : %6.3f ms\n", game.profiler.t_collision);
            printf("  - Render Pipeline   : %6.3f ms\n", game.profiler.t_render);
            printf(" Active Enemies       : %d\n", game.profiler.active_enemies);
            printf(" Active Projectiles   : %d\n", game.profiler.active_projectiles);
            printf(" Occupied Grid Cells  : %d\n", game.profiler.occupied_cells);
            printf(" Max Entities/Cell    : %d\n", game.profiler.max_entities_in_cell);
            printf(" Collision Checks Done: %llu\n", game.profiler.collision_checks_done);
            printf(" Checks Avoided (Grid): %llu\n", game.profiler.collision_checks_avoided);
            printf("=======================================================\n\n");
            break;
        }
    }
    
    Game_Cleanup(&game);
    Pokedex_Cleanup();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    printf("[Main] Game exited cleanly.\n");
    return 0;
}
