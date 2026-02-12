// Syphax-Engine - Ougi Washi

#ifndef SE_GLTF_H
#define SE_GLTF_H

#include "se_defines.h"
#include "syphax/s_array.h"
#include "syphax/s_types.h"

typedef struct s_json s_json;

typedef enum {
	SE_GLTF_ACCESSOR_SCALAR = 0,
	SE_GLTF_ACCESSOR_VEC2,
	SE_GLTF_ACCESSOR_VEC3,
	SE_GLTF_ACCESSOR_VEC4,
	SE_GLTF_ACCESSOR_MAT2,
	SE_GLTF_ACCESSOR_MAT3,
	SE_GLTF_ACCESSOR_MAT4
} se_gltf_accessor_type;

typedef struct {
	char *version;
	char *generator;
	char *min_version;
	char *copyright;
	s_json *extras;
	s_json *extensions;
} se_gltf_asset_info;

typedef s_array(char *, se_gltf_strings);
typedef s_array(i32, se_gltf_i32_array);
typedef s_array(f32, se_gltf_f32_array);

typedef struct {
	char *uri;
	u32 byte_length;
	u8 *data;
	sz data_size;
	b8 owns_data;
	char *name;
	s_json *extras;
	s_json *extensions;
} se_gltf_buffer;
typedef s_array(se_gltf_buffer, se_gltf_buffers);

typedef struct {
	i32 buffer;
	u32 byte_offset;
	u32 byte_length;
	u32 byte_stride;
	i32 target;
	b8 has_byte_stride;
	b8 has_target;
	char *name;
	s_json *extras;
	s_json *extensions;
} se_gltf_buffer_view;
typedef s_array(se_gltf_buffer_view, se_gltf_buffer_views);

typedef struct {
	i32 buffer_view;
	u32 byte_offset;
	i32 component_type;
	b8 has_byte_offset;
	s_json *extras;
	s_json *extensions;
} se_gltf_accessor_sparse_indices;

typedef struct {
	i32 buffer_view;
	u32 byte_offset;
	b8 has_byte_offset;
	s_json *extras;
	s_json *extensions;
} se_gltf_accessor_sparse_values;

typedef struct {
	u32 count;
	se_gltf_accessor_sparse_indices indices;
	se_gltf_accessor_sparse_values values;
	s_json *extras;
	s_json *extensions;
} se_gltf_accessor_sparse;

typedef struct {
	i32 buffer_view;
	u32 byte_offset;
	u32 count;
	i32 component_type;
	se_gltf_accessor_type type;
	b8 normalized;
	b8 has_buffer_view;
	b8 has_byte_offset;
	b8 has_normalized;
	b8 has_min;
	b8 has_max;
	b8 has_sparse;
	f32 min_values[16];
	f32 max_values[16];
	se_gltf_accessor_sparse sparse;
	char *name;
	s_json *extras;
	s_json *extensions;
} se_gltf_accessor;
typedef s_array(se_gltf_accessor, se_gltf_accessors);

typedef struct {
	i32 index;
	i32 tex_coord;
	b8 has_tex_coord;
	s_json *extras;
	s_json *extensions;
} se_gltf_texture_info;

typedef struct {
	se_gltf_texture_info info;
	f32 scale;
	b8 has_scale;
} se_gltf_normal_texture_info;

typedef struct {
	se_gltf_texture_info info;
	f32 strength;
	b8 has_strength;
} se_gltf_occlusion_texture_info;

typedef struct {
	b8 has_base_color_factor;
	f32 base_color_factor[4];
	b8 has_metallic_factor;
	f32 metallic_factor;
	b8 has_roughness_factor;
	f32 roughness_factor;
	b8 has_base_color_texture;
	se_gltf_texture_info base_color_texture;
	b8 has_metallic_roughness_texture;
	se_gltf_texture_info metallic_roughness_texture;
	s_json *extras;
	s_json *extensions;
} se_gltf_pbr_metallic_roughness;

typedef struct {
	char *name;
	b8 has_pbr_metallic_roughness;
	se_gltf_pbr_metallic_roughness pbr_metallic_roughness;
	b8 has_normal_texture;
	se_gltf_normal_texture_info normal_texture;
	b8 has_occlusion_texture;
	se_gltf_occlusion_texture_info occlusion_texture;
	b8 has_emissive_texture;
	se_gltf_texture_info emissive_texture;
	b8 has_emissive_factor;
	f32 emissive_factor[3];
	char *alpha_mode;
	b8 has_alpha_mode;
	f32 alpha_cutoff;
	b8 has_alpha_cutoff;
	b8 double_sided;
	b8 has_double_sided;
	s_json *extras;
	s_json *extensions;
} se_gltf_material;
typedef s_array(se_gltf_material, se_gltf_materials);

typedef struct {
	char *uri;
	char *mime_type;
	i32 buffer_view;
	b8 has_buffer_view;
	u8 *data;
	sz data_size;
	b8 owns_data;
	char *name;
	s_json *extras;
	s_json *extensions;
} se_gltf_image;
typedef s_array(se_gltf_image, se_gltf_images);

typedef struct {
	i32 mag_filter;
	i32 min_filter;
	i32 wrap_s;
	i32 wrap_t;
	b8 has_mag_filter;
	b8 has_min_filter;
	b8 has_wrap_s;
	b8 has_wrap_t;
	char *name;
	s_json *extras;
	s_json *extensions;
} se_gltf_sampler;
typedef s_array(se_gltf_sampler, se_gltf_samplers);

typedef struct {
	i32 sampler;
	i32 source;
	b8 has_sampler;
	b8 has_source;
	char *name;
	s_json *extras;
	s_json *extensions;
} se_gltf_texture;
typedef s_array(se_gltf_texture, se_gltf_textures);

typedef struct {
	char *name;
	i32 accessor;
} se_gltf_attribute;

typedef struct {
	s_array(se_gltf_attribute, attributes);
} se_gltf_attribute_set;

typedef struct {
	se_gltf_attribute_set attributes;
	i32 indices;
	i32 material;
	i32 mode;
	b8 has_indices;
	b8 has_material;
	b8 has_mode;
	s_array(se_gltf_attribute_set, targets);
	s_json *extras;
	s_json *extensions;
} se_gltf_primitive;
typedef s_array(se_gltf_primitive, se_gltf_primitives);

typedef struct {
	char *name;
	se_gltf_primitives primitives;
	se_gltf_f32_array weights;
	b8 has_weights;
	s_json *extras;
	s_json *extensions;
} se_gltf_mesh;
typedef s_array(se_gltf_mesh, se_gltf_meshes);

typedef struct {
	char *name;
	se_gltf_i32_array children;
	i32 mesh;
	i32 skin;
	i32 camera;
	b8 has_mesh;
	b8 has_skin;
	b8 has_camera;
	b8 has_matrix;
	f32 matrix[16];
	b8 has_translation;
	f32 translation[3];
	b8 has_rotation;
	f32 rotation[4];
	b8 has_scale;
	f32 scale[3];
	se_gltf_f32_array weights;
	b8 has_weights;
	s_json *extras;
	s_json *extensions;
} se_gltf_node;
typedef s_array(se_gltf_node, se_gltf_nodes);

typedef struct {
	char *name;
	se_gltf_i32_array nodes;
	s_json *extras;
	s_json *extensions;
} se_gltf_scene;
typedef s_array(se_gltf_scene, se_gltf_scenes);

typedef struct {
	char *name;
	i32 inverse_bind_matrices;
	i32 skeleton;
	b8 has_inverse_bind_matrices;
	b8 has_skeleton;
	se_gltf_i32_array joints;
	s_json *extras;
	s_json *extensions;
} se_gltf_skin;
typedef s_array(se_gltf_skin, se_gltf_skins);

typedef struct {
	i32 input;
	i32 output;
	char *interpolation;
	b8 has_interpolation;
	s_json *extras;
	s_json *extensions;
} se_gltf_animation_sampler;

typedef struct {
	i32 node;
	b8 has_node;
	char *path;
	s_json *extras;
	s_json *extensions;
} se_gltf_animation_channel_target;

typedef struct {
	i32 sampler;
	se_gltf_animation_channel_target target;
	s_json *extras;
	s_json *extensions;
} se_gltf_animation_channel;

typedef struct {
	char *name;
	s_array(se_gltf_animation_sampler, samplers);
	s_array(se_gltf_animation_channel, channels);
	s_json *extras;
	s_json *extensions;
} se_gltf_animation;
typedef s_array(se_gltf_animation, se_gltf_animations);

typedef struct {
	f32 yfov;
	f32 znear;
	f32 zfar;
	f32 aspect_ratio;
	b8 has_zfar;
	b8 has_aspect_ratio;
} se_gltf_camera_perspective;

typedef struct {
	f32 xmag;
	f32 ymag;
	f32 znear;
	f32 zfar;
} se_gltf_camera_orthographic;

typedef struct {
	char *name;
	char *type;
	b8 has_type;
	b8 has_perspective;
	se_gltf_camera_perspective perspective;
	b8 has_orthographic;
	se_gltf_camera_orthographic orthographic;
	s_json *extras;
	s_json *extensions;
} se_gltf_camera;
typedef s_array(se_gltf_camera, se_gltf_cameras);

typedef struct {
	se_gltf_asset_info asset;
	se_gltf_strings extensions_used;
	se_gltf_strings extensions_required;
	se_gltf_buffers buffers;
	se_gltf_buffer_views buffer_views;
	se_gltf_accessors accessors;
	se_gltf_images images;
	se_gltf_samplers samplers;
	se_gltf_textures textures;
	se_gltf_materials materials;
	se_gltf_meshes meshes;
	se_gltf_nodes nodes;
	se_gltf_scenes scenes;
	se_gltf_skins skins;
	se_gltf_animations animations;
	se_gltf_cameras cameras;
	i32 default_scene;
	b8 has_default_scene;
	char *source_path;
	char *base_dir;
	s_json *extras;
	s_json *extensions;
} se_gltf_asset;

typedef struct {
	b8 load_buffers;
	b8 load_images;
	b8 decode_data_uris;
} se_gltf_load_params;

typedef struct {
	b8 embed_buffers;
	b8 embed_images;
	b8 write_glb;
} se_gltf_write_params;

extern se_gltf_asset *se_gltf_load(const char *path, const se_gltf_load_params *params);
extern void se_gltf_free(se_gltf_asset *asset);
extern b8 se_gltf_write(const se_gltf_asset *asset, const char *path, const se_gltf_write_params *params);

#endif // SE_GLTF_H
