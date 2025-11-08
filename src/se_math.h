// Syphax-Engine - Ougi Washi

#ifndef SE_MATH_H
#define SE_MATH_H

#include "syphax/s_types.h"

typedef struct { f32 x, y; } se_vec2;
typedef struct { f32 x, y, z; } se_vec3;
typedef struct { f32 x, y, z, w; } se_vec4;
typedef struct { f32 m[16]; } se_mat4;

#define se_vec(_vec_size, ...) ( se_vec##_vec_size ) { __VA_ARGS__ }

#define PI 3.14159265359

static se_mat4 mat4_identity(void) {
    se_mat4 I = { {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    } };
    return I;
}

// Math (TODO: Change all args to ptrs for performance)
#define PI 3.14159265359
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
f32 vec3_length(se_vec3 v);
se_mat4 mat4_perspective(float fov_y, float aspect, float near, float far);
se_vec3 vec3_sub(se_vec3 a, se_vec3 b);
se_vec3 vec3_norm(se_vec3 v);
se_vec3 vec3_cross(se_vec3 a, se_vec3 b);
se_mat4 mat4_look_at(se_vec3 eye, se_vec3 center, se_vec3 up);
se_mat4 mat4_mul(const se_mat4 A, const se_mat4 B);
se_mat4 mat4_translate(const se_vec3* v); 
se_mat4 mat4_rotate_x(se_mat4 m, f32 angle);
se_mat4 mat4_rotate_y(se_mat4 m, f32 angle);
se_mat4 mat4_rotate_z(se_mat4 m, f32 angle);
se_mat4 mat4_scale(const se_vec3* v);

#endif // SE_MATH_H
