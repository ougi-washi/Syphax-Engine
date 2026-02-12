// Syphax-Engine - Ougi Washi

// Ownership: scene/object storage lives in se_context.

#ifndef SE_SCENE_H
#define SE_SCENE_H

#include "se_render.h"
#include "se_window.h"
#include "se_camera.h"
#include "se_model.h"
#include "se_render_buffer.h"
#include "se_framebuffer.h"

#define SE_MAX_SCENES 128
#define SE_MAX_2D_OBJECTS 1024
#define SE_MAX_3D_OBJECTS 1024

typedef i32 se_instance_id;
typedef s_array(se_instance_id, se_instance_ids);
typedef s_array(s_mat4, se_transforms);
typedef s_array(s_mat4, se_buffers);
typedef struct {
	se_instance_ids ids;
	se_transforms transforms;
	se_buffers buffers;
} se_instances;

typedef s_array(s_mat3, se_transforms_2d);
typedef struct {
	se_instance_ids ids;
	se_transforms_2d transforms;
	se_buffers buffers;
} se_instances_2d;

typedef void (*se_object_custom_callback)(void *data);

#define SE_OBJECT_CUSTOM_DATA_SIZE (16 * 1024)

typedef struct {
	se_object_custom_callback render;
	sz data_size;
	u8 data[SE_OBJECT_CUSTOM_DATA_SIZE];
} se_object_custom;

typedef struct se_object_2d {
	s_mat3 transform;
	union {
		struct {
			se_quad quad;
			se_shader_handle shader;
			se_instances_2d instances;
			se_transforms_2d render_transforms;
		};
		se_object_custom custom;
	};
	b8 is_custom : 1;
	b8 is_visible : 1;
} se_object_2d;

typedef s_array(se_object_2d, se_objects_2d);
typedef se_object_2d_handle se_object_2d_ptr;
typedef s_array(se_object_2d_handle, se_objects_2d_ptr);

typedef struct se_object_3d {
	se_model_handle model;
	s_mat4 transform;
	se_instances instances;
	se_mesh_instances mesh_instances;
	se_transforms render_transforms;
	b8 is_visible : 1;
} se_object_3d;

typedef s_array(se_object_3d, se_objects_3d);
typedef se_object_3d_handle se_object_3d_ptr;
typedef s_array(se_object_3d_handle, se_objects_3d_ptr);

typedef struct se_scene_2d {
	se_objects_2d_ptr objects;
	se_framebuffer_handle output;
} se_scene_2d;

typedef s_array(se_scene_2d, se_scenes_2d);
typedef se_scene_2d_handle se_scene_2d_ptr;
typedef s_array(se_scene_2d_handle, se_scenes_2d_ptr);

typedef struct se_scene_3d {
	se_objects_3d_ptr objects;
	se_camera_handle camera;
	se_render_buffers_ptr post_process;
	se_shader_handle output_shader;
	se_framebuffer_handle output;
	b8 enable_culling : 1;
} se_scene_3d;

typedef s_array(se_scene_3d, se_scenes_3d);
typedef se_scene_3d_handle se_scene_3d_ptr;
typedef s_array(se_scene_3d_handle, se_scenes_3d_ptr);

// 2D objects functions
extern se_object_2d_handle se_object_2d_create(const c8 *fragment_shader_path, const s_mat3 *transform, const sz max_instances_count);
extern se_object_2d_handle se_object_2d_create_custom(se_object_custom *custom, const s_mat3 *transform);
extern void se_object_custom_set_data(se_object_custom *custom, const void *data, const sz size);
extern void se_object_2d_destroy(const se_object_2d_handle object);
extern void se_object_2d_set_transform(const se_object_2d_handle object, const s_mat3 *transform);
extern s_mat3 se_object_2d_get_transform(const se_object_2d_handle object);
extern void se_object_2d_set_position(const se_object_2d_handle object, const s_vec2 *position);
extern s_vec2 se_object_2d_get_position(const se_object_2d_handle object);
extern void se_object_2d_set_scale(const se_object_2d_handle object, const s_vec2 *scale);
extern s_vec2 se_object_2d_get_scale(const se_object_2d_handle object);
extern void se_object_2d_get_box_2d(const se_object_2d_handle object, se_box_2d *out_box);
extern void se_object_2d_set_shader(const se_object_2d_handle object, const se_shader_handle shader);
extern se_shader_handle se_object_2d_get_shader(const se_object_2d_handle object);
extern void se_object_2d_update_uniforms(const se_object_2d_handle object);
extern se_instance_id se_object_2d_add_instance(const se_object_2d_handle object, const s_mat3 *transform, const s_mat4 *buffer);
extern i32 se_object_2d_get_instance_index(const se_object_2d_handle object, const se_instance_id instance_id);
extern void se_object_2d_set_instance_transform(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat3 *transform);
extern void se_object_2d_set_instance_buffer(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat4 *buffer);
extern void se_object_2d_set_instances_transforms(const se_object_2d_handle object, const se_transforms_2d *transforms);
extern void se_object_2d_set_instances_buffers(const se_object_2d_handle object, const se_buffers *buffers);
extern void se_object_2d_set_instances_dirty(const se_object_2d_handle object, const b8 dirty);
extern b8 se_object_2d_are_instances_dirty(const se_object_2d_handle object);
extern sz se_object_2d_get_instance_count(const se_object_2d_handle object);

// 2D scene functions
extern se_scene_2d_handle se_scene_2d_create(const s_vec2 *size, const u16 object_count);
extern void se_scene_2d_set_auto_resize(const se_scene_2d_handle scene, const se_window_handle window, const s_vec2 *ratio);
extern void se_scene_2d_destroy(const se_scene_2d_handle scene);
extern void se_scene_2d_bind(const se_scene_2d_handle scene);
extern void se_scene_2d_unbind(const se_scene_2d_handle scene);
extern void se_scene_2d_render_raw(const se_scene_2d_handle scene);
extern void se_scene_2d_render_to_buffer(const se_scene_2d_handle scene);
extern void se_scene_2d_render_to_screen(const se_scene_2d_handle scene, const se_window_handle window);
extern void se_scene_2d_draw(const se_scene_2d_handle scene, const se_window_handle window);
extern void se_scene_2d_add_object(const se_scene_2d_handle scene, const se_object_2d_handle object);
extern void se_scene_2d_remove_object(const se_scene_2d_handle scene, const se_object_2d_handle object);

// 3D scene functions
extern se_scene_3d_handle se_scene_3d_create(const s_vec2 *size, const u16 object_count);
extern se_scene_3d_handle se_scene_3d_create_for_window(const se_window_handle window, const u16 object_count);
extern void se_scene_3d_set_auto_resize(const se_scene_3d_handle scene, const se_window_handle window, const s_vec2 *ratio);
extern void se_scene_3d_destroy(const se_scene_3d_handle scene);
extern void se_scene_3d_render_to_buffer(const se_scene_3d_handle scene);
extern void se_scene_3d_render_to_screen(const se_scene_3d_handle scene, const se_window_handle window);
extern void se_scene_3d_draw(const se_scene_3d_handle scene, const se_window_handle window);
extern void se_scene_3d_add_object(const se_scene_3d_handle scene, const se_object_3d_handle object);
extern se_object_3d_handle se_scene_3d_add_model(const se_scene_3d_handle scene, const se_model_handle model, const s_mat4 *transform);
extern void se_scene_3d_remove_object(const se_scene_3d_handle scene, const se_object_3d_handle object);
extern void se_scene_3d_set_camera(const se_scene_3d_handle scene, const se_camera_handle camera);
extern se_camera_handle se_scene_3d_get_camera(const se_scene_3d_handle scene);
extern void se_scene_3d_set_culling(const se_scene_3d_handle scene, const b8 enabled);
extern void se_scene_3d_add_post_process_buffer(const se_scene_3d_handle scene, const se_render_buffer_handle buffer);
extern void se_scene_3d_remove_post_process_buffer(const se_scene_3d_handle scene, const se_render_buffer_handle buffer);

// 3D objects functions
extern se_object_3d_handle se_object_3d_create(const se_model_handle model, const s_mat4 *transform, const sz max_instances_count);
extern void se_object_3d_destroy(const se_object_3d_handle object);
extern void se_object_3d_set_transform(const se_object_3d_handle object, const s_mat4 *transform);
extern s_mat4 se_object_3d_get_transform(const se_object_3d_handle object);
extern se_instance_id se_object_3d_add_instance(const se_object_3d_handle object, const s_mat4 *transform, const s_mat4 *buffer);
extern i32 se_object_3d_get_instance_index(const se_object_3d_handle object, const se_instance_id instance_id);
extern void se_object_3d_set_instance_transform(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4 *transform);
extern void se_object_3d_set_instance_buffer(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4 *buffer);
extern void se_object_3d_set_instances_dirty(const se_object_3d_handle object, const b8 dirty);
extern b8 se_object_3d_are_instances_dirty(const se_object_3d_handle object);
extern sz se_object_3d_get_instance_count(const se_object_3d_handle object);

#endif // SE_SCENE_H
