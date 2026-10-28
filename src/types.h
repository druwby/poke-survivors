#ifndef TYPES_H
#define TYPES_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "tilemap.h"

typedef struct {
    float x;
    float y;
} Vec2;

typedef struct {
    SDL_Texture* texture;
    int frame_w;
    int frame_h;
    int num_frames;       // frames along X
    int num_directions;   // directions along Y (usually 8)
    float frame_duration; // seconds per frame
    float scale;
    bool loaded;
} SpriteSheet;

typedef struct {
    int current_frame;
    int direction;        // 0 to 7
    float anim_timer;
    bool is_moving;
} AnimState;

typedef enum {
    POKE_TYPE_NONE = 0,
    POKE_TYPE_NORMAL,
    POKE_TYPE_FIRE,
    POKE_TYPE_WATER,
    POKE_TYPE_GRASS,
    POKE_TYPE_ELECTRIC,
    POKE_TYPE_ICE,
    POKE_TYPE_FIGHTING,
    POKE_TYPE_POISON,
    POKE_TYPE_GROUND,
    POKE_TYPE_FLYING,
    POKE_TYPE_PSYCHIC,
    POKE_TYPE_BUG,
    POKE_TYPE_ROCK,
    POKE_TYPE_GHOST,
    POKE_TYPE_DRAGON
} PokeType;

typedef enum {
    STARTER_NONE       = 0,
    STARTER_BULBASAUR  = 1,
    STARTER_CHARMANDER = 4,
    STARTER_SQUIRTLE   = 7,
} StarterID;

typedef enum {
    APP_STATE_STARTER_SELECT = 0,
    APP_STATE_PLAYING        = 1,
    APP_STATE_LEVEL_UP       = 2,
    APP_STATE_GAME_OVER      = 3,
    APP_STATE_GAME_VICTORY   = 4,
} AppState;

typedef enum {
    PLAYER_STATE_NORMAL        = 0,
    PLAYER_STATE_DASHING       = 1,
    PLAYER_STATE_HYDRO_PUMP    = 2,
    PLAYER_STATE_FLAMETHROWER  = 3,
    PLAYER_STATE_SOLAR_BEAM    = 4,
} PlayerState;

typedef enum {
    BOT_OPTIMAL  = 0,
    BOT_MEDIOCRE = 1,
    BOT_BRUISER  = 2,
    BOT_COWARD   = 3,
} BotArchetype;

typedef struct {
    float enemy_hp_mult;           // Multiplier on enemy base HP (default ~1.35)
    float route_hp_scaling;        // Route progression HP scaling multiplier (default ~1.0)
    float time_hp_alpha;           // Dynamic time scaling coefficient (default ~0.15)
    float time_hp_gamma;           // Dynamic time scaling exponent (default ~1.0)
    float enemy_defense_base;      // Base enemy defense / damage reduction ratio (default ~0.10)
    float enemy_defense_per_stage; // Extra defense per evolution stage (default ~0.08)
    float weapon_damage_mult;      // Global player weapon damage multiplier (default ~1.0)
    float spawn_interval_mult;     // Spawner rate multiplier (default ~1.0)
    float max_enemies_mult;        // Max active enemy cap multiplier (default ~1.0)
    float boss_hp_mult;            // Gym Leader / Boss HP multiplier (default ~1.0)
} BalanceConfig;

typedef enum {
    ENEMY_MOVE_SWARM       = 0, // Direct approach with soft separation (Rattata, Mankey, Caterpie, Weedle)
    ENEMY_MOVE_FLANK_ORBIT = 1, // Aerial circling and swoop-in attacks (Pidgey, Zubat, Butterfree, Fearow)
    ENEMY_MOVE_CHARGER     = 2, // Windup telegraph laser, then 3.5x speed straight rush (Tauros, Rhyhorn, Beedrill, Machop)
    ENEMY_MOVE_MORTAR      = 3, // Roots in place at 380px, lobs arcing explosive/toxic shells (Oddish, Bellsprout, Koffing)
    ENEMY_MOVE_BURROW      = 4, // Subterranean assassin (Diglett, Dugtrio)
    ENEMY_MOVE_STATIONARY  = 5, // Immovable living bunker with massive defense ("Harden" Metapod, Kakuna)
} EnemyMovePattern;

typedef enum {
    ENEMY_ATTACK_TOUCH_ONLY      = 0, // Classic swarmer
    ENEMY_ATTACK_BURROW_ASSASSIN = 1, // Diglett: Underground untargetable stalk -> surface & windup -> swipe -> retreat
    ENEMY_ATTACK_SLAM_AOE        = 2, // Heavy stomp shockwave
    ENEMY_ATTACK_RANGED_EMBER    = 3, // Fire projectile
    ENEMY_ATTACK_RANGED_WATER    = 4, // Water projectile
    ENEMY_ATTACK_RANGED_SPARK    = 5, // Electric spark spread
    ENEMY_ATTACK_RANGED_SPORE    = 6, // Grass/Poison lingering spore
    ENEMY_ATTACK_RANGED_ROCK     = 7, // Rock projectile
    ENEMY_ATTACK_RANGED_GUST     = 8, // Flying gust slice
    ENEMY_ATTACK_RANGED_PSYCHIC  = 9, // Psychic homing orb
    ENEMY_ATTACK_BOSS_HYBRID     = 10 // Elite boss dual pattern
} EnemyAttackType;

typedef enum {
    ENEMY_DIGLETT = 0,
    ENEMY_SNORLAX = 1,
    ENEMY_BOSS    = 2,
} EnemyTypeID;

typedef enum {
    ENEMY_AI_CHASE     = 0, // Moving towards target (Diglett is underground & immune)
    ENEMY_AI_WINDUP    = 1, // Telegraphing/preparing attack (VULNERABLE!)
    ENEMY_AI_ATTACKING = 2, // Actively executing swipe, slam or projectile (VULNERABLE!)
    ENEMY_AI_RETREAT   = 3, // Fleeing underground after strike (IMMUNE!)
    ENEMY_AI_RECOVERY  = 4, // Post-attack delay before next action
} EnemyAIState;

typedef struct {
    int id;                   // 1 to 151
    const char* name;
    PokeType type1;
    PokeType type2;
    int stage;                // 1, 2, 3
    float scale;
    float base_hp;
    float base_speed;
    float touch_damage;
    float radius;
    int xp_value;
    float knockback_resist;
    int frame_w;
    int frame_h;
    int frame_count;
    EnemyAttackType attack_type;
    float attack_range;
    float attack_cooldown;
    float attack_windup;
    float attack_damage;
    float attack_radius;
} PokedexEntry;

#define MAX_ACTIVE_WEAPONS 4
#define MAX_PARTY_MEMBERS 3

typedef enum {
    WEAPON_NONE = 0,
    // Base Weapons
    WEAPON_VINE_WHIP,       // Spline wavy whips targeting close foes (Grass)
    WEAPON_FLAMETHROWER,    // Directional channeled flame cone + scorch (Fire)
    WEAPON_BUBBLE_BEAM,     // Buoyant bubbles trapping & popping enemies (Water)
    WEAPON_PETAL_DANCE,     // Orbiting flower blossoms expanding, spinning, dissipating (Grass)
    WEAPON_FIRE_SPIN,       // Whirling fireballs orbiting closely with high burn DPS (Fire)
    WEAPON_AQUA_ORBS,       // Fast rotating water spheres with massive knockback (Water)
    WEAPON_RAZOR_LEAF,      // Sharp spinning leaves launched at nearby enemies (Grass)
    WEAPON_SWIFT,           // Seeking golden psychic stars that home in on foes (Normal)
    WEAPON_THUNDER_SHOCK,   // Periodic vertical lightning bolt strikes from the sky (Electric)
    WEAPON_TOXIC_SPORE,     // Expanding poison spore ring leaving slowing toxic pools (Poison)
    WEAPON_ROCK_SLIDE,      // Tumbling crashing boulders dealing heavy blunt impact (Rock/Ground)
    
    // Evolved / Combined Weapons (Fusions)
    WEAPON_SOLAR_FRENZY,    // Vine Whip + Petal Dance: 4-way solar spline whips + perpetual petal storm
    WEAPON_FIRE_BLAST,      // Flamethrower + Fire Spin: Molten channeled torrent + explosive kanji starbursts & lava
    WEAPON_HYDRO_VORTEX,    // Bubble Beam + Aqua Orbs: Iridescent bubble maelstrom suctioning and detonating crowds
    WEAPON_PLASMA_TEMPEST,  // Swift + Thunder Shock: Homing golden stars triggering chain lightning arcs
    WEAPON_VENOM_CATACLYSM, // Razor Leaf + Toxic Spore: Corrosive acid leaves melting armor and leaving toxic mires
    WEAPON_SCALD_MAELSTROM, // Flamethrower + Bubble Beam: Boiling steam jets + scalding mist geysers (Fire + Water)
    WEAPON_WILDFIRE,        // Flamethrower + Razor Leaf: Piercing flaming leaves igniting ground wildfires (Fire + Grass)
    WEAPON_VERDANT_SURGE,   // Bubble Beam + Petal Dance: Aquatic lotus blossom shields that heal on hit (Water + Grass)
    WEAPON_STORM_SURGE,     // Bubble Beam + Thunder Shock: Electrified water bubbles exploding into thunder (Water + Electric)
    WEAPON_MAGMA_ERUPTION,  // Fire Spin + Rock Slide: Molten flaming meteors erupting into volcanic lava (Fire + Rock)

    WEAPON_TYPE_COUNT
} WeaponType;

typedef struct {
    WeaponType type;
    int level;               // 1 to 5 (or 1 for evolved)
    float cooldown;
    float cooldown_timer;
    float damage;
    float proj_speed;
    float proj_radius;
    int count;               // e.g. number of projectiles / whips / orbs
    int pierce;
    float knockback;
    float range;
    float state_timer;       // for custom cycle timers (e.g. Petal Dance bloom, Fire Blast star burst)
} ActiveWeaponSlot;

typedef struct {
    bool active;
    int pokedex_id;
    char name[32];
    PokeType type1;
    PokeType type2;
    Vec2 pos;
    Vec2 vel;
    float sprite_scale;
    SpriteSheet sprite;
    AnimState anim;
    float attack_cooldown;
    float attack_timer;
    float follow_angle_offset;
} PartyMember;

typedef struct {
    StarterID starter;
    Vec2 pos;
    Vec2 vel;
    float speed;
    float max_hp;
    float hp;
    float hp_regen;       // hp per second
    float radius;
    float pickup_radius;
    float invuln_timer;
    float invuln_duration;
    float damage_flash_timer;  // Red hit-flash ONLY when taking damage
    float damage_mult;
    bool god_mode;
    int level;
    int xp;
    int xp_to_next_level;
    int kills;
    
    // Active Abilities State
    PlayerState state;
    
    // Dash Ability (SHIFT)
    bool dash_unlocked;
    float dash_cooldown;        // default: 0.8s
    float dash_cooldown_timer;  // remaining cd
    float dash_duration;        // default: 0.20s
    float dash_timer;           // current time left in dash
    float dash_speed;           // default: 600.0 px/s
    Vec2  dash_dir;             // direction vector of dash
    
    // Active Damaging Ability (Q)
    bool special_q_unlocked;
    float special_q_cooldown;       // default: 4.0s
    float special_q_cooldown_timer; // remaining cd
    float special_q_duration;       // default: 1.6s
    float special_q_timer;          // current time left
    float special_q_damage;         // default: 95.0
    float special_q_range;          // default: 145.0 px
    float special_q_knockback;      // default: 750.0
    float special_q_spin_speed;     // rad/sec
    float special_q_current_angle;  // current spin/aim angle
    float special_q_tick_timer;     // damage tick rate
    
    // Move progression levels
    int move_basic_level; // Starting move: Vine Whip / Ember / Water Gun
    int move_a_level;     // Razor Leaf / Fire Spin / Aqua Orbiters
    int move_b_level;     // Petal Dance / Dragon Breath / Aqua Cleave
    int move_c_level;     // Toxic Spore / Flame Burst / Bubble Beam
    
    // Active Weapons Inventory (Roguelike System)
    ActiveWeaponSlot weapons[MAX_ACTIVE_WEAPONS];
    int weapon_count;
    Vec2 aim_dir;
    int pokeball_count; // Number of available Poké Balls to catch wild Pokémon (max 2)

    SpriteSheet sprite;          // default walk sprite
    SpriteSheet sprite_charge;   // Charge-Anim.png for Dash
    SpriteSheet sprite_special;  // Withdraw / Attack for Special Q
    AnimState anim;
} Player;

#define hydro_cooldown special_q_cooldown
#define hydro_cooldown_timer special_q_cooldown_timer
#define hydro_duration special_q_duration
#define hydro_timer special_q_timer
#define hydro_damage special_q_damage
#define hydro_range special_q_range
#define hydro_knockback special_q_knockback
#define hydro_spin_speed special_q_spin_speed
#define hydro_current_angle special_q_current_angle
#define hydro_tick_timer special_q_tick_timer

typedef struct {
    int pokedex_id;
    char name[32];
    float max_hp;
    float defense;          // Damage reduction ratio (0.0 to 0.70)
    float speed;
    float touch_damage;
    float radius;
    int xp_value;
    float knockback_resist; // 0.0 (full knockback) to 1.0 (unmovable)
    float spawn_weight;
    float sprite_scale;
    SDL_Color debug_color;
    SpriteSheet sprite;        // Base walk/burrow spritesheet
    
    // Modular Attack Configuration
    EnemyAttackType attack_type;
    EnemyMovePattern move_pattern; // Movement AI archetype
    float attack_range;
    float attack_cooldown;
    float attack_windup;
    float attack_duration;
    float attack_damage;
    float attack_radius;
    SpriteSheet sprite_attack;
    bool has_attack_sprite;
    bool is_boss;
} EnemyTypeConfig;

typedef struct {
    bool active;
    uint32_t uid;         // Monotonic unique entity ID for telemetry tracking
    int type; // index into game->enemy_types
    int pokedex_id;
    Vec2 pos;
    Vec2 vel;
    float hp;
    float max_hp;
    float defense;        // Dynamic damage reduction ratio
    float spawn_time;     // Sim timestamp when entity was spawned
    bool was_hit;         // True once entity has taken combat damage
    float first_hit_time; // Sim timestamp of first damage taken
    float radius;
    float touch_damage;
    float knockback_x;
    float knockback_y;
    float flash_timer;    // white hit flash
    float orbiter_hit_cooldown; // Cooldown between taking damage from persistent orbiters
    AnimState anim;       // walk/burrow animation state
    
    // Attack & AI State
    EnemyAIState ai_state;
    EnemyMovePattern move_pattern;
    float state_timer;
    float attack_cooldown_timer;
    bool is_underground;  // Diglett: untargetable while moving underground!
    bool attack_has_hit;  // Ensures swipe/stomp deals damage only once per cycle
    AnimState attack_anim;// attack spritesheet anim state

    // Specialized Movement Parameters
    float orbit_angle;       // For FLANK_ORBIT (circling flyers)
    float orbit_dist;
    Vec2 charge_dir;         // For CHARGER straight line rush
    float charge_speed;
    float mortar_cooldown_timer; // For MORTAR artillery lob

    // Bubble Trap State (Squirtle Bubble Beam)
    bool is_trapped;
    float trap_timer;
    float trap_duration;
    Vec2 trap_drift_vel;
} Enemy;

typedef enum {
    PROJ_WATER_BOLT    = 0,
    PROJ_ORBITER       = 1,
    PROJ_SLASH         = 2,
    PROJ_PULSE_RING    = 3,
    PROJ_EMBER         = 4,
    PROJ_RAZOR_LEAF    = 5,
    PROJ_FIRE_SPIN     = 6,
    PROJ_PETAL_DANCE   = 7,
    PROJ_ENEMY_BULLET  = 8,
    PROJ_BUBBLE        = 9,  // Squirtle: buoyant floating bubble that pops on impact
    PROJ_MORTAR_SHELL  = 10, // Enemy/Player arcing artillery shell
    PROJ_SWIFT_STAR    = 11, // Homing golden star
    PROJ_LIGHTNING_BOLT= 12, // Vertical crackling lightning strike
    PROJ_FIRE_BLAST    = 13, // Giant kanji "大" starburst
    PROJ_POKEBALL      = 14, // Thrown Poké Ball capture device
    PROJ_ROCK_SLIDE    = 15, // Tumbling crushing boulder
    PROJ_SCALD_BUBBLE  = 16, // Superheated boiling steam bubble
} ProjectileType;

typedef enum {
    HAZARD_FIRE_SCORCH   = 0, // Charmander: fiery burning patch on the ground
    HAZARD_TOXIC_SPORE   = 1, // Bulbasaur: slowing poison pollen puddle
    HAZARD_MORTAR_TARGET = 2, // Enemy mortar telegraph shadow / danger zone
    HAZARD_ACID_MIRE     = 3, // Corrosive toxic mire pool (Venom Cataclysm)
    HAZARD_LAVA_POOL     = 4, // Molten lava zone (Fire Blast)
    HAZARD_SCALD_MIST    = 5, // Boiling scalding steam geyser (Scald Maelstrom)
} HazardType;

typedef struct {
    bool active;
    HazardType type;
    Vec2 pos;
    float radius;
    float timer;
    float max_duration;
    float tick_timer;
    float damage;
    bool is_enemy;
    SDL_Color color;
} GroundHazard;

typedef struct {
    bool active;
    Vec2 start_pos;
    Vec2 tip_pos;
    Vec2 target_pos;
    Vec2 dir;
    float length;
    float max_length;
    float timer;
    float max_duration;
    float damage;
    float knockback;
    float wave_seed;
    bool struck;
} VineWhipStrike;

typedef struct {
    bool active;
    ProjectileType type;
    Vec2 pos;
    Vec2 vel;
    float radius;
    float damage;
    float lifetime;
    float max_lifetime;
    int pierce_left;
    float knockback;
    float orbit_angle;
    float orbit_dist;
    bool is_enemy;        // true if fired by an enemy (damages player)
    SDL_Color color;      // visual tint/color
    uint32_t hit_uids[8]; // Unique enemy IDs already struck by this projectile
    uint8_t hit_uids_count;
} Projectile;

typedef struct {
    bool active;
    Vec2 pos;
    Vec2 vel;
    int value;
    float radius;
    bool being_pulled;
} Gem;

typedef struct {
    bool active;
    Vec2 pos;
    float damage;
    float lifetime;
    float max_lifetime;
    SDL_Color color;
} DamageText;

// Weapon Configs
typedef struct {
    bool enabled;
    float cooldown;
    float cooldown_timer;
    float damage;
    float proj_speed;
    float proj_radius;
    int pierce;
    float knockback;
    // Weapon-specific parameters
    int orbiter_count;
    float orbiter_radius;
    float orbiter_speed;
    float slash_angle;
    float slash_range;
    float pulse_max_radius;

    // Petal Dance / Orbiter active/dissipate cycle
    float orbiter_cycle_timer;
    float orbiter_active_duration;
    float orbiter_cooldown_duration;
    float orbiter_current_radius;
    float orbiter_alpha;
} WeaponConfig;

typedef struct {
    WeaponConfig water_wand; // Water Gun / Ember / Razor Leaf (single/burst projectile)
    WeaponConfig orbiters;   // Aqua Orbiters / Fire Spin / Petal Dance
    WeaponConfig cleave;     // Aqua Jet / Dragon Breath / Vine Whip (arc sweep)
    WeaponConfig pulse;      // Bubble Beam / Flame Burst / Toxic Spore (radial pulse)
} WeaponsManager;

typedef struct {
    float spawn_interval;
    float spawn_timer;
    int max_enemies;
    int batch_size;
    float game_time;
    float difficulty_mult;
    bool separation_enabled;
    float separation_strength;
} SpawnerConfig;

typedef struct {
    bool open;
    int current_tab;       // 0: Player, 1: Weapons, 2: Enemies, 3: Spawner, 4: Profiler
    int selected_enemy_type;
    bool pause_when_open;
    bool show_hitboxes;
    bool show_spatial_grid;
    bool show_pickup_range;
    bool show_damage_numbers;
} DevMenuState;

typedef struct {
    // Microsecond timing
    uint64_t frame_start_counter;
    double frame_time_ms;     // Total wall-clock frame interval
    double t_cpu;             // Actual active CPU execution time
    double t_vsync;           // Idle time waiting on display refresh
    double fps;
    double avg_fps;
    double min_fps;
    double max_potential_fps;
    
    // Subsystem timings (in milliseconds)
    double t_input;
    double t_player;
    double t_weapons;
    double t_enemies;
    double t_spatial_grid;
    double t_collision;
    double t_render;
    
    // Entity counters
    int active_enemies;
    int peak_enemies;
    int active_projectiles;
    int active_gems;
    int active_particles;
    
    // Profiling history
    float frame_time_history[120];
    int history_index;
    
    // Spatial grid metrics
    int occupied_cells;
    int max_entities_in_cell;
    uint64_t collision_checks_done;
    uint64_t collision_checks_avoided;
} ProfilerMetrics;

typedef enum {
    CHOICE_UPGRADE_BASIC = 0,
    CHOICE_LEARN_MOVE_A,
    CHOICE_LEARN_MOVE_B,
    CHOICE_LEARN_MOVE_C,
    CHOICE_LEARN_ULT_Q,
    CHOICE_STAT_HP,
    CHOICE_STAT_SPEED,
    CHOICE_STAT_DAMAGE,
    CHOICE_LEARN_WEAPON,
    CHOICE_UPGRADE_WEAPON,
    CHOICE_EVOLVE_WEAPON,
} ChoiceType;

typedef struct {
    ChoiceType type;
    WeaponType weapon_type;
    int slot_index;
    int slot_ingredient_b;
    char title[32];
    char desc[96];
    char tag[16];
    SDL_Color color;
} LevelUpCard;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    bool paused;
    bool vsync_enabled;
    
    AppState app_state;
    StarterID selected_starter_hover;
    
    // Level Up modal state
    LevelUpCard level_up_cards[3];
    int selected_card_hover;
    
    // Route & Wave progression
    int current_section;     // 0 to 7 (Brock to Giovanni/Elite Four)
    int current_route_index; // route index within section
    float route_timer;       // counts down from 60.0s
    int route_kills;         // current kills on this route
    int route_kill_quota;    // e.g. 40
    bool gym_boss_spawned;
    float route_banner_timer;
    char route_banner_text[64];
    
    Vec2 camera;
    Vec2 mouse_pos;
    bool mouse_active;
    
    Player player;
    PartyMember party[MAX_PARTY_MEMBERS];
    int party_count;
    EnemyTypeConfig enemy_types[MAX_ENEMY_TYPES];
    int active_enemy_type_count;
    Enemy enemies[MAX_ENEMIES];
    int active_enemy_indices[MAX_ENEMIES];
    int active_enemy_count;
    int enemy_index_in_active[MAX_ENEMIES];
    
#define MAX_GROUND_HAZARDS 64
#define MAX_VINE_STRIKES 16

    Projectile projectiles[MAX_PROJECTILES];
    Gem gems[MAX_GEMS];
    DamageText damage_texts[MAX_PARTICLES];
    GroundHazard ground_hazards[MAX_GROUND_HAZARDS];
    VineWhipStrike vine_strikes[MAX_VINE_STRIKES];
    
    WeaponsManager weapons;
    SpawnerConfig spawner;
    DevMenuState dev_menu;
    ProfilerMetrics profiler;
    
    SDL_Texture* tileset_texture;
    SDL_Texture* floor_cache_texture;
    SDL_Texture* font_texture;
    TileMap tilemap;
    
    // Starter selection portraits
    SDL_Texture* starter_portraits[3]; // Bulbasaur (0), Charmander (1), Squirtle (2)
    SpriteSheet starter_sprites[3];

    // High-precision big data telemetry & balance tracking
    float total_damage_dealt;
    float total_overkill_damage;
    float danger_time_sec;
    float prev_player_hp;
    float hp_volatility_accum;
    int hp_samples_count;
    int instant_kills;
    int kills_stage1;
    int kills_stage2;
    int kills_stage3;
    float sum_ttk_stage1;
    float sum_ttk_stage2;
    float sum_ttk_stage3;
    float telemetry_timer;
} GameState;

#endif // TYPES_H
