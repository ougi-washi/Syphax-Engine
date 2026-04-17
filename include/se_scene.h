// Syphax-Engine - Ougi Washi

// Ownership: scene/object storage lives in se_context.

#ifndef SE_SCENE_H
#define SE_SCENE_H

#include "se_quad.h"
#include "se_window.h"
#include "se_camera.h"
#include "se_model.h"
#include "se_render_buffer.h"
#include "se_framebuffer.h"

typedef struct s_json s_json;

#define SE_MAX_SCENES 128
#define SE_MAX_2D_OBJECTS 1024
#define SE_MAX_3D_OBJECTS 1024

typedef i32 se_instance_id;
typedef s_array(se_instance_id, se_instance_ids);
typedef s_array(s_mat4, se_transforms);
typedef s_array(s_mat4, se_buffers);
typedef s_array(b8, se_instance_actives);

typedef b8 (*se_scene_pick_filter_2d)(se_object_2d_handle object, void* user_data);
typedef b8 (*se_scene_pick_filter_3d)(se_object_3d_handle object, void* user_data);
typedef s_handle se_scene_3d_custom_render_handle;
typedef void (*se_scene_3d_custom_render_callback)(se_scene_3d_handle scene, void* user_data);

typedef struct {
	se_object_3d_handle object;
	se_instance_id instance_id;
	s_vec3 point;
	f32 distance;
} se_scene_pick_hit_3d;

typedef struct {
	se_instance_ids ids;
	se_transforms transforms;
	se_buffers buffers;
	se_instance_actives actives;
	se_instance_ids free_indices;
	se_buffers metadata;
	se_instance_id next_id;
} se_instances;

typedef s_array(s_mat3, se_transforms_2d);
typedef struct {
	se_instance_ids ids;
	se_transforms_2d transforms;
	se_buffers buffers;
	se_instance_actives actives;
	se_instance_ids free_indices;
	se_buffers metadata;
	se_instance_id next_id;
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
			se_buffers render_buffers;
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
	s_mat4 transform;
	union {
		struct {
			se_model_handle model;
			se_instances instances;
			se_mesh_instances mesh_instances;
			se_transforms render_transforms;
			se_buffers render_buffers;
			se_buffers render_metadata;
		};
		se_object_custom custom;
	};
	b8 is_custom : 1;
	b8 is_visible : 1;
} se_object_3d;

typedef s_array(se_object_3d, se_objects_3d);
typedef se_object_3d_handle se_object_3d_ptr;
typedef s_array(se_object_3d_handle, se_objects_3d_ptr);

typedef struct {
	se_scene_3d_custom_render_callback callback;
	void* user_data;
} se_scene_3d_custom_render_entry;
typedef s_array(se_scene_3d_custom_render_entry, se_scene_3d_custom_render_entries);

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
	se_render_buffers_ptr post_process; // still wip
	se_shader_handle output_shader;
	se_framebuffer_handle output;
	se_scene_3d_custom_render_entries custom_renders;
	s_mat4 last_vp;
	b8 enable_culling : 1;
	b8 has_last_vp : 1;
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
extern b8 se_object_2d_remove_instance(const se_object_2d_handle object, const se_instance_id instance_id);
extern i32 se_object_2d_get_instance_index(const se_object_2d_handle object, const se_instance_id instance_id);
extern void se_object_2d_set_transform_by_id(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat3 *transform);
extern void se_object_2d_set_buffer_by_id(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat4 *buffer);
extern void se_object_2d_set_instances_transforms(const se_object_2d_handle object, const se_transforms_2d *transforms);
extern void se_object_2d_set_instances_buffers(const se_object_2d_handle object, const se_buffers *buffers);
extern void se_object_2d_set_transforms_by_id(const se_object_2d_handle object, const se_instance_id* instance_ids, const s_mat3* transforms, const sz count);
extern void se_object_2d_set_buffers_by_id(const se_object_2d_handle object, const se_instance_id* instance_ids, const s_mat4* buffers, const sz count);
extern b8 se_object_2d_set_active_by_id(const se_object_2d_handle object, const se_instance_id instance_id, const b8 active);
extern b8 se_object_2d_is_active_by_id(const se_object_2d_handle object, const se_instance_id instance_id);
extern sz se_object_2d_get_inactive_slot_count(const se_object_2d_handle object);
extern b8 se_object_2d_set_metadata_by_id(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat4* metadata);
extern b8 se_object_2d_get_metadata_by_id(const se_object_2d_handle object, const se_instance_id instance_id, s_mat4* out_metadata);
extern void se_object_2d_set_instances_dirty(const se_object_2d_handle object, const b8 dirty);
extern b8 se_object_2d_are_instances_dirty(const se_object_2d_handle object);
extern sz se_object_2d_get_instance_count(const se_object_2d_handle object);
extern s_json* se_object_2d_to_json(const se_object_2d_handle object);
extern b8 se_object_2d_to_json_file(const se_object_2d_handle object, const c8* path);
extern b8 se_object_2d_from_json(const se_object_2d_handle object, const s_json* root);
extern b8 se_object_2d_from_json_file(const se_object_2d_handle object, const c8* path);

// 2D scene functions
extern se_scene_2d_handle se_scene_2d_create(const s_vec2 *size, const u16 object_count);
extern void se_scene_2d_set_fit_to_window(const se_scene_2d_handle scene, const se_window_handle window, const s_vec2 *ratio);
extern void se_scene_2d_destroy(const se_scene_2d_handle scene);
// Destroys every object referenced by the scene, optionally destroys their shaders,
// then destroys the scene itself.
extern void se_scene_2d_destroy_full(const se_scene_2d_handle scene, const b8 destroy_object_shaders);
extern void se_scene_2d_bind(const se_scene_2d_handle scene);
extern void se_scene_2d_unbind(const se_scene_2d_handle scene);
extern void se_scene_2d_render_raw(const se_scene_2d_handle scene);
extern void se_scene_2d_render_to_buffer(const se_scene_2d_handle scene);
extern void se_scene_2d_render_to_screen(const se_scene_2d_handle scene, const se_window_handle window);
extern void se_scene_2d_draw(const se_scene_2d_handle scene, const se_window_handle window);
extern void se_scene_2d_add_object(const se_scene_2d_handle scene, const se_object_2d_handle object);
extern void se_scene_2d_remove_object(const se_scene_2d_handle scene, const se_object_2d_handle object);
extern b8 se_scene_2d_pick_object(const se_scene_2d_handle scene, const s_vec2* point_ndc, se_scene_pick_filter_2d filter, void* user_data, se_object_2d_handle* out_object);
extern s_json* se_scene_2d_to_json(const se_scene_2d_handle scene);
// Saves a scene JSON snapshot using the engine's platform-aware writable path rules.
extern b8 se_scene_2d_to_json_file(const se_scene_2d_handle scene, const c8* path);
extern b8 se_scene_2d_from_json(const se_scene_2d_handle scene, const s_json* root);
// Loads a scene JSON snapshot from either normal files or packaged assets.
extern b8 se_scene_2d_from_json_file(const se_scene_2d_handle scene, const c8* path);

// 3D scene functions
extern se_scene_3d_handle se_scene_3d_create(const s_vec2 *size, const u16 object_count);
extern se_scene_3d_handle se_scene_3d_create_for_window(const se_window_handle window, const u16 object_count);
extern void se_scene_3d_set_fit_to_window(const se_scene_3d_handle scene, const se_window_handle window, const s_vec2 *ratio);
extern void se_scene_3d_destroy(const se_scene_3d_handle scene);
// Destroys every object referenced by the scene. Optional flags can also destroy
// referenced models and their shaders before destroying the scene itself.
extern void se_scene_3d_destroy_full(const se_scene_3d_handle scene, const b8 destroy_models, const b8 destroy_model_shaders);
extern void se_scene_3d_render_to_buffer(const se_scene_3d_handle scene);
extern void se_scene_3d_render_to_screen(const se_scene_3d_handle scene, const se_window_handle window);
extern void se_scene_3d_draw(const se_scene_3d_handle scene, const se_window_handle window);
extern void se_scene_3d_add_object(const se_scene_3d_handle scene, const se_object_3d_handle object);
extern se_object_3d_handle se_scene_3d_add_model(const se_scene_3d_handle scene, const se_model_handle model, const s_mat4 *transform);
extern void se_scene_3d_remove_object(const se_scene_3d_handle scene, const se_object_3d_handle object);
extern b8 se_scene_3d_pick_instance_screen(const se_scene_3d_handle scene, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, const f32 pick_radius, se_scene_pick_filter_3d filter, void* user_data, se_scene_pick_hit_3d* out_hit);
extern b8 se_scene_3d_pick_object_screen(const se_scene_3d_handle scene, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, const f32 pick_radius, se_scene_pick_filter_3d filter, void* user_data, se_object_3d_handle* out_object, f32* out_distance);
extern void se_scene_3d_set_camera(const se_scene_3d_handle scene, const se_camera_handle camera);
extern se_camera_handle se_scene_3d_get_camera(const se_scene_3d_handle scene);
extern b8 se_scene_3d_get_output_depth_texture(const se_scene_3d_handle scene, u32* out_depth_texture);
extern void se_scene_3d_set_culling(const se_scene_3d_handle scene, const b8 enabled);
extern void se_scene_3d_add_post_process_buffer(const se_scene_3d_handle scene, const se_render_buffer_handle buffer);
extern void se_scene_3d_remove_post_process_buffer(const se_scene_3d_handle scene, const se_render_buffer_handle buffer);
extern se_scene_3d_custom_render_handle se_scene_3d_register_custom_render(const se_scene_3d_handle scene, se_scene_3d_custom_render_callback callback, void* user_data);
extern b8 se_scene_3d_unregister_custom_render(const se_scene_3d_handle scene, const se_scene_3d_custom_render_handle callback_handle);
extern s_json* se_scene_3d_to_json(const se_scene_3d_handle scene);
// Saves a scene JSON snapshot using the engine's platform-aware writable path rules.
extern b8 se_scene_3d_to_json_file(const se_scene_3d_handle scene, const c8* path);
extern b8 se_scene_3d_from_json(const se_scene_3d_handle scene, const s_json* root);
// Loads a scene JSON snapshot from either normal files or packaged assets.
extern b8 se_scene_3d_from_json_file(const se_scene_3d_handle scene, const c8* path);

// 3D objects functions
extern se_object_3d_handle se_object_3d_create(const se_model_handle model, const s_mat4 *transform, const sz max_instances_count);
extern se_object_3d_handle se_object_3d_create_custom(se_object_custom *custom, const s_mat4 *transform);
extern void se_object_3d_destroy(const se_object_3d_handle object);
extern void se_object_3d_set_transform(const se_object_3d_handle object, const s_mat4 *transform);
extern void se_object_3d_set_location(const se_object_3d_handle object, const s_vec3 *location);
extern void se_object_3d_set_rotation(const se_object_3d_handle object, const s_vec3 *rotation);
extern void se_object_3d_set_scale(const se_object_3d_handle object, const s_vec3 *scale);
extern s_mat4 se_object_3d_get_transform(const se_object_3d_handle object);
extern b8 se_object_3d_reserve_instances(const se_object_3d_handle object, const sz instance_capacity);
extern se_instance_id se_object_3d_add_instance(const se_object_3d_handle object, const s_mat4 *transform, const s_mat4 *buffer);
extern b8 se_object_3d_remove_instance(const se_object_3d_handle object, const se_instance_id instance_id);
extern i32 se_object_3d_get_instance_index(const se_object_3d_handle object, const se_instance_id instance_id);
extern void se_object_3d_set_transform_by_id(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4 *transform);
extern void se_object_3d_set_buffer_by_id(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4 *buffer);
extern void se_object_3d_set_transforms_by_id(const se_object_3d_handle object, const se_instance_id* instance_ids, const s_mat4* transforms, const sz count);
extern void se_object_3d_set_buffers_by_id(const se_object_3d_handle object, const se_instance_id* instance_ids, const s_mat4* buffers, const sz count);
extern b8 se_object_3d_set_active_by_id(const se_object_3d_handle object, const se_instance_id instance_id, const b8 active);
extern b8 se_object_3d_is_active_by_id(const se_object_3d_handle object, const se_instance_id instance_id);
extern sz se_object_3d_get_inactive_slot_count(const se_object_3d_handle object);
extern b8 se_object_3d_set_metadata_by_id(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4* metadata);
extern b8 se_object_3d_get_metadata_by_id(const se_object_3d_handle object, const se_instance_id instance_id, s_mat4* out_metadata);
extern void se_object_3d_set_instances_dirty(const se_object_3d_handle object, const b8 dirty);
extern b8 se_object_3d_are_instances_dirty(const se_object_3d_handle object);
extern sz se_object_3d_get_instance_count(const se_object_3d_handle object);
extern s_json* se_object_3d_to_json(const se_object_3d_handle object);
extern b8 se_object_3d_to_json_file(const se_object_3d_handle object, const c8* path);
extern b8 se_object_3d_from_json(const se_object_3d_handle object, const s_json* root);
extern b8 se_object_3d_from_json_file(const se_object_3d_handle object, const c8* path);

#endif // SE_SCENE_H
