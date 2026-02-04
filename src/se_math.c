// Syphax-Engine - Ougi Washi

#include "se_math.h"
#include <math.h>

f32 vec3_length(se_vec3 v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

se_mat3 se_mat3_identity() {
	return (se_mat3)
    {1, 0, 0,
     0, 1, 0,
     0, 0, 1};
}

se_mat3 se_mat3_mul(const se_mat3* a, const se_mat3* b) {
    se_mat3 res;
    for(int row=0 ; row<3 ; row++){
        for(int col=0 ; col<3 ; col++){
            float sum = 0.0f;
            for(int k=0 ; k<3 ; k++){
                sum += a->m[row + 3*k] * b->m[k + 3*col];
            }
            res.m[row + 3*col] = sum;
        }
    }
    return res;
}

se_mat3 se_mat3_translate(const se_vec2 *v) {
	se_mat3 T = se_mat3_identity();
    T.m[2] = v->x;
    T.m[5] = v->y;
    return T;
}

se_mat3 se_mat3_rotate(f32 angle) {
	se_mat3 R = se_mat3_identity();
	R.m[0] =	cos(angle);
	R.m[1] =	-sin(angle);
	R.m[4] =	sin(angle);
	R.m[5] =	cos(angle);
    return R;
}

se_mat3 se_mat3_scale(const se_vec2 *v) {
	se_mat3 S = se_mat3_identity();
	S.m[0] = v->x;
	S.m[4] = v->y;
	return S;
}

void se_mat3_set_position(se_mat3 *mat, const se_vec2 *new_pos) {
    mat->m[6] = new_pos->x;
    mat->m[7] = new_pos->y;
}

void se_mat3_get_position(se_mat3 *mat, se_vec2 *out_pos) {
    out_pos->x = mat->m[6];
    out_pos->y = mat->m[7];
}

void se_mat3_set_rotation(se_mat3 *mat, const se_vec2 *new_rot) {
    mat->m[0] = cos(new_rot->x);
    mat->m[1] = -sin(new_rot->x);
    mat->m[4] = sin(new_rot->y);
    mat->m[5] = cos(new_rot->y);
}

void se_mat3_get_rotation(se_mat3 *mat, se_vec2 *out_rot) {
    // TODO: Verify implementation. Also is this the most efficient way?
    out_rot->x = atan2(mat->m[1], mat->m[0]);
    out_rot->y = atan2(mat->m[5], mat->m[4]);
}

void se_mat3_set_scale(se_mat3 *mat, const se_vec2 *new_scale) {
    mat->m[0] = new_scale->x;
    mat->m[5] = new_scale->y;
}

void se_mat3_get_scale(se_mat3 *mat, se_vec2 *out_scale) {
    out_scale->x = mat->m[0];
    out_scale->y = mat->m[5];
}

se_mat4 se_mat4_identity() {
	return (se_mat4)
    {1, 0, 0, 0,
     0, 1, 0, 0,
     0, 0, 1, 0,
     0, 0, 0, 1};
}

se_mat4 mat4_perspective(float fov_y, float aspect, float near, float far) {
	float f = 1.0f / tanf(fov_y * 0.5f);
	se_mat4 P = { {
		f/aspect, 0, 0,							0,
		0,		f, 0,							0,
		0,		0, (far+near)/(near-far),		 -1,
		0,		0, (2*far*near)/(near-far),		0
	} };
	return P;
}

se_mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far) {
	float rl = right - left;
	float tb = top - bottom;
	float fn = far - near;
	se_mat4 P = { {
		2/rl, 0, 0, left - right,
		0, 2/tb, 0, bottom - top,
		0, 0, -2/fn, near - far,
		0, 0, 0, 1
	} };
	return P;
}

se_vec3 vec3_sub(se_vec3 a, se_vec3 b){
	return (se_vec3){ a.x-b.x, a.y-b.y, a.z-b.z };
}

se_vec3 vec3_norm(se_vec3 v){
	const f32 len = vec3_length(v);
	return (se_vec3){ v.x/len, v.y/len, v.z/len };
}

se_vec3 vec3_cross(se_vec3 a, se_vec3 b){
	return (se_vec3){
		a.y*b.z - a.z*b.y,
		a.z*b.x - a.x*b.z,
		a.x*b.y - a.y*b.x
	};
}

// eye = camera pos, center = look at point, up = world up (e.g. {0,1,0})
se_mat4 mat4_look_at(se_vec3 eye, se_vec3 center, se_vec3 up) {
	se_vec3 f = vec3_norm(vec3_sub(center, eye));
	se_vec3 s = vec3_norm(vec3_cross(f, up));
	se_vec3 u = vec3_cross(s, f);

	se_mat4 M = se_mat4_identity();
	// rotation
	M.m[0] =	s.x; M.m[1] =	u.x; M.m[2] = -f.x;
	M.m[4] =	s.y; M.m[5] =	u.y; M.m[6] = -f.y;
	M.m[8] =	s.z; M.m[9] =	u.z; M.m[10]= -f.z;
	// translation
	M.m[12] = - (s.x*eye.x + s.y*eye.y + s.z*eye.z);
	M.m[13] = - (u.x*eye.x + u.y*eye.y + u.z*eye.z);
	M.m[14] =	 (f.x*eye.x + f.y*eye.y + f.z*eye.z);
	return M;
}

se_mat4 mat4_mul(const se_mat4 A, const se_mat4 B) {
	se_mat4 R;
	for(int row=0; row<4; row++){
		for(int col=0; col<4; col++){
			float sum = 0.0f;
			for(int k=0; k<4; k++){
				sum += A.m[row + 4*k] * B.m[k + 4*col];
			}
			R.m[row + 4*col] = sum;
		}
	}
	return R;
}

se_mat4 mat4_translate(const se_vec3* v) {
	se_mat4 T = se_mat4_identity();
	T.m[12] = v->x;
	T.m[13] = v->y;
	T.m[14] = v->z;
	return T;
}

se_mat4 mat4_rotate_x(se_mat4 m, f32 angle) {
	se_mat4 R = se_mat4_identity();
	R.m[5] =	cos(angle);
	R.m[6] = -sin(angle);
	R.m[9] =	sin(angle);
	R.m[10] = cos(angle);
	return mat4_mul(m, R);
}

se_mat4 mat4_rotate_y(se_mat4 m, f32 angle) {
	se_mat4 R = se_mat4_identity();
	R.m[0] =	cos(angle);
	R.m[2] =	sin(angle);
	R.m[8] = -sin(angle);
	R.m[10] = cos(angle);
	return mat4_mul(m, R);
}

se_mat4 mat4_rotate_z(se_mat4 m, f32 angle) {
	se_mat4 R = se_mat4_identity();
	R.m[0] =	cos(angle);
	R.m[1] = -sin(angle);
	R.m[4] =	sin(angle);
	R.m[5] =	cos(angle);
	return mat4_mul(m, R);
}

se_mat4 mat4_scale(const se_vec3* v) {
	se_mat4 S = se_mat4_identity();
	S.m[0] = v->x;
	S.m[5] = v->y;
	S.m[10] = v->z;
	return S;
}

void se_box_2d_make(se_box_2d *out_box, const se_mat3 *transform) {
    se_vec2 pos = {0};
    se_vec2 scale = {0};
    se_mat3_get_position(transform, &pos);
    se_mat3_get_scale(transform, &scale);
	out_box->min = se_vec2(pos.x - scale.x, -pos.y - scale.y);
	out_box->max = se_vec2(pos.x + scale.x, -pos.y + scale.y);
}

b8 se_box_2d_is_inside(const se_box_2d* a, const se_vec2* p) {
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
