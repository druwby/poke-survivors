#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H

#include "config.h"
#include <stdbool.h>

#define TOTAL_CELLS (GRID_COLS * GRID_ROWS)

typedef struct {
    int cell_head[TOTAL_CELLS];
    int enemy_next[MAX_ENEMIES];
    int dirty_cells[TOTAL_CELLS];
    int num_dirty_cells;
    int occupied_cells;
    int max_cell_count;
} SpatialGrid;

void Grid_Init(SpatialGrid* grid);
void Grid_Clear(SpatialGrid* grid);
void Grid_Insert(SpatialGrid* grid, int enemy_idx, float x, float y);
int Grid_QueryCircle(const SpatialGrid* grid, float x, float y, float radius, int* out_indices, int max_out);
void Grid_ComputeMetrics(SpatialGrid* grid);

#endif // SPATIAL_GRID_H
