#include "se_navigation.h"

int main(void) {
	se_navigation_grid grid = {0};
	s_vec2 origin = s_vec2(0.0f, 0.0f);
	se_navigation_grid_create(&grid, 32, 32, 1.0f, &origin);
	se_navigation_cell start = {0, 0};
	se_navigation_cell goal = {15, 20};
	se_navigation_path path = {0};
	se_navigation_path_init(&path, 64);
	se_navigation_find_path_simple(&grid, start, goal, 1, &path);
	return 0;
}
