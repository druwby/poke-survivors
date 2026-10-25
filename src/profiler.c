#include "profiler.h"
#include "font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint64_t perf_frequency = 0;

void Profiler_Init(ProfilerMetrics* prof) {
    memset(prof, 0, sizeof(ProfilerMetrics));
    perf_frequency = SDL_GetPerformanceFrequency();
    prof->min_fps = 999.0;
}

uint64_t Profiler_GetTimestamp(void) {
    return SDL_GetPerformanceCounter();
}

double Profiler_ElapsedMs(uint64_t start_counter) {
    if (perf_frequency == 0) perf_frequency = SDL_GetPerformanceFrequency();
    uint64_t now = SDL_GetPerformanceCounter();
    return ((double)(now - start_counter) * 1000.0) / (double)perf_frequency;
}

void Profiler_StartFrame(ProfilerMetrics* prof) {
    prof->frame_start_counter = SDL_GetPerformanceCounter();
    prof->collision_checks_done = 0;
    prof->collision_checks_avoided = 0;
}

static int compare_floats(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa > fb) - (fa < fb);
}

void Profiler_EndFrame(ProfilerMetrics* prof) {
    prof->frame_time_ms = Profiler_ElapsedMs(prof->frame_start_counter);
    if (prof->frame_time_ms > 0.0001) {
        prof->fps = 1000.0 / prof->frame_time_ms;
    }
    
    // Theoretical max throughput
    if (prof->t_cpu > 0.0001) {
        prof->max_potential_fps = 1000.0 / prof->t_cpu;
    } else {
        prof->max_potential_fps = 9999.0;
    }
    
    // Store CPU execution time in history for sparkline graph!
    prof->frame_time_history[prof->history_index] = (float)prof->t_cpu;
    prof->history_index = (prof->history_index + 1) % 120;
    
    // Compute rolling average and 1% low of CPU time
    float sorted[120];
    memcpy(sorted, prof->frame_time_history, sizeof(sorted));
    qsort(sorted, 120, sizeof(float), compare_floats);
    
    float sum = 0.0f;
    for (int i = 0; i < 120; i++) sum += sorted[i];
    float avg_time = sum / 120.0f;
    prof->avg_fps = (avg_time > 0.001f) ? (1000.0 / avg_time) : 0.0;
    
    // 99th percentile CPU time
    float p99_time = sorted[118];
    prof->min_fps = (p99_time > 0.001f) ? (1000.0 / p99_time) : 0.0;
    
    if (prof->active_enemies > prof->peak_enemies) {
        prof->peak_enemies = prof->active_enemies;
    }
}

void Profiler_DrawOverlay(GameState* game) {
    SDL_Renderer* renderer = game->renderer;
    ProfilerMetrics* prof = &game->profiler;
    SDL_Texture* font = game->font_texture;
    
    // Semi-transparent dark background card in top-right
    int card_w = 290;
    int card_h = 245;
    int card_x = WINDOW_WIDTH - card_w - 12;
    int card_y = 12;
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 15, 20, 30, 220);
    SDL_Rect bg = { card_x, card_y, card_w, card_h };
    SDL_RenderFillRect(renderer, &bg);
    
    SDL_SetRenderDrawColor(renderer, 60, 90, 140, 240);
    SDL_RenderDrawRect(renderer, &bg);
    
    char buf[64];
    int ty = card_y + 8;
    int tx = card_x + 10;
    
    // 1. Header: CPU Work vs Display VSync
    SDL_Color header_col = (prof->t_cpu <= 1.0) ? (SDL_Color){ 80, 255, 120, 255 } :
                           (prof->t_cpu <= 2.5) ? (SDL_Color){ 255, 220, 60, 255 } :
                                                  (SDL_Color){ 255, 70, 70, 255 };
    snprintf(buf, sizeof(buf), "CPU WORK : %4.2f ms (%4.0f MAX FPS)", prof->t_cpu, prof->max_potential_fps);
    Font_DrawText(renderer, font, buf, tx, ty, 1, header_col);
    ty += 12;
    
    snprintf(buf, sizeof(buf), "Display  : %4.1f FPS (VSync: %4.1f ms)", prof->fps, prof->t_vsync);
    SDL_Color white = { 220, 225, 235, 255 };
    Font_DrawText(renderer, font, buf, tx, ty, 1, white);
    ty += 14;
    
    // 2. Sparkline graph (0.0 to 2.0 ms CPU execution range)
    int graph_x = tx;
    int graph_y = ty;
    int graph_w = card_w - 20;
    int graph_h = 32;
    float max_graph_ms = 2.0f; // 2.0 ms scale (500 FPS threshold)
    
    SDL_SetRenderDrawColor(renderer, 8, 12, 18, 255);
    SDL_Rect graph_bg = { graph_x, graph_y, graph_w, graph_h };
    SDL_RenderFillRect(renderer, &graph_bg);
    
    // 1.0 ms target line (1000 FPS line)
    int line_1ms = graph_y + graph_h - (int)((1.0f / max_graph_ms) * graph_h);
    SDL_SetRenderDrawColor(renderer, 45, 120, 70, 200);
    SDL_RenderDrawLine(renderer, graph_x, line_1ms, graph_x + graph_w, line_1ms);
    
    // Draw CPU load bars
    for (int i = 0; i < 120 && i < graph_w; i++) {
        int idx = (prof->history_index + i) % 120;
        float ms = prof->frame_time_history[idx];
        int bar_h = (int)((ms / max_graph_ms) * graph_h);
        if (bar_h > graph_h) bar_h = graph_h;
        if (bar_h < 1) bar_h = 1;
        
        if (ms <= 0.8f) {
            SDL_SetRenderDrawColor(renderer, 60, 235, 120, 255); // Sub-millisecond emerald (< 0.8 ms!)
        } else if (ms <= 1.5f) {
            SDL_SetRenderDrawColor(renderer, 240, 200, 50, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 60, 60, 255);
        }
        
        SDL_RenderDrawLine(renderer, graph_x + i, graph_y + graph_h - 1, 
                                     graph_x + i, graph_y + graph_h - bar_h);
    }
    ty += graph_h + 8;
    
    // 3. Timings breakdown
    SDL_Color cyan = { 100, 210, 255, 255 };
    Font_DrawText(renderer, font, "CPU TIMING BREAKDOWN (sub-ms):", tx, ty, 1, cyan);
    ty += 11;
    
    snprintf(buf, sizeof(buf), " - Update   : %5.3f ms", 
             prof->t_player + prof->t_weapons + prof->t_enemies + prof->t_collision);
    Font_DrawText(renderer, font, buf, tx, ty, 1, white);
    ty += 10;
    
    snprintf(buf, sizeof(buf), "   - Enemies: %5.3f ms", prof->t_enemies);
    Font_DrawText(renderer, font, buf, tx, ty, 1, white);
    ty += 10;
    
    snprintf(buf, sizeof(buf), "   - Grid   : %5.3f ms", prof->t_spatial_grid);
    Font_DrawText(renderer, font, buf, tx, ty, 1, white);
    ty += 10;
    
    snprintf(buf, sizeof(buf), "   - Collsn : %5.3f ms", prof->t_collision);
    Font_DrawText(renderer, font, buf, tx, ty, 1, white);
    ty += 10;
    
    snprintf(buf, sizeof(buf), " - Render   : %5.3f ms", prof->t_render);
    Font_DrawText(renderer, font, buf, tx, ty, 1, white);
    ty += 13;
    
    // 4. Entity counters
    Font_DrawText(renderer, font, "ACTIVE ENTITIES:", tx, ty, 1, cyan);
    ty += 11;
    
    snprintf(buf, sizeof(buf), " Enemies : %4d (Peak: %4d)", prof->active_enemies, prof->peak_enemies);
    Font_DrawText(renderer, font, buf, tx, ty, 1, white);
    ty += 10;
    
    snprintf(buf, sizeof(buf), " Projs   : %4d | Gems: %4d", prof->active_projectiles, prof->active_gems);
    Font_DrawText(renderer, font, buf, tx, ty, 1, white);
    ty += 10;
    
    snprintf(buf, sizeof(buf), " Grid Occ: %4d cells (max: %d)", prof->occupied_cells, prof->max_entities_in_cell);
    Font_DrawText(renderer, font, buf, tx, ty, 1, white);
}
