// Syphax-Engine - Ougi Washi

#ifndef SE_LOADER_H
#define SE_LOADER_H

#include "loader/se_gltf.h"
#include "se_scene.h"

extern se_model *se_gltf_model_load_ex(se_render_handle *render_handle, const se_gltf_asset *asset, const i32 mesh_index, const se_mesh_data_flags mesh_data_flags);
extern se_model *se_gltf_model_load(se_render_handle *render_handle, const se_gltf_asset *asset, const i32 mesh_index);
extern se_model *se_gltf_model_load_first(se_render_handle *render_handle, const char *path, const se_gltf_load_params *params);
extern se_texture *se_gltf_image_load(se_render_handle *render_handle, const se_gltf_asset *asset, const i32 image_index, const se_texture_wrap wrap);
extern se_texture *se_gltf_texture_load(se_render_handle *render_handle, const se_gltf_asset *asset, const i32 texture_index, const se_texture_wrap wrap);
extern sz se_gltf_scene_load_from_asset(se_render_handle *render_handle, se_scene_handle *scene_handle, se_scene_3d *scene, const se_gltf_asset *asset, se_shader *mesh_shader, se_texture *default_texture, const se_texture_wrap wrap);
extern se_gltf_asset *se_gltf_scene_load(se_render_handle *render_handle, se_scene_handle *scene_handle, se_scene_3d *scene, const char *path, const se_gltf_load_params *load_params, se_shader *mesh_shader, se_texture *default_texture, const se_texture_wrap wrap, sz *out_objects_loaded);
extern b8 se_gltf_scene_compute_bounds(const se_gltf_asset *asset, s_vec3 *out_center, f32 *out_radius);
extern void se_gltf_scene_fit_camera(se_scene_3d *scene, const se_gltf_asset *asset);
extern void se_gltf_scene_get_navigation_speeds(const se_gltf_asset *asset, f32 *out_base_speed, f32 *out_fast_speed);

#endif // SE_LOADER_H
