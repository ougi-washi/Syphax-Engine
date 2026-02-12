// Syphax Engine - Ougi Washi

#ifndef SE_MODEL_H
#define SE_MODEL_H

#include "se.h"
#include "se_shader.h"
#include "se_render.h"
#include "se_camera.h"

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
	se_shader_handle shader;
	s_mat4 matrix;
	se_mesh_data_flags data_flags;
} se_mesh;
typedef s_array(se_mesh, se_meshes);

typedef struct se_model {
	se_meshes meshes;
} se_model;
typedef s_array(se_model, se_models);
typedef se_model_handle se_model_ptr;
typedef s_array(se_model_handle, se_models_ptr);

extern void se_mesh_translate(se_mesh *mesh, const s_vec3 *v);
extern void se_mesh_rotate(se_mesh *mesh, const s_vec3 *v);
extern void se_mesh_scale(se_mesh *mesh, const s_vec3 *v);

extern se_model_handle se_model_load_obj_ex(const char *path, const se_shaders_ptr *shaders, const se_mesh_data_flags mesh_data_flags);
extern se_model_handle se_model_load_obj(const char *path, const se_shaders_ptr *shaders);
extern se_model_handle se_model_load_obj_simple_ex(const char *obj_path, const char *vertex_shader_path, const char *fragment_shader_path, const se_mesh_data_flags mesh_data_flags);
extern se_model_handle se_model_load_obj_simple(const char *obj_path, const char *vertex_shader_path, const char *fragment_shader_path);
extern void se_model_destroy(const se_model_handle model);
extern void se_model_render(const se_model_handle model, const se_camera_handle camera);
extern void se_model_translate(const se_model_handle model, const s_vec3 *v);
extern void se_model_rotate(const se_model_handle model, const s_vec3 *v);
extern void se_model_scale(const se_model_handle model, const s_vec3 *v);
extern b8 se_mesh_has_cpu_data(const se_mesh *mesh);
extern b8 se_mesh_has_gpu_data(const se_mesh *mesh);
extern void se_mesh_discard_cpu_data(se_mesh *mesh);
extern void se_model_discard_cpu_data(se_model *model);
extern void se_mesh_instance_create(se_mesh_instance *out_instance, const se_mesh *mesh, const u32 instance_count);
extern void se_mesh_instance_add_buffer(se_mesh_instance *instance, const s_mat4 *buffer, const sz instance_count);
extern void se_mesh_instance_update(se_mesh_instance *instance);
extern void se_mesh_instance_destroy(se_mesh_instance *instance);

#endif // SE_MODEL_H
