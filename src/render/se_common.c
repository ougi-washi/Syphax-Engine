// Syphax-Engine - Ougi Washi

#include "se_render.h"
#include <stdio.h>

void se_render_print_mat4(const s_mat4 *mat) {
	s_assertf(mat, "se_render_print_mat4 :: mat is null");
	for (i32 row = 0; row < 4; ++row) {
		printf("[%f, %f, %f, %f]\n",
			mat->m[row][0],
			mat->m[row][1],
			mat->m[row][2],
			mat->m[row][3]);
	}
}
