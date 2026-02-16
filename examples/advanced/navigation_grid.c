// Syphax-Engine - Ougi Washi

#include "se_navigation.h"

#include <stdio.h>

int main(void) {
	printf("advanced/navigation_grid :: Advanced example (reference)\n");
	se_navigation_grid grid = {0};
	se_navigation_path path = {0};
	se_navigation_path ray = {0};

	if (!se_navigation_grid_create(&grid, 32, 32, 1.0f, &s_vec2(0.0f, 0.0f))) {
		printf("17_navigation :: grid create failed\n");
		return 1;
	}
	se_navigation_path_init(&path, 128);
	se_navigation_path_init(&ray, 64);

	se_navigation_grid_set_blocked_rect(&grid, (se_navigation_cell){10, 8}, (se_navigation_cell){18, 22}, true);
	se_navigation_grid_set_blocked_circle(&grid, (se_navigation_cell){22, 10}, 3, true);
	se_navigation_grid_set_dynamic_obstacle(&grid, (se_navigation_cell){9, 9}, true);
	se_navigation_grid_set_dynamic_obstacle_rect(&grid, (se_navigation_cell){4, 14}, (se_navigation_cell){6, 18}, true);

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

	se_navigation_path_clear(&ray);
	se_navigation_path_clear(&path);
	se_navigation_grid_destroy(&grid);
	return found ? 0 : 2;
}
