// Syphax-Engine - Ougi Washi

#include "se_sdf.h"

#include "render/se_gl.h"
#include "se_camera.h"
#include "se_debug.h"
#include "se_framebuffer.h"
#include "se_shader.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SE_SDF_VERTEX_SHADER_CAPACITY 2048u
#define SE_SDF_FRAGMENT_SHADER_CAPACITY (64u * 1024u)
#define SE_SDF_FAR_DISTANCE 256.0f
#define SE_SDF_HIT_EPSILON 0.001f
#define SE_SDF_MATRIX_EPSILON 0.00001f
#define SE_SDF_HANDLE_FMT "%llu"
#define SE_SDF_HANDLE_ARG(_handle) ((unsigned long long)(_handle))

static se_sdf* se_sdf_from_handle(se_context* ctx, const se_sdf_handle sdf);
static b8 se_sdf_transform_is_zero(const s_mat4* transform);
static b8 se_sdf_has_primitive(const se_sdf* sdf);
static void se_sdf_append(c8* out, const sz capacity, const c8* fmt, ...);
static void se_sdf_gen_uniform_recursive(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_gen_function_recursive(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_upload_uniforms_recursive(const se_shader_handle shader, const se_sdf_handle sdf);

static se_sdf* se_sdf_from_handle(se_context* ctx, const se_sdf_handle sdf) {
	if (!ctx || sdf == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&ctx->sdfs, sdf);
}

static b8 se_sdf_transform_is_zero(const s_mat4* transform) {
	if (!transform) {
		return true;
	}
	for (u32 row = 0; row < 4u; ++row) {
		for (u32 col = 0; col < 4u; ++col) {
			if (fabsf(transform->m[row][col]) > SE_SDF_MATRIX_EPSILON) {
				return false;
			}
		}
	}
	return true;
}

static b8 se_sdf_has_primitive(const se_sdf* sdf) {
	return sdf && (sdf->type == SE_SDF_SPHERE || sdf->type == SE_SDF_BOX);
}

static void se_sdf_append(c8* out, const sz capacity, const c8* fmt, ...) {
	if (!out || capacity == 0 || !fmt) {
		return;
	}
	const sz current_length = strlen(out);
	if (current_length >= capacity - 1u) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	vsnprintf(out + current_length, capacity - current_length, fmt, args);
	va_end(args);
}

static void se_sdf_gen_uniform_recursive(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	if (!sdf_ptr) {
		return;
	}
	if (sdf_ptr->type == SE_SDF_SPHERE) {
		se_sdf_append(out, capacity, "uniform mat4 _" SE_SDF_HANDLE_FMT "_inv_transform;\n", SE_SDF_HANDLE_ARG(sdf));
		se_sdf_append(out, capacity, "uniform float _" SE_SDF_HANDLE_FMT "_radius;\n", SE_SDF_HANDLE_ARG(sdf));
	} else if (sdf_ptr->type == SE_SDF_BOX) {
		se_sdf_append(out, capacity, "uniform mat4 _" SE_SDF_HANDLE_FMT "_inv_transform;\n", SE_SDF_HANDLE_ARG(sdf));
		se_sdf_append(out, capacity, "uniform vec3 _" SE_SDF_HANDLE_FMT "_size;\n", SE_SDF_HANDLE_ARG(sdf));
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_gen_uniform_recursive(out, capacity, *child);
	}
}

static void se_sdf_gen_function_recursive(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	if (!sdf_ptr) {
		return;
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_gen_function_recursive(out, capacity, *child);
	}
	if (sdf_ptr->type == SE_SDF_SPHERE) {
		se_sdf_append(
			out,
			capacity,
			"float sdf_" SE_SDF_HANDLE_FMT "_primitive(vec3 p) {\n"
			"\tvec3 local = (_" SE_SDF_HANDLE_FMT "_inv_transform * vec4(p, 1.0)).xyz;\n"
			"\treturn length(local) - _" SE_SDF_HANDLE_FMT "_radius;\n"
			"}\n\n",
			SE_SDF_HANDLE_ARG(sdf),
			SE_SDF_HANDLE_ARG(sdf),
			SE_SDF_HANDLE_ARG(sdf));
	} else if (sdf_ptr->type == SE_SDF_BOX) {
		se_sdf_append(
			out,
			capacity,
			"float sdf_" SE_SDF_HANDLE_FMT "_primitive(vec3 p) {\n"
			"\tvec3 local = (_" SE_SDF_HANDLE_FMT "_inv_transform * vec4(p, 1.0)).xyz;\n"
			"\tvec3 q = abs(local) - _" SE_SDF_HANDLE_FMT "_size;\n"
			"\treturn length(max(q, vec3(0.0))) + min(max(q.x, max(q.y, q.z)), 0.0);\n"
			"}\n\n",
			SE_SDF_HANDLE_ARG(sdf),
			SE_SDF_HANDLE_ARG(sdf),
			SE_SDF_HANDLE_ARG(sdf));
	}
	se_sdf_append(out, capacity, "float sdf_" SE_SDF_HANDLE_FMT "(vec3 p) {\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "\tfloat d = %.1f;\n", SE_SDF_FAR_DISTANCE);
	if (se_sdf_has_primitive(sdf_ptr)) {
		se_sdf_append(out, capacity, "\td = min(d, sdf_" SE_SDF_HANDLE_FMT "_primitive(p));\n", SE_SDF_HANDLE_ARG(sdf));
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_append(out, capacity, "\td = min(d, sdf_" SE_SDF_HANDLE_FMT "(p));\n", SE_SDF_HANDLE_ARG(*child));
	}
	se_sdf_append(out, capacity, "\treturn d;\n}\n\n");
}

void se_sdf_gen_uniform(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_sdf_append(out, capacity, "uniform mat4 u_inv_view_projection;\n");
	se_sdf_append(out, capacity, "uniform vec3 u_camera_position;\n");
	se_sdf_append(out, capacity, "uniform int u_use_orthographic;\n");
	se_sdf_gen_uniform_recursive(out, capacity, sdf);
}

void se_sdf_gen_function(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_sdf_gen_function_recursive(out, capacity, sdf);
}

void se_sdf_gen_fragment(c8* out, const sz capacity, const se_sdf_handle sdf) {
	if (!out || capacity == 0) {
		return;
	}
	out[0] = '\0';
	se_sdf_append(out, capacity, "#version 330 core\n");
	se_sdf_append(out, capacity, "in vec2 v_uv;\n");
	se_sdf_append(out, capacity, "out vec4 frag_color;\n\n");
	se_sdf_gen_uniform(out, capacity, sdf);
	se_sdf_append(out, capacity, "\n");
	se_sdf_gen_function(out, capacity, sdf);
	se_sdf_append(out, capacity, "float scene_sdf(vec3 p) {\n");
	se_sdf_append(out, capacity, "\treturn sdf_" SE_SDF_HANDLE_FMT "(p);\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "}\n\n");
	se_sdf_append(
		out,
		capacity,
		"vec3 sdf_estimate_normal(vec3 p) {\n"
		"\tconst float e = 0.001;\n"
		"\tfloat dx = scene_sdf(p + vec3(e, 0.0, 0.0)) - scene_sdf(p - vec3(e, 0.0, 0.0));\n"
		"\tfloat dy = scene_sdf(p + vec3(0.0, e, 0.0)) - scene_sdf(p - vec3(0.0, e, 0.0));\n"
		"\tfloat dz = scene_sdf(p + vec3(0.0, 0.0, e)) - scene_sdf(p - vec3(0.0, 0.0, e));\n"
		"\treturn normalize(vec3(dx, dy, dz));\n"
		"}\n\n");
	se_sdf_append(
		out,
		capacity,
		"void main() {\n"
		"\tvec2 ndc = v_uv * 2.0 - 1.0;\n"
		"\tvec4 near_clip = vec4(ndc, -1.0, 1.0);\n"
		"\tvec4 far_clip = vec4(ndc, 1.0, 1.0);\n"
		"\tvec4 near_world_h = u_inv_view_projection * near_clip;\n"
		"\tvec4 far_world_h = u_inv_view_projection * far_clip;\n"
		"\tvec3 near_world = near_world_h.xyz / max(near_world_h.w, 0.00001);\n"
		"\tvec3 far_world = far_world_h.xyz / max(far_world_h.w, 0.00001);\n"
		"\tvec3 ray_origin = near_world;\n"
		"\tvec3 ray_dir = normalize(far_world - near_world);\n"
		"\tif (u_use_orthographic == 0) {\n"
		"\t\tray_origin = u_camera_position;\n"
		"\t\tray_dir = normalize(far_world - u_camera_position);\n"
		"\t}\n"
		"\tfloat travel = 0.0;\n"
		"\tvec3 hit_position = ray_origin;\n"
		"\tbool hit = false;\n"
		"\tfor (int i = 0; i < 128; ++i) {\n"
		"\t\thit_position = ray_origin + ray_dir * travel;\n"
		"\t\tfloat distance_to_surface = scene_sdf(hit_position);\n"
		"\t\tif (distance_to_surface < 0.001) {\n"
		"\t\t\thit = true;\n"
		"\t\t\tbreak;\n"
		"\t\t}\n"
		"\t\ttravel += distance_to_surface;\n"
		"\t\tif (travel > 256.0) {\n"
		"\t\t\tbreak;\n"
		"\t\t}\n"
		"\t}\n"
		"\tvec3 sky = mix(vec3(0.05, 0.06, 0.08), vec3(0.16, 0.19, 0.24), clamp(ray_dir.y * 0.5 + 0.5, 0.0, 1.0));\n"
		"\tif (!hit) {\n"
		"\t\tfrag_color = vec4(sky, 1.0);\n"
		"\t\treturn;\n"
		"\t}\n"
		"\tvec3 normal = sdf_estimate_normal(hit_position);\n"
		"\tvec3 light_dir = normalize(vec3(0.45, 0.85, 0.35));\n"
		"\tfloat diffuse = max(dot(normal, light_dir), 0.0);\n"
		"\tfloat ambient = 0.22;\n"
		"\tvec3 base = vec3(0.84, 0.86, 0.90);\n"
		"\tvec3 color = base * (ambient + diffuse * 0.78);\n"
		"\tfloat fog = clamp(travel / 48.0, 0.0, 1.0);\n"
		"\tfrag_color = vec4(mix(color, sky, fog), 1.0);\n"
		"}\n");
}

void se_sdf_gen_vertex(c8* out, const sz capacity) {
	if (!out || capacity == 0) {
		return;
	}
	out[0] = '\0';
	se_sdf_append(
		out,
		capacity,
		"#version 330 core\n"
		"layout(location = 0) in vec2 in_position;\n"
		"layout(location = 1) in vec2 in_uv;\n"
		"out vec2 v_uv;\n\n"
		"void main() {\n"
		"\tv_uv = in_uv;\n"
		"\tgl_Position = vec4(in_position, 0.0, 1.0);\n"
		"}\n");
}

static void se_sdf_upload_uniforms_recursive(const se_shader_handle shader, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	if (!sdf_ptr) {
		return;
	}
	if (sdf_ptr->type == SE_SDF_SPHERE || sdf_ptr->type == SE_SDF_BOX) {
		const s_mat4 inverse = s_mat4_inverse(&sdf_ptr->transform);
		c8 uniform_name[64] = {0};
		snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_inv_transform", SE_SDF_HANDLE_ARG(sdf));
		se_shader_set_mat4(shader, uniform_name, &inverse);
		if (sdf_ptr->type == SE_SDF_SPHERE) {
			snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_radius", SE_SDF_HANDLE_ARG(sdf));
			se_shader_set_float(shader, uniform_name, sdf_ptr->sphere.radius);
		} else {
			snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_size", SE_SDF_HANDLE_ARG(sdf));
			se_shader_set_vec3(shader, uniform_name, &sdf_ptr->box.size);
		}
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_upload_uniforms_recursive(shader, *child);
	}
}

se_sdf_handle se_sdf_create_internal(const se_sdf* sdf) {
	se_context* ctx = se_current_context();
	se_sdf_handle new_sdf = S_HANDLE_NULL;
	se_sdf* sdf_ptr = NULL;
	if (!ctx) {
		return S_HANDLE_NULL;
	}
	new_sdf = s_array_increment(&ctx->sdfs);
	sdf_ptr = se_sdf_from_handle(ctx, new_sdf);
	if (!sdf_ptr) {
		return S_HANDLE_NULL;
	}
	memset(sdf_ptr, 0, sizeof(*sdf_ptr));
	if (sdf) {
		memcpy(sdf_ptr, sdf, sizeof(*sdf_ptr));
	}
	if (se_sdf_transform_is_zero(&sdf_ptr->transform)) {
		sdf_ptr->transform = s_mat4_identity;
	}
	sdf_ptr->parent = S_HANDLE_NULL;
	s_array_clear(&sdf_ptr->children);
	return new_sdf;
}

void se_sdf_shutdown(void) {
	se_context* ctx = se_current_context();
	if (!ctx) {
		return;
	}
	while (s_array_get_size(&ctx->sdfs) > 0) {
		const se_sdf_handle handle = s_array_handle(&ctx->sdfs, (u32)(s_array_get_size(&ctx->sdfs) - 1u));
		se_sdf* sdf_ptr = se_sdf_from_handle(ctx, handle);
		if (sdf_ptr) {
			if (sdf_ptr->shader != S_HANDLE_NULL) {
				se_shader_destroy(sdf_ptr->shader);
				sdf_ptr->shader = S_HANDLE_NULL;
			}
			if (sdf_ptr->output != S_HANDLE_NULL) {
				se_framebuffer_destroy(sdf_ptr->output);
				sdf_ptr->output = S_HANDLE_NULL;
			}
			se_quad_destroy(&sdf_ptr->quad);
			s_array_clear(&sdf_ptr->children);
		}
		s_array_remove(&ctx->sdfs, handle);
	}
}

void se_sdf_add_child(se_sdf_handle parent, se_sdf_handle child) {
	se_context* ctx = se_current_context();
	se_sdf* parent_ptr = se_sdf_from_handle(ctx, parent);
	se_sdf* child_ptr = se_sdf_from_handle(ctx, child);
	if (!parent_ptr || !child_ptr) {
		return;
	}
	child_ptr->parent = parent;
	s_array_add(&parent_ptr->children, child);
}

void se_sdf_render_raw(se_sdf_handle sdf, se_camera_handle camera) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_camera* camera_ptr = se_camera_get(camera);
	s_mat4 view_projection = s_mat4_identity;
	s_mat4 inverse_view_projection = s_mat4_identity;
	if (!sdf_ptr || !camera_ptr || sdf_ptr->shader == S_HANDLE_NULL) {
		return;
	}
	view_projection = se_camera_get_view_projection_matrix(camera);
	inverse_view_projection = s_mat4_inverse(&view_projection);
	se_shader_set_mat4(sdf_ptr->shader, "u_inv_view_projection", &inverse_view_projection);
	se_shader_set_vec3(sdf_ptr->shader, "u_camera_position", &camera_ptr->position);
	se_shader_set_int(sdf_ptr->shader, "u_use_orthographic", camera_ptr->use_orthographic ? 1 : 0);
	se_sdf_upload_uniforms_recursive(sdf_ptr->shader, sdf);
	se_shader_use(sdf_ptr->shader, true, true);
	se_quad_render(&sdf_ptr->quad, 0);
}

void se_sdf_render(se_sdf_handle sdf, se_camera_handle camera) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!ctx || !sdf_ptr || camera == S_HANDLE_NULL) {
		return;
	}
	if (sdf_ptr->quad.vao == 0) {
		se_quad_2d_create(&sdf_ptr->quad, 0);
	}
	if (sdf_ptr->shader == S_HANDLE_NULL) {
		c8* vs = calloc(SE_SDF_VERTEX_SHADER_CAPACITY, sizeof(c8));
		c8* fs = calloc(SE_SDF_FRAGMENT_SHADER_CAPACITY, sizeof(c8));
		if (!vs || !fs) {
			free(vs);
			free(fs);
			return;
		}
		se_sdf_gen_vertex(vs, SE_SDF_VERTEX_SHADER_CAPACITY);
		se_sdf_gen_fragment(fs, SE_SDF_FRAGMENT_SHADER_CAPACITY, sdf);
		sdf_ptr->shader = se_shader_load_from_memory(vs, fs);
		free(vs);
		free(fs);
		if (sdf_ptr->shader == S_HANDLE_NULL) {
			se_log("se_sdf_render :: failed to create shader for sdf: " SE_SDF_HANDLE_FMT, SE_SDF_HANDLE_ARG(sdf));
			return;
		}
	}
	const GLboolean depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
	const GLboolean cull_face_enabled = glIsEnabled(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	se_sdf_render_raw(sdf, camera);
	if (depth_test_enabled) {
		glEnable(GL_DEPTH_TEST);
	}
	if (cull_face_enabled) {
		glEnable(GL_CULL_FACE);
	}
}

void se_sdf_bake(se_sdf_handle sdf) {
	(void)sdf;
}

void se_sdf_set_position(se_sdf_handle sdf, const s_vec3* position) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr || !position) {
		return;
	}
	s_mat4_set_translation(&sdf_ptr->transform, position);
}
