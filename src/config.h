#ifndef CONFIG_H
#define CONFIG_H

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define TARGET_FPS    60

#define MAX_ENEMIES     4000
#define MAX_PROJECTILES 2000
#define MAX_GEMS        4000
#define MAX_PARTICLES   1000

#define GRID_CELL_SIZE  64
#define GRID_COLS       128
#define GRID_ROWS       128
#define GRID_MAX_PER_CELL 64

#define WORLD_SIZE_X    (GRID_COLS * GRID_CELL_SIZE)
#define WORLD_SIZE_Y    (GRID_ROWS * GRID_CELL_SIZE)

#define MAX_ENEMY_TYPES 8

#endif // CONFIG_H
