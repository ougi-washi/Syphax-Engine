// Syphax-Engine - Ougi Washi

// Scenes are used as a collection of pointers to rendering instances
// They are not responsible for handling memory, and are only used for
// referencing the rendering instances Render handles are the ones responsible
// for handling memory, so no allocation and deallocation is needed here

#ifndef SE_SCENE_H
#define SE_SCENE_H

#include "se_render.h"
#include "se_window.h"

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
typedef s_array(s_mat3, se_buffers_2d);
typedef struct {
	se_instance_ids ids;
	se_transforms_2d transforms;
	se_buffers_2d buffers;
} se_instances_2d;

typedef void (*se_object_custom_callback)(se_render_handle *render_handle, void *data);

#define SE_OBJECT_CUSTOM_DATA_SIZE (16 * 1024)

typedef struct {
	se_object_custom_callback render;
	sz data_size;
	u8 data[SE_OBJECT_CUSTOM_DATA_SIZE];
} se_object_custom;

typedef struct {
	s_mat3 transform;
	union {
		struct {
			se_quad quad;
			se_shader_ptr shader;
			se_instances_2d instances;
		};
		se_object_custom custom;
	};
	b8 is_custom : 1;
	b8 is_visible : 1;
} se_object_2d;

typedef s_array(se_object_2d, se_objects_2d);
typedef se_object_2d *se_object_2d_ptr;
typedef s_array(se_object_2d_ptr, se_objects_2d_ptr);

typedef struct {
	se_model *model;
	s_mat4 transform;
	se_instances instances;
	se_mesh_instances mesh_instances;
	b8 is_visible : 1;
} se_object_3d;
typedef s_array(se_object_3d, se_objects_3d);
typedef se_object_3d *se_object_3d_ptr;
typedef s_array(se_object_3d_ptr, se_objects_3d_ptr);

typedef struct {
	se_objects_2d_ptr objects;
	se_framebuffer_ptr output;
} se_scene_2d;
typedef s_array(se_scene_2d, se_scenes_2d);
typedef se_scene_2d *se_scene_2d_ptr;
typedef s_array(se_scene_2d_ptr, se_scenes_2d_ptr);

typedef struct {
	se_objects_3d_ptr objects;
	se_camera_ptr camera;
	se_render_buffers_ptr post_process;
	se_shader_ptr output_shader;
	se_framebuffer_ptr output;
} se_scene_3d;
typedef s_array(se_scene_3d, se_scenes_3d);
typedef se_scene_3d *se_scene_3d_ptr;
typedef s_array(se_scene_3d_ptr, se_scenes_3d_ptr);

typedef struct {
	u16 objects_2d_count;
	u16 objects_3d_count;
	u16 scenes_2d_count;
	u16 scenes_3d_count;
} se_scene_handle_params;

typedef struct {
	se_render_handle *render_handle;
	se_objects_2d objects_2d;
	se_objects_3d objects_3d;
	se_scenes_2d scenes_2d;
	se_scenes_3d scenes_3d;
} se_scene_handle;

// scene handle functions
extern se_scene_handle *se_scene_handle_create(se_render_handle *render_handle, const se_scene_handle_params *params);
extern void se_scene_handle_cleanup(se_scene_handle *scene_handle);

// 2D objects functions
extern se_object_2d *se_object_2d_create(se_scene_handle *scene_handle, const c8 *fragment_shader_path, const s_mat3 *transform, const sz max_instances_count);
extern se_object_2d *se_object_2d_create_custom(se_scene_handle *scene_handle, se_object_custom *custom, const s_mat3 *transform);
extern void se_object_custom_set_data(se_object_custom *custom, const void *data, const sz size);
extern void se_object_2d_destroy(se_scene_handle *scene_handle, se_object_2d *object);
extern void se_object_2d_set_transform(se_object_2d *object, const s_mat3 *transform);
extern s_mat3 se_object_2d_get_transform(se_object_2d *object);
extern void se_object_2d_set_position(se_object_2d *object, const s_vec2 *position);
extern s_vec2 se_object_2d_get_position(se_object_2d *object);
extern void se_object_2d_set_scale(se_object_2d *object, const s_vec2 *scale);
extern s_vec2 se_object_2d_get_scale(se_object_2d *object);
extern void se_object_2d_get_box_2d(se_object_2d *object, se_box_2d *out_box);
extern void se_object_2d_set_shader(se_object_2d *object, se_shader *shader);
extern void se_object_2d_update_uniforms(se_object_2d *object);
extern se_instance_id se_object_2d_add_instance(se_object_2d *object, const s_mat3 *transform, const s_mat3 *buffer);
extern i32 se_object_2d_get_instance_index(se_object_2d *object, const se_instance_id instance_id);
extern void se_object_2d_set_instance_transform(se_object_2d *object, const se_instance_id instance_id, const s_mat3 *transform);
extern void se_object_2d_set_instance_buffer(se_object_2d *object, const se_instance_id instance_id, const s_mat3 *buffer);
extern void se_object_2d_set_instances_transforms(se_object_2d *object, const se_transforms_2d *transforms);
extern void se_object_2d_set_instances_buffers(se_object_2d *object, const se_buffers_2d *buffers);
extern void se_object_2d_set_instances_dirty(se_object_2d *object, const b8 dirty);
extern b8 se_object_2d_are_instances_dirty(se_object_2d *object);
extern sz se_object_2d_get_instance_count(se_object_2d *object);

// 2D scene functions
extern se_scene_2d *se_scene_2d_create(se_scene_handle *scene_handle, const s_vec2 *size, const u16 object_count);
extern void se_scene_2d_set_auto_resize(se_scene_2d *scene, se_window *window, const s_vec2 *ratio); // ratio can only be 1 for now
extern void se_scene_2d_destroy(se_scene_handle *scene_handle, se_scene_2d *scene);
extern void se_scene_2d_bind(se_scene_2d *scene);
extern void se_scene_2d_unbind(se_scene_2d *scene);
extern void se_scene_2d_render_raw(se_scene_2d *scene, se_render_handle *render_handle); // Does not bind/unbind
extern void se_scene_2d_render(se_scene_2d *scene, se_render_handle *render_handle); // Binds/unbinds the scene
extern void se_scene_2d_render_to_screen(se_scene_2d *scene, se_render_handle *render_handle, se_window *window);
extern void se_scene_2d_add_object(se_scene_2d *scene, se_object_2d *object);
extern void se_scene_2d_remove_object(se_scene_2d *scene, se_object_2d *object);

// 3D scene functions
extern se_scene_3d *se_scene_3d_create(se_scene_handle *scene_handle, const s_vec2 *size, const u16 object_count);
extern void se_scene_3d_set_auto_resize(se_scene_3d *scene, se_window *window, const s_vec2 *ratio);
extern void se_scene_3d_destroy(se_scene_handle *scene_handle, se_scene_3d *scene);
extern void se_scene_3d_render(se_scene_3d *scene, se_render_handle *render_handle);
extern void se_scene_3d_render_to_screen(se_scene_3d *scene, se_render_handle *render_handle, se_window *window);
extern void se_scene_3d_add_object(se_scene_3d *scene, se_object_3d *object);
extern void se_scene_3d_remove_object(se_scene_3d *scene, se_object_3d *object);
extern void se_scene_3d_set_camera(se_scene_3d *scene, se_camera *camera);
extern void se_scene_3d_add_post_process_buffer(se_scene_3d *scene, se_render_buffer *buffer);
extern void se_scene_3d_remove_post_process_buffer(se_scene_3d *scene, se_render_buffer *buffer);

// 3D objects functions
extern se_object_3d *se_object_3d_create(se_scene_handle *scene_handle, se_model *model, const s_mat4 *transform, const sz max_instances_count);
extern void se_object_3d_destroy(se_scene_handle *scene_handle, se_object_3d *object);
extern void se_object_3d_set_transform(se_object_3d *object, const s_mat4 *transform);
extern s_mat4 se_object_3d_get_transform(se_object_3d *object);
extern se_instance_id se_object_3d_add_instance(se_object_3d *object, const s_mat4 *transform, const s_mat4 *buffer);
extern i32 se_object_3d_get_instance_index(se_object_3d *object, const se_instance_id instance_id);
extern void se_object_3d_set_instance_transform(se_object_3d *object, const se_instance_id instance_id, const s_mat4 *transform);
extern void se_object_3d_set_instance_buffer(se_object_3d *object, const se_instance_id instance_id, const s_mat4 *buffer);
extern void se_object_3d_set_instances_dirty(se_object_3d *object, const b8 dirty);
extern b8 se_object_3d_are_instances_dirty(se_object_3d *object);
extern sz se_object_3d_get_instance_count(se_object_3d *object);

#endif // SE_SCENE_H
