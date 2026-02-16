// Syphax-Engine - Ougi Washi

#ifndef SE_QUAD_H
#define SE_QUAD_H

#include "syphax/s_array.h"

typedef struct {
	s_vec2 position;
	s_vec2 uv;
} se_vertex_2d;

typedef struct {
	s_vec3 position;
	s_vec3 normal;
	s_vec2 uv;
} se_vertex_3d;

typedef struct {
	u32 vbo;
	const void *buffer_ptr;
	sz buffer_size;
} se_instance_buffer;

typedef struct {
	u32 vao;
	s_array(se_instance_buffer, instance_buffers);
	b8 instance_buffers_dirty;
} se_mesh_instance;
typedef s_array(se_mesh_instance, se_mesh_instances);

typedef struct {
	u32 vao;
	u32 ebo;
	u32 vbo;
	s_array(se_instance_buffer, instance_buffers);
	b8 instance_buffers_dirty;
	u32 last_attribute;
} se_quad;

// Quad functions
extern void se_quad_3d_create(se_quad *out_quad);
extern void se_quad_2d_create(se_quad *out_quad, const u32 instance_count);
extern void se_quad_2d_add_instance_buffer(se_quad *quad, const s_mat4 *buffer, const sz instance_count);
extern void se_quad_2d_add_instance_buffer_mat3(se_quad *quad, const s_mat3 *buffer, const sz instance_count);
extern void se_quad_render(se_quad *quad, const sz instance_count);
extern void se_quad_destroy(se_quad *quad);

#endif // SE_QUAD_H
