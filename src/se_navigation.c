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

static f32 se_navigation_step_distance(const se_navigation_cell a, const se_navigation_cell b) {
	return (a.x != b.x && a.y != b.y) ? 1.4142135f : 1.0f;
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

static i32 se_navigation_field_cell_index(const se_navigation_field* field, const se_navigation_cell cell) {
	return cell.y * field->width + cell.x;
}

static se_navigation_cell se_navigation_field_index_cell(const se_navigation_field* field, const i32 index) {
	se_navigation_cell cell = {0};
	cell.x = index % field->width;
	cell.y = index / field->width;
	return cell;
}

static void se_navigation_field_make_default_sample(const se_navigation_field* field, const se_navigation_cell cell, se_navigation_sample* out_sample) {
	if (!field || !out_sample) {
		return;
	}
	out_sample->world = s_vec3(
		field->origin.x + ((f32)cell.x + 0.5f) * field->cell_size,
		field->origin.y,
		field->origin.z + ((f32)cell.y + 0.5f) * field->cell_size);
	out_sample->height = out_sample->world.y;
	out_sample->normal = s_vec3(0.0f, 1.0f, 0.0f);
	out_sample->cost = 1.0f;
	out_sample->walkable = true;
}

static void se_navigation_field_sanitize_sample(const se_navigation_sample* default_sample, se_navigation_sample* sample) {
	if (!default_sample || !sample) {
		return;
	}

	if (!isfinite(sample->world.x)) {
		sample->world.x = default_sample->world.x;
	}
	if (!isfinite(sample->world.z)) {
		sample->world.z = default_sample->world.z;
	}

	f32 height = sample->height;
	f32 world_y = sample->world.y;
	if (!isfinite(height)) {
		height = default_sample->height;
	}
	if (!isfinite(world_y)) {
		world_y = default_sample->world.y;
	}

	const b8 changed_height = !s_f32_equal(height, default_sample->height, S_EPSILON);
	const b8 changed_world_y = !s_f32_equal(world_y, default_sample->world.y, S_EPSILON);
	sample->world.y = world_y;
	sample->height = height;
	if (changed_height && !changed_world_y) {
		sample->world.y = sample->height;
	} else {
		sample->height = sample->world.y;
	}

	const f32 normal_length = s_vec3_length(&sample->normal);
	if (!isfinite(normal_length) || normal_length <= S_EPSILON) {
		sample->normal = default_sample->normal;
	} else {
		sample->normal = s_vec3_normalize(&sample->normal);
	}

	if (!isfinite(sample->cost) || sample->cost <= 0.0f) {
		sample->cost = 1.0f;
	}
}

static const se_navigation_sample* se_navigation_field_sample_ptr_const(const se_navigation_field* field, const se_navigation_cell cell) {
	if (!field || !se_navigation_field_is_valid_cell(field, cell)) {
		return NULL;
	}
	return s_array_get((se_navigation_samples*)&field->samples, s_array_handle((se_navigation_samples*)&field->samples, (u32)se_navigation_field_cell_index(field, cell)));
}

static se_navigation_sample* se_navigation_field_sample_ptr(se_navigation_field* field, const se_navigation_cell cell) {
	if (!field || !se_navigation_field_is_valid_cell(field, cell)) {
		return NULL;
	}
	return s_array_get(&field->samples, s_array_handle(&field->samples, (u32)se_navigation_field_cell_index(field, cell)));
}

static f32 se_navigation_field_min_cost(const se_navigation_field* field) {
	if (!field) {
		return 1.0f;
	}
	f32 min_cost = FLT_MAX;
	for (sz i = 0; i < s_array_get_size(&field->samples); ++i) {
		const se_navigation_sample* sample = s_array_get((se_navigation_samples*)&field->samples, s_array_handle((se_navigation_samples*)&field->samples, (u32)i));
		if (!sample || !sample->walkable) {
			continue;
		}
		const f32 cost = (!isfinite(sample->cost) || sample->cost <= 0.0f) ? 1.0f : sample->cost;
		if (cost < min_cost) {
			min_cost = cost;
		}
	}
	return min_cost == FLT_MAX ? 1.0f : min_cost;
}

static void se_navigation_field_recompute_min_cost(se_navigation_field* field) {
	if (!field) {
		return;
	}
	field->min_cost = se_navigation_field_min_cost(field);
}

static b8 se_navigation_field_sync_storage(se_navigation_field* field) {
	if (!field) {
		return false;
	}
	const sz expected_count = (sz)(field->width * field->height);
	if (s_array_get_size(&field->samples) == expected_count) {
		return true;
	}
	s_array_clear(&field->samples);
	s_array_init(&field->samples);
	s_array_reserve(&field->samples, expected_count);
	for (i32 y = 0; y < field->height; ++y) {
		for (i32 x = 0; x < field->width; ++x) {
			se_navigation_sample sample = {0};
			se_navigation_field_make_default_sample(field, (se_navigation_cell){x, y}, &sample);
			s_array_add(&field->samples, sample);
		}
	}
	return true;
}

static b8 se_navigation_field_is_walkable(const se_navigation_field* field, const se_navigation_cell cell) {
	const se_navigation_sample* sample = se_navigation_field_sample_ptr_const(field, cell);
	return sample ? sample->walkable : false;
}

static f32 se_navigation_field_step_cost(const se_navigation_field* field, const se_navigation_cell from, const se_navigation_cell to) {
	const se_navigation_sample* sample = se_navigation_field_sample_ptr_const(field, to);
	const f32 weight = (sample && isfinite(sample->cost) && sample->cost > 0.0f) ? sample->cost : 1.0f;
	return se_navigation_step_distance(from, to) * weight;
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

void se_navigation_grid_set_dynamic_cell(se_navigation_grid* grid, const se_navigation_cell cell, const b8 blocked) {
	se_navigation_grid_set_blocked(grid, cell, blocked);
}

void se_navigation_grid_set_dynamic_rect(se_navigation_grid* grid, const se_navigation_cell min_cell, const se_navigation_cell max_cell, const b8 blocked) {
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

b8 se_navigation_field_create(se_navigation_field* out_field, const i32 width, const i32 height, const f32 cell_size, const s_vec3* origin) {
	if (!SE_VALIDATE(out_field && origin && width > 0 && height > 0 && cell_size > 0.0f, SE_RESULT_INVALID_ARGUMENT)) {
		return false;
	}
	memset(out_field, 0, sizeof(*out_field));
	out_field->width = width;
	out_field->height = height;
	out_field->cell_size = cell_size;
	out_field->origin = *origin;
	out_field->min_cost = 1.0f;
	s_array_init(&out_field->samples);
	se_navigation_field_sync_storage(out_field);
	se_navigation_field_recompute_min_cost(out_field);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_navigation_field_destroy(se_navigation_field* field) {
	if (!field) {
		return;
	}
	s_array_clear(&field->samples);
	memset(field, 0, sizeof(*field));
}

b8 se_navigation_field_build(se_navigation_field* field, se_navigation_sampler sampler, void* user_data) {
	if (!SE_VALIDATE(field && field->width > 0 && field->height > 0 && field->cell_size > 0.0f, SE_RESULT_INVALID_ARGUMENT)) {
		return false;
	}
	se_navigation_field_sync_storage(field);
	for (i32 y = 0; y < field->height; ++y) {
		for (i32 x = 0; x < field->width; ++x) {
			const se_navigation_cell cell = {x, y};
			se_navigation_sample sample = {0};
			se_navigation_sample defaults = {0};
			se_navigation_field_make_default_sample(field, cell, &sample);
			defaults = sample;
			if (sampler && !sampler(field, cell, &sample, user_data)) {
				if (se_get_last_error() == SE_RESULT_OK) {
					se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				}
				return false;
			}
			se_navigation_field_sanitize_sample(&defaults, &sample);
			se_navigation_sample* slot = se_navigation_field_sample_ptr(field, cell);
			if (!slot) {
				se_set_last_error(SE_RESULT_NOT_FOUND);
				return false;
			}
			*slot = sample;
		}
	}
	se_navigation_field_recompute_min_cost(field);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_navigation_field_is_valid_cell(const se_navigation_field* field, const se_navigation_cell cell) {
	if (!field) {
		return false;
	}
	return cell.x >= 0 && cell.y >= 0 && cell.x < field->width && cell.y < field->height;
}

b8 se_navigation_field_get_sample(const se_navigation_field* field, const se_navigation_cell cell, se_navigation_sample* out_sample) {
	if (!field || !out_sample || !se_navigation_field_is_valid_cell(field, cell)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const se_navigation_sample* sample = se_navigation_field_sample_ptr_const(field, cell);
	if (!sample) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_sample = *sample;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_navigation_field_set_sample(se_navigation_field* field, const se_navigation_cell cell, const se_navigation_sample* sample) {
	if (!field || !sample || !se_navigation_field_is_valid_cell(field, cell)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_navigation_sample normalized = *sample;
	se_navigation_sample defaults = {0};
	se_navigation_field_make_default_sample(field, cell, &defaults);
	se_navigation_field_sanitize_sample(&defaults, &normalized);
	se_navigation_sample* slot = se_navigation_field_sample_ptr(field, cell);
	if (!slot) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*slot = normalized;
	se_navigation_field_recompute_min_cost(field);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_navigation_field_world_to_cell(const se_navigation_field* field, const s_vec3* world, se_navigation_cell* out_cell) {
	if (!field || !world || !out_cell || field->cell_size <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const f32 rel_x = world->x - field->origin.x;
	const f32 rel_z = world->z - field->origin.z;
	se_navigation_cell cell = {0};
	cell.x = (i32)floorf(rel_x / field->cell_size);
	cell.y = (i32)floorf(rel_z / field->cell_size);
	if (!se_navigation_field_is_valid_cell(field, cell)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_cell = cell;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_navigation_field_cell_to_world(const se_navigation_field* field, const se_navigation_cell cell, s_vec3* out_world) {
	if (!field || !out_world || !se_navigation_field_is_valid_cell(field, cell)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const se_navigation_sample* sample = se_navigation_field_sample_ptr_const(field, cell);
	if (!sample) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_world = sample->world;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_navigation_area_init(se_navigation_area* area, const sz reserve_count) {
	if (!area) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	memset(area, 0, sizeof(*area));
	s_array_init(&area->cells);
	if (reserve_count > 0) {
		s_array_reserve(&area->cells, reserve_count);
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_navigation_area_clear(se_navigation_area* area) {
	if (!area) {
		return;
	}
	s_array_clear(&area->cells);
	memset(area, 0, sizeof(*area));
}

void se_navigation_area_reset(se_navigation_area* area) {
	if (!area) {
		return;
	}
	while (s_array_get_size(&area->cells) > 0) {
		s_array_remove(&area->cells, s_array_handle(&area->cells, (u32)(s_array_get_size(&area->cells) - 1)));
	}
}

sz se_navigation_field_get_neighbors(const se_navigation_field* field, const se_navigation_cell cell, const b8 allow_diagonal, se_navigation_cell out_neighbors[8]) {
	if (!field || !out_neighbors) {
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
		const se_navigation_cell neighbor = {cell.x + k_cardinal[i].x, cell.y + k_cardinal[i].y};
		if (se_navigation_field_is_valid_cell(field, neighbor) && se_navigation_field_is_walkable(field, neighbor)) {
			out_neighbors[count++] = neighbor;
		}
	}
	if (allow_diagonal) {
		for (sz i = 0; i < 4; ++i) {
			const se_navigation_cell neighbor = {cell.x + k_diagonal[i].x, cell.y + k_diagonal[i].y};
			if (se_navigation_field_is_valid_cell(field, neighbor) && se_navigation_field_is_walkable(field, neighbor)) {
				out_neighbors[count++] = neighbor;
			}
		}
	}
	return count;
}

b8 se_navigation_field_find_path(const se_navigation_field* field, const se_navigation_cell start, const se_navigation_cell goal, const b8 allow_diagonal, se_navigation_path* out_path) {
	se_debug_trace_begin("navigation_field_astar");
	if (!SE_VALIDATE(field && out_path && se_navigation_field_is_valid_cell(field, start) && se_navigation_field_is_valid_cell(field, goal), SE_RESULT_INVALID_ARGUMENT)) {
		se_debug_trace_end("navigation_field_astar");
		return false;
	}
	if (!se_navigation_field_is_walkable(field, start) || !se_navigation_field_is_walkable(field, goal)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		se_debug_trace_end("navigation_field_astar");
		return false;
	}
	if (s_array_get_capacity(&out_path->cells) == 0) {
		s_array_init(&out_path->cells);
		s_array_reserve(&out_path->cells, (sz)(field->width * field->height));
	}
	se_navigation_path_reset(out_path);

	const i32 node_count = field->width * field->height;
	f32* g_score = malloc(sizeof(f32) * (sz)node_count);
	f32* f_score = malloc(sizeof(f32) * (sz)node_count);
	i32* came_from = malloc(sizeof(i32) * (sz)node_count);
	u8* is_open = malloc(sizeof(u8) * (sz)node_count);
	u8* is_closed = malloc(sizeof(u8) * (sz)node_count);
	if (!g_score || !f_score || !came_from || !is_open || !is_closed) {
		free(g_score);
		free(f_score);
		free(came_from);
		free(is_open);
		free(is_closed);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		se_debug_trace_end("navigation_field_astar");
		return false;
	}

	for (i32 i = 0; i < node_count; ++i) {
		g_score[i] = FLT_MAX;
		f_score[i] = FLT_MAX;
		came_from[i] = -1;
		is_open[i] = 0u;
		is_closed[i] = 0u;
	}

	const f32 heuristic_scale = se_navigation_field_min_cost(field);
	const i32 start_idx = se_navigation_field_cell_index(field, start);
	const i32 goal_idx = se_navigation_field_cell_index(field, goal);
	g_score[start_idx] = 0.0f;
	f_score[start_idx] = se_navigation_heuristic(start, goal, allow_diagonal) * heuristic_scale;
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
		const se_navigation_cell current_cell = se_navigation_field_index_cell(field, current);
		se_navigation_cell neighbors[8] = {0};
		const sz neighbor_count = se_navigation_field_get_neighbors(field, current_cell, allow_diagonal, neighbors);
		for (sz i = 0; i < neighbor_count; ++i) {
			const se_navigation_cell neighbor = neighbors[i];
			const i32 neighbor_idx = se_navigation_field_cell_index(field, neighbor);
			if (is_closed[neighbor_idx]) {
				continue;
			}
			const f32 tentative_g = g_score[current] + se_navigation_field_step_cost(field, current_cell, neighbor);
			if (!is_open[neighbor_idx] || tentative_g < g_score[neighbor_idx]) {
				came_from[neighbor_idx] = current;
				g_score[neighbor_idx] = tentative_g;
				f_score[neighbor_idx] = tentative_g + se_navigation_heuristic(neighbor, goal, allow_diagonal) * heuristic_scale;
				is_open[neighbor_idx] = 1u;
			}
		}
	}

	if (found) {
		i32 current = goal_idx;
		while (current >= 0) {
			const se_navigation_cell cell = se_navigation_field_index_cell(field, current);
			s_array_add(&out_path->cells, cell);
			current = came_from[current];
		}
		const sz count = s_array_get_size(&out_path->cells);
		for (sz i = 0; i < count / 2; ++i) {
			se_navigation_cell* a = s_array_get(&out_path->cells, s_array_handle(&out_path->cells, (u32)i));
			se_navigation_cell* b = s_array_get(&out_path->cells, s_array_handle(&out_path->cells, (u32)(count - i - 1)));
			const se_navigation_cell tmp = *a;
			*a = *b;
			*b = tmp;
		}
	}

	free(g_score);
	free(f_score);
	free(came_from);
	free(is_open);
	free(is_closed);

	se_set_last_error(found ? SE_RESULT_OK : SE_RESULT_NOT_FOUND);
	se_debug_log(
		found ? SE_DEBUG_LEVEL_DEBUG : SE_DEBUG_LEVEL_INFO,
		SE_DEBUG_CATEGORY_NAVIGATION,
		"se_navigation_field_find_path :: %s start=(%d,%d) goal=(%d,%d) nodes=%zu",
		found ? "found" : "failed",
		start.x,
		start.y,
		goal.x,
		goal.y,
		s_array_get_size(&out_path->cells));
	se_debug_trace_end("navigation_field_astar");
	return found;
}

b8 se_navigation_field_find_reachable(const se_navigation_field* field, const se_navigation_cell start, const f32 max_cost, const b8 allow_diagonal, se_navigation_area* out_area) {
	se_debug_trace_begin("navigation_field_reachable");
	if (!SE_VALIDATE(field && out_area && max_cost >= 0.0f && se_navigation_field_is_valid_cell(field, start), SE_RESULT_INVALID_ARGUMENT)) {
		se_debug_trace_end("navigation_field_reachable");
		return false;
	}
	if (!se_navigation_field_is_walkable(field, start)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		se_debug_trace_end("navigation_field_reachable");
		return false;
	}
	if (s_array_get_capacity(&out_area->cells) == 0) {
		s_array_init(&out_area->cells);
		s_array_reserve(&out_area->cells, (sz)(field->width * field->height));
	}
	se_navigation_area_reset(out_area);

	const i32 node_count = field->width * field->height;
	f32* best_cost = malloc(sizeof(f32) * (sz)node_count);
	u8* is_open = malloc(sizeof(u8) * (sz)node_count);
	u8* is_closed = malloc(sizeof(u8) * (sz)node_count);
	if (!best_cost || !is_open || !is_closed) {
		free(best_cost);
		free(is_open);
		free(is_closed);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		se_debug_trace_end("navigation_field_reachable");
		return false;
	}

	for (i32 i = 0; i < node_count; ++i) {
		best_cost[i] = FLT_MAX;
		is_open[i] = 0u;
		is_closed[i] = 0u;
	}

	const i32 start_idx = se_navigation_field_cell_index(field, start);
	best_cost[start_idx] = 0.0f;
	is_open[start_idx] = 1u;

	while (true) {
		i32 current = -1;
		f32 current_cost = FLT_MAX;
		for (i32 i = 0; i < node_count; ++i) {
			if (!is_open[i]) {
				continue;
			}
			if (best_cost[i] < current_cost) {
				current = i;
				current_cost = best_cost[i];
			}
		}
		if (current < 0 || current_cost > max_cost) {
			break;
		}

		is_open[current] = 0u;
		if (is_closed[current]) {
			continue;
		}
		is_closed[current] = 1u;

		const se_navigation_cell current_cell = se_navigation_field_index_cell(field, current);
		s_array_add(&out_area->cells, ((se_navigation_area_cell){
			.cell = current_cell,
			.cost = current_cost
		}));

		se_navigation_cell neighbors[8] = {0};
		const sz neighbor_count = se_navigation_field_get_neighbors(field, current_cell, allow_diagonal, neighbors);
		for (sz i = 0; i < neighbor_count; ++i) {
			const se_navigation_cell neighbor = neighbors[i];
			const i32 neighbor_idx = se_navigation_field_cell_index(field, neighbor);
			if (is_closed[neighbor_idx]) {
				continue;
			}
			const f32 tentative_cost = current_cost + se_navigation_field_step_cost(field, current_cell, neighbor);
			if (tentative_cost > max_cost) {
				continue;
			}
			if (!is_open[neighbor_idx] || tentative_cost < best_cost[neighbor_idx]) {
				best_cost[neighbor_idx] = tentative_cost;
				is_open[neighbor_idx] = 1u;
			}
		}
	}

	free(best_cost);
	free(is_open);
	free(is_closed);

	se_set_last_error(SE_RESULT_OK);
	se_debug_trace_end("navigation_field_reachable");
	return true;
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
		"se_navigation_find_path :: A* %s start=(%d,%d) goal=(%d,%d) nodes=%zu",
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
