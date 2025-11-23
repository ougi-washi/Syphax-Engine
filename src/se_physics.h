// Syphax-Engine - Ougi Washi

#ifndef SE_PHYSICS_H
#define SE_PHYSICS_H
#include "se_math.h"

typedef struct {
    se_vec2 position;
    se_vec2 size;
} se_box_2d;

typedef struct {
    se_vec3 position;
    se_vec3 size;
} se_box_3d;

typedef struct {
    se_vec3 position;
    f32 radius;
} se_sphere;

extern b8 se_box_2d_intersects(const se_box_2d* a, const se_box_2d* b);
extern b8 se_box_3d_intersects(const se_box_3d* a, const se_box_3d* b);
extern b8 se_sphere_intersects(const se_sphere* a, const se_sphere* b);

#endif // SE_PHYSICS_H
