// Syphax-Engine - Ougi Washi

#ifndef SE_MATH_H
#define SE_MATH_H

#include "syphax/s_types.h"

// 2D
typedef struct { s_vec2 min, max; } se_box_2d;
typedef struct { s_vec2 position; f32 radius; } se_circle;

// 3D
typedef struct { s_vec3 min, max; } se_box_3d;
typedef struct { s_vec3 position; f32 radius; } se_sphere;

extern void se_box_2d_make(se_box_2d *out_box, const s_mat3 *transform);
extern b8 se_box_2d_is_inside(const se_box_2d *a, const s_vec2 *p);
extern b8 se_box_2d_intersects(const se_box_2d *a, const se_box_2d *b);
extern b8 se_box_3d_intersects(const se_box_3d *a, const se_box_3d *b);
extern b8 se_circle_intersects(const se_circle *a, const se_circle *b);
extern b8 se_sphere_intersects(const se_sphere *a, const se_sphere *b);

#endif // SE_MATH_H
