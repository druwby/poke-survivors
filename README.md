# Rogue Survivors (Pure C Engine)

A high-performance, lightweight top-down horde survival roguelike built in pure C99 and SDL2.

## Key Features
- **Extreme Efficiency**: 71 KB binary. Contiguous static entity pools (zero heap allocations in game loop).
- **Spatial Hash Partitioning**: 16,384 uniform cells. Simulates thousands of enemies and collisions in < 0.25 ms.
- **8-Directional Spritesheet Animation**: Native PNG decoding via macOS `ImageIO`/`CoreGraphics`. Automatically slices 8-directional sprite sheets (e.g., Squirtle, Diglett, Snorlax) and maps velocity vectors to directional rows.
- **Auto-Attacking Weapons**:
  - *Water Wand*: Rapid auto-homing bolts with piercing.
  - *Aqua Orbiters*: Rotating orbital shields dealing continuous contact damage and knockback.
  - *Aqua Cleave*: Wide slashing arc in facing direction.
  - *Radial Pulse*: Expanding shockwaves.
- **Interactive Dev Menu** (`F1`, `TAB`, or `` ` ``):
  - 5 interactive tabs: Player, Weapons, Enemies, Spawner, and Visuals/System.
  - Live sliders with instant numerical adjustment.
  - Per-enemy archetype submenus (Diglett, Snorlax, Stalker, Boss).
- **Real-Time Profiler & Latency Graph**:
  - Microsecond-level breakdown (AI, Grid, Collision, Render).
  - Live FPS and 1% low FPS.
  - Active entity counters (Enemies, Projectiles, Gems, Particles).
  - 120-frame sparkline latency graph.

## Controls
| Action | Key |
|---|---|
| **Move** | `W`, `A`, `S`, `D` or Arrow Keys |
| **Dev Menu** | `F1`, `TAB`, or `` ` `` (backtick) |
| **Dev Menu Interaction** | Mouse Click, Drag Sliders, Toggle Checkboxes |
| **Quit Game** | `ESC` |

## Building & Running

### Requirements
- macOS (Apple Silicon or Intel)
- `SDL2` (`brew install sdl2`)
- Apple Command Line Tools (`clang`, `make`)

### Compile & Run
```bash
# Build the executable
make

# Run the game
./rogue_game

# Run high-load 1,200-entity stress benchmark
./rogue_game --benchmark
```

## Folder Structure
```
testsprites/
├── assets/
│   ├── characters/
│   │   ├── squirtle/       # Player spritesheet (squirtle_walksprite.png)
│   │   ├── diglett/        # Swarm enemy (Walk-Anim.png)
│   │   └── snorlax/        # Brute & Boss enemy (Walk-Anim.png)
│   └── tilesets/
│       └── tileset1.png    # Dungeon floor tileset
├── src/                    # Pure C99 game engine code
└── rogue_game              # Compiled binary
```

## Adding New Spritesheets
To drop in a new character or enemy:
1. Place your spritesheet PNG inside `assets/characters/<name>/`.
2. In `src/game.c`, load it using:
   ```c
   SpriteSheet sheet = Sprite_Load(renderer, GetAssetPath("characters/<name>/Walk-Anim.png"), frame_w, frame_h, frames_count, directions_count, frame_duration, scale);
   ```
3. The engine will automatically slice each frame, calculate 8-directional movement vectors, and animate playback.
