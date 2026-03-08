// Syphax-Engine - Ougi Washi

#include "se_navigation.h"

#include <math.h>
#include <stdio.h>

static b8 navigation_terrain_sampler(const se_navigation_field* field, const se_navigation_cell cell, se_navigation_sample* sample, void* user_data) {
	(void)user_data;
	const f32 fx = field->width > 1 ? (f32)cell.x / (f32)(field->width - 1) : 0.0f;
	const f32 fy = field->height > 1 ? (f32)cell.y / (f32)(field->height - 1) : 0.0f;
	const f32 terrain_height = sinf(fx * PI * 2.0f) * 1.1f + cosf(fy * PI * 1.5f) * 0.6f;
	const f32 slope_x = cosf(fx * PI * 2.0f) * 0.35f;
	const f32 slope_z = -sinf(fy * PI * 1.5f) * 0.25f;
	s_vec3 normal = s_vec3(-slope_x, 1.0f, -slope_z);

	sample->world.y = terrain_height;
	sample->height = terrain_height;
	sample->normal = s_vec3_normalize(&normal);
	sample->cost = 1.0f + fabsf(terrain_height) * 0.45f + fabsf(slope_x + slope_z) * 0.2f;
	sample->walkable = terrain_height < 1.25f;
	if (cell.x >= 13 && cell.x <= 18 && cell.y >= 11 && cell.y <= 17) {
		sample->walkable = false;
	}
	return true;
}

int main(void) {
	printf("advanced/navigation_grid :: Advanced example (reference)\n");
	se_navigation_grid grid = {0};
	se_navigation_path path = {0};
	se_navigation_field field = {0};
	se_navigation_path field_path = {0};
	se_navigation_path ray = {0};
	se_navigation_area area = {0};

	if (!se_navigation_grid_create(&grid, 32, 32, 1.0f, &s_vec2(0.0f, 0.0f))) {
		printf("17_navigation :: grid create failed\n");
		return 1;
	}
	if (!se_navigation_field_create(&field, 32, 32, 1.0f, &s_vec3(0.0f, 0.0f, 0.0f))) {
		printf("17_navigation :: field create failed\n");
		se_navigation_grid_destroy(&grid);
		return 1;
	}
	se_navigation_path_init(&path, 128);
	se_navigation_path_init(&field_path, 128);
	se_navigation_path_init(&ray, 64);
	se_navigation_area_init(&area, 256);

	se_navigation_grid_set_blocked_rect(&grid, (se_navigation_cell){10, 8}, (se_navigation_cell){18, 22}, true);
	se_navigation_grid_set_blocked_circle(&grid, (se_navigation_cell){22, 10}, 3, true);
	se_navigation_grid_set_dynamic_cell(&grid, (se_navigation_cell){9, 9}, true);
	se_navigation_grid_set_dynamic_rect(&grid, (se_navigation_cell){4, 14}, (se_navigation_cell){6, 18}, true);
	if (!se_navigation_field_build(&field, navigation_terrain_sampler, NULL)) {
		printf("17_navigation :: field build failed\n");
		se_navigation_area_clear(&area);
		se_navigation_path_clear(&ray);
		se_navigation_path_clear(&field_path);
		se_navigation_path_clear(&path);
		se_navigation_field_destroy(&field);
		se_navigation_grid_destroy(&grid);
		return 1;
	}

	const se_navigation_cell start = {2, 2};
	const se_navigation_cell goal = {29, 28};
	const b8 found = se_navigation_find_path_simple(&grid, start, goal, true, &path);
	if (found) {
		printf("17_navigation :: path found, raw nodes=%zu\n", s_array_get_size(&path.cells));
		se_navigation_smooth_path(&grid, &path);
		printf("17_navigation :: smoothed nodes=%zu\n", s_array_get_size(&path.cells));
	} else {
		printf("17_navigation :: path not found\n");
	}

	const b8 clear_los = se_navigation_raycast_cells(&grid, start, goal, &ray);
	printf("17_navigation :: raycast clear=%d traversed=%zu\n", clear_los ? 1 : 0, s_array_get_size(&ray.cells));

	const se_navigation_cell field_start = {2, 2};
	const se_navigation_cell field_goal = {27, 25};
	const b8 field_found = se_navigation_field_find_path(&field, field_start, field_goal, true, &field_path);
	const b8 reachable = se_navigation_field_find_reachable(&field, field_start, 18.0f, true, &area);
	se_navigation_sample goal_sample = {0};
	s_vec3 goal_world = s_vec3(0.0f, 0.0f, 0.0f);
	(void)se_navigation_field_get_sample(&field, field_goal, &goal_sample);
	(void)se_navigation_field_cell_to_world(&field, field_goal, &goal_world);
	printf(
		"17_navigation :: field path=%d nodes=%zu reachable=%d cells=%zu goal_y=%.2f goal_cost=%.2f world=(%.2f, %.2f, %.2f)\n",
		field_found ? 1 : 0,
		s_array_get_size(&field_path.cells),
		reachable ? 1 : 0,
		s_array_get_size(&area.cells),
		goal_sample.height,
		goal_sample.cost,
		goal_world.x,
		goal_world.y,
		goal_world.z);

	se_navigation_area_clear(&area);
	se_navigation_path_clear(&ray);
	se_navigation_path_clear(&field_path);
	se_navigation_path_clear(&path);
	se_navigation_field_destroy(&field);
	se_navigation_grid_destroy(&grid);
	return (found && field_found && reachable) ? 0 : 2;
}
