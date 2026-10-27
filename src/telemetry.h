#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdbool.h>
#include <stdint.h>
#include "types.h"

// Global balance configuration
extern BalanceConfig g_balance_config;
extern uint32_t g_enemy_uid_counter;

// Initialize balance configuration with default tuned values
void BalanceConfig_InitDefaults(void);

// Load balance configuration overrides from a JSON file
bool BalanceConfig_LoadFromFile(const char* filepath);

// Initialize telemetry streaming to a NDJSON/JSONL file
bool Telemetry_Init(const char* filepath);

// Close telemetry file and flush buffers
void Telemetry_Close(void);

// Check if telemetry streaming is currently active
bool Telemetry_IsEnabled(void);

// High-throughput per-event telemetry loggers
void Telemetry_LogSpawn(float time_sec, uint32_t uid, int pokedex_id, const char* name, int stage, float max_hp, const char* route_name, int section);
void Telemetry_LogHit(float time_sec, uint32_t uid, int pokedex_id, float damage, float remaining_hp, const char* source);
void Telemetry_LogDeath(float time_sec, uint32_t uid, int pokedex_id, const char* name, int stage, float ttk, float lifetime, float overkill, const char* killer_source, const char* route_name);
void Telemetry_LogPlayerStat(float time_sec, float hp, float max_hp, float danger_pct, int level, int kills, const char* archetype);
void Telemetry_LogRunSummary(const char* starter, const char* archetype, bool survived, float time_survived, int level, int kills,
                            float avg_ttk_s1, float avg_ttk_s2, float avg_ttk_boss, float overkill_ratio, float danger_pct, int instant_kills);

#endif // TELEMETRY_H
