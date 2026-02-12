// Syphax-Engine - Ougi Washi

#include "se_physics.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SE_PHYSICS_EPSILON 1e-6f

s_vec2 se_physics_rotate_vec2(const s_vec2 *v, const f32 angle) {
	f32 c = cosf(angle);
	f32 s = sinf(angle);
	return s_vec2(v->x * c - v->y * s, v->x * s + v->y * c);
}

s_vec2 se_physics_rotate_vec2_inv(const s_vec2 *v, const f32 angle) {
	f32 c = cosf(angle);
	f32 s = sinf(angle);
	return s_vec2(v->x * c + v->y * s, -v->x * s + v->y * c);
}

s_vec3 se_physics_rotate_vec3(const s_vec3 *v, const s_vec3 *rot) {
	const f32 cx = cosf(rot->x);
	const f32 sx = sinf(rot->x);
	const f32 cy = cosf(rot->y);
	const f32 sy = sinf(rot->y);
	const f32 cz = cosf(rot->z);
	const f32 sz = sinf(rot->z);
	const f32 m00 = cz * cy;
	const f32 m01 = cz * sy * sx - sz * cx;
	const f32 m02 = cz * sy * cx + sz * sx;
	const f32 m10 = sz * cy;
	const f32 m11 = sz * sy * sx + cz * cx;
	const f32 m12 = sz * sy * cx - cz * sx;
	const f32 m20 = -sy;
	const f32 m21 = cy * sx;
	const f32 m22 = cy * cx;
	return s_vec3(
		v->x * m00 + v->y * m01 + v->z * m02,
		v->x * m10 + v->y * m11 + v->z * m12,
		v->x * m20 + v->y * m21 + v->z * m22);
}

s_vec3 se_physics_rotate_vec3_inv(const s_vec3 *v, const s_vec3 *rot) {
	const f32 cx = cosf(rot->x);
	const f32 sx = sinf(rot->x);
	const f32 cy = cosf(rot->y);
	const f32 sy = sinf(rot->y);
	const f32 cz = cosf(rot->z);
	const f32 sz = sinf(rot->z);
	const f32 m00 = cz * cy;
	const f32 m01 = cz * sy * sx - sz * cx;
	const f32 m02 = cz * sy * cx + sz * sx;
	const f32 m10 = sz * cy;
	const f32 m11 = sz * sy * sx + cz * cx;
	const f32 m12 = sz * sy * cx - cz * sx;
	const f32 m20 = -sy;
	const f32 m21 = cy * sx;
	const f32 m22 = cy * cx;
	return s_vec3(
		v->x * m00 + v->y * m10 + v->z * m20,
		v->x * m01 + v->y * m11 + v->z * m21,
		v->x * m02 + v->y * m12 + v->z * m22);
}

s_vec2 se_physics_perp(const s_vec2 *v) {
	return s_vec2(-v->y, v->x);
}

f32 se_physics_clamp(f32 v, f32 min_v, f32 max_v) {
	if (v < min_v) return min_v;
	if (v > max_v) return max_v;
	return v;
}

static f32 se_physics_maxf(f32 v, f32 min_v) {
	return v < min_v ? min_v : v;
}

s_vec2 se_physics_vec2_clamp(const s_vec2 *v, const s_vec2 *min_v, const s_vec2 *max_v) {
	s_vec2 out = *v;
	out.x = se_physics_clamp(out.x, min_v->x, max_v->x);
	out.y = se_physics_clamp(out.y, min_v->y, max_v->y);
	return out;
}

s_vec3 se_physics_vec3_clamp(const s_vec3 *v, const s_vec3 *min_v, const s_vec3 *max_v) {
	s_vec3 out = *v;
	out.x = se_physics_clamp(out.x, min_v->x, max_v->x);
	out.y = se_physics_clamp(out.y, min_v->y, max_v->y);
	out.z = se_physics_clamp(out.z, min_v->z, max_v->z);
	return out;
}

f32 se_physics_vec2_length_sq(const s_vec2 *v) {
	return v->x * v->x + v->y * v->y;
}

f32 se_physics_vec3_length_sq(const s_vec3 *v) {
	return v->x * v->x + v->y * v->y + v->z * v->z;
}

void se_physics_shape_2d_bounds_from_mesh(se_physics_shape_2d *shape) {
	shape->local_bounds.min = s_vec2(0.0f, 0.0f);
	shape->local_bounds.max = s_vec2(0.0f, 0.0f);
	if (!shape->mesh.vertices || shape->mesh.vertex_count == 0) {
		return;
	}
	s_vec2 min_v = shape->mesh.vertices[0];
	s_vec2 max_v = shape->mesh.vertices[0];
	for (sz i = 1; i < shape->mesh.vertex_count; i++) {
		s_vec2 v = shape->mesh.vertices[i];
		min_v.x = s_min(min_v.x, v.x);
		min_v.y = s_min(min_v.y, v.y);
		max_v.x = s_max(max_v.x, v.x);
		max_v.y = s_max(max_v.y, v.y);
	}
	shape->local_bounds.min = min_v;
	shape->local_bounds.max = max_v;
}

void se_physics_shape_3d_bounds_from_mesh(se_physics_shape_3d *shape) {
	shape->local_bounds.min = s_vec3(0.0f, 0.0f, 0.0f);
	shape->local_bounds.max = s_vec3(0.0f, 0.0f, 0.0f);
	if (!shape->mesh.vertices || shape->mesh.vertex_count == 0) {
		return;
	}
	s_vec3 min_v = shape->mesh.vertices[0];
	s_vec3 max_v = shape->mesh.vertices[0];
	for (sz i = 1; i < shape->mesh.vertex_count; i++) {
		s_vec3 v = shape->mesh.vertices[i];
		min_v.x = s_min(min_v.x, v.x);
		min_v.y = s_min(min_v.y, v.y);
		min_v.z = s_min(min_v.z, v.z);
		max_v.x = s_max(max_v.x, v.x);
		max_v.y = s_max(max_v.y, v.y);
		max_v.z = s_max(max_v.z, v.z);
	}
	shape->local_bounds.min = min_v;
	shape->local_bounds.max = max_v;
}

se_box_2d se_physics_shape_2d_world_aabb(const se_physics_body_2d *body, const se_physics_shape_2d *shape) {
	se_box_2d box = {0};
	s_vec2 offset = se_physics_rotate_vec2(&shape->offset, body->rotation);
	s_vec2 center = s_vec2(body->position.x + offset.x, body->position.y + offset.y);
	if (shape->type == SE_PHYSICS_SHAPE_2D_CIRCLE) {
		box.min = s_vec2(center.x - shape->circle.radius, center.y - shape->circle.radius);
		box.max = s_vec2(center.x + shape->circle.radius, center.y + shape->circle.radius);
		return box;
	}
	if (shape->type == SE_PHYSICS_SHAPE_2D_AABB || (shape->type == SE_PHYSICS_SHAPE_2D_BOX && shape->box.is_aabb)) {
		s_vec2 he = shape->box.half_extents;
		box.min = s_vec2(center.x - he.x, center.y - he.y);
		box.max = s_vec2(center.x + he.x, center.y + he.y);
		return box;
	}
	if (shape->type == SE_PHYSICS_SHAPE_2D_BOX) {
		f32 rot = body->rotation + shape->rotation;
		s_vec2 he = shape->box.half_extents;
		s_vec2 corners[4] = {
			s_vec2(-he.x, -he.y),
			s_vec2(he.x, -he.y),
			s_vec2(he.x, he.y),
			s_vec2(-he.x, he.y)
		};
		s_vec2 min_v = s_vec2(FLT_MAX, FLT_MAX);
		s_vec2 max_v = s_vec2(-FLT_MAX, -FLT_MAX);
		for (u32 i = 0; i < 4; i++) {
			s_vec2 w = se_physics_rotate_vec2(&corners[i], rot);
			w.x += center.x;
			w.y += center.y;
			min_v.x = s_min(min_v.x, w.x);
			min_v.y = s_min(min_v.y, w.y);
			max_v.x = s_max(max_v.x, w.x);
			max_v.y = s_max(max_v.y, w.y);
		}
		box.min = min_v;
		box.max = max_v;
		return box;
	}
	if (shape->type == SE_PHYSICS_SHAPE_2D_MESH) {
		s_vec2 min_v = s_vec2(FLT_MAX, FLT_MAX);
		s_vec2 max_v = s_vec2(-FLT_MAX, -FLT_MAX);
		s_vec2 corners[4] = {
			shape->local_bounds.min,
			s_vec2(shape->local_bounds.max.x, shape->local_bounds.min.y),
			shape->local_bounds.max,
			s_vec2(shape->local_bounds.min.x, shape->local_bounds.max.y)
		};
		f32 rot = body->rotation + shape->rotation;
		for (u32 i = 0; i < 4; i++) {
			s_vec2 w = se_physics_rotate_vec2(&corners[i], rot);
			w.x += center.x;
			w.y += center.y;
			min_v.x = s_min(min_v.x, w.x);
			min_v.y = s_min(min_v.y, w.y);
			max_v.x = s_max(max_v.x, w.x);
			max_v.y = s_max(max_v.y, w.y);
		}
		box.min = min_v;
		box.max = max_v;
		return box;
	}
	return box;
}

se_box_3d se_physics_shape_3d_world_aabb(const se_physics_body_3d *body, const se_physics_shape_3d *shape) {
	se_box_3d box = {0};
	s_vec3 offset = se_physics_rotate_vec3(&shape->offset, &body->rotation);
	s_vec3 center = s_vec3(body->position.x + offset.x, body->position.y + offset.y, body->position.z + offset.z);
	if (shape->type == SE_PHYSICS_SHAPE_3D_SPHERE) {
		f32 r = shape->sphere.radius;
		box.min = s_vec3(center.x - r, center.y - r, center.z - r);
		box.max = s_vec3(center.x + r, center.y + r, center.z + r);
		return box;
	}
	if (shape->type == SE_PHYSICS_SHAPE_3D_AABB || (shape->type == SE_PHYSICS_SHAPE_3D_BOX && shape->box.is_aabb)) {
		s_vec3 he = shape->box.half_extents;
		box.min = s_vec3(center.x - he.x, center.y - he.y, center.z - he.z);
		box.max = s_vec3(center.x + he.x, center.y + he.y, center.z + he.z);
		return box;
	}
	if (shape->type == SE_PHYSICS_SHAPE_3D_BOX) {
		s_vec3 he = shape->box.half_extents;
		s_vec3 corners[8] = {
			s_vec3(-he.x, -he.y, -he.z),
			s_vec3(he.x, -he.y, -he.z),
			s_vec3(he.x, he.y, -he.z),
			s_vec3(-he.x, he.y, -he.z),
			s_vec3(-he.x, -he.y, he.z),
			s_vec3(he.x, -he.y, he.z),
			s_vec3(he.x, he.y, he.z),
			s_vec3(-he.x, he.y, he.z)
		};
		s_vec3 min_v = s_vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		s_vec3 max_v = s_vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		s_vec3 rot = s_vec3(body->rotation.x + shape->rotation.x, body->rotation.y + shape->rotation.y, body->rotation.z + shape->rotation.z);
		for (u32 i = 0; i < 8; i++) {
			s_vec3 w = se_physics_rotate_vec3(&corners[i], &rot);
			w.x += center.x;
			w.y += center.y;
			w.z += center.z;
			min_v.x = s_min(min_v.x, w.x);
			min_v.y = s_min(min_v.y, w.y);
			min_v.z = s_min(min_v.z, w.z);
			max_v.x = s_max(max_v.x, w.x);
			max_v.y = s_max(max_v.y, w.y);
			max_v.z = s_max(max_v.z, w.z);
		}
		box.min = min_v;
		box.max = max_v;
		return box;
	}
	if (shape->type == SE_PHYSICS_SHAPE_3D_MESH) {
		s_vec3 min_v = s_vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		s_vec3 max_v = s_vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		s_vec3 corners[8] = {
			shape->local_bounds.min,
			s_vec3(shape->local_bounds.max.x, shape->local_bounds.min.y, shape->local_bounds.min.z),
			s_vec3(shape->local_bounds.max.x, shape->local_bounds.max.y, shape->local_bounds.min.z),
			s_vec3(shape->local_bounds.min.x, shape->local_bounds.max.y, shape->local_bounds.min.z),
			s_vec3(shape->local_bounds.min.x, shape->local_bounds.min.y, shape->local_bounds.max.z),
			s_vec3(shape->local_bounds.max.x, shape->local_bounds.min.y, shape->local_bounds.max.z),
			s_vec3(shape->local_bounds.max.x, shape->local_bounds.max.y, shape->local_bounds.max.z),
			shape->local_bounds.max
		};
		s_vec3 rot = s_vec3(body->rotation.x + shape->rotation.x, body->rotation.y + shape->rotation.y, body->rotation.z + shape->rotation.z);
		for (u32 i = 0; i < 8; i++) {
			s_vec3 w = se_physics_rotate_vec3(&corners[i], &rot);
			w.x += center.x;
			w.y += center.y;
			w.z += center.z;
			min_v.x = s_min(min_v.x, w.x);
			min_v.y = s_min(min_v.y, w.y);
			min_v.z = s_min(min_v.z, w.z);
			max_v.x = s_max(max_v.x, w.x);
			max_v.y = s_max(max_v.y, w.y);
			max_v.z = s_max(max_v.z, w.z);
		}
		box.min = min_v;
		box.max = max_v;
		return box;
	}
	return box;
}

f32 se_physics_shape_2d_area(const se_physics_shape_2d *shape) {
	switch (shape->type) {
		case SE_PHYSICS_SHAPE_2D_CIRCLE:
			return PI * shape->circle.radius * shape->circle.radius;
		case SE_PHYSICS_SHAPE_2D_AABB:
		case SE_PHYSICS_SHAPE_2D_BOX: {
			s_vec2 he = shape->box.half_extents;
			return (he.x * 2.0f) * (he.y * 2.0f);
		}
		case SE_PHYSICS_SHAPE_2D_MESH:
		default:
			return 0.0f;
	}
}

f32 se_physics_shape_3d_volume(const se_physics_shape_3d *shape) {
	switch (shape->type) {
		case SE_PHYSICS_SHAPE_3D_SPHERE:
			return (4.0f / 3.0f) * PI * shape->sphere.radius * shape->sphere.radius * shape->sphere.radius;
		case SE_PHYSICS_SHAPE_3D_AABB:
		case SE_PHYSICS_SHAPE_3D_BOX: {
			s_vec3 he = shape->box.half_extents;
			return (he.x * 2.0f) * (he.y * 2.0f) * (he.z * 2.0f);
		}
		case SE_PHYSICS_SHAPE_3D_MESH:
		default:
			return 0.0f;
	}
}

void se_physics_body_2d_update_mass(se_physics_body_2d *body) {
	if (body->type != SE_PHYSICS_BODY_DYNAMIC || body->mass <= 0.0f) {
		body->mass = 0.0f;
		body->inv_mass = 0.0f;
		body->inertia = 0.0f;
		body->inv_inertia = 0.0f;
		return;
	}
	f32 total_area = 0.0f;
	se_physics_shape_2d *shape = NULL;
	s_foreach(&body->shapes, shape) {
		if (shape->type == SE_PHYSICS_SHAPE_2D_MESH) {
			continue;
		}
		total_area += se_physics_shape_2d_area(shape);
	}
	if (total_area <= 0.0f) {
		body->inv_mass = 1.0f / body->mass;
		body->inertia = body->mass;
		body->inv_inertia = 1.0f / body->inertia;
		return;
	}
	f32 inertia = 0.0f;
	shape = NULL;
	s_foreach(&body->shapes, shape) {
		f32 area = se_physics_shape_2d_area(shape);
		if (area <= 0.0f) {
			continue;
		}
		f32 shape_mass = body->mass * (area / total_area);
		s_vec2 offset = shape->offset;
		f32 dist_sq = offset.x * offset.x + offset.y * offset.y;
		if (shape->type == SE_PHYSICS_SHAPE_2D_CIRCLE) {
			f32 r = shape->circle.radius;
			inertia += 0.5f * shape_mass * r * r + shape_mass * dist_sq;
		} else if (shape->type == SE_PHYSICS_SHAPE_2D_AABB || shape->type == SE_PHYSICS_SHAPE_2D_BOX) {
			f32 w = shape->box.half_extents.x * 2.0f;
			f32 h = shape->box.half_extents.y * 2.0f;
			inertia += (shape_mass * (w * w + h * h) / 12.0f) + shape_mass * dist_sq;
		}
	}
	if (inertia <= 0.0f) {
		inertia = body->mass;
	}
	body->inv_mass = 1.0f / body->mass;
	body->inertia = inertia;
	body->inv_inertia = 1.0f / inertia;
}

void se_physics_body_3d_update_mass(se_physics_body_3d *body) {
	if (body->type != SE_PHYSICS_BODY_DYNAMIC || body->mass <= 0.0f) {
		body->mass = 0.0f;
		body->inv_mass = 0.0f;
		body->inertia = 0.0f;
		body->inv_inertia = 0.0f;
		return;
	}
	f32 total_volume = 0.0f;
	se_physics_shape_3d *shape3d = NULL;
	s_foreach(&body->shapes, shape3d) {
		if (shape3d->type == SE_PHYSICS_SHAPE_3D_MESH) {
			continue;
		}
		total_volume += se_physics_shape_3d_volume(shape3d);
	}
	if (total_volume <= 0.0f) {
		body->inv_mass = 1.0f / body->mass;
		body->inertia = body->mass;
		body->inv_inertia = 1.0f / body->inertia;
		return;
	}
	f32 inertia = 0.0f;
	shape3d = NULL;
	s_foreach(&body->shapes, shape3d) {
		f32 volume = se_physics_shape_3d_volume(shape3d);
		if (volume <= 0.0f) {
			continue;
		}
		f32 shape_mass = body->mass * (volume / total_volume);
		s_vec3 offset = shape3d->offset;
		f32 dist_sq = offset.x * offset.x + offset.y * offset.y + offset.z * offset.z;
		if (shape3d->type == SE_PHYSICS_SHAPE_3D_SPHERE) {
			f32 r = shape3d->sphere.radius;
			inertia += 0.4f * shape_mass * r * r + shape_mass * dist_sq;
		} else if (shape3d->type == SE_PHYSICS_SHAPE_3D_AABB || shape3d->type == SE_PHYSICS_SHAPE_3D_BOX) {
			f32 w = shape3d->box.half_extents.x * 2.0f;
			f32 h = shape3d->box.half_extents.y * 2.0f;
			f32 d = shape3d->box.half_extents.z * 2.0f;
			inertia += (shape_mass * (w * w + h * h + d * d) / 12.0f) + shape_mass * dist_sq;
		}
	}
	if (inertia <= 0.0f) {
		inertia = body->mass;
	}
	body->inv_mass = 1.0f / body->mass;
	body->inertia = inertia;
	body->inv_inertia = 1.0f / inertia;
}

s_vec2 se_physics_box2d_local_point(const s_vec2 *point, const s_vec2 *center, f32 rotation) {
	s_vec2 local = s_vec2(point->x - center->x, point->y - center->y);
	return se_physics_rotate_vec2_inv(&local, rotation);
}

s_vec3 se_physics_box3d_local_point(const s_vec3 *point, const s_vec3 *center, const s_vec3 *rotation) {
	s_vec3 local = s_vec3(point->x - center->x, point->y - center->y, point->z - center->z);
	return se_physics_rotate_vec3_inv(&local, rotation);
}

b8 se_physics_circle_circle(const s_vec2 *a_pos, f32 a_r, const s_vec2 *b_pos, f32 b_r, se_physics_contact_2d *out) {
	s_vec2 delta = s_vec2(b_pos->x - a_pos->x, b_pos->y - a_pos->y);
	f32 dist_sq = se_physics_vec2_length_sq(&delta);
	f32 r = a_r + b_r;
	if (dist_sq > r * r) return false;
	f32 dist = sqrtf(dist_sq);
	if (dist < SE_PHYSICS_EPSILON) {
		out->normal = s_vec2(0.0f, 1.0f);
		out->penetration = r;
		out->contact_point = *a_pos;
		return true;
	}
	out->normal = s_vec2(delta.x / dist, delta.y / dist);
	out->penetration = r - dist;
	out->contact_point = s_vec2(a_pos->x + out->normal.x * a_r, a_pos->y + out->normal.y * a_r);
	return true;
}

b8 se_physics_circle_box(const s_vec2 *circle_pos, f32 radius, const s_vec2 *box_center, const s_vec2 *box_half, f32 rotation, se_physics_contact_2d *out) {
	s_vec2 local = se_physics_box2d_local_point(circle_pos, box_center, rotation);
	s_vec2 min_v = s_vec2(-box_half->x, -box_half->y);
	s_vec2 max_v = s_vec2(box_half->x, box_half->y);
	s_vec2 closest = se_physics_vec2_clamp(&local, &min_v, &max_v);
	s_vec2 diff = s_vec2(local.x - closest.x, local.y - closest.y);
	f32 dist_sq = se_physics_vec2_length_sq(&diff);
	if (dist_sq > radius * radius) return false;
	f32 dist = sqrtf(dist_sq);
	if (dist < SE_PHYSICS_EPSILON) {
		f32 dx = box_half->x - fabsf(local.x);
		f32 dy = box_half->y - fabsf(local.y);
		f32 min_face_distance = s_min(dx, dy);
		s_vec2 axis = dx < dy ? s_vec2((local.x < 0.0f) ? -1.0f : 1.0f, 0.0f) : s_vec2(0.0f, (local.y < 0.0f) ? -1.0f : 1.0f);
		s_vec2 world_axis = se_physics_rotate_vec2(&axis, rotation);
		out->normal = world_axis;
		out->penetration = radius + min_face_distance;
		out->contact_point = *circle_pos;
		return true;
	}
	s_vec2 normal_local = s_vec2(diff.x / dist, diff.y / dist);
	s_vec2 normal_world = se_physics_rotate_vec2(&normal_local, rotation);
	out->normal = normal_world;
	out->penetration = radius - dist;
	s_vec2 world_closest = se_physics_rotate_vec2(&closest, rotation);
	world_closest.x += box_center->x;
	world_closest.y += box_center->y;
	out->contact_point = world_closest;
	return true;
}

void se_physics_project_box2d(const s_vec2 *center, const s_vec2 *half, f32 rotation, const s_vec2 *axis, f32 *out_min, f32 *out_max) {
	s_vec2 corners[4] = {
		s_vec2(-half->x, -half->y),
		s_vec2(half->x, -half->y),
		s_vec2(half->x, half->y),
		s_vec2(-half->x, half->y)
	};
	f32 min_v = FLT_MAX;
	f32 max_v = -FLT_MAX;
	for (u32 i = 0; i < 4; i++) {
		s_vec2 w = se_physics_rotate_vec2(&corners[i], rotation);
		w.x += center->x;
		w.y += center->y;
		f32 proj = s_vec2_dot(&w, axis);
		min_v = s_min(min_v, proj);
		max_v = s_max(max_v, proj);
	}
	*out_min = min_v;
	*out_max = max_v;
}

b8 se_physics_box_box_2d(const s_vec2 *a_center, const s_vec2 *a_half, f32 a_rot, const s_vec2 *b_center, const s_vec2 *b_half, f32 b_rot, se_physics_contact_2d *out) {
	s_vec2 axes[4] = {
		se_physics_rotate_vec2(&s_vec2(1.0f, 0.0f), a_rot),
		se_physics_rotate_vec2(&s_vec2(0.0f, 1.0f), a_rot),
		se_physics_rotate_vec2(&s_vec2(1.0f, 0.0f), b_rot),
		se_physics_rotate_vec2(&s_vec2(0.0f, 1.0f), b_rot)
	};
	f32 min_overlap = FLT_MAX;
	s_vec2 best_axis = s_vec2(0.0f, 1.0f);
	for (u32 i = 0; i < 4; i++) {
		s_vec2 axis = axes[i];
		axis = s_vec2_normalize(&axis);
		f32 min_a, max_a, min_b, max_b;
		se_physics_project_box2d(a_center, a_half, a_rot, &axis, &min_a, &max_a);
		se_physics_project_box2d(b_center, b_half, b_rot, &axis, &min_b, &max_b);
		if (max_a < min_b || max_b < min_a) {
			return false;
		}
		f32 overlap = s_min(max_a, max_b) - s_max(min_a, min_b);
		if (overlap < min_overlap) {
			min_overlap = overlap;
			best_axis = axis;
		}
	}
	s_vec2 dir = s_vec2(b_center->x - a_center->x, b_center->y - a_center->y);
	if (s_vec2_dot(&dir, &best_axis) < 0.0f) {
		best_axis.x = -best_axis.x;
		best_axis.y = -best_axis.y;
	}
	out->normal = best_axis;
	out->penetration = min_overlap;
	out->contact_point = s_vec2((a_center->x + b_center->x) * 0.5f, (a_center->y + b_center->y) * 0.5f);
	return true;
}

b8 se_physics_aabb_aabb_2d(const s_vec2 *a_min, const s_vec2 *a_max, const s_vec2 *b_min, const s_vec2 *b_max, se_physics_contact_2d *out) {
	if (a_max->x < b_min->x || a_min->x > b_max->x) return false;
	if (a_max->y < b_min->y || a_min->y > b_max->y) return false;
	f32 overlap_x = s_min(a_max->x, b_max->x) - s_max(a_min->x, b_min->x);
	f32 overlap_y = s_min(a_max->y, b_max->y) - s_max(a_min->y, b_min->y);
	f32 a_center_x = (a_min->x + a_max->x) * 0.5f;
	f32 a_center_y = (a_min->y + a_max->y) * 0.5f;
	f32 b_center_x = (b_min->x + b_max->x) * 0.5f;
	f32 b_center_y = (b_min->y + b_max->y) * 0.5f;
	if (overlap_x < overlap_y) {
		out->normal = s_vec2(b_center_x >= a_center_x ? 1.0f : -1.0f, 0.0f);
		out->penetration = overlap_x;
	} else {
		out->normal = s_vec2(0.0f, b_center_y >= a_center_y ? 1.0f : -1.0f);
		out->penetration = overlap_y;
	}
	out->contact_point = s_vec2((a_min->x + a_max->x + b_min->x + b_max->x) * 0.25f,
		(a_min->y + a_max->y + b_min->y + b_max->y) * 0.25f);
	return true;
}

s_vec2 se_physics_closest_point_on_segment_2d(const s_vec2 *a, const s_vec2 *b, const s_vec2 *p) {
	s_vec2 ab = s_vec2(b->x - a->x, b->y - a->y);
	f32 t = s_vec2_dot(p, &ab) - s_vec2_dot(a, &ab);
	f32 denom = s_vec2_dot(&ab, &ab);
	if (denom > SE_PHYSICS_EPSILON) {
		t /= denom;
	} else {
		t = 0.0f;
	}
	t = se_physics_clamp(t, 0.0f, 1.0f);
	return s_vec2(a->x + ab.x * t, a->y + ab.y * t);
}

s_vec3 se_physics_closest_point_on_segment_3d(const s_vec3 *a, const s_vec3 *b, const s_vec3 *p) {
	s_vec3 ab = s_vec3(b->x - a->x, b->y - a->y, b->z - a->z);
	f32 t = s_vec3_dot(p, &ab) - s_vec3_dot(a, &ab);
	f32 denom = s_vec3_dot(&ab, &ab);
	if (denom > SE_PHYSICS_EPSILON) {
		t /= denom;
	} else {
		t = 0.0f;
	}
	t = se_physics_clamp(t, 0.0f, 1.0f);
	return s_vec3(a->x + ab.x * t, a->y + ab.y * t, a->z + ab.z * t);
}

b8 se_physics_ray_aabb_2d(const s_vec2 *origin, const s_vec2 *dir, const se_box_2d *box, f32 *out_t) {
	f32 tmin = 0.0f;
	f32 tmax = FLT_MAX;
	if (fabsf(dir->x) < SE_PHYSICS_EPSILON) {
		if (origin->x < box->min.x || origin->x > box->max.x) return false;
	} else {
		f32 inv = 1.0f / dir->x;
		f32 t1 = (box->min.x - origin->x) * inv;
		f32 t2 = (box->max.x - origin->x) * inv;
		if (t1 > t2) { f32 tmp = t1; t1 = t2; t2 = tmp; }
		tmin = s_max(tmin, t1);
		tmax = s_min(tmax, t2);
		if (tmin > tmax) return false;
	}
	if (fabsf(dir->y) < SE_PHYSICS_EPSILON) {
		if (origin->y < box->min.y || origin->y > box->max.y) return false;
	} else {
		f32 inv = 1.0f / dir->y;
		f32 t1 = (box->min.y - origin->y) * inv;
		f32 t2 = (box->max.y - origin->y) * inv;
		if (t1 > t2) { f32 tmp = t1; t1 = t2; t2 = tmp; }
		tmin = s_max(tmin, t1);
		tmax = s_min(tmax, t2);
		if (tmin > tmax) return false;
	}
	if (out_t) *out_t = tmin;
	return true;
}

b8 se_physics_ray_aabb_3d(const s_vec3 *origin, const s_vec3 *dir, const se_box_3d *box, f32 *out_t) {
	f32 tmin = 0.0f;
	f32 tmax = FLT_MAX;
	const f32 o[3] = { origin->x, origin->y, origin->z };
	const f32 d[3] = { dir->x, dir->y, dir->z };
	const f32 bmin[3] = { box->min.x, box->min.y, box->min.z };
	const f32 bmax[3] = { box->max.x, box->max.y, box->max.z };
	for (u32 i = 0; i < 3; i++) {
		if (fabsf(d[i]) < SE_PHYSICS_EPSILON) {
			if (o[i] < bmin[i] || o[i] > bmax[i]) return false;
			continue;
		}
		f32 inv = 1.0f / d[i];
		f32 t1 = (bmin[i] - o[i]) * inv;
		f32 t2 = (bmax[i] - o[i]) * inv;
		if (t1 > t2) { f32 tmp = t1; t1 = t2; t2 = tmp; }
		tmin = s_max(tmin, t1);
		tmax = s_min(tmax, t2);
		if (tmin > tmax) return false;
	}
	if (out_t) *out_t = tmin;
	return true;
}

s_vec2 se_physics_closest_point_on_triangle_2d(const s_vec2 *a, const s_vec2 *b, const s_vec2 *c, const s_vec2 *p) {
	s_vec2 ab = s_vec2(b->x - a->x, b->y - a->y);
	s_vec2 ac = s_vec2(c->x - a->x, c->y - a->y);
	s_vec2 ap = s_vec2(p->x - a->x, p->y - a->y);
	f32 d1 = s_vec2_dot(&ab, &ap);
	f32 d2 = s_vec2_dot(&ac, &ap);
	if (d1 <= 0.0f && d2 <= 0.0f) return *a;

	s_vec2 bp = s_vec2(p->x - b->x, p->y - b->y);
	f32 d3 = s_vec2_dot(&ab, &bp);
	f32 d4 = s_vec2_dot(&ac, &bp);
	if (d3 >= 0.0f && d4 <= d3) return *b;

	f32 vc = d1 * d4 - d3 * d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
		f32 v = d1 / (d1 - d3);
		return s_vec2(a->x + ab.x * v, a->y + ab.y * v);
	}

	s_vec2 cp = s_vec2(p->x - c->x, p->y - c->y);
	f32 d5 = s_vec2_dot(&ab, &cp);
	f32 d6 = s_vec2_dot(&ac, &cp);
	if (d6 >= 0.0f && d5 <= d6) return *c;

	f32 vb = d5 * d2 - d1 * d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
		f32 w = d2 / (d2 - d6);
		return s_vec2(a->x + ac.x * w, a->y + ac.y * w);
	}

	f32 va = d3 * d6 - d5 * d4;
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
		s_vec2 bc = s_vec2(c->x - b->x, c->y - b->y);
		f32 w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		return s_vec2(b->x + bc.x * w, b->y + bc.y * w);
	}

	f32 denom = 1.0f / (va + vb + vc);
	f32 v = vb * denom;
	f32 w = vc * denom;
	return s_vec2(a->x + ab.x * v + ac.x * w, a->y + ab.y * v + ac.y * w);
}

s_vec3 se_physics_closest_point_on_triangle_3d(const s_vec3 *a, const s_vec3 *b, const s_vec3 *c, const s_vec3 *p) {
	s_vec3 ab = s_vec3(b->x - a->x, b->y - a->y, b->z - a->z);
	s_vec3 ac = s_vec3(c->x - a->x, c->y - a->y, c->z - a->z);
	s_vec3 ap = s_vec3(p->x - a->x, p->y - a->y, p->z - a->z);
	f32 d1 = s_vec3_dot(&ab, &ap);
	f32 d2 = s_vec3_dot(&ac, &ap);
	if (d1 <= 0.0f && d2 <= 0.0f) return *a;

	s_vec3 bp = s_vec3(p->x - b->x, p->y - b->y, p->z - b->z);
	f32 d3 = s_vec3_dot(&ab, &bp);
	f32 d4 = s_vec3_dot(&ac, &bp);
	if (d3 >= 0.0f && d4 <= d3) return *b;

	f32 vc = d1 * d4 - d3 * d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
		f32 v = d1 / (d1 - d3);
		return s_vec3(a->x + ab.x * v, a->y + ab.y * v, a->z + ab.z * v);
	}

	s_vec3 cp = s_vec3(p->x - c->x, p->y - c->y, p->z - c->z);
	f32 d5 = s_vec3_dot(&ab, &cp);
	f32 d6 = s_vec3_dot(&ac, &cp);
	if (d6 >= 0.0f && d5 <= d6) return *c;

	f32 vb = d5 * d2 - d1 * d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
		f32 w = d2 / (d2 - d6);
		return s_vec3(a->x + ac.x * w, a->y + ac.y * w, a->z + ac.z * w);
	}

	f32 va = d3 * d6 - d5 * d4;
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
		s_vec3 bc = s_vec3(c->x - b->x, c->y - b->y, c->z - b->z);
		f32 w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		return s_vec3(b->x + bc.x * w, b->y + bc.y * w, b->z + bc.z * w);
	}

	f32 denom = 1.0f / (va + vb + vc);
	f32 v = vb * denom;
	f32 w = vc * denom;
	return s_vec3(a->x + ab.x * v + ac.x * w, a->y + ab.y * v + ac.y * w, a->z + ab.z * v + ac.z * w);
}

b8 se_physics_mesh_circle_2d(const se_physics_mesh_2d *mesh, const s_vec2 *mesh_pos, f32 mesh_rot, const s_vec2 *circle_pos, f32 radius, se_physics_contact_2d *out) {
	if (!mesh->vertices || mesh->vertex_count < 3) return false;
	sz tri_count = mesh->index_count ? mesh->index_count / 3 : mesh->vertex_count / 3;
	for (sz t = 0; t < tri_count; t++) {
		u32 i0 = mesh->indices ? mesh->indices[t * 3 + 0] : (u32)(t * 3 + 0);
		u32 i1 = mesh->indices ? mesh->indices[t * 3 + 1] : (u32)(t * 3 + 1);
		u32 i2 = mesh->indices ? mesh->indices[t * 3 + 2] : (u32)(t * 3 + 2);
		if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) continue;
		s_vec2 a = se_physics_rotate_vec2(&mesh->vertices[i0], mesh_rot);
		s_vec2 b = se_physics_rotate_vec2(&mesh->vertices[i1], mesh_rot);
		s_vec2 c = se_physics_rotate_vec2(&mesh->vertices[i2], mesh_rot);
		a.x += mesh_pos->x; a.y += mesh_pos->y;
		b.x += mesh_pos->x; b.y += mesh_pos->y;
		c.x += mesh_pos->x; c.y += mesh_pos->y;
		s_vec2 closest = se_physics_closest_point_on_triangle_2d(&a, &b, &c, circle_pos);
		s_vec2 diff = s_vec2(circle_pos->x - closest.x, circle_pos->y - closest.y);
		f32 dist_sq = se_physics_vec2_length_sq(&diff);
		if (dist_sq <= radius * radius) {
			f32 dist = sqrtf(dist_sq);
			if (dist < SE_PHYSICS_EPSILON) {
				out->normal = s_vec2(0.0f, 1.0f);
				out->penetration = radius;
				out->contact_point = closest;
			} else {
				out->normal = s_vec2(diff.x / dist, diff.y / dist);
				out->penetration = radius - dist;
				out->contact_point = closest;
			}
			return true;
		}
	}
	return false;
}

s_vec2 se_physics_triangle_centroid_2d(const se_physics_mesh_2d *mesh, u32 tri_index) {
	u32 i0 = mesh->indices ? mesh->indices[tri_index * 3 + 0] : tri_index * 3 + 0;
	u32 i1 = mesh->indices ? mesh->indices[tri_index * 3 + 1] : tri_index * 3 + 1;
	u32 i2 = mesh->indices ? mesh->indices[tri_index * 3 + 2] : tri_index * 3 + 2;
	s_vec2 a = mesh->vertices[i0];
	s_vec2 b = mesh->vertices[i1];
	s_vec2 c = mesh->vertices[i2];
	return s_vec2((a.x + b.x + c.x) / 3.0f, (a.y + b.y + c.y) / 3.0f);
}

f32 se_physics_triangle_centroid_axis_2d(const se_physics_mesh_2d *mesh, u32 tri_index, i32 axis) {
	s_vec2 c = se_physics_triangle_centroid_2d(mesh, tri_index);
	return axis == 0 ? c.x : c.y;
}

void se_physics_sort_triangles_2d(const se_physics_mesh_2d *mesh, u32 *triangles, i32 axis, i32 left, i32 right) {
	if (left >= right) return;
	i32 i = left;
	i32 j = right;
	f32 pivot = se_physics_triangle_centroid_axis_2d(mesh, triangles[(left + right) / 2], axis);
	while (i <= j) {
		while (se_physics_triangle_centroid_axis_2d(mesh, triangles[i], axis) < pivot) i++;
		while (se_physics_triangle_centroid_axis_2d(mesh, triangles[j], axis) > pivot) j--;
		if (i <= j) {
			u32 tmp = triangles[i];
			triangles[i] = triangles[j];
			triangles[j] = tmp;
			i++;
			j--;
		}
	}
	if (left < j) se_physics_sort_triangles_2d(mesh, triangles, axis, left, j);
	if (i < right) se_physics_sort_triangles_2d(mesh, triangles, axis, i, right);
}

se_box_2d se_physics_triangle_bounds_2d(const se_physics_mesh_2d *mesh, u32 tri_index) {
	u32 i0 = mesh->indices ? mesh->indices[tri_index * 3 + 0] : tri_index * 3 + 0;
	u32 i1 = mesh->indices ? mesh->indices[tri_index * 3 + 1] : tri_index * 3 + 1;
	u32 i2 = mesh->indices ? mesh->indices[tri_index * 3 + 2] : tri_index * 3 + 2;
	s_vec2 a = mesh->vertices[i0];
	s_vec2 b = mesh->vertices[i1];
	s_vec2 c = mesh->vertices[i2];
	se_box_2d bounds = {0};
	bounds.min = s_vec2(s_min(a.x, s_min(b.x, c.x)), s_min(a.y, s_min(b.y, c.y)));
	bounds.max = s_vec2(s_max(a.x, s_max(b.x, c.x)), s_max(a.y, s_max(b.y, c.y)));
	return bounds;
}

u32 se_physics_bvh_build_2d(se_physics_shape_2d *shape, u32 start, u32 count) {
	u32 node_index = (u32)s_array_get_size(&shape->bvh_nodes);
	s_handle node_handle = s_array_increment(&shape->bvh_nodes);
	se_physics_bvh_node_2d *node = s_array_get(&shape->bvh_nodes, node_handle);
	memset(node, 0, sizeof(*node));

	se_box_2d bounds = {0};
	bounds.min = s_vec2(FLT_MAX, FLT_MAX);
	bounds.max = s_vec2(-FLT_MAX, -FLT_MAX);
	for (u32 i = 0; i < count; i++) {
		u32 tri = shape->bvh_triangles[start + i];
		se_box_2d tri_bounds = se_physics_triangle_bounds_2d(&shape->mesh, tri);
		bounds.min.x = s_min(bounds.min.x, tri_bounds.min.x);
		bounds.min.y = s_min(bounds.min.y, tri_bounds.min.y);
		bounds.max.x = s_max(bounds.max.x, tri_bounds.max.x);
		bounds.max.y = s_max(bounds.max.y, tri_bounds.max.y);
	}
	node->bounds = bounds;
	if (count <= 4) {
		node->is_leaf = true;
		node->start = start;
		node->count = count;
		return node_index;
	}
	f32 extent_x = bounds.max.x - bounds.min.x;
	f32 extent_y = bounds.max.y - bounds.min.y;
	i32 axis = extent_x >= extent_y ? 0 : 1;
	se_physics_sort_triangles_2d(&shape->mesh, shape->bvh_triangles, axis, (i32)start, (i32)(start + count - 1));
	u32 mid = count / 2;
	node->left = se_physics_bvh_build_2d(shape, start, mid);
	node->right = se_physics_bvh_build_2d(shape, start + mid, count - mid);
	return node_index;
}

b8 se_physics_mesh_box_2d(const se_physics_mesh_2d *mesh, const s_vec2 *mesh_pos, f32 mesh_rot, const s_vec2 *box_center, const s_vec2 *half, f32 rotation, se_physics_contact_2d *out) {
	if (!mesh->vertices || mesh->vertex_count < 3) return false;
	sz tri_count = mesh->index_count ? mesh->index_count / 3 : mesh->vertex_count / 3;
	for (sz t = 0; t < tri_count; t++) {
		u32 i0 = mesh->indices ? mesh->indices[t * 3 + 0] : (u32)(t * 3 + 0);
		u32 i1 = mesh->indices ? mesh->indices[t * 3 + 1] : (u32)(t * 3 + 1);
		u32 i2 = mesh->indices ? mesh->indices[t * 3 + 2] : (u32)(t * 3 + 2);
		if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) continue;
		s_vec2 a = se_physics_rotate_vec2(&mesh->vertices[i0], mesh_rot);
		s_vec2 b = se_physics_rotate_vec2(&mesh->vertices[i1], mesh_rot);
		s_vec2 c = se_physics_rotate_vec2(&mesh->vertices[i2], mesh_rot);
		a.x += mesh_pos->x; a.y += mesh_pos->y;
		b.x += mesh_pos->x; b.y += mesh_pos->y;
		c.x += mesh_pos->x; c.y += mesh_pos->y;
		s_vec2 axes[5] = {
			se_physics_rotate_vec2(&s_vec2(1.0f, 0.0f), rotation),
			se_physics_rotate_vec2(&s_vec2(0.0f, 1.0f), rotation),
			se_physics_perp(&(s_vec2){ b.x - a.x, b.y - a.y }),
			se_physics_perp(&(s_vec2){ c.x - b.x, c.y - b.y }),
			se_physics_perp(&(s_vec2){ a.x - c.x, a.y - c.y })
		};
		f32 min_overlap = FLT_MAX;
		s_vec2 best_axis = s_vec2(0.0f, 1.0f);
		b8 separated = false;
		for (u32 i = 0; i < 5; i++) {
			s_vec2 axis = s_vec2_normalize(&axes[i]);
			f32 min_a, max_a, min_b, max_b;
			se_physics_project_box2d(box_center, half, rotation, &axis, &min_a, &max_a);
			f32 proj0 = s_vec2_dot(&a, &axis);
			f32 proj1 = s_vec2_dot(&b, &axis);
			f32 proj2 = s_vec2_dot(&c, &axis);
			min_b = s_min(proj0, s_min(proj1, proj2));
			max_b = s_max(proj0, s_max(proj1, proj2));
			if (max_a < min_b || max_b < min_a) {
				separated = true;
				break;
			}
			f32 overlap = s_min(max_a, max_b) - s_max(min_a, min_b);
			if (overlap < min_overlap) {
				min_overlap = overlap;
				best_axis = axis;
			}
		}
		if (!separated) {
			s_vec2 dir = s_vec2(box_center->x - mesh_pos->x, box_center->y - mesh_pos->y);
			if (s_vec2_dot(&dir, &best_axis) < 0.0f) {
				best_axis.x = -best_axis.x;
				best_axis.y = -best_axis.y;
			}
			out->normal = best_axis;
			out->penetration = min_overlap;
			out->contact_point = s_vec2((a.x + b.x + c.x) / 3.0f, (a.y + b.y + c.y) / 3.0f);
			return true;
		}
	}
	return false;
}

void se_physics_box3d_axes(const s_vec3 *rotation, s_vec3 *x_axis, s_vec3 *y_axis, s_vec3 *z_axis) {
	s_vec3 vx = s_vec3(1.0f, 0.0f, 0.0f);
	s_vec3 vy = s_vec3(0.0f, 1.0f, 0.0f);
	s_vec3 vz = s_vec3(0.0f, 0.0f, 1.0f);
	*x_axis = se_physics_rotate_vec3(&vx, rotation);
	*y_axis = se_physics_rotate_vec3(&vy, rotation);
	*z_axis = se_physics_rotate_vec3(&vz, rotation);
}

void se_physics_project_box3d(const s_vec3 *center, const s_vec3 *half, const s_vec3 *rotation, const s_vec3 *axis, f32 *out_min, f32 *out_max) {
	s_vec3 axes[3];
	se_physics_box3d_axes(rotation, &axes[0], &axes[1], &axes[2]);
	f32 c = s_vec3_dot(center, axis);
	f32 r = fabsf(half->x * s_vec3_dot(&axes[0], axis)) + fabsf(half->y * s_vec3_dot(&axes[1], axis)) + fabsf(half->z * s_vec3_dot(&axes[2], axis));
	*out_min = c - r;
	*out_max = c + r;
}

b8 se_physics_sphere_sphere(const s_vec3 *a_pos, f32 a_r, const s_vec3 *b_pos, f32 b_r, se_physics_contact_3d *out) {
	s_vec3 delta = s_vec3(b_pos->x - a_pos->x, b_pos->y - a_pos->y, b_pos->z - a_pos->z);
	f32 dist_sq = se_physics_vec3_length_sq(&delta);
	f32 r = a_r + b_r;
	if (dist_sq > r * r) return false;
	f32 dist = sqrtf(dist_sq);
	if (dist < SE_PHYSICS_EPSILON) {
		out->normal = s_vec3(0.0f, 1.0f, 0.0f);
		out->penetration = r;
		out->contact_point = *a_pos;
		return true;
	}
	out->normal = s_vec3(delta.x / dist, delta.y / dist, delta.z / dist);
	out->penetration = r - dist;
	out->contact_point = s_vec3(a_pos->x + out->normal.x * a_r, a_pos->y + out->normal.y * a_r, a_pos->z + out->normal.z * a_r);
	return true;
}

b8 se_physics_sphere_box(const s_vec3 *sphere_pos, f32 radius, const s_vec3 *box_center, const s_vec3 *half, const s_vec3 *rotation, se_physics_contact_3d *out) {
	s_vec3 local = se_physics_box3d_local_point(sphere_pos, box_center, rotation);
	s_vec3 min_v = s_vec3(-half->x, -half->y, -half->z);
	s_vec3 max_v = s_vec3(half->x, half->y, half->z);
	s_vec3 closest = se_physics_vec3_clamp(&local, &min_v, &max_v);
	s_vec3 diff = s_vec3(local.x - closest.x, local.y - closest.y, local.z - closest.z);
	f32 dist_sq = se_physics_vec3_length_sq(&diff);
	if (dist_sq > radius * radius) return false;
	f32 dist = sqrtf(dist_sq);
	if (dist < SE_PHYSICS_EPSILON) {
		f32 dx = half->x - fabsf(local.x);
		f32 dy = half->y - fabsf(local.y);
		f32 dz = half->z - fabsf(local.z);
		f32 min_face_distance = s_min(dx, s_min(dy, dz));
		s_vec3 axis = s_vec3(0.0f, 1.0f, 0.0f);
		if (dx <= dy && dx <= dz) axis = s_vec3((local.x < 0.0f) ? -1.0f : 1.0f, 0.0f, 0.0f);
		else if (dy <= dz) axis = s_vec3(0.0f, (local.y < 0.0f) ? -1.0f : 1.0f, 0.0f);
		else axis = s_vec3(0.0f, 0.0f, (local.z < 0.0f) ? -1.0f : 1.0f);
		out->normal = se_physics_rotate_vec3(&axis, rotation);
		out->penetration = radius + min_face_distance;
		out->contact_point = *sphere_pos;
		return true;
	}
	s_vec3 normal_local = s_vec3(diff.x / dist, diff.y / dist, diff.z / dist);
	s_vec3 normal_world = se_physics_rotate_vec3(&normal_local, rotation);
	out->normal = normal_world;
	out->penetration = radius - dist;
	s_vec3 world_closest = se_physics_rotate_vec3(&closest, rotation);
	world_closest.x += box_center->x;
	world_closest.y += box_center->y;
	world_closest.z += box_center->z;
	out->contact_point = world_closest;
	return true;
}

b8 se_physics_box_box_3d(const s_vec3 *a_center, const s_vec3 *a_half, const s_vec3 *a_rot, const s_vec3 *b_center, const s_vec3 *b_half, const s_vec3 *b_rot, se_physics_contact_3d *out) {
	s_vec3 a_axes[3];
	s_vec3 b_axes[3];
	se_physics_box3d_axes(a_rot, &a_axes[0], &a_axes[1], &a_axes[2]);
	se_physics_box3d_axes(b_rot, &b_axes[0], &b_axes[1], &b_axes[2]);
	s_vec3 axes[15];
	u32 axis_count = 0;
	for (u32 i = 0; i < 3; i++) axes[axis_count++] = a_axes[i];
	for (u32 i = 0; i < 3; i++) axes[axis_count++] = b_axes[i];
	for (u32 i = 0; i < 3; i++) {
		for (u32 j = 0; j < 3; j++) {
			s_vec3 axis = s_vec3_cross(&a_axes[i], &b_axes[j]);
			f32 len = s_vec3_length(&axis);
			if (len > SE_PHYSICS_EPSILON) {
				axis = s_vec3_divs(&axis, len);
				axes[axis_count++] = axis;
			}
		}
	}
	f32 min_overlap = FLT_MAX;
	s_vec3 best_axis = s_vec3(0.0f, 1.0f, 0.0f);
	for (u32 i = 0; i < axis_count; i++) {
		s_vec3 axis = axes[i];
		f32 min_a, max_a, min_b, max_b;
		se_physics_project_box3d(a_center, a_half, a_rot, &axis, &min_a, &max_a);
		se_physics_project_box3d(b_center, b_half, b_rot, &axis, &min_b, &max_b);
		if (max_a < min_b || max_b < min_a) return false;
		f32 overlap = s_min(max_a, max_b) - s_max(min_a, min_b);
		if (overlap < min_overlap) {
			min_overlap = overlap;
			best_axis = axis;
		}
	}
	s_vec3 dir = s_vec3(b_center->x - a_center->x, b_center->y - a_center->y, b_center->z - a_center->z);
	if (s_vec3_dot(&dir, &best_axis) < 0.0f) {
		best_axis.x = -best_axis.x;
		best_axis.y = -best_axis.y;
		best_axis.z = -best_axis.z;
	}
	out->normal = best_axis;
	out->penetration = min_overlap;
	out->contact_point = s_vec3((a_center->x + b_center->x) * 0.5f, (a_center->y + b_center->y) * 0.5f, (a_center->z + b_center->z) * 0.5f);
	return true;
}

b8 se_physics_aabb_aabb_3d(const s_vec3 *a_min, const s_vec3 *a_max, const s_vec3 *b_min, const s_vec3 *b_max, se_physics_contact_3d *out) {
	if (a_max->x < b_min->x || a_min->x > b_max->x) return false;
	if (a_max->y < b_min->y || a_min->y > b_max->y) return false;
	if (a_max->z < b_min->z || a_min->z > b_max->z) return false;
	f32 overlap_x = s_min(a_max->x, b_max->x) - s_max(a_min->x, b_min->x);
	f32 overlap_y = s_min(a_max->y, b_max->y) - s_max(a_min->y, b_min->y);
	f32 overlap_z = s_min(a_max->z, b_max->z) - s_max(a_min->z, b_min->z);
	f32 a_center_x = (a_min->x + a_max->x) * 0.5f;
	f32 a_center_y = (a_min->y + a_max->y) * 0.5f;
	f32 a_center_z = (a_min->z + a_max->z) * 0.5f;
	f32 b_center_x = (b_min->x + b_max->x) * 0.5f;
	f32 b_center_y = (b_min->y + b_max->y) * 0.5f;
	f32 b_center_z = (b_min->z + b_max->z) * 0.5f;
	if (overlap_x <= overlap_y && overlap_x <= overlap_z) {
		out->normal = s_vec3(b_center_x >= a_center_x ? 1.0f : -1.0f, 0.0f, 0.0f);
		out->penetration = overlap_x;
	} else if (overlap_y <= overlap_z) {
		out->normal = s_vec3(0.0f, b_center_y >= a_center_y ? 1.0f : -1.0f, 0.0f);
		out->penetration = overlap_y;
	} else {
		out->normal = s_vec3(0.0f, 0.0f, b_center_z >= a_center_z ? 1.0f : -1.0f);
		out->penetration = overlap_z;
	}
	out->contact_point = s_vec3(
		(a_min->x + a_max->x + b_min->x + b_max->x) * 0.25f,
		(a_min->y + a_max->y + b_min->y + b_max->y) * 0.25f,
		(a_min->z + a_max->z + b_min->z + b_max->z) * 0.25f);
	return true;
}

b8 se_physics_mesh_sphere_3d(const se_physics_mesh_3d *mesh, const s_vec3 *mesh_pos, const s_vec3 *mesh_rot, const s_vec3 *sphere_pos, f32 radius, se_physics_contact_3d *out) {
	if (!mesh->vertices || mesh->vertex_count < 3) return false;
	sz tri_count = mesh->index_count ? mesh->index_count / 3 : mesh->vertex_count / 3;
	for (sz t = 0; t < tri_count; t++) {
		u32 i0 = mesh->indices ? mesh->indices[t * 3 + 0] : (u32)(t * 3 + 0);
		u32 i1 = mesh->indices ? mesh->indices[t * 3 + 1] : (u32)(t * 3 + 1);
		u32 i2 = mesh->indices ? mesh->indices[t * 3 + 2] : (u32)(t * 3 + 2);
		if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) continue;
		s_vec3 a = se_physics_rotate_vec3(&mesh->vertices[i0], mesh_rot);
		s_vec3 b = se_physics_rotate_vec3(&mesh->vertices[i1], mesh_rot);
		s_vec3 c = se_physics_rotate_vec3(&mesh->vertices[i2], mesh_rot);
		a.x += mesh_pos->x; a.y += mesh_pos->y; a.z += mesh_pos->z;
		b.x += mesh_pos->x; b.y += mesh_pos->y; b.z += mesh_pos->z;
		c.x += mesh_pos->x; c.y += mesh_pos->y; c.z += mesh_pos->z;
		s_vec3 closest = se_physics_closest_point_on_triangle_3d(&a, &b, &c, sphere_pos);
		s_vec3 diff = s_vec3(sphere_pos->x - closest.x, sphere_pos->y - closest.y, sphere_pos->z - closest.z);
		f32 dist_sq = se_physics_vec3_length_sq(&diff);
		if (dist_sq <= radius * radius) {
			f32 dist = sqrtf(dist_sq);
			if (dist < SE_PHYSICS_EPSILON) {
				out->normal = s_vec3(0.0f, 1.0f, 0.0f);
				out->penetration = radius;
				out->contact_point = closest;
			} else {
				out->normal = s_vec3(diff.x / dist, diff.y / dist, diff.z / dist);
				out->penetration = radius - dist;
				out->contact_point = closest;
			}
			return true;
		}
	}
	return false;
}

s_vec3 se_physics_triangle_centroid_3d(const se_physics_mesh_3d *mesh, u32 tri_index) {
	u32 i0 = mesh->indices ? mesh->indices[tri_index * 3 + 0] : tri_index * 3 + 0;
	u32 i1 = mesh->indices ? mesh->indices[tri_index * 3 + 1] : tri_index * 3 + 1;
	u32 i2 = mesh->indices ? mesh->indices[tri_index * 3 + 2] : tri_index * 3 + 2;
	s_vec3 a = mesh->vertices[i0];
	s_vec3 b = mesh->vertices[i1];
	s_vec3 c = mesh->vertices[i2];
	return s_vec3((a.x + b.x + c.x) / 3.0f, (a.y + b.y + c.y) / 3.0f, (a.z + b.z + c.z) / 3.0f);
}

f32 se_physics_triangle_centroid_axis_3d(const se_physics_mesh_3d *mesh, u32 tri_index, i32 axis) {
	s_vec3 c = se_physics_triangle_centroid_3d(mesh, tri_index);
	if (axis == 0) return c.x;
	if (axis == 1) return c.y;
	return c.z;
}

void se_physics_sort_triangles_3d(const se_physics_mesh_3d *mesh, u32 *triangles, i32 axis, i32 left, i32 right) {
	if (left >= right) return;
	i32 i = left;
	i32 j = right;
	f32 pivot = se_physics_triangle_centroid_axis_3d(mesh, triangles[(left + right) / 2], axis);
	while (i <= j) {
		while (se_physics_triangle_centroid_axis_3d(mesh, triangles[i], axis) < pivot) i++;
		while (se_physics_triangle_centroid_axis_3d(mesh, triangles[j], axis) > pivot) j--;
		if (i <= j) {
			u32 tmp = triangles[i];
			triangles[i] = triangles[j];
			triangles[j] = tmp;
			i++;
			j--;
		}
	}
	if (left < j) se_physics_sort_triangles_3d(mesh, triangles, axis, left, j);
	if (i < right) se_physics_sort_triangles_3d(mesh, triangles, axis, i, right);
}

se_box_3d se_physics_triangle_bounds_3d(const se_physics_mesh_3d *mesh, u32 tri_index) {
	u32 i0 = mesh->indices ? mesh->indices[tri_index * 3 + 0] : tri_index * 3 + 0;
	u32 i1 = mesh->indices ? mesh->indices[tri_index * 3 + 1] : tri_index * 3 + 1;
	u32 i2 = mesh->indices ? mesh->indices[tri_index * 3 + 2] : tri_index * 3 + 2;
	s_vec3 a = mesh->vertices[i0];
	s_vec3 b = mesh->vertices[i1];
	s_vec3 c = mesh->vertices[i2];
	se_box_3d bounds = {0};
	bounds.min = s_vec3(s_min(a.x, s_min(b.x, c.x)), s_min(a.y, s_min(b.y, c.y)), s_min(a.z, s_min(b.z, c.z)));
	bounds.max = s_vec3(s_max(a.x, s_max(b.x, c.x)), s_max(a.y, s_max(b.y, c.y)), s_max(a.z, s_max(b.z, c.z)));
	return bounds;
}

u32 se_physics_bvh_build_3d(se_physics_shape_3d *shape, u32 start, u32 count) {
	u32 node_index = (u32)s_array_get_size(&shape->bvh_nodes);
	s_handle node_handle = s_array_increment(&shape->bvh_nodes);
	se_physics_bvh_node_3d *node = s_array_get(&shape->bvh_nodes, node_handle);
	memset(node, 0, sizeof(*node));

	se_box_3d bounds = {0};
	bounds.min = s_vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	bounds.max = s_vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (u32 i = 0; i < count; i++) {
		u32 tri = shape->bvh_triangles[start + i];
		se_box_3d tri_bounds = se_physics_triangle_bounds_3d(&shape->mesh, tri);
		bounds.min.x = s_min(bounds.min.x, tri_bounds.min.x);
		bounds.min.y = s_min(bounds.min.y, tri_bounds.min.y);
		bounds.min.z = s_min(bounds.min.z, tri_bounds.min.z);
		bounds.max.x = s_max(bounds.max.x, tri_bounds.max.x);
		bounds.max.y = s_max(bounds.max.y, tri_bounds.max.y);
		bounds.max.z = s_max(bounds.max.z, tri_bounds.max.z);
	}
	node->bounds = bounds;
	if (count <= 4) {
		node->is_leaf = true;
		node->start = start;
		node->count = count;
		return node_index;
	}
	f32 extent_x = bounds.max.x - bounds.min.x;
	f32 extent_y = bounds.max.y - bounds.min.y;
	f32 extent_z = bounds.max.z - bounds.min.z;
	i32 axis = 0;
	if (extent_x >= extent_y && extent_x >= extent_z) axis = 0;
	else if (extent_y >= extent_z) axis = 1;
	else axis = 2;
	se_physics_sort_triangles_3d(&shape->mesh, shape->bvh_triangles, axis, (i32)start, (i32)(start + count - 1));
	u32 mid = count / 2;
	node->left = se_physics_bvh_build_3d(shape, start, mid);
	node->right = se_physics_bvh_build_3d(shape, start + mid, count - mid);
	return node_index;
}

b8 se_physics_box_triangle_3d(const s_vec3 *box_center, const s_vec3 *half, const s_vec3 *rot, const s_vec3 *a, const s_vec3 *b, const s_vec3 *c, se_physics_contact_3d *out) {
	s_vec3 axes[13];
	u32 axis_count = 0;
	s_vec3 box_axes[3];
	se_physics_box3d_axes(rot, &box_axes[0], &box_axes[1], &box_axes[2]);
	for (u32 i = 0; i < 3; i++) axes[axis_count++] = box_axes[i];
	s_vec3 ab = s_vec3(b->x - a->x, b->y - a->y, b->z - a->z);
	s_vec3 bc = s_vec3(c->x - b->x, c->y - b->y, c->z - b->z);
	s_vec3 ca = s_vec3(a->x - c->x, a->y - c->y, a->z - c->z);
	s_vec3 normal = s_vec3_cross(&ab, &bc);
	f32 normal_len = s_vec3_length(&normal);
	if (normal_len > SE_PHYSICS_EPSILON) {
		axes[axis_count++] = s_vec3_divs(&normal, normal_len);
	}
	s_vec3 edges[3] = { ab, bc, ca };
	for (u32 e = 0; e < 3; e++) {
		for (u32 i = 0; i < 3; i++) {
			s_vec3 axis = s_vec3_cross(&edges[e], &box_axes[i]);
			f32 len = s_vec3_length(&axis);
			if (len > SE_PHYSICS_EPSILON) {
				axes[axis_count++] = s_vec3_divs(&axis, len);
			}
		}
	}
	f32 min_overlap = FLT_MAX;
	s_vec3 best_axis = s_vec3(0.0f, 1.0f, 0.0f);
	for (u32 i = 0; i < axis_count; i++) {
		s_vec3 axis = axes[i];
		f32 min_box, max_box;
		se_physics_project_box3d(box_center, half, rot, &axis, &min_box, &max_box);
		f32 proj0 = s_vec3_dot(a, &axis);
		f32 proj1 = s_vec3_dot(b, &axis);
		f32 proj2 = s_vec3_dot(c, &axis);
		f32 min_tri = s_min(proj0, s_min(proj1, proj2));
		f32 max_tri = s_max(proj0, s_max(proj1, proj2));
		if (max_box < min_tri || max_tri < min_box) return false;
		f32 overlap = s_min(max_box, max_tri) - s_max(min_box, min_tri);
		if (overlap < min_overlap) {
			min_overlap = overlap;
			best_axis = axis;
		}
	}
	s_vec3 tri_center = s_vec3((a->x + b->x + c->x) / 3.0f, (a->y + b->y + c->y) / 3.0f, (a->z + b->z + c->z) / 3.0f);
	s_vec3 dir = s_vec3(tri_center.x - box_center->x, tri_center.y - box_center->y, tri_center.z - box_center->z);
	if (s_vec3_dot(&dir, &best_axis) < 0.0f) {
		best_axis.x = -best_axis.x;
		best_axis.y = -best_axis.y;
		best_axis.z = -best_axis.z;
	}
	out->normal = best_axis;
	out->penetration = min_overlap;
	out->contact_point = tri_center;
	return true;
}

b8 se_physics_mesh_box_3d(const se_physics_mesh_3d *mesh, const s_vec3 *mesh_pos, const s_vec3 *mesh_rot, const s_vec3 *box_center, const s_vec3 *half, const s_vec3 *rotation, se_physics_contact_3d *out) {
	if (!mesh->vertices || mesh->vertex_count < 3) return false;
	sz tri_count = mesh->index_count ? mesh->index_count / 3 : mesh->vertex_count / 3;
	for (sz t = 0; t < tri_count; t++) {
		u32 i0 = mesh->indices ? mesh->indices[t * 3 + 0] : (u32)(t * 3 + 0);
		u32 i1 = mesh->indices ? mesh->indices[t * 3 + 1] : (u32)(t * 3 + 1);
		u32 i2 = mesh->indices ? mesh->indices[t * 3 + 2] : (u32)(t * 3 + 2);
		if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) continue;
		s_vec3 a = se_physics_rotate_vec3(&mesh->vertices[i0], mesh_rot);
		s_vec3 b = se_physics_rotate_vec3(&mesh->vertices[i1], mesh_rot);
		s_vec3 c = se_physics_rotate_vec3(&mesh->vertices[i2], mesh_rot);
		a.x += mesh_pos->x; a.y += mesh_pos->y; a.z += mesh_pos->z;
		b.x += mesh_pos->x; b.y += mesh_pos->y; b.z += mesh_pos->z;
		c.x += mesh_pos->x; c.y += mesh_pos->y; c.z += mesh_pos->z;
		if (se_physics_box_triangle_3d(box_center, half, rotation, &a, &b, &c, out)) {
			out->normal.x = -out->normal.x;
			out->normal.y = -out->normal.y;
			out->normal.z = -out->normal.z;
			return true;
		}
	}
	return false;
}

b8 se_physics_ray_circle_2d(const s_vec2 *origin, const s_vec2 *dir, f32 max_distance, const s_vec2 *center, f32 radius, f32 *out_t, s_vec2 *out_normal) {
	s_vec2 m = s_vec2(origin->x - center->x, origin->y - center->y);
	f32 a = s_vec2_dot(dir, dir);
	if (a <= SE_PHYSICS_EPSILON) return false;
	f32 b = s_vec2_dot(&m, dir);
	f32 c = s_vec2_dot(&m, &m) - radius * radius;
	f32 disc = b * b - a * c;
	if (disc < 0.0f) return false;
	f32 sqrt_disc = sqrtf(disc);
	f32 t = (-b - sqrt_disc) / a;
	if (t < 0.0f) t = (-b + sqrt_disc) / a;
	if (t < 0.0f || t > max_distance) return false;
	if (out_t) *out_t = t;
	if (out_normal) {
		s_vec2 hit = s_vec2(origin->x + dir->x * t, origin->y + dir->y * t);
		s_vec2 diff = s_vec2(hit.x - center->x, hit.y - center->y);
		f32 len = s_vec2_length(&diff);
		if (len < SE_PHYSICS_EPSILON) {
			*out_normal = s_vec2(0.0f, 1.0f);
		} else {
			*out_normal = s_vec2(diff.x / len, diff.y / len);
		}
	}
	return true;
}

b8 se_physics_ray_sphere_3d(const s_vec3 *origin, const s_vec3 *dir, f32 max_distance, const s_vec3 *center, f32 radius, f32 *out_t, s_vec3 *out_normal) {
	s_vec3 m = s_vec3(origin->x - center->x, origin->y - center->y, origin->z - center->z);
	f32 a = s_vec3_dot(dir, dir);
	if (a <= SE_PHYSICS_EPSILON) return false;
	f32 b = s_vec3_dot(&m, dir);
	f32 c = s_vec3_dot(&m, &m) - radius * radius;
	f32 disc = b * b - a * c;
	if (disc < 0.0f) return false;
	f32 sqrt_disc = sqrtf(disc);
	f32 t = (-b - sqrt_disc) / a;
	if (t < 0.0f) t = (-b + sqrt_disc) / a;
	if (t < 0.0f || t > max_distance) return false;
	if (out_t) *out_t = t;
	if (out_normal) {
		s_vec3 hit = s_vec3(origin->x + dir->x * t, origin->y + dir->y * t, origin->z + dir->z * t);
		s_vec3 n = s_vec3(hit.x - center->x, hit.y - center->y, hit.z - center->z);
		f32 len = s_vec3_length(&n);
		if (len < SE_PHYSICS_EPSILON) {
			*out_normal = s_vec3(0.0f, 1.0f, 0.0f);
		} else {
			*out_normal = s_vec3_divs(&n, len);
		}
	}
	return true;
}

b8 se_physics_ray_triangle_3d(const s_vec3 *origin, const s_vec3 *dir, f32 max_distance, const s_vec3 *a, const s_vec3 *b, const s_vec3 *c, f32 *out_t, s_vec3 *out_normal) {
	s_vec3 edge1 = s_vec3(b->x - a->x, b->y - a->y, b->z - a->z);
	s_vec3 edge2 = s_vec3(c->x - a->x, c->y - a->y, c->z - a->z);
	s_vec3 pvec = s_vec3_cross(dir, &edge2);
	f32 det = s_vec3_dot(&edge1, &pvec);
	if (fabsf(det) < SE_PHYSICS_EPSILON) return false;
	f32 inv_det = 1.0f / det;
	s_vec3 tvec = s_vec3(origin->x - a->x, origin->y - a->y, origin->z - a->z);
	f32 u = s_vec3_dot(&tvec, &pvec) * inv_det;
	if (u < 0.0f || u > 1.0f) return false;
	s_vec3 qvec = s_vec3_cross(&tvec, &edge1);
	f32 v = s_vec3_dot(dir, &qvec) * inv_det;
	if (v < 0.0f || u + v > 1.0f) return false;
	f32 t = s_vec3_dot(&edge2, &qvec) * inv_det;
	if (t < 0.0f || t > max_distance) return false;
	if (out_t) *out_t = t;
	if (out_normal) {
		s_vec3 n = s_vec3_cross(&edge1, &edge2);
		f32 len = s_vec3_length(&n);
		if (len < SE_PHYSICS_EPSILON) {
			*out_normal = s_vec3(0.0f, 1.0f, 0.0f);
		} else {
			*out_normal = s_vec3_divs(&n, len);
		}
	}
	return true;
}

void se_physics_apply_impulse_2d(se_physics_body_2d *body, const s_vec2 *impulse, const s_vec2 *point) {
	if (body->inv_mass == 0.0f) return;
	body->velocity.x += impulse->x * body->inv_mass;
	body->velocity.y += impulse->y * body->inv_mass;
	if (point) {
		s_vec2 r = s_vec2(point->x - body->position.x, point->y - body->position.y);
		f32 cross = s_vec2_cross(&r, impulse);
		body->angular_velocity += cross * body->inv_inertia;
	}
}

void se_physics_apply_impulse_3d(se_physics_body_3d *body, const s_vec3 *impulse, const s_vec3 *point) {
	if (body->inv_mass == 0.0f) return;
	body->velocity.x += impulse->x * body->inv_mass;
	body->velocity.y += impulse->y * body->inv_mass;
	body->velocity.z += impulse->z * body->inv_mass;
	if (point) {
		s_vec3 r = s_vec3(point->x - body->position.x, point->y - body->position.y, point->z - body->position.z);
		s_vec3 cross = s_vec3_cross(&r, impulse);
		body->angular_velocity.x += cross.x * body->inv_inertia;
		body->angular_velocity.y += cross.y * body->inv_inertia;
		body->angular_velocity.z += cross.z * body->inv_inertia;
	}
}

void se_physics_resolve_contact_2d(se_physics_contact_2d *contact) {
	se_physics_body_2d *a = contact->a;
	se_physics_body_2d *b = contact->b;
	f32 inv_mass_sum = a->inv_mass + b->inv_mass;
	if (inv_mass_sum == 0.0f) return;

	s_vec2 ra = s_vec2(contact->contact_point.x - a->position.x, contact->contact_point.y - a->position.y);
	s_vec2 rb = s_vec2(contact->contact_point.x - b->position.x, contact->contact_point.y - b->position.y);
	s_vec2 va = s_vec2(a->velocity.x - a->angular_velocity * ra.y, a->velocity.y + a->angular_velocity * ra.x);
	s_vec2 vb = s_vec2(b->velocity.x - b->angular_velocity * rb.y, b->velocity.y + b->angular_velocity * rb.x);
	s_vec2 rv = s_vec2(vb.x - va.x, vb.y - va.y);
	f32 vel_along_normal = s_vec2_dot(&rv, &contact->normal);
	if (vel_along_normal > 0.0f) return;

	f32 ra_cross_n = s_vec2_cross(&ra, &contact->normal);
	f32 rb_cross_n = s_vec2_cross(&rb, &contact->normal);
	f32 denom = inv_mass_sum + ra_cross_n * ra_cross_n * a->inv_inertia + rb_cross_n * rb_cross_n * b->inv_inertia;
	if (denom <= SE_PHYSICS_EPSILON) return;
	f32 e = contact->restitution;
	f32 j = -(1.0f + e) * vel_along_normal;
	j /= denom;
	s_vec2 impulse = s_vec2(contact->normal.x * j, contact->normal.y * j);
	se_physics_apply_impulse_2d(a, &s_vec2(-impulse.x, -impulse.y), &contact->contact_point);
	se_physics_apply_impulse_2d(b, &impulse, &contact->contact_point);

	va = s_vec2(a->velocity.x - a->angular_velocity * ra.y, a->velocity.y + a->angular_velocity * ra.x);
	vb = s_vec2(b->velocity.x - b->angular_velocity * rb.y, b->velocity.y + b->angular_velocity * rb.x);
	rv = s_vec2(vb.x - va.x, vb.y - va.y);
	s_vec2 tangent = s_vec2(rv.x - contact->normal.x * s_vec2_dot(&rv, &contact->normal), rv.y - contact->normal.y * s_vec2_dot(&rv, &contact->normal));
	if (se_physics_vec2_length_sq(&tangent) > SE_PHYSICS_EPSILON) {
		tangent = s_vec2_normalize(&tangent);
		f32 ra_cross_t = s_vec2_cross(&ra, &tangent);
		f32 rb_cross_t = s_vec2_cross(&rb, &tangent);
		f32 tangent_denom = inv_mass_sum + ra_cross_t * ra_cross_t * a->inv_inertia + rb_cross_t * rb_cross_t * b->inv_inertia;
		if (tangent_denom <= SE_PHYSICS_EPSILON) {
			return;
		}
		f32 jt = -s_vec2_dot(&rv, &tangent);
		jt /= tangent_denom;
		f32 mu = contact->friction;
		f32 max_friction = fabsf(j) * mu;
		jt = se_physics_clamp(jt, -max_friction, max_friction);
		s_vec2 friction_impulse = s_vec2(tangent.x * jt, tangent.y * jt);
		se_physics_apply_impulse_2d(a, &s_vec2(-friction_impulse.x, -friction_impulse.y), &contact->contact_point);
		se_physics_apply_impulse_2d(b, &friction_impulse, &contact->contact_point);
	}
}

void se_physics_resolve_contact_3d(se_physics_contact_3d *contact) {
	se_physics_body_3d *a = contact->a;
	se_physics_body_3d *b = contact->b;
	f32 inv_mass_sum = a->inv_mass + b->inv_mass;
	if (inv_mass_sum == 0.0f) return;

	s_vec3 ra = s_vec3(contact->contact_point.x - a->position.x, contact->contact_point.y - a->position.y, contact->contact_point.z - a->position.z);
	s_vec3 rb = s_vec3(contact->contact_point.x - b->position.x, contact->contact_point.y - b->position.y, contact->contact_point.z - b->position.z);
	s_vec3 ra_cross = s_vec3_cross(&a->angular_velocity, &ra);
	s_vec3 rb_cross = s_vec3_cross(&b->angular_velocity, &rb);
	s_vec3 va = s_vec3(a->velocity.x + ra_cross.x, a->velocity.y + ra_cross.y, a->velocity.z + ra_cross.z);
	s_vec3 vb = s_vec3(b->velocity.x + rb_cross.x, b->velocity.y + rb_cross.y, b->velocity.z + rb_cross.z);
	s_vec3 rv = s_vec3(vb.x - va.x, vb.y - va.y, vb.z - va.z);
	f32 vel_along_normal = s_vec3_dot(&rv, &contact->normal);
	if (vel_along_normal > 0.0f) return;

	s_vec3 ra_cn = s_vec3_cross(&ra, &contact->normal);
	s_vec3 rb_cn = s_vec3_cross(&rb, &contact->normal);
	f32 denom = inv_mass_sum + s_vec3_dot(&ra_cn, &ra_cn) * a->inv_inertia + s_vec3_dot(&rb_cn, &rb_cn) * b->inv_inertia;
	if (denom <= SE_PHYSICS_EPSILON) return;
	f32 e = contact->restitution;
	f32 j = -(1.0f + e) * vel_along_normal;
	j /= denom;
	s_vec3 impulse = s_vec3(contact->normal.x * j, contact->normal.y * j, contact->normal.z * j);
	se_physics_apply_impulse_3d(a, &s_vec3(-impulse.x, -impulse.y, -impulse.z), &contact->contact_point);
	se_physics_apply_impulse_3d(b, &impulse, &contact->contact_point);

	ra_cross = s_vec3_cross(&a->angular_velocity, &ra);
	rb_cross = s_vec3_cross(&b->angular_velocity, &rb);
	va = s_vec3(a->velocity.x + ra_cross.x, a->velocity.y + ra_cross.y, a->velocity.z + ra_cross.z);
	vb = s_vec3(b->velocity.x + rb_cross.x, b->velocity.y + rb_cross.y, b->velocity.z + rb_cross.z);
	rv = s_vec3(vb.x - va.x, vb.y - va.y, vb.z - va.z);
	s_vec3 tangent = s_vec3(rv.x - contact->normal.x * s_vec3_dot(&rv, &contact->normal),
		rv.y - contact->normal.y * s_vec3_dot(&rv, &contact->normal),
		rv.z - contact->normal.z * s_vec3_dot(&rv, &contact->normal));
	if (se_physics_vec3_length_sq(&tangent) > SE_PHYSICS_EPSILON) {
		f32 len = s_vec3_length(&tangent);
		tangent = s_vec3_divs(&tangent, len);
		s_vec3 ra_ct = s_vec3_cross(&ra, &tangent);
		s_vec3 rb_ct = s_vec3_cross(&rb, &tangent);
		f32 tangent_denom = inv_mass_sum + s_vec3_dot(&ra_ct, &ra_ct) * a->inv_inertia + s_vec3_dot(&rb_ct, &rb_ct) * b->inv_inertia;
		if (tangent_denom <= SE_PHYSICS_EPSILON) {
			return;
		}
		f32 jt = -s_vec3_dot(&rv, &tangent);
		jt /= tangent_denom;
		f32 mu = contact->friction;
		f32 max_friction = fabsf(j) * mu;
		jt = se_physics_clamp(jt, -max_friction, max_friction);
		s_vec3 friction_impulse = s_vec3(tangent.x * jt, tangent.y * jt, tangent.z * jt);
		se_physics_apply_impulse_3d(a, &s_vec3(-friction_impulse.x, -friction_impulse.y, -friction_impulse.z), &contact->contact_point);
		se_physics_apply_impulse_3d(b, &friction_impulse, &contact->contact_point);
	}
}

void se_physics_positional_correction_2d(se_physics_contact_2d *contact) {
	se_physics_body_2d *a = contact->a;
	se_physics_body_2d *b = contact->b;
	f32 inv_mass_sum = a->inv_mass + b->inv_mass;
	if (inv_mass_sum == 0.0f) return;
	const f32 percent = 0.2f;
	const f32 slop = 0.01f;
	f32 correction_mag = s_max(contact->penetration - slop, 0.0f) / inv_mass_sum * percent;
	s_vec2 correction = s_vec2(contact->normal.x * correction_mag, contact->normal.y * correction_mag);
	a->position.x -= correction.x * a->inv_mass;
	a->position.y -= correction.y * a->inv_mass;
	b->position.x += correction.x * b->inv_mass;
	b->position.y += correction.y * b->inv_mass;
}

void se_physics_positional_correction_3d(se_physics_contact_3d *contact) {
	se_physics_body_3d *a = contact->a;
	se_physics_body_3d *b = contact->b;
	f32 inv_mass_sum = a->inv_mass + b->inv_mass;
	if (inv_mass_sum == 0.0f) return;
	const f32 percent = 0.2f;
	const f32 slop = 0.01f;
	f32 correction_mag = s_max(contact->penetration - slop, 0.0f) / inv_mass_sum * percent;
	s_vec3 correction = s_vec3(contact->normal.x * correction_mag, contact->normal.y * correction_mag, contact->normal.z * correction_mag);
	a->position.x -= correction.x * a->inv_mass;
	a->position.y -= correction.y * a->inv_mass;
	a->position.z -= correction.z * a->inv_mass;
	b->position.x += correction.x * b->inv_mass;
	b->position.y += correction.y * b->inv_mass;
	b->position.z += correction.z * b->inv_mass;
}

b8 se_physics_shapes_collide_2d(se_physics_body_2d *a, se_physics_shape_2d *shape_a, se_physics_body_2d *b, se_physics_shape_2d *shape_b, se_physics_contact_2d *out) {
	se_box_2d aabb_a = se_physics_shape_2d_world_aabb(a, shape_a);
	se_box_2d aabb_b = se_physics_shape_2d_world_aabb(b, shape_b);
	if (!se_box_2d_intersects(&aabb_a, &aabb_b)) return false;

	s_vec2 a_offset = se_physics_rotate_vec2(&shape_a->offset, a->rotation);
	s_vec2 b_offset = se_physics_rotate_vec2(&shape_b->offset, b->rotation);
	s_vec2 a_center = s_vec2(a->position.x + a_offset.x, a->position.y + a_offset.y);
	s_vec2 b_center = s_vec2(b->position.x + b_offset.x, b->position.y + b_offset.y);

	if (shape_a->type == SE_PHYSICS_SHAPE_2D_CIRCLE && shape_b->type == SE_PHYSICS_SHAPE_2D_CIRCLE) {
		return se_physics_circle_circle(&a_center, shape_a->circle.radius, &b_center, shape_b->circle.radius, out);
	}

	if (shape_a->type == SE_PHYSICS_SHAPE_2D_CIRCLE && (shape_b->type == SE_PHYSICS_SHAPE_2D_BOX || shape_b->type == SE_PHYSICS_SHAPE_2D_AABB)) {
		f32 rot = b->rotation + shape_b->rotation;
		if (shape_b->type == SE_PHYSICS_SHAPE_2D_AABB || shape_b->box.is_aabb) rot = 0.0f;
		b8 hit = se_physics_circle_box(&a_center, shape_a->circle.radius, &b_center, &shape_b->box.half_extents, rot, out);
		if (hit) {
			out->normal.x = -out->normal.x;
			out->normal.y = -out->normal.y;
		}
		return hit;
	}

	if (shape_b->type == SE_PHYSICS_SHAPE_2D_CIRCLE && (shape_a->type == SE_PHYSICS_SHAPE_2D_BOX || shape_a->type == SE_PHYSICS_SHAPE_2D_AABB)) {
		f32 rot = a->rotation + shape_a->rotation;
		if (shape_a->type == SE_PHYSICS_SHAPE_2D_AABB || shape_a->box.is_aabb) rot = 0.0f;
		return se_physics_circle_box(&b_center, shape_b->circle.radius, &a_center, &shape_a->box.half_extents, rot, out);
	}

	if ((shape_a->type == SE_PHYSICS_SHAPE_2D_BOX || shape_a->type == SE_PHYSICS_SHAPE_2D_AABB) && (shape_b->type == SE_PHYSICS_SHAPE_2D_BOX || shape_b->type == SE_PHYSICS_SHAPE_2D_AABB)) {
		if ((shape_a->type == SE_PHYSICS_SHAPE_2D_AABB || shape_a->box.is_aabb) && (shape_b->type == SE_PHYSICS_SHAPE_2D_AABB || shape_b->box.is_aabb)) {
			return se_physics_aabb_aabb_2d(&aabb_a.min, &aabb_a.max, &aabb_b.min, &aabb_b.max, out);
		}
		f32 rot_a = (shape_a->type == SE_PHYSICS_SHAPE_2D_AABB || shape_a->box.is_aabb) ? 0.0f : a->rotation + shape_a->rotation;
		f32 rot_b = (shape_b->type == SE_PHYSICS_SHAPE_2D_AABB || shape_b->box.is_aabb) ? 0.0f : b->rotation + shape_b->rotation;
		return se_physics_box_box_2d(&a_center, &shape_a->box.half_extents, rot_a, &b_center, &shape_b->box.half_extents, rot_b, out);
	}

	if (shape_a->type == SE_PHYSICS_SHAPE_2D_MESH && shape_b->type == SE_PHYSICS_SHAPE_2D_CIRCLE) {
		f32 rot = a->rotation + shape_a->rotation;
		return se_physics_mesh_circle_2d(&shape_a->mesh, &a_center, rot, &b_center, shape_b->circle.radius, out);
	}
	if (shape_b->type == SE_PHYSICS_SHAPE_2D_MESH && shape_a->type == SE_PHYSICS_SHAPE_2D_CIRCLE) {
		f32 rot = b->rotation + shape_b->rotation;
		b8 hit = se_physics_mesh_circle_2d(&shape_b->mesh, &b_center, rot, &a_center, shape_a->circle.radius, out);
		if (hit) {
			out->normal.x = -out->normal.x;
			out->normal.y = -out->normal.y;
		}
		return hit;
	}
	if (shape_a->type == SE_PHYSICS_SHAPE_2D_MESH && (shape_b->type == SE_PHYSICS_SHAPE_2D_BOX || shape_b->type == SE_PHYSICS_SHAPE_2D_AABB)) {
		f32 rot_a = a->rotation + shape_a->rotation;
		f32 rot_b = (shape_b->type == SE_PHYSICS_SHAPE_2D_AABB || shape_b->box.is_aabb) ? 0.0f : b->rotation + shape_b->rotation;
		return se_physics_mesh_box_2d(&shape_a->mesh, &a_center, rot_a, &b_center, &shape_b->box.half_extents, rot_b, out);
	}
	if (shape_b->type == SE_PHYSICS_SHAPE_2D_MESH && (shape_a->type == SE_PHYSICS_SHAPE_2D_BOX || shape_a->type == SE_PHYSICS_SHAPE_2D_AABB)) {
		f32 rot_b = b->rotation + shape_b->rotation;
		f32 rot_a = (shape_a->type == SE_PHYSICS_SHAPE_2D_AABB || shape_a->box.is_aabb) ? 0.0f : a->rotation + shape_a->rotation;
		b8 hit = se_physics_mesh_box_2d(&shape_b->mesh, &b_center, rot_b, &a_center, &shape_a->box.half_extents, rot_a, out);
		if (hit) {
			out->normal.x = -out->normal.x;
			out->normal.y = -out->normal.y;
		}
		return hit;
	}

	return false;
}

b8 se_physics_shapes_collide_3d(se_physics_body_3d *a, se_physics_shape_3d *shape_a, se_physics_body_3d *b, se_physics_shape_3d *shape_b, se_physics_contact_3d *out) {
	se_box_3d aabb_a = se_physics_shape_3d_world_aabb(a, shape_a);
	se_box_3d aabb_b = se_physics_shape_3d_world_aabb(b, shape_b);
	if (!se_box_3d_intersects(&aabb_a, &aabb_b)) return false;

	s_vec3 a_offset = se_physics_rotate_vec3(&shape_a->offset, &a->rotation);
	s_vec3 b_offset = se_physics_rotate_vec3(&shape_b->offset, &b->rotation);
	s_vec3 a_center = s_vec3(a->position.x + a_offset.x, a->position.y + a_offset.y, a->position.z + a_offset.z);
	s_vec3 b_center = s_vec3(b->position.x + b_offset.x, b->position.y + b_offset.y, b->position.z + b_offset.z);

	if (shape_a->type == SE_PHYSICS_SHAPE_3D_SPHERE && shape_b->type == SE_PHYSICS_SHAPE_3D_SPHERE) {
		return se_physics_sphere_sphere(&a_center, shape_a->sphere.radius, &b_center, shape_b->sphere.radius, out);
	}
	if (shape_a->type == SE_PHYSICS_SHAPE_3D_SPHERE && (shape_b->type == SE_PHYSICS_SHAPE_3D_BOX || shape_b->type == SE_PHYSICS_SHAPE_3D_AABB)) {
		s_vec3 rot_b = shape_b->type == SE_PHYSICS_SHAPE_3D_AABB || shape_b->box.is_aabb ? s_vec3(0.0f, 0.0f, 0.0f) : s_vec3(b->rotation.x + shape_b->rotation.x, b->rotation.y + shape_b->rotation.y, b->rotation.z + shape_b->rotation.z);
		b8 hit = se_physics_sphere_box(&a_center, shape_a->sphere.radius, &b_center, &shape_b->box.half_extents, &rot_b, out);
		if (hit) {
			out->normal.x = -out->normal.x;
			out->normal.y = -out->normal.y;
			out->normal.z = -out->normal.z;
		}
		return hit;
	}
	if (shape_b->type == SE_PHYSICS_SHAPE_3D_SPHERE && (shape_a->type == SE_PHYSICS_SHAPE_3D_BOX || shape_a->type == SE_PHYSICS_SHAPE_3D_AABB)) {
		s_vec3 rot_a = shape_a->type == SE_PHYSICS_SHAPE_3D_AABB || shape_a->box.is_aabb ? s_vec3(0.0f, 0.0f, 0.0f) : s_vec3(a->rotation.x + shape_a->rotation.x, a->rotation.y + shape_a->rotation.y, a->rotation.z + shape_a->rotation.z);
		return se_physics_sphere_box(&b_center, shape_b->sphere.radius, &a_center, &shape_a->box.half_extents, &rot_a, out);
	}
	if ((shape_a->type == SE_PHYSICS_SHAPE_3D_BOX || shape_a->type == SE_PHYSICS_SHAPE_3D_AABB) && (shape_b->type == SE_PHYSICS_SHAPE_3D_BOX || shape_b->type == SE_PHYSICS_SHAPE_3D_AABB)) {
		if ((shape_a->type == SE_PHYSICS_SHAPE_3D_AABB || shape_a->box.is_aabb) && (shape_b->type == SE_PHYSICS_SHAPE_3D_AABB || shape_b->box.is_aabb)) {
			return se_physics_aabb_aabb_3d(&aabb_a.min, &aabb_a.max, &aabb_b.min, &aabb_b.max, out);
		}
		s_vec3 rot_a = shape_a->type == SE_PHYSICS_SHAPE_3D_AABB || shape_a->box.is_aabb ? s_vec3(0.0f, 0.0f, 0.0f) : s_vec3(a->rotation.x + shape_a->rotation.x, a->rotation.y + shape_a->rotation.y, a->rotation.z + shape_a->rotation.z);
		s_vec3 rot_b = shape_b->type == SE_PHYSICS_SHAPE_3D_AABB || shape_b->box.is_aabb ? s_vec3(0.0f, 0.0f, 0.0f) : s_vec3(b->rotation.x + shape_b->rotation.x, b->rotation.y + shape_b->rotation.y, b->rotation.z + shape_b->rotation.z);
		return se_physics_box_box_3d(&a_center, &shape_a->box.half_extents, &rot_a, &b_center, &shape_b->box.half_extents, &rot_b, out);
	}
	if (shape_a->type == SE_PHYSICS_SHAPE_3D_MESH && shape_b->type == SE_PHYSICS_SHAPE_3D_SPHERE) {
		s_vec3 rot_a = s_vec3(a->rotation.x + shape_a->rotation.x, a->rotation.y + shape_a->rotation.y, a->rotation.z + shape_a->rotation.z);
		return se_physics_mesh_sphere_3d(&shape_a->mesh, &a_center, &rot_a, &b_center, shape_b->sphere.radius, out);
	}
	if (shape_b->type == SE_PHYSICS_SHAPE_3D_MESH && shape_a->type == SE_PHYSICS_SHAPE_3D_SPHERE) {
		s_vec3 rot_b = s_vec3(b->rotation.x + shape_b->rotation.x, b->rotation.y + shape_b->rotation.y, b->rotation.z + shape_b->rotation.z);
		b8 hit = se_physics_mesh_sphere_3d(&shape_b->mesh, &b_center, &rot_b, &a_center, shape_a->sphere.radius, out);
		if (hit) {
			out->normal.x = -out->normal.x;
			out->normal.y = -out->normal.y;
			out->normal.z = -out->normal.z;
		}
		return hit;
	}
	if (shape_a->type == SE_PHYSICS_SHAPE_3D_MESH && (shape_b->type == SE_PHYSICS_SHAPE_3D_BOX || shape_b->type == SE_PHYSICS_SHAPE_3D_AABB)) {
		s_vec3 rot_a = s_vec3(a->rotation.x + shape_a->rotation.x, a->rotation.y + shape_a->rotation.y, a->rotation.z + shape_a->rotation.z);
		s_vec3 rot_b = shape_b->type == SE_PHYSICS_SHAPE_3D_AABB || shape_b->box.is_aabb ? s_vec3(0.0f, 0.0f, 0.0f) : s_vec3(b->rotation.x + shape_b->rotation.x, b->rotation.y + shape_b->rotation.y, b->rotation.z + shape_b->rotation.z);
		return se_physics_mesh_box_3d(&shape_a->mesh, &a_center, &rot_a, &b_center, &shape_b->box.half_extents, &rot_b, out);
	}
	if (shape_b->type == SE_PHYSICS_SHAPE_3D_MESH && (shape_a->type == SE_PHYSICS_SHAPE_3D_BOX || shape_a->type == SE_PHYSICS_SHAPE_3D_AABB)) {
		s_vec3 rot_b = s_vec3(b->rotation.x + shape_b->rotation.x, b->rotation.y + shape_b->rotation.y, b->rotation.z + shape_b->rotation.z);
		s_vec3 rot_a = shape_a->type == SE_PHYSICS_SHAPE_3D_AABB || shape_a->box.is_aabb ? s_vec3(0.0f, 0.0f, 0.0f) : s_vec3(a->rotation.x + shape_a->rotation.x, a->rotation.y + shape_a->rotation.y, a->rotation.z + shape_a->rotation.z);
		b8 hit = se_physics_mesh_box_3d(&shape_b->mesh, &b_center, &rot_b, &a_center, &shape_a->box.half_extents, &rot_a, out);
		if (hit) {
			out->normal.x = -out->normal.x;
			out->normal.y = -out->normal.y;
			out->normal.z = -out->normal.z;
		}
		return hit;
	}

	return false;
}

se_physics_world_2d *se_physics_world_2d_create(const se_physics_world_params_2d *params) {
	se_physics_world_params_2d cfg = SE_PHYSICS_WORLD_PARAMS_2D_DEFAULTS;
	if (params) {
		cfg = *params;
		if (cfg.bodies_count == 0) cfg.bodies_count = SE_PHYSICS_WORLD_PARAMS_2D_DEFAULTS.bodies_count;
		if (cfg.shapes_per_body == 0) cfg.shapes_per_body = SE_PHYSICS_WORLD_PARAMS_2D_DEFAULTS.shapes_per_body;
		if (cfg.contacts_count == 0) cfg.contacts_count = SE_PHYSICS_WORLD_PARAMS_2D_DEFAULTS.contacts_count;
		if (cfg.solver_iterations == 0) cfg.solver_iterations = SE_PHYSICS_WORLD_PARAMS_2D_DEFAULTS.solver_iterations;
	}
	se_physics_world_2d *world = malloc(sizeof(se_physics_world_2d));
	if (!world) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	memset(world, 0, sizeof(*world));
	world->gravity = cfg.gravity;
	world->solver_iterations = cfg.solver_iterations;
	world->shapes_per_body = cfg.shapes_per_body;
	world->on_contact = cfg.on_contact;
	world->user_data = cfg.user_data;
	s_array_init(&world->bodies);
	s_array_reserve(&world->bodies, cfg.bodies_count);
	s_array_init(&world->contacts);
	s_array_reserve(&world->contacts, cfg.contacts_count);
	se_set_last_error(SE_RESULT_OK);
	return world;
}

void se_physics_world_2d_destroy(se_physics_world_2d *world) {
	if (!world) return;
	se_physics_body_2d *body = NULL;
	s_foreach(&world->bodies, body) {
		if (!body->is_valid) continue;
		se_physics_shape_2d *shape = NULL;
		s_foreach(&body->shapes, shape) {
			if (shape->bvh_triangles) {
				free(shape->bvh_triangles);
				shape->bvh_triangles = NULL;
			}
			if (s_array_get_capacity(&shape->bvh_nodes) > 0) {
				s_array_clear(&shape->bvh_nodes);
			}
			shape->bvh_built = false;
		}
		s_array_clear(&body->shapes);
		body->is_valid = false;
	}
	s_array_clear(&world->bodies);
	s_array_clear(&world->contacts);
	free(world);
}

se_physics_world_3d *se_physics_world_3d_create(const se_physics_world_params_3d *params) {
	se_physics_world_params_3d cfg = SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS;
	if (params) {
		cfg = *params;
		if (cfg.bodies_count == 0) cfg.bodies_count = SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS.bodies_count;
		if (cfg.shapes_per_body == 0) cfg.shapes_per_body = SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS.shapes_per_body;
		if (cfg.contacts_count == 0) cfg.contacts_count = SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS.contacts_count;
		if (cfg.solver_iterations == 0) cfg.solver_iterations = SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS.solver_iterations;
	}
	se_physics_world_3d *world = malloc(sizeof(se_physics_world_3d));
	if (!world) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	memset(world, 0, sizeof(*world));
	world->gravity = cfg.gravity;
	world->solver_iterations = cfg.solver_iterations;
	world->shapes_per_body = cfg.shapes_per_body;
	world->on_contact = cfg.on_contact;
	world->user_data = cfg.user_data;
	s_array_init(&world->bodies);
	s_array_reserve(&world->bodies, cfg.bodies_count);
	s_array_init(&world->contacts);
	s_array_reserve(&world->contacts, cfg.contacts_count);
	se_set_last_error(SE_RESULT_OK);
	return world;
}

void se_physics_world_3d_destroy(se_physics_world_3d *world) {
	if (!world) return;
	se_physics_body_3d *body = NULL;
	s_foreach(&world->bodies, body) {
		if (!body->is_valid) continue;
		se_physics_shape_3d *shape = NULL;
		s_foreach(&body->shapes, shape) {
			if (shape->bvh_triangles) {
				free(shape->bvh_triangles);
				shape->bvh_triangles = NULL;
			}
			if (s_array_get_capacity(&shape->bvh_nodes) > 0) {
				s_array_clear(&shape->bvh_nodes);
			}
			shape->bvh_built = false;
		}
		s_array_clear(&body->shapes);
		body->is_valid = false;
	}
	s_array_clear(&world->bodies);
	s_array_clear(&world->contacts);
	free(world);
}

se_physics_body_2d *se_physics_body_2d_create(se_physics_world_2d *world, const se_physics_body_params_2d *params) {
	if (!world) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_physics_body_params_2d cfg = SE_PHYSICS_BODY_PARAMS_2D_DEFAULTS;
	if (params) cfg = *params;
	cfg.restitution = se_physics_clamp(cfg.restitution, 0.0f, 1.0f);
	cfg.friction = se_physics_maxf(cfg.friction, 0.0f);
	cfg.linear_damping = se_physics_maxf(cfg.linear_damping, 0.0f);
	cfg.angular_damping = se_physics_maxf(cfg.angular_damping, 0.0f);
	if (cfg.type == SE_PHYSICS_BODY_DYNAMIC && cfg.mass <= SE_PHYSICS_EPSILON) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_physics_body_2d *body = NULL;
	s_foreach(&world->bodies, body) {
		se_physics_body_2d *slot = body;
		if (!slot->is_valid) {
			body = slot;
			break;
		}
	}
	if (!body) {
		if (s_array_get_capacity(&world->bodies) == s_array_get_size(&world->bodies)) {
			se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
			return NULL;
		}
		s_handle body_handle = s_array_increment(&world->bodies);
		body = s_array_get(&world->bodies, body_handle);
	}
	memset(body, 0, sizeof(*body));
	body->type = cfg.type;
	body->position = cfg.position;
	body->rotation = cfg.rotation;
	body->velocity = cfg.velocity;
	body->angular_velocity = cfg.angular_velocity;
	body->mass = cfg.mass;
	body->restitution = cfg.restitution;
	body->friction = cfg.friction;
	body->linear_damping = cfg.linear_damping;
	body->angular_damping = cfg.angular_damping;
	if (body->type != SE_PHYSICS_BODY_DYNAMIC) {
		body->mass = 0.0f;
	}
	s_array_init(&body->shapes);
	s_array_reserve(&body->shapes, world->shapes_per_body);
	body->is_valid = true;
	se_physics_body_2d_update_mass(body);
	se_set_last_error(SE_RESULT_OK);
	return body;
}

void se_physics_body_2d_destroy(se_physics_world_2d *world, se_physics_body_2d *body) {
	if (!world || !body) return;
	if (!body->is_valid) return;
	se_physics_shape_2d *shape = NULL;
	s_foreach(&body->shapes, shape) {
		if (shape->bvh_triangles) {
			free(shape->bvh_triangles);
			shape->bvh_triangles = NULL;
		}
		if (s_array_get_capacity(&shape->bvh_nodes) > 0) {
			s_array_clear(&shape->bvh_nodes);
		}
		shape->bvh_built = false;
	}
	s_array_clear(&body->shapes);
	body->is_valid = false;
}

se_physics_body_3d *se_physics_body_3d_create(se_physics_world_3d *world, const se_physics_body_params_3d *params) {
	if (!world) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_physics_body_params_3d cfg = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
	if (params) cfg = *params;
	cfg.restitution = se_physics_clamp(cfg.restitution, 0.0f, 1.0f);
	cfg.friction = se_physics_maxf(cfg.friction, 0.0f);
	cfg.linear_damping = se_physics_maxf(cfg.linear_damping, 0.0f);
	cfg.angular_damping = se_physics_maxf(cfg.angular_damping, 0.0f);
	if (cfg.type == SE_PHYSICS_BODY_DYNAMIC && cfg.mass <= SE_PHYSICS_EPSILON) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_physics_body_3d *body = NULL;
	s_foreach(&world->bodies, body) {
		se_physics_body_3d *slot = body;
		if (!slot->is_valid) {
			body = slot;
			break;
		}
	}
	if (!body) {
		if (s_array_get_capacity(&world->bodies) == s_array_get_size(&world->bodies)) {
			se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
			return NULL;
		}
		s_handle body_handle = s_array_increment(&world->bodies);
		body = s_array_get(&world->bodies, body_handle);
	}
	memset(body, 0, sizeof(*body));
	body->type = cfg.type;
	body->position = cfg.position;
	body->rotation = cfg.rotation;
	body->velocity = cfg.velocity;
	body->angular_velocity = cfg.angular_velocity;
	body->mass = cfg.mass;
	body->restitution = cfg.restitution;
	body->friction = cfg.friction;
	body->linear_damping = cfg.linear_damping;
	body->angular_damping = cfg.angular_damping;
	if (body->type != SE_PHYSICS_BODY_DYNAMIC) {
		body->mass = 0.0f;
	}
	s_array_init(&body->shapes);
	s_array_reserve(&body->shapes, world->shapes_per_body);
	body->is_valid = true;
	se_physics_body_3d_update_mass(body);
	se_set_last_error(SE_RESULT_OK);
	return body;
}

void se_physics_body_3d_destroy(se_physics_world_3d *world, se_physics_body_3d *body) {
	if (!world || !body) return;
	if (!body->is_valid) return;
	se_physics_shape_3d *shape = NULL;
	s_foreach(&body->shapes, shape) {
		if (shape->bvh_triangles) {
			free(shape->bvh_triangles);
			shape->bvh_triangles = NULL;
		}
		if (s_array_get_capacity(&shape->bvh_nodes) > 0) {
			s_array_clear(&shape->bvh_nodes);
		}
		shape->bvh_built = false;
	}
	s_array_clear(&body->shapes);
	body->is_valid = false;
}

se_physics_shape_2d *se_physics_body_2d_add_circle(se_physics_body_2d *body, const s_vec2 *offset, const f32 radius, const b8 is_trigger) {
	if (!body || !offset || radius <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_size(&body->shapes) >= s_array_get_capacity(&body->shapes)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}
	s_handle shape_handle = s_array_increment(&body->shapes);
	se_physics_shape_2d *shape = s_array_get(&body->shapes, shape_handle);
	memset(shape, 0, sizeof(*shape));
	shape->type = SE_PHYSICS_SHAPE_2D_CIRCLE;
	shape->offset = *offset;
	shape->circle.radius = radius;
	shape->local_bounds.min = s_vec2(-radius, -radius);
	shape->local_bounds.max = s_vec2(radius, radius);
	shape->is_trigger = is_trigger;
	se_physics_body_2d_update_mass(body);
	se_set_last_error(SE_RESULT_OK);
	return shape;
}

se_physics_shape_2d *se_physics_body_2d_add_aabb(se_physics_body_2d *body, const s_vec2 *offset, const s_vec2 *half_extents, const b8 is_trigger) {
	if (!body || !offset || !half_extents || half_extents->x <= 0.0f || half_extents->y <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_size(&body->shapes) >= s_array_get_capacity(&body->shapes)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}
	s_handle shape_handle = s_array_increment(&body->shapes);
	se_physics_shape_2d *shape = s_array_get(&body->shapes, shape_handle);
	memset(shape, 0, sizeof(*shape));
	shape->type = SE_PHYSICS_SHAPE_2D_AABB;
	shape->offset = *offset;
	shape->box.half_extents = *half_extents;
	shape->box.is_aabb = true;
	shape->local_bounds.min = s_vec2(-half_extents->x, -half_extents->y);
	shape->local_bounds.max = s_vec2(half_extents->x, half_extents->y);
	shape->is_trigger = is_trigger;
	se_physics_body_2d_update_mass(body);
	se_set_last_error(SE_RESULT_OK);
	return shape;
}

se_physics_shape_2d *se_physics_body_2d_add_box(se_physics_body_2d *body, const s_vec2 *offset, const s_vec2 *half_extents, const f32 rotation, const b8 is_trigger) {
	if (!body || !offset || !half_extents || half_extents->x <= 0.0f || half_extents->y <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_size(&body->shapes) >= s_array_get_capacity(&body->shapes)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}
	s_handle shape_handle = s_array_increment(&body->shapes);
	se_physics_shape_2d *shape = s_array_get(&body->shapes, shape_handle);
	memset(shape, 0, sizeof(*shape));
	shape->type = SE_PHYSICS_SHAPE_2D_BOX;
	shape->offset = *offset;
	shape->rotation = rotation;
	shape->box.half_extents = *half_extents;
	shape->box.is_aabb = false;
	shape->local_bounds.min = s_vec2(-half_extents->x, -half_extents->y);
	shape->local_bounds.max = s_vec2(half_extents->x, half_extents->y);
	shape->is_trigger = is_trigger;
	se_physics_body_2d_update_mass(body);
	se_set_last_error(SE_RESULT_OK);
	return shape;
}

se_physics_shape_2d *se_physics_body_2d_add_mesh(se_physics_body_2d *body, const se_physics_mesh_2d *mesh, const s_vec2 *offset, const f32 rotation, const b8 is_trigger) {
	if (!body || !mesh || !offset || !mesh->vertices || mesh->vertex_count < 3) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_size(&body->shapes) >= s_array_get_capacity(&body->shapes)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}
	s_handle shape_handle = s_array_increment(&body->shapes);
	se_physics_shape_2d *shape = s_array_get(&body->shapes, shape_handle);
	memset(shape, 0, sizeof(*shape));
	shape->type = SE_PHYSICS_SHAPE_2D_MESH;
	shape->offset = *offset;
	shape->rotation = rotation;
	shape->mesh = *mesh;
	shape->is_trigger = is_trigger;
	se_physics_shape_2d_bounds_from_mesh(shape);
	shape->bvh_triangle_count = shape->mesh.index_count ? shape->mesh.index_count / 3 : shape->mesh.vertex_count / 3;
	if (shape->bvh_triangle_count > 0) {
		shape->bvh_triangles = malloc(sizeof(u32) * shape->bvh_triangle_count);
		if (shape->bvh_triangles) {
			for (u32 i = 0; i < shape->bvh_triangle_count; i++) {
				shape->bvh_triangles[i] = i;
			}
			s_array_init(&shape->bvh_nodes);
			s_array_reserve(&shape->bvh_nodes, shape->bvh_triangle_count * 2);
			se_physics_bvh_build_2d(shape, 0, (u32)shape->bvh_triangle_count);
			shape->bvh_built = true;
		}
	}
	se_physics_body_2d_update_mass(body);
	se_set_last_error(SE_RESULT_OK);
	return shape;
}

se_physics_shape_3d *se_physics_body_3d_add_sphere(se_physics_body_3d *body, const s_vec3 *offset, const f32 radius, const b8 is_trigger) {
	if (!body || !offset || radius <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_size(&body->shapes) >= s_array_get_capacity(&body->shapes)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}
	s_handle shape_handle = s_array_increment(&body->shapes);
	se_physics_shape_3d *shape = s_array_get(&body->shapes, shape_handle);
	memset(shape, 0, sizeof(*shape));
	shape->type = SE_PHYSICS_SHAPE_3D_SPHERE;
	shape->offset = *offset;
	shape->sphere.radius = radius;
	shape->local_bounds.min = s_vec3(-radius, -radius, -radius);
	shape->local_bounds.max = s_vec3(radius, radius, radius);
	shape->is_trigger = is_trigger;
	se_physics_body_3d_update_mass(body);
	se_set_last_error(SE_RESULT_OK);
	return shape;
}

se_physics_shape_3d *se_physics_body_3d_add_aabb(se_physics_body_3d *body, const s_vec3 *offset, const s_vec3 *half_extents, const b8 is_trigger) {
	if (!body || !offset || !half_extents || half_extents->x <= 0.0f || half_extents->y <= 0.0f || half_extents->z <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_size(&body->shapes) >= s_array_get_capacity(&body->shapes)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}
	s_handle shape_handle = s_array_increment(&body->shapes);
	se_physics_shape_3d *shape = s_array_get(&body->shapes, shape_handle);
	memset(shape, 0, sizeof(*shape));
	shape->type = SE_PHYSICS_SHAPE_3D_AABB;
	shape->offset = *offset;
	shape->box.half_extents = *half_extents;
	shape->box.is_aabb = true;
	shape->local_bounds.min = s_vec3(-half_extents->x, -half_extents->y, -half_extents->z);
	shape->local_bounds.max = s_vec3(half_extents->x, half_extents->y, half_extents->z);
	shape->is_trigger = is_trigger;
	se_physics_body_3d_update_mass(body);
	se_set_last_error(SE_RESULT_OK);
	return shape;
}

se_physics_shape_3d *se_physics_body_3d_add_box(se_physics_body_3d *body, const s_vec3 *offset, const s_vec3 *half_extents, const s_vec3 *rotation, const b8 is_trigger) {
	if (!body || !offset || !half_extents || !rotation || half_extents->x <= 0.0f || half_extents->y <= 0.0f || half_extents->z <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_size(&body->shapes) >= s_array_get_capacity(&body->shapes)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}
	s_handle shape_handle = s_array_increment(&body->shapes);
	se_physics_shape_3d *shape = s_array_get(&body->shapes, shape_handle);
	memset(shape, 0, sizeof(*shape));
	shape->type = SE_PHYSICS_SHAPE_3D_BOX;
	shape->offset = *offset;
	shape->rotation = *rotation;
	shape->box.half_extents = *half_extents;
	shape->box.is_aabb = false;
	shape->local_bounds.min = s_vec3(-half_extents->x, -half_extents->y, -half_extents->z);
	shape->local_bounds.max = s_vec3(half_extents->x, half_extents->y, half_extents->z);
	shape->is_trigger = is_trigger;
	se_physics_body_3d_update_mass(body);
	se_set_last_error(SE_RESULT_OK);
	return shape;
}

se_physics_shape_3d *se_physics_body_3d_add_mesh(se_physics_body_3d *body, const se_physics_mesh_3d *mesh, const s_vec3 *offset, const s_vec3 *rotation, const b8 is_trigger) {
	if (!body || !mesh || !offset || !rotation || !mesh->vertices || mesh->vertex_count < 3) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_size(&body->shapes) >= s_array_get_capacity(&body->shapes)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}
	s_handle shape_handle = s_array_increment(&body->shapes);
	se_physics_shape_3d *shape = s_array_get(&body->shapes, shape_handle);
	memset(shape, 0, sizeof(*shape));
	shape->type = SE_PHYSICS_SHAPE_3D_MESH;
	shape->offset = *offset;
	shape->rotation = *rotation;
	shape->mesh = *mesh;
	shape->is_trigger = is_trigger;
	se_physics_shape_3d_bounds_from_mesh(shape);
	shape->bvh_triangle_count = shape->mesh.index_count ? shape->mesh.index_count / 3 : shape->mesh.vertex_count / 3;
	if (shape->bvh_triangle_count > 0) {
		shape->bvh_triangles = malloc(sizeof(u32) * shape->bvh_triangle_count);
		if (shape->bvh_triangles) {
			for (u32 i = 0; i < shape->bvh_triangle_count; i++) {
				shape->bvh_triangles[i] = i;
			}
			s_array_init(&shape->bvh_nodes);
			s_array_reserve(&shape->bvh_nodes, shape->bvh_triangle_count * 2);
			se_physics_bvh_build_3d(shape, 0, (u32)shape->bvh_triangle_count);
			shape->bvh_built = true;
		}
	}
	se_physics_body_3d_update_mass(body);
	se_set_last_error(SE_RESULT_OK);
	return shape;
}

void se_physics_body_2d_set_position(se_physics_body_2d *body, const s_vec2 *position) {
	if (!body || !position) return;
	body->position = *position;
}

void se_physics_body_2d_set_velocity(se_physics_body_2d *body, const s_vec2 *velocity) {
	if (!body || !velocity) return;
	body->velocity = *velocity;
}

void se_physics_body_2d_set_rotation(se_physics_body_2d *body, const f32 rotation) {
	if (!body) return;
	body->rotation = rotation;
}

void se_physics_body_2d_apply_force(se_physics_body_2d *body, const s_vec2 *force) {
	if (!body || !force || body->inv_mass == 0.0f) return;
	body->force.x += force->x;
	body->force.y += force->y;
}

void se_physics_body_2d_apply_impulse(se_physics_body_2d *body, const s_vec2 *impulse, const s_vec2 *point) {
	if (!body || !impulse) return;
	se_physics_apply_impulse_2d(body, impulse, point);
}

void se_physics_body_3d_set_position(se_physics_body_3d *body, const s_vec3 *position) {
	if (!body || !position) return;
	body->position = *position;
}

void se_physics_body_3d_set_velocity(se_physics_body_3d *body, const s_vec3 *velocity) {
	if (!body || !velocity) return;
	body->velocity = *velocity;
}

void se_physics_body_3d_set_rotation(se_physics_body_3d *body, const s_vec3 *rotation) {
	if (!body || !rotation) return;
	body->rotation = *rotation;
}

void se_physics_body_3d_apply_force(se_physics_body_3d *body, const s_vec3 *force) {
	if (!body || !force || body->inv_mass == 0.0f) return;
	body->force.x += force->x;
	body->force.y += force->y;
	body->force.z += force->z;
}

void se_physics_body_3d_apply_impulse(se_physics_body_3d *body, const s_vec3 *impulse, const s_vec3 *point) {
	if (!body || !impulse) return;
	se_physics_apply_impulse_3d(body, impulse, point);
}

void se_physics_world_2d_set_gravity(se_physics_world_2d *world, const s_vec2 *gravity) {
	if (!world || !gravity) return;
	world->gravity = *gravity;
}

void se_physics_world_3d_set_gravity(se_physics_world_3d *world, const s_vec3 *gravity) {
	if (!world || !gravity) return;
	world->gravity = *gravity;
}

static void se_physics_integrate_body_2d(se_physics_body_2d *body, const s_vec2 *gravity, f32 dt) {
	if (body->inv_mass == 0.0f) return;
	f32 linear_damping = se_physics_maxf(body->linear_damping, 0.0f);
	f32 angular_damping = se_physics_maxf(body->angular_damping, 0.0f);
	f32 linear_factor = expf(-linear_damping * dt);
	f32 angular_factor = expf(-angular_damping * dt);
	body->velocity.x += (gravity->x + body->force.x * body->inv_mass) * dt;
	body->velocity.y += (gravity->y + body->force.y * body->inv_mass) * dt;
	body->velocity.x *= linear_factor;
	body->velocity.y *= linear_factor;
	body->position.x += body->velocity.x * dt;
	body->position.y += body->velocity.y * dt;
	body->angular_velocity += body->torque * body->inv_inertia * dt;
	body->angular_velocity *= angular_factor;
	body->rotation += body->angular_velocity * dt;
	body->force = s_vec2(0.0f, 0.0f);
	body->torque = 0.0f;
}

static void se_physics_integrate_body_3d(se_physics_body_3d *body, const s_vec3 *gravity, f32 dt) {
	if (body->inv_mass == 0.0f) return;
	f32 linear_damping = se_physics_maxf(body->linear_damping, 0.0f);
	f32 angular_damping = se_physics_maxf(body->angular_damping, 0.0f);
	f32 linear_factor = expf(-linear_damping * dt);
	f32 angular_factor = expf(-angular_damping * dt);
	body->velocity.x += (gravity->x + body->force.x * body->inv_mass) * dt;
	body->velocity.y += (gravity->y + body->force.y * body->inv_mass) * dt;
	body->velocity.z += (gravity->z + body->force.z * body->inv_mass) * dt;
	body->velocity.x *= linear_factor;
	body->velocity.y *= linear_factor;
	body->velocity.z *= linear_factor;
	body->position.x += body->velocity.x * dt;
	body->position.y += body->velocity.y * dt;
	body->position.z += body->velocity.z * dt;
	body->angular_velocity.x += body->torque.x * body->inv_inertia * dt;
	body->angular_velocity.y += body->torque.y * body->inv_inertia * dt;
	body->angular_velocity.z += body->torque.z * body->inv_inertia * dt;
	body->angular_velocity.x *= angular_factor;
	body->angular_velocity.y *= angular_factor;
	body->angular_velocity.z *= angular_factor;
	body->rotation.x += body->angular_velocity.x * dt;
	body->rotation.y += body->angular_velocity.y * dt;
	body->rotation.z += body->angular_velocity.z * dt;
	body->force = s_vec3(0.0f, 0.0f, 0.0f);
	body->torque = s_vec3(0.0f, 0.0f, 0.0f);
}

static void se_physics_integrate_kinematic_body_2d(se_physics_body_2d *body, f32 dt) {
	body->position.x += body->velocity.x * dt;
	body->position.y += body->velocity.y * dt;
	body->rotation += body->angular_velocity * dt;
	body->force = s_vec2(0.0f, 0.0f);
	body->torque = 0.0f;
}

static void se_physics_integrate_kinematic_body_3d(se_physics_body_3d *body, f32 dt) {
	body->position.x += body->velocity.x * dt;
	body->position.y += body->velocity.y * dt;
	body->position.z += body->velocity.z * dt;
	body->rotation.x += body->angular_velocity.x * dt;
	body->rotation.y += body->angular_velocity.y * dt;
	body->rotation.z += body->angular_velocity.z * dt;
	body->force = s_vec3(0.0f, 0.0f, 0.0f);
	body->torque = s_vec3(0.0f, 0.0f, 0.0f);
}

void se_physics_world_2d_step(se_physics_world_2d *world, const f32 dt) {
	if (!world || dt <= 0.0f) return;
	const sz contacts_capacity = s_array_get_capacity(&world->contacts);
	s_array_clear(&world->contacts);
	s_array_reserve(&world->contacts, contacts_capacity);

	se_physics_body_2d *body = NULL;
	s_foreach(&world->bodies, body) {
		if (!body->is_valid) continue;
		if (body->type == SE_PHYSICS_BODY_DYNAMIC) {
			se_physics_integrate_body_2d(body, &world->gravity, dt);
		} else if (body->type == SE_PHYSICS_BODY_KINEMATIC) {
			se_physics_integrate_kinematic_body_2d(body, dt);
		}
	}

	sz body_count = s_array_get_size(&world->bodies);
	for (sz i = 0; i < body_count; i++) {
		s_handle a_handle = s_array_handle(&world->bodies, (u32)i);
		se_physics_body_2d *a = s_array_get(&world->bodies, a_handle);
		if (!a->is_valid) continue;
		for (sz j = i + 1; j < body_count; j++) {
			s_handle b_handle = s_array_handle(&world->bodies, (u32)j);
			se_physics_body_2d *b = s_array_get(&world->bodies, b_handle);
			if (!b->is_valid) continue;
				se_physics_shape_2d *shape_a = NULL;
				s_foreach(&a->shapes, shape_a) {
					se_physics_shape_2d *shape_b = NULL;
					s_foreach(&b->shapes, shape_b) {
						se_physics_contact_2d contact = {0};
						if (se_physics_shapes_collide_2d(a, shape_a, b, shape_b, &contact)) {
							contact.a = a;
							contact.b = b;
						contact.shape_a = shape_a;
						contact.shape_b = shape_b;
						contact.restitution = (a->restitution + b->restitution) * 0.5f;
						contact.friction = (a->friction + b->friction) * 0.5f;
						contact.is_trigger = (shape_a->is_trigger || shape_b->is_trigger);
						if (!contact.is_trigger && contact.a->inv_mass == 0.0f && contact.b->inv_mass == 0.0f) {
							continue;
						}
						if (s_array_get_size(&world->contacts) < s_array_get_capacity(&world->contacts)) {
							s_handle contact_handle = s_array_increment(&world->contacts);
							se_physics_contact_2d *contact_ptr = s_array_get(&world->contacts, contact_handle);
							*contact_ptr = contact;
						}
					}
				}
			}
		}
	}

	for (u32 iter = 0; iter < world->solver_iterations; iter++) {
		se_physics_contact_2d *contact = NULL;
		s_foreach(&world->contacts, contact) {
			if (contact->is_trigger) continue;
			if (contact->a->inv_mass == 0.0f && contact->b->inv_mass == 0.0f) continue;
			se_physics_resolve_contact_2d(contact);
		}
	}

	se_physics_contact_2d *contact = NULL;
	s_foreach(&world->contacts, contact) {
		if (!contact->is_trigger) {
			se_physics_positional_correction_2d(contact);
		}
		if (world->on_contact) {
			world->on_contact(contact, world->user_data);
		}
	}
}

void se_physics_world_3d_step(se_physics_world_3d *world, const f32 dt) {
	if (!world || dt <= 0.0f) return;
	const sz contacts_capacity = s_array_get_capacity(&world->contacts);
	s_array_clear(&world->contacts);
	s_array_reserve(&world->contacts, contacts_capacity);

	se_physics_body_3d *body = NULL;
	s_foreach(&world->bodies, body) {
		if (!body->is_valid) continue;
		if (body->type == SE_PHYSICS_BODY_DYNAMIC) {
			se_physics_integrate_body_3d(body, &world->gravity, dt);
		} else if (body->type == SE_PHYSICS_BODY_KINEMATIC) {
			se_physics_integrate_kinematic_body_3d(body, dt);
		}
	}

	sz body_count = s_array_get_size(&world->bodies);
	for (sz i = 0; i < body_count; i++) {
		s_handle a_handle = s_array_handle(&world->bodies, (u32)i);
		se_physics_body_3d *a = s_array_get(&world->bodies, a_handle);
		if (!a->is_valid) continue;
		for (sz j = i + 1; j < body_count; j++) {
			s_handle b_handle = s_array_handle(&world->bodies, (u32)j);
			se_physics_body_3d *b = s_array_get(&world->bodies, b_handle);
			if (!b->is_valid) continue;
				se_physics_shape_3d *shape_a = NULL;
				s_foreach(&a->shapes, shape_a) {
					se_physics_shape_3d *shape_b = NULL;
					s_foreach(&b->shapes, shape_b) {
						se_physics_contact_3d contact = {0};
						if (se_physics_shapes_collide_3d(a, shape_a, b, shape_b, &contact)) {
						contact.a = a;
						contact.b = b;
						contact.shape_a = shape_a;
						contact.shape_b = shape_b;
						contact.restitution = (a->restitution + b->restitution) * 0.5f;
						contact.friction = (a->friction + b->friction) * 0.5f;
						contact.is_trigger = (shape_a->is_trigger || shape_b->is_trigger);
						if (!contact.is_trigger && contact.a->inv_mass == 0.0f && contact.b->inv_mass == 0.0f) {
							continue;
						}
						if (s_array_get_size(&world->contacts) < s_array_get_capacity(&world->contacts)) {
							s_handle contact_handle = s_array_increment(&world->contacts);
							se_physics_contact_3d *contact_ptr = s_array_get(&world->contacts, contact_handle);
							*contact_ptr = contact;
						}
					}
				}
			}
		}
	}

	for (u32 iter = 0; iter < world->solver_iterations; iter++) {
		se_physics_contact_3d *contact = NULL;
		s_foreach(&world->contacts, contact) {
			if (contact->is_trigger) continue;
			if (contact->a->inv_mass == 0.0f && contact->b->inv_mass == 0.0f) continue;
			se_physics_resolve_contact_3d(contact);
		}
	}

	se_physics_contact_3d *contact = NULL;
	s_foreach(&world->contacts, contact) {
		if (!contact->is_trigger) {
			se_physics_positional_correction_3d(contact);
		}
		if (world->on_contact) {
			world->on_contact(contact, world->user_data);
		}
	}
}

se_physics_world_2d *se_physics_example_2d_create(void) {
	se_physics_world_2d *world = se_physics_world_2d_create(NULL);
	if (!world) return NULL;
	se_physics_body_params_2d ground_params = SE_PHYSICS_BODY_PARAMS_2D_DEFAULTS;
	ground_params.type = SE_PHYSICS_BODY_STATIC;
	ground_params.position = s_vec2(0.0f, -5.0f);
	se_physics_body_2d *ground = se_physics_body_2d_create(world, &ground_params);
	if (ground) {
		se_physics_body_2d_add_aabb(ground, &s_vec2(0.0f, 0.0f), &s_vec2(10.0f, 1.0f), false);
	}
	for (u32 i = 0; i < 5; i++) {
		se_physics_body_params_2d params = SE_PHYSICS_BODY_PARAMS_2D_DEFAULTS;
		params.position = s_vec2(-2.0f + (f32)i, 2.0f + (f32)i);
		se_physics_body_2d *body = se_physics_body_2d_create(world, &params);
		if (body) {
			se_physics_body_2d_add_circle(body, &s_vec2(0.0f, 0.0f), 0.5f, false);
		}
	}
	return world;
}

se_physics_world_3d *se_physics_example_3d_create(void) {
	se_physics_world_3d *world = se_physics_world_3d_create(NULL);
	if (!world) return NULL;
	se_physics_body_params_3d ground_params = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
	ground_params.type = SE_PHYSICS_BODY_STATIC;
	ground_params.position = s_vec3(0.0f, -5.0f, 0.0f);
	se_physics_body_3d *ground = se_physics_body_3d_create(world, &ground_params);
	if (ground) {
		se_physics_body_3d_add_aabb(ground, &s_vec3(0.0f, 0.0f, 0.0f), &s_vec3(10.0f, 1.0f, 10.0f), false);
	}
	for (u32 i = 0; i < 5; i++) {
		se_physics_body_params_3d params = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
		params.position = s_vec3(-2.0f + (f32)i, 2.0f + (f32)i, 0.0f);
		se_physics_body_3d *body = se_physics_body_3d_create(world, &params);
		if (body) {
			se_physics_body_3d_add_sphere(body, &s_vec3(0.0f, 0.0f, 0.0f), 0.5f, false);
		}
	}
	return world;
}

b8 se_physics_raycast_shape_2d(se_physics_body_2d *body, se_physics_shape_2d *shape, const s_vec2 *origin, const s_vec2 *dir, f32 max_distance, f32 *out_t, s_vec2 *out_normal) {
	s_vec2 offset = se_physics_rotate_vec2(&shape->offset, body->rotation);
	s_vec2 center = s_vec2(body->position.x + offset.x, body->position.y + offset.y);
	if (shape->type == SE_PHYSICS_SHAPE_2D_CIRCLE) {
		return se_physics_ray_circle_2d(origin, dir, max_distance, &center, shape->circle.radius, out_t, out_normal);
	}
	if (shape->type == SE_PHYSICS_SHAPE_2D_AABB || (shape->type == SE_PHYSICS_SHAPE_2D_BOX && shape->box.is_aabb)) {
		se_box_2d box = se_physics_shape_2d_world_aabb(body, shape);
		f32 t = 0.0f;
		if (!se_physics_ray_aabb_2d(origin, dir, &box, &t)) return false;
		if (t < 0.0f || t > max_distance) return false;
		if (out_t) *out_t = t;
		if (out_normal) {
			s_vec2 hit = s_vec2(origin->x + dir->x * t, origin->y + dir->y * t);
			f32 dx = s_min(fabsf(hit.x - box.min.x), fabsf(hit.x - box.max.x));
			f32 dy = s_min(fabsf(hit.y - box.min.y), fabsf(hit.y - box.max.y));
			*out_normal = dx < dy ? s_vec2((hit.x < (box.min.x + box.max.x) * 0.5f) ? -1.0f : 1.0f, 0.0f) : s_vec2(0.0f, (hit.y < (box.min.y + box.max.y) * 0.5f) ? -1.0f : 1.0f);
		}
		return true;
	}
	if (shape->type == SE_PHYSICS_SHAPE_2D_BOX) {
		f32 rot = body->rotation + shape->rotation;
		s_vec2 local_origin = s_vec2(origin->x - center.x, origin->y - center.y);
		local_origin = se_physics_rotate_vec2_inv(&local_origin, rot);
		s_vec2 local_dir = se_physics_rotate_vec2_inv(dir, rot);
		se_box_2d box = { .min = s_vec2(-shape->box.half_extents.x, -shape->box.half_extents.y), .max = s_vec2(shape->box.half_extents.x, shape->box.half_extents.y) };
		f32 t = 0.0f;
		if (!se_physics_ray_aabb_2d(&local_origin, &local_dir, &box, &t)) return false;
		if (t < 0.0f || t > max_distance) return false;
		if (out_t) *out_t = t;
		if (out_normal) {
			s_vec2 hit = s_vec2(local_origin.x + local_dir.x * t, local_origin.y + local_dir.y * t);
			f32 dx = shape->box.half_extents.x - fabsf(hit.x);
			f32 dy = shape->box.half_extents.y - fabsf(hit.y);
			s_vec2 n = dx < dy ? s_vec2((hit.x < 0.0f) ? -1.0f : 1.0f, 0.0f) : s_vec2(0.0f, (hit.y < 0.0f) ? -1.0f : 1.0f);
			*out_normal = se_physics_rotate_vec2(&n, rot);
		}
		return true;
	}
	if (shape->type == SE_PHYSICS_SHAPE_2D_MESH && shape->bvh_built) {
		f32 best_t = max_distance;
		b8 hit = false;
		sz node_count = s_array_get_size(&shape->bvh_nodes);
		u32 *stack = malloc(sizeof(u32) * (node_count ? node_count : 1));
		u32 stack_size = 0;
		if (!stack) return false;
		stack[stack_size++] = 0;
		f32 rot = body->rotation + shape->rotation;
		while (stack_size > 0) {
			u32 node_index = stack[--stack_size];
			se_physics_bvh_node_2d *node = s_array_get(&shape->bvh_nodes, node_index);
			se_box_2d node_box = node->bounds;
			se_box_2d world_box = {0};
			s_vec2 corners[4] = {
				node_box.min,
				s_vec2(node_box.max.x, node_box.min.y),
				node_box.max,
				s_vec2(node_box.min.x, node_box.max.y)
			};
			s_vec2 min_v = s_vec2(FLT_MAX, FLT_MAX);
			s_vec2 max_v = s_vec2(-FLT_MAX, -FLT_MAX);
			for (u32 i = 0; i < 4; i++) {
				s_vec2 w = se_physics_rotate_vec2(&corners[i], rot);
				w.x += center.x;
				w.y += center.y;
				min_v.x = s_min(min_v.x, w.x);
				min_v.y = s_min(min_v.y, w.y);
				max_v.x = s_max(max_v.x, w.x);
				max_v.y = s_max(max_v.y, w.y);
			}
			world_box.min = min_v;
			world_box.max = max_v;
			f32 t = 0.0f;
			if (!se_physics_ray_aabb_2d(origin, dir, &world_box, &t) || t > best_t) {
				continue;
			}
			if (node->is_leaf) {
				for (u32 i = 0; i < node->count; i++) {
					u32 tri = shape->bvh_triangles[node->start + i];
					u32 i0 = shape->mesh.indices ? shape->mesh.indices[tri * 3 + 0] : tri * 3 + 0;
					u32 i1 = shape->mesh.indices ? shape->mesh.indices[tri * 3 + 1] : tri * 3 + 1;
					u32 i2 = shape->mesh.indices ? shape->mesh.indices[tri * 3 + 2] : tri * 3 + 2;
					s_vec2 a = se_physics_rotate_vec2(&shape->mesh.vertices[i0], rot);
					s_vec2 b = se_physics_rotate_vec2(&shape->mesh.vertices[i1], rot);
					s_vec2 c = se_physics_rotate_vec2(&shape->mesh.vertices[i2], rot);
					a.x += center.x; a.y += center.y;
					b.x += center.x; b.y += center.y;
					c.x += center.x; c.y += center.y;
					s_vec2 edges[3][2] = { { a, b }, { b, c }, { c, a } };
					for (u32 e = 0; e < 3; e++) {
						s_vec2 p1 = edges[e][0];
						s_vec2 p2 = edges[e][1];
						s_vec2 v1 = s_vec2(p2.x - p1.x, p2.y - p1.y);
						f32 denom = s_vec2_cross(dir, &v1);
						if (fabsf(denom) < SE_PHYSICS_EPSILON) continue;
						s_vec2 diff = s_vec2(p1.x - origin->x, p1.y - origin->y);
						f32 t_ray = s_vec2_cross(&diff, &v1) / denom;
						f32 t_seg = s_vec2_cross(&diff, dir) / denom;
						if (t_ray >= 0.0f && t_ray <= best_t && t_seg >= 0.0f && t_seg <= 1.0f) {
							best_t = t_ray;
							hit = true;
							if (out_normal) {
								s_vec2 edge = s_vec2(p2.x - p1.x, p2.y - p1.y);
								s_vec2 perp = se_physics_perp(&edge);
								s_vec2 n = s_vec2_normalize(&perp);
								*out_normal = n;
							}
						}
					}
				}
			} else {
				if (node->left < node_count) stack[stack_size++] = node->left;
				if (node->right < node_count) stack[stack_size++] = node->right;
			}
		}
		free(stack);
		if (hit) {
			if (out_t) *out_t = best_t;
		}
		return hit;
	}
	return false;
}

b8 se_physics_raycast_shape_3d(se_physics_body_3d *body, se_physics_shape_3d *shape, const s_vec3 *origin, const s_vec3 *dir, f32 max_distance, f32 *out_t, s_vec3 *out_normal) {
	s_vec3 offset = se_physics_rotate_vec3(&shape->offset, &body->rotation);
	s_vec3 center = s_vec3(body->position.x + offset.x, body->position.y + offset.y, body->position.z + offset.z);
	if (shape->type == SE_PHYSICS_SHAPE_3D_SPHERE) {
		return se_physics_ray_sphere_3d(origin, dir, max_distance, &center, shape->sphere.radius, out_t, out_normal);
	}
	if (shape->type == SE_PHYSICS_SHAPE_3D_AABB || (shape->type == SE_PHYSICS_SHAPE_3D_BOX && shape->box.is_aabb)) {
		se_box_3d box = se_physics_shape_3d_world_aabb(body, shape);
		f32 t = 0.0f;
		if (!se_physics_ray_aabb_3d(origin, dir, &box, &t)) return false;
		if (t < 0.0f || t > max_distance) return false;
		if (out_t) *out_t = t;
		if (out_normal) {
			s_vec3 hit = s_vec3(origin->x + dir->x * t, origin->y + dir->y * t, origin->z + dir->z * t);
			f32 dx = s_min(fabsf(hit.x - box.min.x), fabsf(hit.x - box.max.x));
			f32 dy = s_min(fabsf(hit.y - box.min.y), fabsf(hit.y - box.max.y));
			f32 dz = s_min(fabsf(hit.z - box.min.z), fabsf(hit.z - box.max.z));
			if (dx <= dy && dx <= dz) *out_normal = s_vec3((hit.x < (box.min.x + box.max.x) * 0.5f) ? -1.0f : 1.0f, 0.0f, 0.0f);
			else if (dy <= dz) *out_normal = s_vec3(0.0f, (hit.y < (box.min.y + box.max.y) * 0.5f) ? -1.0f : 1.0f, 0.0f);
			else *out_normal = s_vec3(0.0f, 0.0f, (hit.z < (box.min.z + box.max.z) * 0.5f) ? -1.0f : 1.0f);
		}
		return true;
	}
	if (shape->type == SE_PHYSICS_SHAPE_3D_BOX) {
		s_vec3 rot = s_vec3(body->rotation.x + shape->rotation.x, body->rotation.y + shape->rotation.y, body->rotation.z + shape->rotation.z);
		s_vec3 local_origin = s_vec3(origin->x - center.x, origin->y - center.y, origin->z - center.z);
		local_origin = se_physics_rotate_vec3_inv(&local_origin, &rot);
		s_vec3 local_dir = se_physics_rotate_vec3_inv(dir, &rot);
		se_box_3d box = { .min = s_vec3(-shape->box.half_extents.x, -shape->box.half_extents.y, -shape->box.half_extents.z), .max = s_vec3(shape->box.half_extents.x, shape->box.half_extents.y, shape->box.half_extents.z) };
		f32 t = 0.0f;
		if (!se_physics_ray_aabb_3d(&local_origin, &local_dir, &box, &t)) return false;
		if (t < 0.0f || t > max_distance) return false;
		if (out_t) *out_t = t;
		if (out_normal) {
			s_vec3 hit = s_vec3(local_origin.x + local_dir.x * t, local_origin.y + local_dir.y * t, local_origin.z + local_dir.z * t);
			f32 dx = shape->box.half_extents.x - fabsf(hit.x);
			f32 dy = shape->box.half_extents.y - fabsf(hit.y);
			f32 dz = shape->box.half_extents.z - fabsf(hit.z);
			s_vec3 n = s_vec3(0.0f, 1.0f, 0.0f);
			if (dx <= dy && dx <= dz) n = s_vec3((hit.x < 0.0f) ? -1.0f : 1.0f, 0.0f, 0.0f);
			else if (dy <= dz) n = s_vec3(0.0f, (hit.y < 0.0f) ? -1.0f : 1.0f, 0.0f);
			else n = s_vec3(0.0f, 0.0f, (hit.z < 0.0f) ? -1.0f : 1.0f);
			*out_normal = se_physics_rotate_vec3(&n, &rot);
		}
		return true;
	}
	if (shape->type == SE_PHYSICS_SHAPE_3D_MESH && shape->bvh_built) {
		f32 best_t = max_distance;
		b8 hit = false;
		sz node_count = s_array_get_size(&shape->bvh_nodes);
		u32 *stack = malloc(sizeof(u32) * (node_count ? node_count : 1));
		u32 stack_size = 0;
		if (!stack) return false;
		stack[stack_size++] = 0;
		s_vec3 rot = s_vec3(body->rotation.x + shape->rotation.x, body->rotation.y + shape->rotation.y, body->rotation.z + shape->rotation.z);
		while (stack_size > 0) {
			u32 node_index = stack[--stack_size];
			se_physics_bvh_node_3d *node = s_array_get(&shape->bvh_nodes, node_index);
			se_box_3d node_box = node->bounds;
			se_box_3d world_box = {0};
			s_vec3 corners[8] = {
				node_box.min,
				s_vec3(node_box.max.x, node_box.min.y, node_box.min.z),
				s_vec3(node_box.max.x, node_box.max.y, node_box.min.z),
				s_vec3(node_box.min.x, node_box.max.y, node_box.min.z),
				s_vec3(node_box.min.x, node_box.min.y, node_box.max.z),
				s_vec3(node_box.max.x, node_box.min.y, node_box.max.z),
				s_vec3(node_box.max.x, node_box.max.y, node_box.max.z),
				node_box.max
			};
			s_vec3 min_v = s_vec3(FLT_MAX, FLT_MAX, FLT_MAX);
			s_vec3 max_v = s_vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			for (u32 i = 0; i < 8; i++) {
				s_vec3 w = se_physics_rotate_vec3(&corners[i], &rot);
				w.x += center.x;
				w.y += center.y;
				w.z += center.z;
				min_v.x = s_min(min_v.x, w.x);
				min_v.y = s_min(min_v.y, w.y);
				min_v.z = s_min(min_v.z, w.z);
				max_v.x = s_max(max_v.x, w.x);
				max_v.y = s_max(max_v.y, w.y);
				max_v.z = s_max(max_v.z, w.z);
			}
			world_box.min = min_v;
			world_box.max = max_v;
			f32 t = 0.0f;
			if (!se_physics_ray_aabb_3d(origin, dir, &world_box, &t) || t > best_t) {
				continue;
			}
			if (node->is_leaf) {
				for (u32 i = 0; i < node->count; i++) {
					u32 tri = shape->bvh_triangles[node->start + i];
					u32 i0 = shape->mesh.indices ? shape->mesh.indices[tri * 3 + 0] : tri * 3 + 0;
					u32 i1 = shape->mesh.indices ? shape->mesh.indices[tri * 3 + 1] : tri * 3 + 1;
					u32 i2 = shape->mesh.indices ? shape->mesh.indices[tri * 3 + 2] : tri * 3 + 2;
					s_vec3 a = se_physics_rotate_vec3(&shape->mesh.vertices[i0], &rot);
					s_vec3 b = se_physics_rotate_vec3(&shape->mesh.vertices[i1], &rot);
					s_vec3 c = se_physics_rotate_vec3(&shape->mesh.vertices[i2], &rot);
					a.x += center.x; a.y += center.y; a.z += center.z;
					b.x += center.x; b.y += center.y; b.z += center.z;
					c.x += center.x; c.y += center.y; c.z += center.z;
					f32 tri_t = 0.0f;
					s_vec3 tri_n = s_vec3(0.0f, 1.0f, 0.0f);
					if (se_physics_ray_triangle_3d(origin, dir, best_t, &a, &b, &c, &tri_t, &tri_n)) {
						best_t = tri_t;
						hit = true;
						if (out_normal) {
							*out_normal = tri_n;
						}
					}
				}
			} else {
				if (node->left < node_count) stack[stack_size++] = node->left;
				if (node->right < node_count) stack[stack_size++] = node->right;
			}
		}
		free(stack);
		if (hit) {
			if (out_t) *out_t = best_t;
		}
		return hit;
	}
	return false;
}

b8 se_physics_world_2d_raycast(se_physics_world_2d *world, const s_vec2 *origin, const s_vec2 *direction, const f32 max_distance, se_physics_raycast_hit_2d *out_hit) {
	if (!world || !origin || !direction || !out_hit || max_distance <= 0.0f) return false;
	f32 dir_len = s_vec2_length(direction);
	if (dir_len < SE_PHYSICS_EPSILON) return false;
	s_vec2 dir = s_vec2(direction->x / dir_len, direction->y / dir_len);
	b8 hit = false;
	f32 best_t = max_distance;
	se_physics_raycast_hit_2d best = {0};
	se_physics_body_2d *body = NULL;
	s_foreach(&world->bodies, body) {
		if (!body->is_valid) continue;
		se_physics_shape_2d *shape = NULL;
		s_foreach(&body->shapes, shape) {
			f32 t = 0.0f;
			s_vec2 normal = s_vec2(0.0f, 1.0f);
			if (se_physics_raycast_shape_2d(body, shape, origin, &dir, max_distance, &t, &normal)) {
				if (t < best_t) {
					best_t = t;
					best.body = body;
					best.shape = shape;
					best.distance = t;
					best.normal = normal;
					best.point = s_vec2(origin->x + dir.x * t, origin->y + dir.y * t);
					hit = true;
				}
			}
		}
	}
	if (hit) {
		*out_hit = best;
	}
	return hit;
}

b8 se_physics_world_3d_raycast(se_physics_world_3d *world, const s_vec3 *origin, const s_vec3 *direction, const f32 max_distance, se_physics_raycast_hit_3d *out_hit) {
	if (!world || !origin || !direction || !out_hit || max_distance <= 0.0f) return false;
	f32 dir_len = s_vec3_length(direction);
	if (dir_len < SE_PHYSICS_EPSILON) return false;
	s_vec3 dir = s_vec3(direction->x / dir_len, direction->y / dir_len, direction->z / dir_len);
	b8 hit = false;
	f32 best_t = max_distance;
	se_physics_raycast_hit_3d best = {0};
	se_physics_body_3d *body = NULL;
	s_foreach(&world->bodies, body) {
		if (!body->is_valid) continue;
		se_physics_shape_3d *shape = NULL;
		s_foreach(&body->shapes, shape) {
			f32 t = 0.0f;
			s_vec3 normal = s_vec3(0.0f, 1.0f, 0.0f);
			if (se_physics_raycast_shape_3d(body, shape, origin, &dir, max_distance, &t, &normal)) {
				if (t < best_t) {
					best_t = t;
					best.body = body;
					best.shape = shape;
					best.distance = t;
					best.normal = normal;
					best.point = s_vec3(origin->x + dir.x * t, origin->y + dir.y * t, origin->z + dir.z * t);
					hit = true;
				}
			}
		}
	}
	if (hit) {
		*out_hit = best;
	}
	return hit;
}
