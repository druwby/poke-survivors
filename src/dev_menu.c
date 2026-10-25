#include "dev_menu.h"
#include "game.h"
#include "weapons.h"
#include "font.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int mouse_x = 0;
static int mouse_y = 0;
static bool mouse_down = false;
static bool mouse_clicked = false;
static int active_drag_id = -1;

void DevMenu_Init(DevMenuState* menu) {
    menu->open = false;
    menu->current_tab = 0; // Player tab
    menu->selected_enemy_type = 0; // Diglett
    menu->pause_when_open = false; // By default let the game run so changes are live!
    menu->show_hitboxes = false;
    menu->show_spatial_grid = false;
    menu->show_pickup_range = false;
    menu->show_damage_numbers = true;
}

void DevMenu_HandleEvent(GameState* game, const SDL_Event* event) {
    if (event->type == SDL_KEYDOWN) {
        if (event->key.keysym.sym == SDLK_F1 || 
            event->key.keysym.sym == SDLK_BACKQUOTE ||
            event->key.keysym.sym == SDLK_TAB) {
            game->dev_menu.open = !game->dev_menu.open;
            if (game->dev_menu.pause_when_open) {
                game->paused = game->dev_menu.open;
            }
        }
    }
    
    if (!game->dev_menu.open) return;
    
    if (event->type == SDL_MOUSEMOTION) {
        mouse_x = event->motion.x;
        mouse_y = event->motion.y;
    } else if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            mouse_down = true;
            mouse_clicked = true;
            mouse_x = event->button.x;
            mouse_y = event->button.y;
        }
    } else if (event->type == SDL_MOUSEBUTTONUP) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            mouse_down = false;
            active_drag_id = -1;
        }
    }
}

void DevMenu_Update(GameState* game) {
    // Reset click trigger at the end of each frame
    mouse_clicked = false;
}

// UI Widget Helpers
static bool PointInRect(int px, int py, int rx, int ry, int rw, int rh) {
    return (px >= rx && px < rx + rw && py >= ry && py < ry + rh);
}

static bool UI_Tab(SDL_Renderer* renderer, SDL_Texture* font, const char* label, int x, int y, int w, int h, bool is_active) {
    bool hovered = PointInRect(mouse_x, mouse_y, x, y, w, h);
    bool clicked = hovered && mouse_clicked;
    
    if (is_active) {
        SDL_SetRenderDrawColor(renderer, 50, 110, 200, 255);
    } else if (hovered) {
        SDL_SetRenderDrawColor(renderer, 45, 60, 85, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 25, 32, 45, 255);
    }
    
    SDL_Rect r = { x, y, w, h };
    SDL_RenderFillRect(renderer, &r);
    
    SDL_SetRenderDrawColor(renderer, 70, 95, 135, 255);
    SDL_RenderDrawRect(renderer, &r);
    
    SDL_Color col = is_active ? (SDL_Color){ 255, 255, 255, 255 } : (SDL_Color){ 170, 185, 205, 255 };
    Font_DrawText(renderer, font, label, x + (w - (int)strlen(label) * 8) / 2, y + (h - 8) / 2, 1, col);
    
    return clicked;
}

static bool UI_Button(SDL_Renderer* renderer, SDL_Texture* font, const char* label, int x, int y, int w, int h, SDL_Color base_color) {
    bool hovered = PointInRect(mouse_x, mouse_y, x, y, w, h);
    bool clicked = hovered && mouse_clicked;
    
    if (clicked) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    } else if (hovered) {
        SDL_SetRenderDrawColor(renderer, (Uint8)fminf(base_color.r + 40, 255), (Uint8)fminf(base_color.g + 40, 255), (Uint8)fminf(base_color.b + 40, 255), base_color.a);
    } else {
        SDL_SetRenderDrawColor(renderer, base_color.r, base_color.g, base_color.b, base_color.a);
    }
    
    SDL_Rect r = { x, y, w, h };
    SDL_RenderFillRect(renderer, &r);
    
    SDL_SetRenderDrawColor(renderer, 180, 200, 220, 200);
    SDL_RenderDrawRect(renderer, &r);
    
    SDL_Color text_col = { 255, 255, 255, 255 };
    Font_DrawText(renderer, font, label, x + (w - (int)strlen(label) * 8) / 2, y + (h - 8) / 2, 1, text_col);
    
    return clicked;
}

static bool UI_Checkbox(SDL_Renderer* renderer, SDL_Texture* font, const char* label, int x, int y, bool* val) {
    int box_size = 14;
    bool hovered = PointInRect(mouse_x, mouse_y, x, y, 200, box_size);
    bool clicked = hovered && mouse_clicked;
    
    if (clicked) {
        *val = !(*val);
    }
    
    SDL_SetRenderDrawColor(renderer, 30, 40, 55, 255);
    SDL_Rect r = { x, y, box_size, box_size };
    SDL_RenderFillRect(renderer, &r);
    
    SDL_SetRenderDrawColor(renderer, hovered ? 120 : 70, hovered ? 180 : 100, hovered ? 255 : 150, 255);
    SDL_RenderDrawRect(renderer, &r);
    
    if (*val) {
        SDL_SetRenderDrawColor(renderer, 50, 220, 120, 255);
        SDL_Rect inner = { x + 3, y + 3, box_size - 6, box_size - 6 };
        SDL_RenderFillRect(renderer, &inner);
    }
    
    SDL_Color col = hovered ? (SDL_Color){ 255, 255, 255, 255 } : (SDL_Color){ 200, 215, 230, 255 };
    Font_DrawText(renderer, font, label, x + box_size + 8, y + 3, 1, col);
    
    return clicked;
}

static void UI_SliderFloatEx(SDL_Renderer* renderer, SDL_Texture* font, int id, const char* label, int x, int y, int label_w, int bar_w, float* val, float min, float max, const char* fmt) {
    int bar_h = 10;
    int bar_x = x + label_w;
    int bar_y = y + 2;
    
    bool hovered = PointInRect(mouse_x, mouse_y, bar_x - 4, bar_y - 4, bar_w + 8, bar_h + 8);
    if (hovered && mouse_clicked) {
        active_drag_id = id;
    }
    
    if (active_drag_id == id && mouse_down) {
        float frac = (float)(mouse_x - bar_x) / (float)bar_w;
        if (frac < 0.0f) frac = 0.0f;
        if (frac > 1.0f) frac = 1.0f;
        *val = min + frac * (max - min);
    }
    
    // Draw label
    SDL_Color lbl_col = { 190, 205, 220, 255 };
    Font_DrawText(renderer, font, label, x, y + 2, 1, lbl_col);
    
    // Draw bar background
    SDL_SetRenderDrawColor(renderer, 25, 35, 48, 255);
    SDL_Rect bar = { bar_x, bar_y, bar_w, bar_h };
    SDL_RenderFillRect(renderer, &bar);
    
    // Draw filled portion
    float frac = (*val - min) / (max - min);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    int fill_w = (int)(frac * bar_w);
    
    SDL_SetRenderDrawColor(renderer, 50, 140, 240, 255);
    SDL_Rect fill = { bar_x, bar_y, fill_w, bar_h };
    SDL_RenderFillRect(renderer, &fill);
    
    // Draw handle
    SDL_SetRenderDrawColor(renderer, 220, 240, 255, 255);
    SDL_Rect handle = { bar_x + fill_w - 3, bar_y - 2, 6, bar_h + 4 };
    SDL_RenderFillRect(renderer, &handle);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, 65, 85, 115, 255);
    SDL_RenderDrawRect(renderer, &bar);
    
    // Draw value text
    char val_str[32];
    snprintf(val_str, sizeof(val_str), fmt, *val);
    SDL_Color val_col = { 100, 220, 255, 255 };
    Font_DrawText(renderer, font, val_str, bar_x + bar_w + 8, y + 2, 1, val_col);
}

static void UI_SliderFloat(SDL_Renderer* renderer, SDL_Texture* font, int id, const char* label, int x, int y, int bar_w, float* val, float min, float max, const char* fmt) {
    UI_SliderFloatEx(renderer, font, id, label, x, y, 160, bar_w, val, min, max, fmt);
}

static void UI_SliderInt(SDL_Renderer* renderer, SDL_Texture* font, int id, const char* label, int x, int y, int bar_w, int* val, int min, int max) {
    float fval = (float)(*val);
    UI_SliderFloat(renderer, font, id, label, x, y, bar_w, &fval, (float)min, (float)max, "%d");
    *val = (int)roundf(fval);
}

void DevMenu_Draw(GameState* game) {
    if (!game->dev_menu.open) return;
    
    SDL_Renderer* renderer = game->renderer;
    SDL_Texture* font = game->font_texture;
    DevMenuState* menu = &game->dev_menu;
    
    // Master Window Panel
    int win_x = 35;
    int win_y = 30;
    int win_w = 700;
    int win_h = 570;
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 16, 22, 34, 245);
    SDL_Rect win_rect = { win_x, win_y, win_w, win_h };
    SDL_RenderFillRect(renderer, &win_rect);
    
    SDL_SetRenderDrawColor(renderer, 70, 110, 175, 255);
    SDL_RenderDrawRect(renderer, &win_rect);
    
    // Title Bar
    SDL_SetRenderDrawColor(renderer, 28, 40, 62, 255);
    SDL_Rect title_rect = { win_x + 1, win_y + 1, win_w - 2, 30 };
    SDL_RenderFillRect(renderer, &title_rect);
    
    SDL_Color title_col = { 255, 220, 90, 255 };
    Font_DrawText(renderer, font, "[ROGUE SURVIVORS ENGINE - DEVELOPER CONSOLE]", win_x + 16, win_y + 11, 1, title_col);
    
    SDL_Color close_col = { 180, 180, 180, 255 };
    Font_DrawText(renderer, font, "Press F1/TAB/` to close", win_x + win_w - 200, win_y + 11, 1, close_col);
    
    // Tab Navigation Bar
    const char* tabs[5] = { "PLAYER", "WEAPONS", "ENEMIES", "SPAWNER", "SYSTEM" };
    int tab_w = (win_w - 20) / 5;
    int tab_y = win_y + 36;
    for (int i = 0; i < 5; i++) {
        if (UI_Tab(renderer, font, tabs[i], win_x + 10 + i * tab_w, tab_y, tab_w - 4, 24, menu->current_tab == i)) {
            menu->current_tab = i;
        }
    }
    
    int content_x = win_x + 20;
    int content_y = tab_y + 36;
    int slider_w = 220;
    int line_h = 24;
    int cur_id = 100;
    
    // -------------------------------------------------------------
    // TAB 0: PLAYER
    // -------------------------------------------------------------
    if (menu->current_tab == 0) {
        SDL_Color sec_col = { 100, 220, 255, 255 };
        SDL_Color sub_col = { 80, 230, 255, 255 };
        
        // --- LEFT COLUMN: BASE ATTRIBUTES & CHEATS ---
        int col1_x = content_x;
        int col1_y = content_y;
        int c1_lbl_w = 135;
        int c1_bar_w = 80;
        
        Font_DrawText(renderer, font, "PLAYER ATTRIBUTES", col1_x, col1_y, 1, sec_col);
        col1_y += 20;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Move Speed", col1_x, col1_y, c1_lbl_w, c1_bar_w, &game->player.speed, 50.0f, 600.0f, "%.0f");
        col1_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Max HP", col1_x, col1_y, c1_lbl_w, c1_bar_w, &game->player.max_hp, 10.0f, 1000.0f, "%.0f");
        col1_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "HP Regen /sec", col1_x, col1_y, c1_lbl_w, c1_bar_w, &game->player.hp_regen, 0.0f, 50.0f, "%.1f");
        col1_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Pickup Radius", col1_x, col1_y, c1_lbl_w, c1_bar_w, &game->player.pickup_radius, 20.0f, 600.0f, "%.0f");
        col1_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Invuln Timer (s)", col1_x, col1_y, c1_lbl_w, c1_bar_w, &game->player.invuln_duration, 0.0f, 2.5f, "%.2f");
        col1_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Damage Mult", col1_x, col1_y, c1_lbl_w, c1_bar_w, &game->player.damage_mult, 0.1f, 10.0f, "%.2fx");
        col1_y += line_h + 8;
        
        UI_Checkbox(renderer, font, "God Mode (Invincible)", col1_x, col1_y, &game->player.god_mode);
        col1_y += line_h + 12;
        
        // Action Buttons
        if (UI_Button(renderer, font, "+1 LEVEL UP", col1_x, col1_y, 130, 26, (SDL_Color){ 40, 140, 70, 255 })) {
            game->player.level++;
            game->player.xp = 0;
            game->player.xp_to_next_level = (int)(game->player.xp_to_next_level * 1.35f);
            game->player.damage_mult += 0.15f;
            game->player.max_hp += 15.0f;
            game->player.hp = game->player.max_hp;
        }
        
        if (UI_Button(renderer, font, "HEAL TO FULL", col1_x + 142, col1_y, 130, 26, (SDL_Color){ 30, 100, 160, 255 })) {
            game->player.hp = game->player.max_hp;
        }
        
        // --- RIGHT COLUMN: ACTIVE ABILITIES (DASH & HYDRO PUMP) ---
        int col2_x = win_x + 355;
        int col2_y = content_y;
        int c2_lbl_w = 140;
        int c2_bar_w = 90;
        
        Font_DrawText(renderer, font, "ACTIVE ABILITIES (SQUIRTLE)", col2_x, col2_y, 1, sec_col);
        col2_y += 18;
        
        // Dash section
        Font_DrawText(renderer, font, "-- DASH ABILITY [SHIFT] --", col2_x, col2_y, 1, sub_col);
        col2_y += 16;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Dash Cooldown (s)", col2_x, col2_y, c2_lbl_w, c2_bar_w, &game->player.dash_cooldown, 0.10f, 3.00f, "%.2fs");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Dash Speed", col2_x, col2_y, c2_lbl_w, c2_bar_w, &game->player.dash_speed, 200.0f, 1500.0f, "%.0f");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Dash Duration (s)", col2_x, col2_y, c2_lbl_w, c2_bar_w, &game->player.dash_duration, 0.08f, 0.60f, "%.2fs");
        col2_y += line_h + 8;
        
        // Hydro Pump section
        Font_DrawText(renderer, font, "-- HYDRO PUMP [Q] --", col2_x, col2_y, 1, sub_col);
        col2_y += 16;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Hydro Cooldown (s)", col2_x, col2_y, c2_lbl_w, c2_bar_w, &game->player.hydro_cooldown, 0.5f, 10.0f, "%.1fs");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Hydro Duration (s)", col2_x, col2_y, c2_lbl_w, c2_bar_w, &game->player.hydro_duration, 0.5f, 4.0f, "%.2fs");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Hydro Damage", col2_x, col2_y, c2_lbl_w, c2_bar_w, &game->player.hydro_damage, 10.0f, 500.0f, "%.0f");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Hydro Range (px)", col2_x, col2_y, c2_lbl_w, c2_bar_w, &game->player.hydro_range, 60.0f, 300.0f, "%.0f");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Hydro Knockback", col2_x, col2_y, c2_lbl_w, c2_bar_w, &game->player.hydro_knockback, 100.0f, 2500.0f, "%.0f");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Spin Speed (rad/s)", col2_x, col2_y, c2_lbl_w, c2_bar_w, &game->player.hydro_spin_speed, 4.0f, 36.0f, "%.1f");
        col2_y += line_h + 10;
        
        SDL_Color note_col = { 150, 170, 195, 255 };
        Font_DrawText(renderer, font, "* Dash uses Charge sheet; Hydro uses Withdraw sheet.", col2_x, col2_y, 1, note_col);
    }
    // -------------------------------------------------------------
    // TAB 1: WEAPONS
    // -------------------------------------------------------------
    else if (menu->current_tab == 1) {
        SDL_Color sec_col = { 100, 220, 255, 255 };
        
        // 1. Water Wand
        Font_DrawText(renderer, font, "1. WATER WAND (Auto Projectiles)", content_x, content_y, 1, sec_col);
        content_y += 16;
        UI_Checkbox(renderer, font, "Enabled", content_x, content_y, &game->weapons.water_wand.enabled);
        content_y += line_h;
        UI_SliderFloat(renderer, font, cur_id++, "Cooldown (s)", content_x, content_y, slider_w, &game->weapons.water_wand.cooldown, 0.05f, 2.0f, "%.2f s");
        content_y += line_h;
        UI_SliderFloat(renderer, font, cur_id++, "Damage", content_x, content_y, slider_w, &game->weapons.water_wand.damage, 1.0f, 200.0f, "%.0f");
        content_y += line_h;
        UI_SliderFloat(renderer, font, cur_id++, "Proj Speed", content_x, content_y, slider_w, &game->weapons.water_wand.proj_speed, 100.0f, 1200.0f, "%.0f");
        content_y += line_h;
        UI_SliderInt(renderer, font, cur_id++, "Pierce Count", content_x, content_y, slider_w, &game->weapons.water_wand.pierce, 1, 10);
        content_y += line_h + 10;
        
        // 2. Aqua Orbiters
        Font_DrawText(renderer, font, "2. AQUA ORBITERS (King Bible Spheres)", content_x, content_y, 1, sec_col);
        content_y += 16;
        UI_Checkbox(renderer, font, "Enabled", content_x, content_y, &game->weapons.orbiters.enabled);
        content_y += line_h;
        UI_SliderInt(renderer, font, cur_id++, "Orbiter Count", content_x, content_y, slider_w, &game->weapons.orbiters.orbiter_count, 1, 12);
        content_y += line_h;
        UI_SliderFloat(renderer, font, cur_id++, "Orbit Radius", content_x, content_y, slider_w, &game->weapons.orbiters.orbiter_radius, 30.0f, 280.0f, "%.0f px");
        content_y += line_h;
        UI_SliderFloat(renderer, font, cur_id++, "Rotation Speed", content_x, content_y, slider_w, &game->weapons.orbiters.orbiter_speed, 0.5f, 10.0f, "%.1f r/s");
        content_y += line_h;
        UI_SliderFloat(renderer, font, cur_id++, "Orbiter Damage", content_x, content_y, slider_w, &game->weapons.orbiters.damage, 1.0f, 150.0f, "%.0f");
        content_y += line_h + 10;
        
        // 3. Aqua Cleave
        Font_DrawText(renderer, font, "3. AQUA CLEAVE (Whip Arc)", content_x, content_y, 1, sec_col);
        content_y += 16;
        UI_Checkbox(renderer, font, "Enabled", content_x, content_y, &game->weapons.cleave.enabled);
        content_y += line_h;
        UI_SliderFloat(renderer, font, cur_id++, "Cooldown (s)", content_x, content_y, slider_w, &game->weapons.cleave.cooldown, 0.2f, 3.0f, "%.2f s");
        content_y += line_h;
        UI_SliderFloat(renderer, font, cur_id++, "Cleave Damage", content_x, content_y, slider_w, &game->weapons.cleave.damage, 10.0f, 300.0f, "%.0f");
        content_y += line_h;
        UI_SliderFloat(renderer, font, cur_id++, "Slash Range", content_x, content_y, slider_w, &game->weapons.cleave.slash_range, 60.0f, 300.0f, "%.0f px");
    }
    // -------------------------------------------------------------
    // TAB 2: ENEMIES (PER-TYPE TUNING!)
    // -------------------------------------------------------------
    else if (menu->current_tab == 2) {
        SDL_Color sec_col = { 100, 220, 255, 255 };
        Font_DrawText(renderer, font, "ENEMY SUB-MENU: SELECT ENEMY ARCHETYPE", content_x, content_y, 1, sec_col);
        content_y += 18;
        
        // Archetype Selector Buttons
        const char* type_labels[3] = { "0: DIGLETT (Assassin)", "1: SNORLAX (Brute)", "2: BOSS (Elite)" };
        int sub_btn_w = (win_w - 40) / 3;
        for (int i = 0; i < 3; i++) {
            SDL_Color b_col = (menu->selected_enemy_type == i) ? (SDL_Color){ 60, 130, 210, 255 } : (SDL_Color){ 35, 45, 60, 255 };
            if (UI_Button(renderer, font, type_labels[i], content_x + i * sub_btn_w, content_y, sub_btn_w - 6, 22, b_col)) {
                menu->selected_enemy_type = i;
            }
        }
        content_y += 32;
        
        EnemyTypeConfig* cfg = &game->enemy_types[menu->selected_enemy_type];
        
        char title_buf[64];
        snprintf(title_buf, sizeof(title_buf), "CONFIGURING: %s", cfg->name);
        SDL_Color highlight = { 255, 210, 80, 255 };
        Font_DrawText(renderer, font, title_buf, content_x, content_y, 1, highlight);
        content_y += 20;
        
        // --- TWO COLUMN LAYOUT: BASE STATS vs UNIQUE ATTACK PROFILE ---
        int col1_x = content_x;
        int col1_y = content_y;
        int c1_lbl_w = 135;
        int c1_bar_w = 80;
        
        Font_DrawText(renderer, font, "BASE ATTRIBUTES", col1_x, col1_y, 1, sec_col);
        col1_y += 18;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Max Health", col1_x, col1_y, c1_lbl_w, c1_bar_w, &cfg->max_hp, 5.0f, 3000.0f, "%.0f");
        col1_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Move Speed", col1_x, col1_y, c1_lbl_w, c1_bar_w, &cfg->speed, 10.0f, 400.0f, "%.0f");
        col1_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Touch Damage", col1_x, col1_y, c1_lbl_w, c1_bar_w, &cfg->touch_damage, 0.0f, 150.0f, "%.0f");
        col1_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Hitbox Radius", col1_x, col1_y, c1_lbl_w, c1_bar_w, &cfg->radius, 6.0f, 70.0f, "%.0f");
        col1_y += line_h;
        
        UI_SliderInt(renderer, font, cur_id++, "XP Drop Value", col1_x, col1_y, c1_bar_w, &cfg->xp_value, 1, 100);
        col1_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Knockback Resist", col1_x, col1_y, c1_lbl_w, c1_bar_w, &cfg->knockback_resist, 0.0f, 1.0f, "%.2f");
        col1_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Spawn Weight", col1_x, col1_y, c1_lbl_w, c1_bar_w, &cfg->spawn_weight, 0.0f, 50.0f, "%.1f");
        col1_y += line_h + 10;
        
        // --- RIGHT COLUMN: UNIQUE ATTACK PROFILE ---
        int col2_x = win_x + 355;
        int col2_y = content_y;
        int c2_lbl_w = 140;
        int c2_bar_w = 90;
        SDL_Color sub_col = { 80, 230, 255, 255 };
        
        Font_DrawText(renderer, font, "UNIQUE ATTACK PROFILE", col2_x, col2_y, 1, sec_col);
        col2_y += 18;
        
        const char* atk_name = (cfg->attack_type == ENEMY_ATTACK_BURROW_ASSASSIN) ? "[BURROW ASSASSIN SWIPE]" :
                               (cfg->attack_type == ENEMY_ATTACK_SLAM_AOE)        ? "[GROUND STOMP AOE]" :
                               (cfg->attack_type == ENEMY_ATTACK_BOSS_HYBRID)     ? "[BOSS HYBRID STOMP]" : "[TOUCH ONLY]";
        Font_DrawText(renderer, font, atk_name, col2_x, col2_y, 1, sub_col);
        col2_y += 18;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Attack Range (px)", col2_x, col2_y, c2_lbl_w, c2_bar_w, &cfg->attack_range, 30.0f, 250.0f, "%.0f");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Attack Cooldown", col2_x, col2_y, c2_lbl_w, c2_bar_w, &cfg->attack_cooldown, 0.5f, 8.0f, "%.1fs");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Attack Damage", col2_x, col2_y, c2_lbl_w, c2_bar_w, &cfg->attack_damage, 5.0f, 150.0f, "%.0f");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Attack Radius (px)", col2_x, col2_y, c2_lbl_w, c2_bar_w, &cfg->attack_radius, 20.0f, 220.0f, "%.0f");
        col2_y += line_h;
        
        UI_SliderFloatEx(renderer, font, cur_id++, "Windup Time (s)", col2_x, col2_y, c2_lbl_w, c2_bar_w, &cfg->attack_windup, 0.10f, 1.50f, "%.2fs");
        col2_y += line_h + 12;
        
        SDL_Color note_col = { 150, 165, 185, 255 };
        if (cfg->attack_type == ENEMY_ATTACK_BURROW_ASSASSIN) {
            Font_DrawText(renderer, font, "* Diglett is IMMUNE while moving underground!", col2_x, col2_y, 1, (SDL_Color){ 255, 210, 80, 255 });
            Font_DrawText(renderer, font, "* Vulnerable only during windup & attack.", col2_x, col2_y + 14, 1, note_col);
        } else {
            Font_DrawText(renderer, font, "* Snorlax halts and stomps ground creating a shockwave.", col2_x, col2_y, 1, note_col);
        }
    }
    // -------------------------------------------------------------
    // TAB 3: SPAWNER & DIFFICULTY
    // -------------------------------------------------------------
    else if (menu->current_tab == 3) {
        SDL_Color sec_col = { 100, 220, 255, 255 };
        Font_DrawText(renderer, font, "SPAWNER & SWARM DIFFICULTY", content_x, content_y, 1, sec_col);
        content_y += 20;
        
        UI_SliderInt(renderer, font, cur_id++, "Max Enemy Cap", content_x, content_y, slider_w, &game->spawner.max_enemies, 50, 4000);
        content_y += line_h;
        
        UI_SliderFloat(renderer, font, cur_id++, "Spawn Interval (s)", content_x, content_y, slider_w, &game->spawner.spawn_interval, 0.02f, 2.0f, "%.2f s");
        content_y += line_h;
        
        UI_SliderInt(renderer, font, cur_id++, "Batch Spawn Size", content_x, content_y, slider_w, &game->spawner.batch_size, 1, 60);
        content_y += line_h;
        
        UI_SliderFloat(renderer, font, cur_id++, "Difficulty Scaling", content_x, content_y, slider_w, &game->spawner.difficulty_mult, 0.0f, 5.0f, "%.2fx");
        content_y += line_h + 8;
        
        UI_Checkbox(renderer, font, "Enable Swarm Soft Separation", content_x, content_y, &game->spawner.separation_enabled);
        content_y += line_h;
        
        UI_SliderFloat(renderer, font, cur_id++, "Separation Strength", content_x, content_y, slider_w, &game->spawner.separation_strength, 10.0f, 400.0f, "%.0f");
        content_y += line_h + 14;
        
        // Immediate Spawner Action Triggers
        Font_DrawText(renderer, font, "INSTANT SPAWN / CLEANUP ACTIONS:", content_x, content_y, 1, sec_col);
        content_y += 18;
        
        if (UI_Button(renderer, font, "SPAWN 100 DIGLETTS", content_x, content_y, 180, 26, (SDL_Color){ 160, 90, 40, 255 })) {
            for (int k = 0; k < 100; k++) {
                // Find inactive slot
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (!game->enemies[i].active) {
                        Game_ActivateEnemy(game, i);
                        game->enemies[i].type = ENEMY_DIGLETT;
                        float ang = (float)(rand() % 360) * ((float)M_PI / 180.0f);
                        float dist = 450.0f + (float)(rand() % 200);
                        game->enemies[i].pos.x = game->player.pos.x + cosf(ang) * dist;
                        game->enemies[i].pos.y = game->player.pos.y + sinf(ang) * dist;
                        game->enemies[i].hp = game->enemy_types[ENEMY_DIGLETT].max_hp;
                        game->enemies[i].max_hp = game->enemies[i].hp;
                        game->enemies[i].radius = game->enemy_types[ENEMY_DIGLETT].radius;
                        game->enemies[i].touch_damage = game->enemy_types[ENEMY_DIGLETT].touch_damage;
                        game->enemies[i].knockback_x = 0.0f;
                        game->enemies[i].knockback_y = 0.0f;
                        game->enemies[i].flash_timer = 0.0f;
                        game->enemies[i].is_underground = true;
                        game->enemies[i].ai_state = ENEMY_AI_CHASE;
                        game->enemies[i].state_timer = 0.0f;
                        game->enemies[i].attack_cooldown_timer = (float)(rand() % 100) * 0.01f * 1.5f;
                        game->enemies[i].attack_has_hit = false;
                        game->enemies[i].attack_anim = (AnimState){0};
                        break;
                    }
                }
            }
        }
        
        if (UI_Button(renderer, font, "SPAWN 25 SNORLAXES", content_x + 195, content_y, 180, 26, (SDL_Color){ 50, 90, 140, 255 })) {
            for (int k = 0; k < 25; k++) {
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (!game->enemies[i].active) {
                        Game_ActivateEnemy(game, i);
                        game->enemies[i].type = ENEMY_SNORLAX;
                        float ang = (float)(rand() % 360) * ((float)M_PI / 180.0f);
                        float dist = 450.0f + (float)(rand() % 200);
                        game->enemies[i].pos.x = game->player.pos.x + cosf(ang) * dist;
                        game->enemies[i].pos.y = game->player.pos.y + sinf(ang) * dist;
                        game->enemies[i].hp = game->enemy_types[ENEMY_SNORLAX].max_hp;
                        game->enemies[i].max_hp = game->enemies[i].hp;
                        game->enemies[i].radius = game->enemy_types[ENEMY_SNORLAX].radius;
                        game->enemies[i].touch_damage = game->enemy_types[ENEMY_SNORLAX].touch_damage;
                        game->enemies[i].knockback_x = 0.0f;
                        game->enemies[i].knockback_y = 0.0f;
                        game->enemies[i].flash_timer = 0.0f;
                        game->enemies[i].is_underground = true;
                        game->enemies[i].ai_state = ENEMY_AI_CHASE;
                        game->enemies[i].state_timer = 0.0f;
                        game->enemies[i].attack_cooldown_timer = (float)(rand() % 100) * 0.01f * 1.5f;
                        game->enemies[i].attack_has_hit = false;
                        game->enemies[i].attack_anim = (AnimState){0};
                        break;
                    }
                }
            }
        }
        
        if (UI_Button(renderer, font, "SPAWN 1 ELITE BOSS", content_x + 390, content_y, 180, 26, (SDL_Color){ 180, 30, 30, 255 })) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!game->enemies[i].active) {
                    Game_ActivateEnemy(game, i);
                    game->enemies[i].type = ENEMY_BOSS;
                    float ang = (float)(rand() % 360) * ((float)M_PI / 180.0f);
                    float dist = 450.0f;
                    game->enemies[i].pos.x = game->player.pos.x + cosf(ang) * dist;
                    game->enemies[i].pos.y = game->player.pos.y + sinf(ang) * dist;
                    game->enemies[i].hp = game->enemy_types[ENEMY_BOSS].max_hp;
                    game->enemies[i].max_hp = game->enemies[i].hp;
                    game->enemies[i].radius = game->enemy_types[ENEMY_BOSS].radius;
                    game->enemies[i].touch_damage = game->enemy_types[ENEMY_BOSS].touch_damage;
                    game->enemies[i].knockback_x = 0.0f;
                    game->enemies[i].knockback_y = 0.0f;
                    game->enemies[i].flash_timer = 0.0f;
                    game->enemies[i].is_underground = false;
                    game->enemies[i].ai_state = ENEMY_AI_CHASE;
                    game->enemies[i].state_timer = 0.0f;
                    game->enemies[i].attack_cooldown_timer = 2.0f;
                    game->enemies[i].attack_has_hit = false;
                    game->enemies[i].attack_anim = (AnimState){0};
                    break;
                }
            }
        }
        content_y += 34;
        
        if (UI_Button(renderer, font, "KILL ALL ENEMIES", content_x, content_y, 180, 26, (SDL_Color){ 180, 40, 40, 255 })) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                game->enemies[i].active = false;
            }
            game->active_enemy_count = 0;
        }
        
        if (UI_Button(renderer, font, "CLEAR ALL GEMS", content_x + 195, content_y, 180, 26, (SDL_Color){ 120, 50, 130, 255 })) {
            for (int i = 0; i < MAX_GEMS; i++) {
                game->gems[i].active = false;
            }
        }
    }
    // -------------------------------------------------------------
    // TAB 4: SYSTEM & VISUALS
    // -------------------------------------------------------------
    else if (menu->current_tab == 4) {
        SDL_Color sec_col = { 100, 220, 255, 255 };
        Font_DrawText(renderer, font, "SYSTEM & VISUAL DEBUGGING", content_x, content_y, 1, sec_col);
        content_y += 20;
        
        if (UI_Checkbox(renderer, font, "VSync (Display Refresh Lock)", content_x, content_y, &game->vsync_enabled)) {
            SDL_RenderSetVSync(renderer, game->vsync_enabled ? 1 : 0);
        }
        content_y += line_h;
        
        UI_Checkbox(renderer, font, "Show Collision Hitboxes", content_x, content_y, &menu->show_hitboxes);
        content_y += line_h;
        
        UI_Checkbox(renderer, font, "Show Spatial Hash Grid Cells", content_x, content_y, &menu->show_spatial_grid);
        content_y += line_h;
        
        UI_Checkbox(renderer, font, "Show Player Pickup Range Circle", content_x, content_y, &menu->show_pickup_range);
        content_y += line_h;
        
        UI_Checkbox(renderer, font, "Show Floating Damage Numbers", content_x, content_y, &menu->show_damage_numbers);
        content_y += line_h;
        
        UI_Checkbox(renderer, font, "Pause Simulation When Menu Open", content_x, content_y, &menu->pause_when_open);
        content_y += line_h + 16;
        
        if (UI_Button(renderer, font, "RESET ALL DEFAULTS", content_x, content_y, 180, 26, (SDL_Color){ 90, 90, 100, 255 })) {
            // Restore defaults
            game->player.speed = 180.0f;
            game->player.max_hp = 100.0f;
            game->player.hp = 100.0f;
            game->player.hp_regen = 1.0f;
            game->player.pickup_radius = 110.0f;
            game->player.damage_mult = 1.0f;
            game->player.god_mode = false;
            
            game->player.dash_cooldown = 0.8f;
            game->player.dash_speed = 600.0f;
            game->player.dash_duration = 0.20f;
            game->player.hydro_cooldown = 4.0f;
            game->player.hydro_duration = 1.6f;
            game->player.hydro_damage = 95.0f;
            game->player.hydro_range = 145.0f;
            game->player.hydro_knockback = 750.0f;
            game->player.hydro_spin_speed = 16.0f;
            
            Weapons_Init(&game->weapons);
            
            game->spawner.max_enemies = 1500;
            game->spawner.spawn_interval = 0.35f;
            game->spawner.batch_size = 4;
            game->spawner.difficulty_mult = 1.0f;
            game->spawner.separation_enabled = true;
            game->spawner.separation_strength = 75.0f;
        }
    }
}
