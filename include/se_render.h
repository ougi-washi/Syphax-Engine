// Syphax-Engine - Ougi Washi

#ifndef SE_RENDER_H
#define SE_RENDER_H

#include "se_defines.h"
#include "se_math.h"
#include "syphax/s_array.h"
#include <assert.h>
#include <pthread.h>
#include <time.h>

#define SE_MAX_NAME_LENGTH 64

typedef struct {
	s_vec2 position;
	s_vec2 uv;
} se_vertex_2d;

typedef struct {
	s_vec3 position;
	s_vec3 normal;
	s_vec2 uv;
} se_vertex_3d;

static const se_vertex_2d se_quad_2d_vertices[4] = {
	{{-1.0f, 1.0f}, {0.0f, 0.0f}},
	{{-1.0f, -1.0f}, {0.0f, 1.0f}},
	{{1.0f, -1.0f}, {1.0f, 1.0f}},
	{{1.0f, 1.0f}, {1.0f, 0.0f}}};

static const se_vertex_3d se_quad_3d_vertices[4] = {
	{{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
	{{1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	{{1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}};

static const u32 se_quad_indices[6] = {0, 1, 2, 0, 2, 3};

typedef enum {
	SE_UNIFORM_FLOAT,
	SE_UNIFORM_VEC2,
	SE_UNIFORM_VEC3,
	SE_UNIFORM_VEC4,
	SE_UNIFORM_INT,
	SE_UNIFORM_MAT3,
	SE_UNIFORM_MAT4,
	SE_UNIFORM_TEXTURE
} se_uniform_type;

typedef struct {
	char name[SE_MAX_NAME_LENGTH];
	se_uniform_type type;
	union {
		f32 f;
		s_vec2 vec2;
		s_vec3 vec3;
		s_vec4 vec4;
		i32 i;
		s_mat3 mat3;
		s_mat4 mat4;
		u32 texture;
	} value;
} se_uniform;
typedef s_array(se_uniform, se_uniforms);

typedef struct {
	u32 program;
	u32 vertex_shader;
	u32 fragment_shader;
	c8 vertex_path[SE_MAX_PATH_LENGTH];
	c8 fragment_path[SE_MAX_PATH_LENGTH];
	time_t vertex_mtime;
	time_t fragment_mtime;
	se_uniforms uniforms;
	b8 needs_reload;
	b8 is_valid : 1;
} se_shader;
typedef s_array(se_shader, se_shaders);
typedef se_shader *se_shader_ptr;
typedef s_array(se_shader_ptr, se_shaders_ptr);

typedef struct se_texture {
	char path[SE_MAX_PATH_LENGTH];
	u32 id;
	i32 width;
	i32 height;
	i32 channels;
	b8 is_valid : 1;
} se_texture;
typedef s_array(se_texture, se_textures);
typedef se_texture *se_texture_ptr;
typedef s_array(se_texture_ptr, se_textures_ptr);

typedef enum {
	SE_MESH_DATA_NONE = 0,
	SE_MESH_DATA_CPU = 1 << 0,
	SE_MESH_DATA_GPU = 1 << 1,
	SE_MESH_DATA_CPU_GPU = SE_MESH_DATA_CPU | SE_MESH_DATA_GPU
} se_mesh_data_flags;

typedef struct {
	s_array(c8, file_path);
	s_array(se_vertex_3d, vertices);
	s_array(u32, indices);
} se_mesh_cpu_data;

typedef struct {
	u32 vertex_count;
	u32 index_count;
	u32 vao;
	u32 vbo;
	u32 ebo;
} se_mesh_gpu_data;

typedef struct {
	se_mesh_cpu_data cpu;
	se_mesh_gpu_data gpu;
	se_shader *shader;
	s_mat4 matrix;
	se_mesh_data_flags data_flags;
} se_mesh;
typedef s_array(se_mesh, se_meshes);

typedef struct {
	se_meshes meshes;
	b8 is_valid : 1;
} se_model;
typedef s_array(se_model, se_models);
typedef se_model *se_model_ptr;
typedef s_array(se_model_ptr, se_models_ptr);

typedef struct {
	s_vec3 position;
	s_vec3 target;
	s_vec3 up;
	s_vec3 right;
	f32 fov;
	f32 near;
	f32 far;
	f32 aspect;
	b8 is_valid : 1;
} se_camera;
typedef s_array(se_camera, se_cameras);
typedef se_camera *se_camera_ptr;
typedef s_array(se_camera_ptr, se_cameras_ptr);

typedef struct {
	u32 framebuffer;
	u32 texture;
	u32 depth_buffer;
	s_vec2 size;
	s_vec2 ratio;
	b8 auto_resize : 1;
	b8 is_valid : 1;
} se_framebuffer;
typedef s_array(se_framebuffer, se_framebuffers);
typedef se_framebuffer *se_framebuffer_ptr;
typedef s_array(se_framebuffer_ptr, se_framebuffers_ptr);

typedef struct {
	u32 framebuffer;
	u32 texture;
	u32 prev_framebuffer;
	u32 prev_texture;
	u32 depth_buffer;
	s_vec2 texture_size;
	s_vec2 scale;
	s_vec2 position;
	se_shader_ptr shader;
	b8 is_valid : 1;
} se_render_buffer;
typedef s_array(se_render_buffer, se_render_buffers);
typedef se_render_buffer *se_render_buffer_ptr;
typedef s_array(se_render_buffer_ptr, se_render_buffers_ptr);

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

typedef struct {
	u16 framebuffers_count;
	u16 render_buffers_count;
	u16 textures_count;
	u16 shaders_count;
	u16 models_count;
	u16 cameras_count;
} se_render_handle_params;

#define SE_RENDER_HANDLE_PARAMS_DEFAULTS ((se_render_handle_params){ .framebuffers_count = 8, .render_buffers_count = 8, .textures_count = 64, .shaders_count = 32, .models_count = 16, .cameras_count = 8 })

// TODO: maybe this should not be accessible by the user, and instead it should
// be in the .c file, as there seem no need for direct access. All the following
// data is accessed through other handles.
typedef struct {
	se_framebuffers framebuffers;
	se_render_buffers render_buffers;
	se_textures textures;
	se_shaders shaders;
	se_uniforms global_uniforms;
	se_cameras cameras;
	se_models models;
} se_render_handle;

// helper functions
extern b8 se_render_init(void);
extern void se_render_set_blending(const b8 active);
extern void se_render_unbind_framebuffer(void); // window framebuffer
extern void se_render_clear(void);
extern void se_render_set_background_color(const s_vec4 color);

// render_handle functions
extern se_render_handle *se_render_handle_create(const se_render_handle_params *params);
extern void se_render_handle_destroy(se_render_handle *render_handle);
extern void se_render_handle_reload_changed_shaders(se_render_handle *render_handle);
extern se_uniforms *se_render_handle_get_global_uniforms(se_render_handle *render_handle);

// Texture functions
// Ownership: render handle owns textures created from it.
typedef enum { SE_REPEAT, SE_CLAMP } se_texture_wrap;
extern se_texture *se_texture_load(se_render_handle *render_handle, const char *path, const se_texture_wrap wrap);
extern se_texture *se_texture_load_from_memory(se_render_handle *render_handle, const u8 *data, const sz size, const se_texture_wrap wrap);
extern void se_render_handle_destroy_texture(se_render_handle *render_handle, se_texture *texture);
#define se_texture_destroy(render_handle, texture) se_render_handle_destroy_texture((render_handle), (texture))

// Shader functions
// Ownership: render handle owns shaders created from it.
extern se_shader *se_shader_load(se_render_handle *render_handle, const char *vertex_file_path, const char *fragment_file_path);
extern se_shader *se_shader_load_from_memory(se_render_handle *render_handle, const char *vertex_data, const char *fragment_data);
extern void se_render_handle_destroy_shader(se_render_handle *render_handle, se_shader *shader);
extern b8 se_shader_reload_if_changed(se_shader *shader);
extern void se_shader_use(se_render_handle *render_handle, se_shader *shader, const b8 update_uniforms, const b8 update_global_uniforms);
#define se_shader_destroy(render_handle, shader) se_render_handle_destroy_shader((render_handle), (shader))
extern i32 se_shader_get_uniform_location(se_shader *shader, const char *name);
extern f32 *se_shader_get_uniform_float(se_shader *shader, const char *name);
extern s_vec2 *se_shader_get_uniform_vec2(se_shader *shader, const char *name);
extern s_vec3 *se_shader_get_uniform_vec3(se_shader *shader, const char *name);
extern s_vec4 *se_shader_get_uniform_vec4(se_shader *shader, const char *name);
extern i32 *se_shader_get_uniform_int(se_shader *shader, const char *name);
extern s_mat4 *se_shader_get_uniform_mat4(se_shader *shader, const char *name);
extern u32 *se_shader_get_uniform_texture(se_shader *shader, const char *name);
extern void se_shader_set_float(se_shader *shader, const char *name, f32 value);
extern void se_shader_set_vec2(se_shader *shader, const char *name, const s_vec2 *value);
extern void se_shader_set_vec3(se_shader *shader, const char *name, const s_vec3 *value);
extern void se_shader_set_vec4(se_shader *shader, const char *name, const s_vec4 *value);
extern void se_shader_set_int(se_shader *shader, const char *name, i32 value);
extern void se_shader_set_mat3(se_shader *shader, const char *name, const s_mat3 *value);
extern void se_shader_set_mat4(se_shader *shader, const char *name, const s_mat4 *value);
extern void se_shader_set_texture(se_shader *shader, const char *name, const u32 texture);
extern void se_shader_set_buffer_texture(se_shader *shader, const char *name, se_render_buffer *buffer);

// Mesh functions
extern void se_mesh_translate(se_mesh *mesh, const s_vec3 *v);
extern void se_mesh_rotate(se_mesh *mesh, const s_vec3 *v);
extern void se_mesh_scale(se_mesh *mesh, const s_vec3 *v);

// Model functions
// Ownership: render handle owns models created from it.
extern se_model *se_model_load_obj_ex(se_render_handle *render_handle, const char *path, se_shaders_ptr *shaders, const se_mesh_data_flags mesh_data_flags);
extern se_model *se_model_load_obj(se_render_handle *render_handle, const char *path, se_shaders_ptr *shaders);
extern se_model *se_model_load_obj_simple_ex(se_render_handle *render_handle, const char *obj_path, const char *vertex_shader_path, const char *fragment_shader_path, const se_mesh_data_flags mesh_data_flags);
extern se_model *se_model_load_obj_simple(se_render_handle *render_handle, const char *obj_path, const char *vertex_shader_path, const char *fragment_shader_path);
extern void se_render_handle_destroy_model(se_render_handle *render_handle, se_model *model);
#define se_model_destroy(render_handle, model) se_render_handle_destroy_model((render_handle), (model))
extern void se_model_render(se_render_handle *render_handle, se_model *model, se_camera *camera);
extern void se_model_translate(se_model *model, const s_vec3 *v);
extern void se_model_rotate(se_model *model, const s_vec3 *v);
extern void se_model_scale(se_model *model, const s_vec3 *v);
extern b8 se_mesh_has_cpu_data(const se_mesh *mesh);
extern b8 se_mesh_has_gpu_data(const se_mesh *mesh);
extern void se_mesh_discard_cpu_data(se_mesh *mesh);
extern void se_model_discard_cpu_data(se_model *model);

// camera functions
// Ownership: render handle owns cameras created from it.
extern se_camera *se_render_handle_create_camera(se_render_handle *render_handle);
extern void se_render_handle_destroy_camera(se_render_handle *render_handle, se_camera *camera);
#define se_camera_create(render_handle) se_render_handle_create_camera((render_handle))
#define se_camera_destroy(render_handle, camera) se_render_handle_destroy_camera((render_handle), (camera))
extern s_mat4 se_camera_get_view_matrix(const se_camera *camera);
extern s_mat4 se_camera_get_projection_matrix(const se_camera *camera);
extern void se_camera_set_aspect(se_camera *camera, const f32 width, const f32 height);

// Framebuffer functions
// Ownership: render handle owns framebuffers created from it.
extern se_framebuffer *se_framebuffer_create(se_render_handle *render_handle, const s_vec2 *size);
extern void se_render_handle_destroy_framebuffer(se_render_handle *render_handle, se_framebuffer *framebuffer);
#define se_framebuffer_destroy(render_handle, framebuffer) se_render_handle_destroy_framebuffer((render_handle), (framebuffer))
extern void se_framebuffer_set_size(se_framebuffer *framebuffer, const s_vec2 *size);
extern void se_framebuffer_get_size(se_framebuffer *framebuffer, s_vec2 *out_size);
extern void se_framebuffer_bind(se_framebuffer *framebuffer);
extern void se_framebuffer_unbind(se_framebuffer *framebuffer);

// Render buffer functions
// Ownership: render handle owns render buffers created from it.
extern se_render_buffer *se_render_buffer_create(se_render_handle *render_handle, const u32 width, const u32 height, const c8 *fragment_shader_path);
extern void se_render_handle_destroy_render_buffer(se_render_handle *render_handle, se_render_buffer *buffer);
#define se_render_buffer_destroy(render_handle, buffer) se_render_handle_destroy_render_buffer((render_handle), (buffer))
extern void se_render_buffer_set_shader(se_render_buffer *buffer, se_shader *shader);
extern void se_render_buffer_unset_shader(se_render_buffer *buffer);
extern void se_render_buffer_bind(se_render_buffer *buffer);
extern void se_render_buffer_unbind(se_render_buffer *buf);
extern void se_render_buffer_set_scale(se_render_buffer *buffer, const s_vec2 *scale);
extern void se_render_buffer_set_position(se_render_buffer *buffer, const s_vec2 *position);

// Uniform functions
extern void se_uniform_set_float(se_uniforms *uniforms, const char *name, f32 value);
extern void se_uniform_set_vec2(se_uniforms *uniforms, const char *name, const s_vec2 *value);
extern void se_uniform_set_vec3(se_uniforms *uniforms, const char *name, const s_vec3 *value);
extern void se_uniform_set_vec4(se_uniforms *uniforms, const char *name, const s_vec4 *value);
extern void se_uniform_set_int(se_uniforms *uniforms, const char *name, i32 value);
extern void se_uniform_set_mat3(se_uniforms *uniforms, const char *name, const s_mat3 *value);
extern void se_uniform_set_mat4(se_uniforms *uniforms, const char *name, const s_mat4 *value);
extern void se_uniform_set_texture(se_uniforms *uniforms, const char *name, const u32 texture);
extern void se_uniform_set_buffer_texture(se_uniforms *uniforms, const char *name, se_render_buffer *buffer);
extern void se_uniform_apply(se_render_handle *render_handle, se_shader *shader, const b8 update_global_uniforms);

// Quad functions
extern void se_quad_3d_create(se_quad *out_quad);
extern void se_quad_2d_create(se_quad *out_quad, const u32 instance_count);
extern void se_quad_2d_add_instance_buffer(se_quad *quad, const s_mat4 *buffer, const sz instance_count);
extern void se_quad_2d_add_instance_buffer_mat3(se_quad *quad, const s_mat3 *buffer, const sz instance_count);
extern void se_quad_render(se_quad *quad, const sz instance_count);
extern void se_quad_destroy(se_quad *quad);

// Mesh instance functions
extern void se_mesh_instance_create(se_mesh_instance *out_instance, const se_mesh *mesh, const u32 instance_count);
extern void se_mesh_instance_add_buffer(se_mesh_instance *instance, const s_mat4 *buffer, const sz instance_count);
extern void se_mesh_instance_update(se_mesh_instance *instance);
extern void se_mesh_instance_destroy(se_mesh_instance *instance);

// Utility functions

#define se_render_command(...) do { __VA_ARGS__ } while (0)

// logging
extern void se_render_print_mat4(const s_mat4 *mat);

#endif // SE_RENDER_H
