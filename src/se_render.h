// Syphax-render_handle - Ougi Washi

#ifndef SE_RENDER_H
#define SE_RENDER_H

#include "se_defines.h"
#include "se_math.h"
#include "syphax/s_array.h"
#include <GLFW/glfw3.h>
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
	GLuint texture;
	} value;
} se_uniform;
typedef s_array(se_uniform, se_uniforms);

typedef struct {
	GLuint program;
	GLuint vertex_shader;
	GLuint fragment_shader;
	c8 vertex_path[SE_MAX_PATH_LENGTH];
	c8 fragment_path[SE_MAX_PATH_LENGTH];
	time_t vertex_mtime;
	time_t fragment_mtime;
	se_uniforms uniforms;
	b8 needs_reload;
} se_shader;
typedef s_array(se_shader, se_shaders);
typedef se_shader *se_shader_ptr;
typedef s_array(se_shader_ptr, se_shaders_ptr);

typedef struct se_texture {
	char path[SE_MAX_PATH_LENGTH];
	GLuint id;
	i32 width;
	i32 height;
	i32 channels;
} se_texture;
typedef s_array(se_texture, se_textures);
typedef se_texture *se_texture_ptr;
typedef s_array(se_texture_ptr, se_textures_ptr);

typedef struct {
	se_vertex_3d
		*vertices; // TODO: no need for this, we can just use the buffer directly
	u32 *indices;
	u32 vertex_count;
	u32 index_count;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	se_shader *shader;
	GLuint texture_id;
	s_mat4 matrix;
} se_mesh;
typedef s_array(se_mesh, se_meshes);

typedef struct {
	se_meshes meshes;
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
} se_camera;
typedef s_array(se_camera, se_cameras);
typedef se_camera *se_camera_ptr;
typedef s_array(se_camera_ptr, se_cameras_ptr);

typedef struct {
	GLuint framebuffer;
	GLuint texture;
	GLuint depth_buffer;
	s_vec2 size;
	s_vec2 ratio;
	b8 auto_resize : 1;
} se_framebuffer;
typedef s_array(se_framebuffer, se_framebuffers);
typedef se_framebuffer *se_framebuffer_ptr;
typedef s_array(se_framebuffer_ptr, se_framebuffers_ptr);

typedef struct {
	GLuint framebuffer;
	GLuint texture;
	GLuint prev_framebuffer;
	GLuint prev_texture;
	GLuint depth_buffer;
	s_vec2 texture_size;
	s_vec2 scale;
	s_vec2 position;
	se_shader_ptr shader;
} se_render_buffer;
typedef s_array(se_render_buffer, se_render_buffers);
typedef se_render_buffer *se_render_buffer_ptr;
typedef s_array(se_render_buffer_ptr, se_render_buffers_ptr);

typedef struct {
	GLuint vbo;
	const void *buffer_ptr;
	sz buffer_size;
} se_instance_buffer;

typedef struct {
	GLuint vao;
	s_array(se_instance_buffer, instance_buffers);
	b8 instance_buffers_dirty;
} se_mesh_instance;
typedef s_array(se_mesh_instance, se_mesh_instances);

typedef struct {
	GLuint vao;
	GLuint ebo;
	GLuint vbo;
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
extern void se_enable_blending();
extern void se_disable_blending();
extern void se_unbind_framebuffer(); // window framebuffer
extern void se_render_clear();
extern void se_render_set_background_color(const s_vec4 color);

// render_handle functions
extern se_render_handle *
se_render_handle_create(const se_render_handle_params *params);
extern void se_render_handle_cleanup(se_render_handle *render_handle);
extern void se_render_handle_reload_changed_shaders(se_render_handle *render_handle);
extern se_uniforms *se_render_handle_get_global_uniforms(se_render_handle *render_handle);

// Texture functions
typedef enum { SE_REPEAT, SE_CLAMP } se_texture_wrap;
extern se_texture *se_texture_load(se_render_handle *render_handle, const char *path, const se_texture_wrap wrap);
extern se_texture *se_texture_load_from_memory(se_render_handle *render_handle, const u8 *data, const sz size, const se_texture_wrap wrap);
extern void se_texture_cleanup(se_texture *texture);

// Shader functions
extern se_shader *se_shader_load(se_render_handle *render_handle, const char *vertex_file_path, const char *fragment_file_path);
extern se_shader *se_shader_load_from_memory(se_render_handle *render_handle, const char *vertex_data, const char *fragment_data);
extern b8 se_shader_reload_if_changed(se_shader *shader);
extern void se_shader_use(se_render_handle *render_handle, se_shader *shader, const b8 update_uniforms, const b8 update_global_uniforms);
extern void se_shader_cleanup(se_shader *shader);
extern GLuint se_shader_get_uniform_location(se_shader *shader, const char *name);
extern f32 *se_shader_get_uniform_float(se_shader *shader, const char *name);
extern s_vec2 *se_shader_get_uniform_vec2(se_shader *shader, const char *name);
extern s_vec3 *se_shader_get_uniform_vec3(se_shader *shader, const char *name);
extern s_vec4 *se_shader_get_uniform_vec4(se_shader *shader, const char *name);
extern i32 *se_shader_get_uniform_int(se_shader *shader, const char *name);
extern s_mat4 *se_shader_get_uniform_mat4(se_shader *shader, const char *name);
extern GLuint *se_shader_get_uniform_texture(se_shader *shader, const char *name);
extern void se_shader_set_float(se_shader *shader, const char *name, f32 value);
extern void se_shader_set_vec2(se_shader *shader, const char *name, const s_vec2 *value);
extern void se_shader_set_vec3(se_shader *shader, const char *name, const s_vec3 *value);
extern void se_shader_set_vec4(se_shader *shader, const char *name, const s_vec4 *value);
extern void se_shader_set_int(se_shader *shader, const char *name, i32 value);
extern void se_shader_set_mat3(se_shader *shader, const char *name, const s_mat3 *value);
extern void se_shader_set_mat4(se_shader *shader, const char *name, const s_mat4 *value);
extern void se_shader_set_texture(se_shader *shader, const char *name, GLuint texture);
extern void se_shader_set_buffer_texture(se_shader *shader, const char *name, se_render_buffer *buffer);

// Mesh functions
extern void se_mesh_translate(se_mesh *mesh, const s_vec3 *v);
extern void se_mesh_rotate(se_mesh *mesh, const s_vec3 *v);
extern void se_mesh_scale(se_mesh *mesh, const s_vec3 *v);

// Model functions
extern se_model *se_model_load_obj(se_render_handle *render_handle, const char *path, se_shaders_ptr *shaders);
extern void se_model_render(se_render_handle *render_handle, se_model *model, se_camera *camera);
extern void se_model_cleanup(se_model *model);
extern void se_model_translate(se_model *model, const s_vec3 *v);
extern void se_model_rotate(se_model *model, const s_vec3 *v);
extern void se_model_scale(se_model *model, const s_vec3 *v);

// camera functions
extern se_camera *se_camera_create(se_render_handle *render_handle);
extern s_mat4 se_camera_get_view_matrix(const se_camera *camera);
extern s_mat4 se_camera_get_projection_matrix(const se_camera *camera);
extern void se_camera_set_aspect(se_camera *camera, const f32 width, const f32 height);
extern void se_camera_destroy(se_render_handle *render_handle, se_camera *camera);

// Framebuffer functions
extern se_framebuffer *se_framebuffer_create(se_render_handle *render_handle, const s_vec2 *size);
extern void se_framebuffer_set_size(se_framebuffer *framebuffer, const s_vec2 *size);
extern void se_framebuffer_get_size(se_framebuffer *framebuffer, s_vec2 *out_size);
extern void se_framebuffer_bind(se_framebuffer *framebuffer);
extern void se_framebuffer_unbind(se_framebuffer *framebuffer);
extern void se_framebuffer_use_quad_shader(se_framebuffer *framebuffer, se_render_handle *render_handle);
extern void se_framebuffer_cleanup(se_framebuffer *framebuffer);

// Render buffer functions
extern se_render_buffer *se_render_buffer_create(se_render_handle *render_handle, const u32 width, const u32 height, const c8 *fragment_shader_path);
extern void se_render_buffer_set_shader(se_render_buffer *buffer, se_shader *shader);
extern void se_render_buffer_unset_shader(se_render_buffer *buffer);
extern void se_render_buffer_bind(se_render_buffer *buffer);
extern void se_render_buffer_unbind(se_render_buffer *buf);
extern void se_render_buffer_set_scale(se_render_buffer *buffer, const s_vec2 *scale);
extern void se_render_buffer_set_position(se_render_buffer *buffer, const s_vec2 *position);
extern void se_render_buffer_cleanup(se_render_buffer *buffer);

// Uniform functions
extern void se_uniform_set_float(se_uniforms *uniforms, const char *name, f32 value);
extern void se_uniform_set_vec2(se_uniforms *uniforms, const char *name, const s_vec2 *value);
extern void se_uniform_set_vec3(se_uniforms *uniforms, const char *name, const s_vec3 *value);
extern void se_uniform_set_vec4(se_uniforms *uniforms, const char *name, const s_vec4 *value);
extern void se_uniform_set_int(se_uniforms *uniforms, const char *name, i32 value);
extern void se_uniform_set_mat3(se_uniforms *uniforms, const char *name, const s_mat3 *value);
extern void se_uniform_set_mat4(se_uniforms *uniforms, const char *name, const s_mat4 *value);
extern void se_uniform_set_texture(se_uniforms *uniforms, const char *name, GLuint texture);
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

// logging
extern void se_print_mat4(const s_mat4 *mat);

#endif // SE_RENDER_H
