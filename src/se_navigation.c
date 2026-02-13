// Syphax-Engine - Ougi Washi

#include "se_navigation.h"
#include "se_debug.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static i32 se_navigation_cell_index(const se_navigation_grid* grid, const se_navigation_cell cell) {
	return cell.y * grid->width + cell.x;
}

static se_navigation_cell se_navigation_index_cell(const se_navigation_grid* grid, const i32 index) {
	se_navigation_cell cell = {0};
	cell.x = index % grid->width;
	cell.y = index / grid->width;
	return cell;
}

static f32 se_navigation_heuristic(const se_navigation_cell a, const se_navigation_cell b, const b8 allow_diagonal) {
	const i32 dx = abs(a.x - b.x);
	const i32 dy = abs(a.y - b.y);
	if (!allow_diagonal) {
		return (f32)(dx + dy);
	}
	const f32 d = 1.0f;
	const f32 d2 = 1.4142135f;
	const i32 min_d = s_min(dx, dy);
	return d * (f32)(dx + dy) + (d2 - 2.0f * d) * (f32)min_d;
}

static b8 se_navigation_los(const se_navigation_grid* grid, const se_navigation_cell start, const se_navigation_cell end) {
	i32 x0 = start.x;
	i32 y0 = start.y;
	const i32 x1 = end.x;
	const i32 y1 = end.y;
	const i32 dx = abs(x1 - x0);
	const i32 dy = abs(y1 - y0);
	const i32 sx = x0 < x1 ? 1 : -1;
	const i32 sy = y0 < y1 ? 1 : -1;
	i32 err = dx - dy;

	while (true) {
		const se_navigation_cell cell = {x0, y0};
		if (se_navigation_grid_is_blocked(grid, cell)) {
			return false;
		}
		if (x0 == x1 && y0 == y1) {
			return true;
		}
		const i32 e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
}

b8 se_navigation_grid_create(se_navigation_grid* out_grid, const i32 width, const i32 height, const f32 cell_size, const s_vec2* origin) {
	if (!SE_VALIDATE(out_grid && origin && width > 0 && height > 0 && cell_size > 0.0f, SE_RESULT_INVALID_ARGUMENT)) {
		return false;
	}
	memset(out_grid, 0, sizeof(*out_grid));
	out_grid->width = width;
	out_grid->height = height;
	out_grid->cell_size = cell_size;
	out_grid->origin = *origin;
	s_array_init(&out_grid->occupancy);
	s_array_reserve(&out_grid->occupancy, (sz)(width * height));
	const sz count = (sz)(width * height);
	for (sz i = 0; i < count; ++i) {
		u8 blocked = 0;
		s_array_add(&out_grid->occupancy, blocked);
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_navigation_grid_destroy(se_navigation_grid* grid) {
	if (!grid) {
		return;
	}
	s_array_clear(&grid->occupancy);
	memset(grid, 0, sizeof(*grid));
}

void se_navigation_grid_clear(se_navigation_grid* grid, const b8 blocked) {
	if (!grid) {
		return;
	}
	const u8 value = blocked ? 1u : 0u;
	for (sz i = 0; i < s_array_get_size(&grid->occupancy); ++i) {
		u8* slot = s_array_get(&grid->occupancy, s_array_handle(&grid->occupancy, (u32)i));
		if (slot) {
			*slot = value;
		}
	}
}

b8 se_navigation_grid_is_valid_cell(const se_navigation_grid* grid, const se_navigation_cell cell) {
	if (!grid) {
		return false;
	}
	return cell.x >= 0 && cell.y >= 0 && cell.x < grid->width && cell.y < grid->height;
}

b8 se_navigation_grid_is_blocked(const se_navigation_grid* grid, const se_navigation_cell cell) {
	if (!se_navigation_grid_is_valid_cell(grid, cell)) {
		return true;
	}
	const i32 index = se_navigation_cell_index(grid, cell);
	const u8* blocked = s_array_get((se_navigation_occupancy*)&grid->occupancy, s_array_handle((se_navigation_occupancy*)&grid->occupancy, (u32)index));
	return blocked ? (*blocked != 0) : true;
}

b8 se_navigation_grid_set_blocked(se_navigation_grid* grid, const se_navigation_cell cell, const b8 blocked) {
	if (!se_navigation_grid_is_valid_cell(grid, cell)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const i32 index = se_navigation_cell_index(grid, cell);
	u8* slot = s_array_get(&grid->occupancy, s_array_handle(&grid->occupancy, (u32)index));
	if (!slot) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*slot = blocked ? 1u : 0u;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_navigation_grid_set_blocked_rect(se_navigation_grid* grid, const se_navigation_cell min_cell, const se_navigation_cell max_cell, const b8 blocked) {
	if (!grid) {
		return;
	}
	const i32 min_x = s_min(min_cell.x, max_cell.x);
	const i32 max_x = s_max(min_cell.x, max_cell.x);
	const i32 min_y = s_min(min_cell.y, max_cell.y);
	const i32 max_y = s_max(min_cell.y, max_cell.y);
	for (i32 y = min_y; y <= max_y; ++y) {
		for (i32 x = min_x; x <= max_x; ++x) {
			se_navigation_grid_set_blocked(grid, (se_navigation_cell){x, y}, blocked);
		}
	}
}

void se_navigation_grid_set_blocked_circle(se_navigation_grid* grid, const se_navigation_cell center, const i32 radius_cells, const b8 blocked) {
	if (!grid || radius_cells < 0) {
		return;
	}
	const i32 r2 = radius_cells * radius_cells;
	for (i32 y = center.y - radius_cells; y <= center.y + radius_cells; ++y) {
		for (i32 x = center.x - radius_cells; x <= center.x + radius_cells; ++x) {
			const i32 dx = x - center.x;
			const i32 dy = y - center.y;
			if (dx * dx + dy * dy > r2) {
				continue;
			}
			se_navigation_grid_set_blocked(grid, (se_navigation_cell){x, y}, blocked);
		}
	}
}

void se_navigation_grid_set_dynamic_obstacle(se_navigation_grid* grid, const se_navigation_cell cell, const b8 blocked) {
	se_navigation_grid_set_blocked(grid, cell, blocked);
}

void se_navigation_grid_set_dynamic_obstacle_rect(se_navigation_grid* grid, const se_navigation_cell min_cell, const se_navigation_cell max_cell, const b8 blocked) {
	se_navigation_grid_set_blocked_rect(grid, min_cell, max_cell, blocked);
}

b8 se_navigation_world_to_cell(const se_navigation_grid* grid, const s_vec2* world, se_navigation_cell* out_cell) {
	if (!grid || !world || !out_cell || grid->cell_size <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const f32 rel_x = world->x - grid->origin.x;
	const f32 rel_y = world->y - grid->origin.y;
	se_navigation_cell cell = {0};
	cell.x = (i32)floorf(rel_x / grid->cell_size);
	cell.y = (i32)floorf(rel_y / grid->cell_size);
	if (!se_navigation_grid_is_valid_cell(grid, cell)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_cell = cell;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_navigation_cell_to_world(const se_navigation_grid* grid, const se_navigation_cell cell, s_vec2* out_world_center) {
	if (!grid || !out_world_center || !se_navigation_grid_is_valid_cell(grid, cell)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	out_world_center->x = grid->origin.x + ((f32)cell.x + 0.5f) * grid->cell_size;
	out_world_center->y = grid->origin.y + ((f32)cell.y + 0.5f) * grid->cell_size;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_navigation_path_init(se_navigation_path* path, const sz reserve_count) {
	if (!path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	memset(path, 0, sizeof(*path));
	s_array_init(&path->cells);
	if (reserve_count > 0) {
		s_array_reserve(&path->cells, reserve_count);
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_navigation_path_clear(se_navigation_path* path) {
	if (!path) {
		return;
	}
	s_array_clear(&path->cells);
	memset(path, 0, sizeof(*path));
}

void se_navigation_path_reset(se_navigation_path* path) {
	if (!path) {
		return;
	}
	while (s_array_get_size(&path->cells) > 0) {
		s_array_remove(&path->cells, s_array_handle(&path->cells, (u32)(s_array_get_size(&path->cells) - 1)));
	}
}

sz se_navigation_get_neighbors(const se_navigation_grid* grid, const se_navigation_cell cell, const b8 allow_diagonal, se_navigation_cell out_neighbors[8]) {
	if (!grid || !out_neighbors) {
		return 0;
	}
	static const se_navigation_cell k_cardinal[4] = {
		{1, 0}, {-1, 0}, {0, 1}, {0, -1}
	};
	static const se_navigation_cell k_diagonal[4] = {
		{1, 1}, {1, -1}, {-1, 1}, {-1, -1}
	};
	sz count = 0;
	for (sz i = 0; i < 4; ++i) {
		se_navigation_cell n = {cell.x + k_cardinal[i].x, cell.y + k_cardinal[i].y};
		if (se_navigation_grid_is_valid_cell(grid, n) && !se_navigation_grid_is_blocked(grid, n)) {
			out_neighbors[count++] = n;
		}
	}
	if (allow_diagonal) {
		for (sz i = 0; i < 4; ++i) {
			se_navigation_cell n = {cell.x + k_diagonal[i].x, cell.y + k_diagonal[i].y};
			if (se_navigation_grid_is_valid_cell(grid, n) && !se_navigation_grid_is_blocked(grid, n)) {
				out_neighbors[count++] = n;
			}
		}
	}
	return count;
}

b8 se_navigation_raycast_cells(const se_navigation_grid* grid, const se_navigation_cell start, const se_navigation_cell end, se_navigation_path* out_cells) {
	if (!grid || !out_cells || !se_navigation_grid_is_valid_cell(grid, start) || !se_navigation_grid_is_valid_cell(grid, end)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (s_array_get_capacity(&out_cells->cells) == 0) {
		s_array_init(&out_cells->cells);
	}
	se_navigation_path_reset(out_cells);

	i32 x0 = start.x;
	i32 y0 = start.y;
	const i32 x1 = end.x;
	const i32 y1 = end.y;
	const i32 dx = abs(x1 - x0);
	const i32 dy = abs(y1 - y0);
	const i32 sx = x0 < x1 ? 1 : -1;
	const i32 sy = y0 < y1 ? 1 : -1;
	i32 err = dx - dy;
	b8 clear = true;

	while (true) {
		se_navigation_cell cell = {x0, y0};
		s_array_add(&out_cells->cells, cell);
		if (se_navigation_grid_is_blocked(grid, cell)) {
			clear = false;
		}
		if (x0 == x1 && y0 == y1) {
			break;
		}
		const i32 e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
	se_set_last_error(clear ? SE_RESULT_OK : SE_RESULT_NOT_FOUND);
	return clear;
}

b8 se_navigation_find_path(const se_navigation_grid* grid, const se_navigation_cell start, const se_navigation_cell goal, const b8 allow_diagonal, se_navigation_debug_hook debug_hook, void* debug_user_data, se_navigation_path* out_path) {
	se_debug_trace_begin("navigation_astar");
	if (!SE_VALIDATE(grid && out_path && se_navigation_grid_is_valid_cell(grid, start) && se_navigation_grid_is_valid_cell(grid, goal), SE_RESULT_INVALID_ARGUMENT)) {
		se_debug_trace_end("navigation_astar");
		return false;
	}
	if (se_navigation_grid_is_blocked(grid, start) || se_navigation_grid_is_blocked(grid, goal)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		se_debug_trace_end("navigation_astar");
		return false;
	}
	if (s_array_get_capacity(&out_path->cells) == 0) {
		s_array_init(&out_path->cells);
		s_array_reserve(&out_path->cells, (sz)(grid->width * grid->height));
	}
	se_navigation_path_reset(out_path);

	const i32 node_count = grid->width * grid->height;
	f32* g_score = malloc(sizeof(f32) * (sz)node_count);
	f32* f_score = malloc(sizeof(f32) * (sz)node_count);
	i32* came_from = malloc(sizeof(i32) * (sz)node_count);
	u8* is_open = malloc(sizeof(u8) * (sz)node_count);
	u8* is_closed = malloc(sizeof(u8) * (sz)node_count);
	se_navigation_cell* open_cells = NULL;
	se_navigation_cell* closed_cells = NULL;
	if (debug_hook) {
		open_cells = malloc(sizeof(se_navigation_cell) * (sz)node_count);
		closed_cells = malloc(sizeof(se_navigation_cell) * (sz)node_count);
	}

	if (!g_score || !f_score || !came_from || !is_open || !is_closed || (debug_hook && (!open_cells || !closed_cells))) {
		free(g_score);
		free(f_score);
		free(came_from);
		free(is_open);
		free(is_closed);
		free(open_cells);
		free(closed_cells);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		se_debug_trace_end("navigation_astar");
		return false;
	}

	for (i32 i = 0; i < node_count; ++i) {
		g_score[i] = FLT_MAX;
		f_score[i] = FLT_MAX;
		came_from[i] = -1;
		is_open[i] = 0u;
		is_closed[i] = 0u;
	}

	const i32 start_idx = se_navigation_cell_index(grid, start);
	const i32 goal_idx = se_navigation_cell_index(grid, goal);
	g_score[start_idx] = 0.0f;
	f_score[start_idx] = se_navigation_heuristic(start, goal, allow_diagonal);
	is_open[start_idx] = 1u;

	b8 found = false;
	while (true) {
		i32 current = -1;
		f32 current_f = FLT_MAX;
		for (i32 i = 0; i < node_count; ++i) {
			if (!is_open[i]) {
				continue;
			}
			if (f_score[i] < current_f) {
				current = i;
				current_f = f_score[i];
			}
		}
		if (current < 0) {
			break;
		}
		if (current == goal_idx) {
			found = true;
			break;
		}

		is_open[current] = 0u;
		is_closed[current] = 1u;
		const se_navigation_cell current_cell = se_navigation_index_cell(grid, current);
		se_navigation_cell neighbors[8] = {0};
		const sz neighbor_count = se_navigation_get_neighbors(grid, current_cell, allow_diagonal, neighbors);
		for (sz i = 0; i < neighbor_count; ++i) {
			const se_navigation_cell neighbor = neighbors[i];
			const i32 neighbor_idx = se_navigation_cell_index(grid, neighbor);
			if (is_closed[neighbor_idx]) {
				continue;
			}
			const f32 step_cost = (neighbor.x != current_cell.x && neighbor.y != current_cell.y) ? 1.4142135f : 1.0f;
			const f32 tentative_g = g_score[current] + step_cost;
			if (!is_open[neighbor_idx] || tentative_g < g_score[neighbor_idx]) {
				came_from[neighbor_idx] = current;
				g_score[neighbor_idx] = tentative_g;
				f_score[neighbor_idx] = tentative_g + se_navigation_heuristic(neighbor, goal, allow_diagonal);
				is_open[neighbor_idx] = 1u;
			}
		}

		if (debug_hook) {
			sz open_count = 0;
			sz closed_count = 0;
			for (i32 i = 0; i < node_count; ++i) {
				if (is_open[i]) {
					open_cells[open_count++] = se_navigation_index_cell(grid, i);
				}
				if (is_closed[i]) {
					closed_cells[closed_count++] = se_navigation_index_cell(grid, i);
				}
			}
			debug_hook(grid, open_cells, open_count, closed_cells, closed_count, debug_user_data);
		}
	}

	if (found) {
		i32 current = goal_idx;
		while (current >= 0) {
			se_navigation_cell cell = se_navigation_index_cell(grid, current);
			s_array_add(&out_path->cells, cell);
			current = came_from[current];
		}
		const sz n = s_array_get_size(&out_path->cells);
		for (sz i = 0; i < n / 2; ++i) {
			se_navigation_cell* a = s_array_get(&out_path->cells, s_array_handle(&out_path->cells, (u32)i));
			se_navigation_cell* b = s_array_get(&out_path->cells, s_array_handle(&out_path->cells, (u32)(n - i - 1)));
			se_navigation_cell tmp = *a;
			*a = *b;
			*b = tmp;
		}
	}

	free(g_score);
	free(f_score);
	free(came_from);
	free(is_open);
	free(is_closed);
	free(open_cells);
	free(closed_cells);

	se_set_last_error(found ? SE_RESULT_OK : SE_RESULT_NOT_FOUND);
	se_debug_log(
		found ? SE_DEBUG_LEVEL_DEBUG : SE_DEBUG_LEVEL_INFO,
		SE_DEBUG_CATEGORY_NAVIGATION,
		"A* %s start=(%d,%d) goal=(%d,%d) nodes=%zu",
		found ? "found" : "failed",
		start.x,
		start.y,
		goal.x,
		goal.y,
		s_array_get_size(&out_path->cells));
	se_debug_trace_end("navigation_astar");
	return found;
}

b8 se_navigation_find_path_simple(const se_navigation_grid* grid, const se_navigation_cell start, const se_navigation_cell goal, const b8 allow_diagonal, se_navigation_path* out_path) {
	return se_navigation_find_path(grid, start, goal, allow_diagonal, NULL, NULL, out_path);
}

void se_navigation_smooth_path(const se_navigation_grid* grid, se_navigation_path* path) {
	if (!grid || !path || s_array_get_size(&path->cells) < 3) {
		return;
	}
	se_navigation_path reduced = {0};
	s_array_init(&reduced.cells);
	s_array_reserve(&reduced.cells, s_array_get_size(&path->cells));

	sz anchor = 0;
	se_navigation_cell first = *s_array_get(&path->cells, s_array_handle(&path->cells, 0));
	s_array_add(&reduced.cells, first);
	while (anchor < s_array_get_size(&path->cells) - 1) {
		sz next = s_array_get_size(&path->cells) - 1;
		const se_navigation_cell* anchor_cell = s_array_get(&path->cells, s_array_handle(&path->cells, (u32)anchor));
		while (next > anchor + 1) {
			const se_navigation_cell* candidate = s_array_get(&path->cells, s_array_handle(&path->cells, (u32)next));
			if (se_navigation_los(grid, *anchor_cell, *candidate)) {
				break;
			}
			next--;
		}
		se_navigation_cell chosen = *s_array_get(&path->cells, s_array_handle(&path->cells, (u32)next));
		s_array_add(&reduced.cells, chosen);
		anchor = next;
	}

	se_navigation_path_reset(path);
	for (sz i = 0; i < s_array_get_size(&reduced.cells); ++i) {
		se_navigation_cell cell = *s_array_get(&reduced.cells, s_array_handle(&reduced.cells, (u32)i));
		s_array_add(&path->cells, cell);
	}
	s_array_clear(&reduced.cells);
}
