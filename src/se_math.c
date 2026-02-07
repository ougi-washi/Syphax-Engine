// Syphax-Engine - Ougi Washi

#include "se_math.h"
#include <math.h>

void se_box_2d_make(se_box_2d *out_box, const s_mat3 *transform) {
    s_vec2 pos = s_mat3_get_translation(transform);
    s_vec2 scale = s_mat3_get_scale(transform);
	out_box->min = s_vec2(pos.x - scale.x, -pos.y - scale.y);
	out_box->max = s_vec2(pos.x + scale.x, -pos.y + scale.y);
}

b8 se_box_2d_is_inside(const se_box_2d* a, const s_vec2* p) {
	return p->x >= a->min.x && p->x <= a->max.x &&
		p->y >= a->min.y && p->y <= a->max.y;
}

b8 se_box_2d_intersects(const se_box_2d* a, const se_box_2d* b) {
	return a->min.x <= b->max.x && a->max.x >= b->min.x &&
		a->min.y <= b->max.y && a->max.y >= b->min.y;
}

b8 se_box_3d_intersects(const se_box_3d* a, const se_box_3d* b) {
	return a->min.x <= b->max.x && a->max.x >= b->min.x &&
		a->min.y <= b->max.y && a->max.y >= b->min.y &&
		a->min.z <= b->max.z && a->max.z >= b->min.z;
}

b8 se_circle_intersects(const se_circle* a, const se_circle* b) {
	return fabs(a->position.x - b->position.x) <= a->radius + b->radius ||
		fabs(a->position.y - b->position.y) <= a->radius + b->radius;
}

b8 se_sphere_intersects(const se_sphere* a, const se_sphere* b) {
	return fabs(a->position.x - b->position.x) <= a->radius + b->radius ||
		fabs(a->position.y - b->position.y) <= a->radius + b->radius ||
		fabs(a->position.z - b->position.z) <= a->radius + b->radius;
}
