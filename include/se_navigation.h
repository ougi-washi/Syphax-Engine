// Syphax-Engine - Ougi Washi

#ifndef SE_NAVIGATION_H
#define SE_NAVIGATION_H

#include "se_math.h"
#include "se_defines.h"

#include "syphax/s_array.h"

typedef struct {
	i32 x;
	i32 y;
} se_navigation_cell;

typedef s_array(se_navigation_cell, se_navigation_cells);
typedef s_array(u8, se_navigation_occupancy);

typedef struct {
	se_navigation_cells cells;
} se_navigation_path;

typedef struct {
	i32 width;
	i32 height;
	f32 cell_size;
	s_vec2 origin;
	se_navigation_occupancy occupancy;
} se_navigation_grid;

typedef void (*se_navigation_debug_hook)(
	const se_navigation_grid* grid,
	const se_navigation_cell* open_cells,
	sz open_count,
	const se_navigation_cell* closed_cells,
	sz closed_count,
	void* user_data);

extern b8 se_navigation_grid_create(se_navigation_grid* out_grid, const i32 width, const i32 height, const f32 cell_size, const s_vec2* origin);
extern void se_navigation_grid_destroy(se_navigation_grid* grid);
extern void se_navigation_grid_clear(se_navigation_grid* grid, const b8 blocked);
extern b8 se_navigation_grid_is_valid_cell(const se_navigation_grid* grid, const se_navigation_cell cell);
extern b8 se_navigation_grid_is_blocked(const se_navigation_grid* grid, const se_navigation_cell cell);
extern b8 se_navigation_grid_set_blocked(se_navigation_grid* grid, const se_navigation_cell cell, const b8 blocked);
extern void se_navigation_grid_set_blocked_rect(se_navigation_grid* grid, const se_navigation_cell min_cell, const se_navigation_cell max_cell, const b8 blocked);
extern void se_navigation_grid_set_blocked_circle(se_navigation_grid* grid, const se_navigation_cell center, const i32 radius_cells, const b8 blocked);
extern void se_navigation_grid_set_dynamic_obstacle(se_navigation_grid* grid, const se_navigation_cell cell, const b8 blocked);
extern void se_navigation_grid_set_dynamic_obstacle_rect(se_navigation_grid* grid, const se_navigation_cell min_cell, const se_navigation_cell max_cell, const b8 blocked);

extern b8 se_navigation_world_to_cell(const se_navigation_grid* grid, const s_vec2* world, se_navigation_cell* out_cell);
extern b8 se_navigation_cell_to_world(const se_navigation_grid* grid, const se_navigation_cell cell, s_vec2* out_world_center);

extern b8 se_navigation_path_init(se_navigation_path* path, const sz reserve_count);
extern void se_navigation_path_clear(se_navigation_path* path);
extern void se_navigation_path_reset(se_navigation_path* path);

extern sz se_navigation_get_neighbors(const se_navigation_grid* grid, const se_navigation_cell cell, const b8 allow_diagonal, se_navigation_cell out_neighbors[8]);
extern b8 se_navigation_raycast_cells(const se_navigation_grid* grid, const se_navigation_cell start, const se_navigation_cell end, se_navigation_path* out_cells);
extern b8 se_navigation_find_path(const se_navigation_grid* grid, const se_navigation_cell start, const se_navigation_cell goal, const b8 allow_diagonal, se_navigation_debug_hook debug_hook, void* debug_user_data, se_navigation_path* out_path);
extern b8 se_navigation_find_path_simple(const se_navigation_grid* grid, const se_navigation_cell start, const se_navigation_cell goal, const b8 allow_diagonal, se_navigation_path* out_path);
extern void se_navigation_smooth_path(const se_navigation_grid* grid, se_navigation_path* path);

#endif // SE_NAVIGATION_H
