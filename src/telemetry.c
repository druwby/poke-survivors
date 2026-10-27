#include "telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

BalanceConfig g_balance_config;
uint32_t g_enemy_uid_counter = 0;

static FILE* s_telemetry_file = NULL;
static char* s_stream_buffer = NULL;
#define TELEMETRY_BUFFER_SIZE (1024 * 1024) // 1MB I/O buffer for high-throughput headless runs

void BalanceConfig_InitDefaults(void) {
    g_balance_config.enemy_hp_mult = 1.35f;
    g_balance_config.route_hp_scaling = 1.0f;
    g_balance_config.time_hp_alpha = 0.15f;
    g_balance_config.time_hp_gamma = 1.0f;
    g_balance_config.enemy_defense_base = 0.10f;
    g_balance_config.enemy_defense_per_stage = 0.08f;
    g_balance_config.weapon_damage_mult = 1.0f;
    g_balance_config.spawn_interval_mult = 1.0f;
    g_balance_config.max_enemies_mult = 1.0f;
    g_balance_config.boss_hp_mult = 1.0f;
}

static void ParseJSONField(const char* key, float value) {
    if (strcmp(key, "enemy_hp_mult") == 0) g_balance_config.enemy_hp_mult = (value < 0.1f) ? 0.1f : value;
    else if (strcmp(key, "route_hp_scaling") == 0) g_balance_config.route_hp_scaling = (value < 0.1f) ? 0.1f : value;
    else if (strcmp(key, "time_hp_alpha") == 0) g_balance_config.time_hp_alpha = (value < 0.0f) ? 0.0f : value;
    else if (strcmp(key, "time_hp_gamma") == 0) g_balance_config.time_hp_gamma = (value < 0.1f) ? 0.1f : value;
    else if (strcmp(key, "enemy_defense_base") == 0) {
        if (value < 0.0f) value = 0.0f;
        if (value > 0.85f) value = 0.85f;
        g_balance_config.enemy_defense_base = value;
    }
    else if (strcmp(key, "enemy_defense_per_stage") == 0) {
        if (value < 0.0f) value = 0.0f;
        if (value > 0.25f) value = 0.25f;
        g_balance_config.enemy_defense_per_stage = value;
    }
    else if (strcmp(key, "weapon_damage_mult") == 0) g_balance_config.weapon_damage_mult = (value < 0.05f) ? 0.05f : value;
    else if (strcmp(key, "spawn_interval_mult") == 0) g_balance_config.spawn_interval_mult = (value < 0.1f) ? 0.1f : value;
    else if (strcmp(key, "max_enemies_mult") == 0) g_balance_config.max_enemies_mult = (value < 0.1f) ? 0.1f : value;
    else if (strcmp(key, "boss_hp_mult") == 0) g_balance_config.boss_hp_mult = (value < 0.1f) ? 0.1f : value;
}

bool BalanceConfig_LoadFromFile(const char* filepath) {
    if (!filepath || !filepath[0]) return false;
    FILE* f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "[Config] Warning: Could not open config file '%s'\n", filepath);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0 || size > 65536) {
        fclose(f);
        return false;
    }

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return false;
    }

    size_t read_bytes = fread(buffer, 1, size, f);
    buffer[read_bytes] = '\0';
    fclose(f);

    // Simple, robust zero-dependency JSON key-value parser
    const char* ptr = buffer;
    while (*ptr) {
        // Find opening quote for key
        const char* key_start = strchr(ptr, '\"');
        if (!key_start) break;
        key_start++;
        const char* key_end = strchr(key_start, '\"');
        if (!key_end) break;

        char key[64];
        size_t key_len = key_end - key_start;
        if (key_len >= sizeof(key)) key_len = sizeof(key) - 1;
        strncpy(key, key_start, key_len);
        key[key_len] = '\0';

        // Find colon
        const char* colon = strchr(key_end + 1, ':');
        if (!colon) break;

        // Parse numeric value
        const char* val_start = colon + 1;
        while (*val_start && isspace((unsigned char)*val_start)) val_start++;
        
        char* val_end = NULL;
        float val = strtof(val_start, &val_end);
        if (val_end && val_end != val_start) {
            ParseJSONField(key, val);
            ptr = val_end;
        } else {
            ptr = colon + 1;
        }
    }

    free(buffer);
    fprintf(stderr, "[Config] Loaded balance override from '%s'\n", filepath);
    return true;
}

bool Telemetry_Init(const char* filepath) {
    if (!filepath || !filepath[0]) return false;
    if (s_telemetry_file) Telemetry_Close();

    s_telemetry_file = fopen(filepath, "w");
    if (!s_telemetry_file) {
        fprintf(stderr, "[Telemetry] Error: Failed to open telemetry log file '%s'\n", filepath);
        return false;
    }

    s_stream_buffer = (char*)malloc(TELEMETRY_BUFFER_SIZE);
    if (s_stream_buffer) {
        setvbuf(s_telemetry_file, s_stream_buffer, _IOFBF, TELEMETRY_BUFFER_SIZE);
    }
    fprintf(stderr, "[Telemetry] Streaming micro-events to '%s'\n", filepath);
    return true;
}

void Telemetry_Close(void) {
    if (s_telemetry_file) {
        fflush(s_telemetry_file);
        fclose(s_telemetry_file);
        s_telemetry_file = NULL;
    }
    if (s_stream_buffer) {
        free(s_stream_buffer);
        s_stream_buffer = NULL;
    }
}

bool Telemetry_IsEnabled(void) {
    return (s_telemetry_file != NULL);
}

void Telemetry_LogSpawn(float time_sec, uint32_t uid, int pokedex_id, const char* name, int stage, float max_hp, const char* route_name, int section) {
    if (!s_telemetry_file) return;
    fprintf(s_telemetry_file,
            "{\"event\":\"spawn\",\"time\":%.2f,\"uid\":%u,\"pokedex_id\":%d,\"name\":\"%s\",\"stage\":%d,\"max_hp\":%.1f,\"route\":\"%s\",\"section\":%d}\n",
            time_sec, uid, pokedex_id, name ? name : "Unknown", stage, max_hp, route_name ? route_name : "", section);
}

void Telemetry_LogHit(float time_sec, uint32_t uid, int pokedex_id, float damage, float remaining_hp, const char* source) {
    if (!s_telemetry_file) return;
    fprintf(s_telemetry_file,
            "{\"event\":\"hit\",\"time\":%.2f,\"uid\":%u,\"pokedex_id\":%d,\"damage\":%.1f,\"remaining_hp\":%.1f,\"source\":\"%s\"}\n",
            time_sec, uid, pokedex_id, damage, remaining_hp, source ? source : "UNKNOWN");
}

void Telemetry_LogDeath(float time_sec, uint32_t uid, int pokedex_id, const char* name, int stage, float ttk, float lifetime, float overkill, const char* killer_source, const char* route_name) {
    if (!s_telemetry_file) return;
    fprintf(s_telemetry_file,
            "{\"event\":\"death\",\"time\":%.2f,\"uid\":%u,\"pokedex_id\":%d,\"name\":\"%s\",\"stage\":%d,\"ttk\":%.2f,\"lifetime\":%.2f,\"overkill\":%.1f,\"killer\":\"%s\",\"route\":\"%s\"}\n",
            time_sec, uid, pokedex_id, name ? name : "Unknown", stage, ttk, lifetime, overkill, killer_source ? killer_source : "UNKNOWN", route_name ? route_name : "");
}

void Telemetry_LogPlayerStat(float time_sec, float hp, float max_hp, float danger_pct, int level, int kills, const char* archetype) {
    if (!s_telemetry_file) return;
    fprintf(s_telemetry_file,
            "{\"event\":\"player\",\"time\":%.2f,\"hp\":%.1f,\"max_hp\":%.1f,\"danger_pct\":%.1f,\"level\":%d,\"kills\":%d,\"archetype\":\"%s\"}\n",
            time_sec, hp, max_hp, danger_pct, level, kills, archetype ? archetype : "optimal");
}

void Telemetry_LogRunSummary(const char* starter, const char* archetype, bool survived, float time_survived, int level, int kills,
                            float avg_ttk_s1, float avg_ttk_s2, float avg_ttk_boss, float overkill_ratio, float danger_pct, int instant_kills) {
    if (!s_telemetry_file) return;
    fprintf(s_telemetry_file,
            "{\"event\":\"summary\",\"starter\":\"%s\",\"archetype\":\"%s\",\"survived\":%s,\"time_survived\":%.2f,\"level\":%d,\"kills\":%d,\"ttk_s1\":%.2f,\"ttk_s2\":%.2f,\"ttk_boss\":%.2f,\"overkill_ratio\":%.3f,\"danger_pct\":%.1f,\"instant_kills\":%d}\n",
            starter ? starter : "Unknown", archetype ? archetype : "Unknown", survived ? "true" : "false",
            time_survived, level, kills, avg_ttk_s1, avg_ttk_s2, avg_ttk_boss, overkill_ratio, danger_pct, instant_kills);
    fflush(s_telemetry_file);
}
