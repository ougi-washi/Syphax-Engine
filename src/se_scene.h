// Syphax-Engine - Ougi Washi

// Scenes are used as a collection of pointers to rendering instances
// They are not responsible for handling memory, and are only used for referencing the rendering instances
// Render handles are the ones responsible for handling memory, so no allocation and deallocation is needed here

#ifndef SE_SCENE_H
#define SE_SCENE_H

#include "se_render.h"
#include "se_window.h"

#define SE_MAX_SCENES 128
#define SE_MAX_2D_OBJECTS 1024
#define SE_MAX_3D_OBJECTS 1024

typedef struct {
    se_vec2 position;
    se_vec2 scale;
    se_shader_ptr shader;
} se_object_2d;
typedef s_array(se_object_2d, se_objects_2d);
typedef se_object_2d* se_object_2d_ptr;
typedef s_array(se_object_2d_ptr, se_objects_2d_ptr);

typedef struct {
    se_model* model;
    se_mat4 transform;
} se_object_3d;
typedef s_array(se_object_3d, se_objects_3d);
typedef se_object_3d* se_object_3d_ptr;
typedef s_array(se_object_3d_ptr, se_objects_3d_ptr);

typedef struct {
    se_objects_2d_ptr objects;
    se_framebuffer_ptr output;
} se_scene_2d;
typedef s_array(se_scene_2d, se_scenes_2d);
typedef se_scene_2d* se_scene_2d_ptr;
typedef s_array(se_scene_2d_ptr, se_scenes_2d_ptr);

typedef struct {
    se_models_ptr models;
    se_camera_ptr camera;
    se_render_buffers_ptr post_process;
    
    se_shader_ptr output_shader;
    se_render_buffer_ptr output;
} se_scene_3d;
typedef s_array(se_scene_3d, se_scenes_3d);
typedef se_scene_3d* se_scene_3d_ptr;
typedef s_array(se_scene_3d_ptr, se_scenes_3d_ptr);

typedef struct {
    u16 objects_2d_count;
    u16 objects_3d_count;
    u16 scenes_2d_count;
    u16 scenes_3d_count;
} se_scene_handle_params;

typedef struct {
    se_render_handle* render_handle;
    se_objects_2d objects_2d;
    se_objects_3d objects_3d;
    se_scenes_2d scenes_2d;
    se_scenes_3d scenes_3d;
} se_scene_handle;

// scene handle functions
extern se_scene_handle* se_scene_handle_create(se_render_handle* render_handle, const se_scene_handle_params* params);
extern void se_scene_handle_cleanup(se_scene_handle* scene_handle);

// 2D objects functions
extern se_object_2d* se_object_2d_create(se_scene_handle* scene_handle, const c8* fragment_shader_path, const se_vec2* position, const se_vec2* scale);
extern void se_object_2d_get_box_2d(se_object_2d* object, se_box_2d* out_box);
extern void se_object_2d_destroy(se_scene_handle* scene_handle, se_object_2d* object);
extern void se_object_2d_set_position(se_object_2d* object, const se_vec2* position);
extern void se_object_2d_set_scale(se_object_2d* object, const se_vec2* scale);
extern void se_object_2d_set_shader(se_object_2d* object, se_shader* shader);
extern void se_object_2d_update_uniforms(se_object_2d* object);

// 2D scene functions
extern se_scene_2d* se_scene_2d_create(se_scene_handle* scene_handle, const se_vec2* size, const u16 object_count);
extern void se_scene_2d_set_auto_resize(se_scene_2d* scene, se_window* window, const se_vec2* ratio); // ratio can only be 1 for now
extern void se_scene_2d_destroy(se_scene_handle *scene_handle, se_scene_2d* scene);
extern void se_scene_2d_render(se_scene_2d* scene, se_render_handle* render_handle, se_window* window);
extern void se_scene_2d_render_to_screen(se_scene_2d* scene, se_render_handle* render_handle, se_window* window);
extern void se_scene_2d_add_object(se_scene_2d* scene, se_object_2d* object);
extern void se_scene_2d_remove_object(se_scene_2d* scene, se_object_2d* object);

// 3D scene functions
extern se_scene_3d* se_scene_3d_create(se_scene_handle* scene_handle, const se_vec2* size, const u16 object_count);
extern void se_scene_3d_destroy(se_scene_handle *scene_handle, se_scene_3d* scene);
extern void se_scene_3d_render(se_scene_3d* scene, se_render_handle* render_handle);
extern void se_scene_3d_add_model(se_scene_3d* scene, se_model* model);
extern void se_scene_3d_remove_model(se_scene_3d* scene, se_model* model);
extern void se_scene_3d_set_camera(se_scene_3d* scene, se_camera* camera);
extern void se_scene_3d_add_post_process_buffer(se_scene_3d* scene, se_render_buffer* buffer);
extern void se_scene_3d_remove_post_process_buffer(se_scene_3d* scene, se_render_buffer* buffer);

#endif // SE_SCENE_H
