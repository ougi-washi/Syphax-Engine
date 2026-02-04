// Syphax-Engine - Ougi Washi

#ifndef SE_MATH_H
#define SE_MATH_H

#include "syphax/s_types.h"

#define PI 3.14159265359

typedef struct { f32 x, y;} se_vec2;
typedef struct { f32 x, y, z; } se_vec3;
typedef struct { f32 x, y, z, w; } se_vec4;
typedef struct { f32 m[9]; } se_mat3;
typedef struct { f32 m[16]; } se_mat4;

// 2D
typedef struct { se_vec2 min, max; } se_box_2d;
typedef struct { se_vec2 position; f32 radius; } se_circle;

// 3D
typedef struct { se_vec3 min, max; } se_box_3d;
typedef struct { se_vec3 position; f32 radius; } se_sphere;

#define se_vec(_vec_size, ...) (se_vec##_vec_size) { __VA_ARGS__ }
#define se_vec2(_x, _y) (se_vec(2, _x, _y))
#define se_vec3(_x, _y, _z) (se_vec(3, _x, _y, _z))
#define se_vec4(_x, _y, _z, _w) (se_vec(4, _x, _y, _z, _w))

#define PI 3.14159265359
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// Math (TODO: Change all args to ptrs for performance)
extern f32 vec3_length(se_vec3 v);

// se_mat3
extern se_mat3 se_mat3_identity();
extern se_mat3 se_mat3_mul(const se_mat3* a, const se_mat3* b);
extern se_mat3 se_mat3_translation(const se_vec2 *v);
extern se_mat3 se_mat3_rotation(f32 angle);
extern se_mat3 se_mat3_scale(const se_vec2 *v);
extern void se_mat3_set_position(se_mat3 *mat, const se_vec2 *new_pos);
extern void se_mat3_get_position(const se_mat3 *mat, se_vec2 *out_pos);
extern void se_mat3_set_rotation(se_mat3 *mat, const se_vec2 *new_rot);
extern void se_mat3_get_rotation(const se_mat3 *mat, se_vec2 *out_rot);
extern void se_mat3_set_scale(se_mat3 *mat, const se_vec2 *new_scale);
extern void se_mat3_get_scale(const se_mat3 *mat, se_vec2 *out_scale);

// se_mat4
extern se_mat4 se_mat4_identity();
extern se_mat4 mat4_perspective(float fov_y, float aspect, float near, float far);
extern se_mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far);
extern se_vec3 vec3_sub(se_vec3 a, se_vec3 b);
extern se_vec3 vec3_norm(se_vec3 v);
extern se_vec3 vec3_cross(se_vec3 a, se_vec3 b);
extern se_mat4 mat4_look_at(se_vec3 eye, se_vec3 center, se_vec3 up);
extern se_mat4 mat4_mul(const se_mat4 A, const se_mat4 B);
extern se_mat4 mat4_translate(const se_vec3 *v);
extern se_mat4 mat4_rotate_x(se_mat4 m, f32 angle);
extern se_mat4 mat4_rotate_y(se_mat4 m, f32 angle);
extern se_mat4 mat4_rotate_z(se_mat4 m, f32 angle);
extern se_mat4 mat4_scale(const se_vec3 *v);

extern void se_box_2d_make(se_box_2d *out_box, const se_mat3 *transform);
extern b8 se_box_2d_is_inside(const se_box_2d *a, const se_vec2 *p);
extern b8 se_box_2d_intersects(const se_box_2d *a, const se_box_2d *b);
extern b8 se_box_3d_intersects(const se_box_3d *a, const se_box_3d *b);
extern b8 se_circle_intersects(const se_circle *a, const se_circle *b);
extern b8 se_sphere_intersects(const se_sphere *a, const se_sphere *b);

#endif // SE_MATH_H
