#include "se_navigation.h"

int main(void) {
	se_navigation_grid grid = {0};
	s_vec2 origin = s_vec2(0.0f, 0.0f);
	se_navigation_grid_create(&grid, 32, 32, 1.0f, &origin);
	return 0;
}
