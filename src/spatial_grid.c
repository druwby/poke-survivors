#include "spatial_grid.h"
#include <string.h>
#include <math.h>

void Grid_Init(SpatialGrid* grid) {
    memset(grid->cell_head, -1, sizeof(grid->cell_head));
    memset(grid->enemy_next, -1, sizeof(grid->enemy_next));
    grid->num_dirty_cells = 0;
    grid->occupied_cells = 0;
    grid->max_cell_count = 0;
}

void Grid_Clear(SpatialGrid* grid) {
    // Ultra-fast clearing: only reset the cells that were actually written to!
    for (int i = 0; i < grid->num_dirty_cells; i++) {
        grid->cell_head[grid->dirty_cells[i]] = -1;
    }
    grid->num_dirty_cells = 0;
    grid->occupied_cells = 0;
    grid->max_cell_count = 0;
}

void Grid_Insert(SpatialGrid* grid, int enemy_idx, float x, float y) {
    if (enemy_idx < 0 || enemy_idx >= MAX_ENEMIES) return;
    
    int cx = (int)(x / GRID_CELL_SIZE);
    int cy = (int)(y / GRID_CELL_SIZE);
    
    // Clamp to world bounds
    if (cx < 0) cx = 0;
    if (cx >= GRID_COLS) cx = GRID_COLS - 1;
    if (cy < 0) cy = 0;
    if (cy >= GRID_ROWS) cy = GRID_ROWS - 1;
    
    int cell = cy * GRID_COLS + cx;
    if (grid->cell_head[cell] == -1) {
        grid->dirty_cells[grid->num_dirty_cells++] = cell;
    }
    grid->enemy_next[enemy_idx] = grid->cell_head[cell];
    grid->cell_head[cell] = enemy_idx;
}

int Grid_QueryCircle(const SpatialGrid* grid, float x, float y, float radius, int* out_indices, int max_out) {
    int min_cx = (int)((x - radius) / GRID_CELL_SIZE);
    int max_cx = (int)((x + radius) / GRID_CELL_SIZE);
    int min_cy = (int)((y - radius) / GRID_CELL_SIZE);
    int max_cy = (int)((y + radius) / GRID_CELL_SIZE);
    
    if (min_cx < 0) min_cx = 0;
    if (max_cx >= GRID_COLS) max_cx = GRID_COLS - 1;
    if (min_cy < 0) min_cy = 0;
    if (max_cy >= GRID_ROWS) max_cy = GRID_ROWS - 1;
    
    int count = 0;
    
    for (int cy = min_cy; cy <= max_cy; cy++) {
        for (int cx = min_cx; cx <= max_cx; cx++) {
            int cell = cy * GRID_COLS + cx;
            int curr = grid->cell_head[cell];
            
            while (curr != -1 && count < max_out) {
                out_indices[count++] = curr;
                curr = grid->enemy_next[curr];
            }
        }
    }
    
    return count;
}

void Grid_ComputeMetrics(SpatialGrid* grid) {
    int max_c = 0;
    for (int i = 0; i < grid->num_dirty_cells; i++) {
        int cell = grid->dirty_cells[i];
        int curr = grid->cell_head[cell];
        int c = 0;
        while (curr != -1) {
            c++;
            curr = grid->enemy_next[curr];
        }
        if (c > max_c) max_c = c;
    }
    grid->occupied_cells = grid->num_dirty_cells;
    grid->max_cell_count = max_c;
}
