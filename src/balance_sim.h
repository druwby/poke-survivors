#ifndef BALANCE_SIM_H
#define BALANCE_SIM_H

#include "types.h"

extern bool g_headless_sim;

typedef struct {
    StarterID starter;
    const char* starter_name;
    bool survived;
    float time_survived_sec;
    int final_level;
    int total_kills;
    float total_damage_dealt;
    float total_damage_taken;
    int dashes_used;
    int special_q_used;
    int pokemon_caught;
    
    // Route telemetry
    int routes_cleared;
    int last_section;
    int last_route;
    
    // Gym battle telemetry
    bool reached_gym;
    bool defeated_gym_boss;
    float gym_boss_fight_duration;
    float player_hp_at_boss_start;
    float player_hp_at_boss_end;

    // Big Data & Machine Learning Balance Metrics
    BotArchetype archetype;
    const char* archetype_name;
    float avg_ttk_stage1;
    float avg_ttk_stage2;
    float avg_ttk_stage3;
    float avg_ttk_boss;
    int kills_stage1;
    int kills_stage2;
    int kills_stage3;
    int instant_kills;          // Enemies killed in < 1.0s (combat pacing red flag)
    float total_overkill_damage;// Wasted damage exceeding enemy max HP
    float overkill_ratio;       // overkill / (damage_dealt + overkill)
    float danger_time_sec;      // Time spent under 40% HP (tension index)
    float danger_time_pct;
    float hp_volatility;        // Delta HP volatility index
    
    // Balance issue diagnosis flags
    bool flag_died_early;            // Died before Route 2
    bool flag_low_level_at_gym;      // Reached gym below level 5
    bool flag_boss_bullet_sponge;    // Boss fight took > 60 seconds
    bool flag_projectile_overwhelm;  // Projectiles dealing excessive damage
} SimRunResult;

typedef struct {
    StarterID starter;
    const char* starter_name;
    BotArchetype archetype;
    const char* archetype_name;
    int trials;
    float max_sim_time_sec;
    
    // Survival rates
    int survived_count;
    float survival_rate; // 0.0 - 100.0%
    
    // Gym telemetry
    int reached_gym_count;
    float reached_gym_rate; // 0.0 - 100.0%
    int defeated_boss_count;
    float defeat_boss_rate; // 0.0 - 100.0%
    
    // Level stats
    float avg_level;
    float std_level;
    int min_level;
    int max_level;
    
    // Kills stats
    float avg_kills;
    float std_kills;
    int min_kills;
    int max_kills;
    
    // Damage taken stats
    float avg_damage_taken;
    float min_damage_taken;
    float max_damage_taken;
    
    // Boss fight duration stats
    float avg_boss_duration;
    float min_boss_duration;
    float max_boss_duration;
    
    // Boss end HP
    float avg_boss_end_hp;
    
    // Ability usage
    float avg_dashes;
    float avg_special_q;
    float avg_caught;

    // Big Data Aggregates
    float avg_ttk_stage1;
    float avg_ttk_stage2;
    float avg_ttk_boss;
    float avg_overkill_ratio;
    float avg_danger_time_pct;
    float avg_hp_volatility;
    int total_instant_kills;
    
    // Diagnostic alert counters
    int early_deaths;
    int low_level_at_gym;
    int bullet_sponge_fights;
} StarterAggregateStats;

// Run an automated balance simulation for a specific starter with a specific random seed
SimRunResult BalanceSim_Run(SDL_Window* window, SDL_Renderer* renderer, StarterID starter, float max_sim_time_sec, unsigned int seed);
SimRunResult BalanceSim_RunEx(SDL_Window* window, SDL_Renderer* renderer, StarterID starter, float max_sim_time_sec, unsigned int seed, BotArchetype archetype);

// Run multiple automated trials for a specific starter and calculate aggregate statistics
StarterAggregateStats BalanceSim_RunStarterTrials(SDL_Window* window, SDL_Renderer* renderer, StarterID starter, int num_trials, float max_sim_time_sec);
StarterAggregateStats BalanceSim_RunStarterTrialsEx(SDL_Window* window, SDL_Renderer* renderer, StarterID starter, int num_trials, float max_sim_time_sec, BotArchetype archetype);

// Run full test suite comparing all 3 starters across N trials and print comprehensive statistical balance report
void BalanceSim_RunTrials(SDL_Window* window, SDL_Renderer* renderer, int num_trials, float max_sim_time_sec);
void BalanceSim_RunTrialsEx(SDL_Window* window, SDL_Renderer* renderer, int num_trials, float max_sim_time_sec, BotArchetype archetype, StarterID filter_starter, bool json_output);

// Run full test suite comparing all 3 starters (defaults to 10 trials)
void BalanceSim_RunAllStarters(SDL_Window* window, SDL_Renderer* renderer);

#endif // BALANCE_SIM_H
