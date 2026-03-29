// Syphax-Engine - Ougi Washi

#ifndef SE_SDF_H
#define SE_SDF_H

#include "se.h"
#include "se_quad.h"

typedef enum se_sdf_type {
    SE_SDF_CUSTOM,
    SE_SDF_SPHERE,
    SE_SDF_BOX,
    //...
} se_sdf_type;

typedef enum se_sdf_operator {
    SE_SDF_UNION,
    SE_SDF_SMOOTH_UNION,
    //...
} se_sdf_operator;

typedef enum se_sdf_noise_type {
    SE_SDF_NOISE_NONE,
    SE_SDF_NOISE_PERLIN,
    SE_SDF_NOISE_VORNOI,
    //...
} se_sdf_noise_type;

typedef struct se_sdf_noise {
    se_sdf_noise_type type;
    f32 frequency;
    s_vec3 offset;
} se_sdf_noise;

typedef struct se_sdf_directional_light {
    s_vec3 direction;
    s_vec3 color;
} se_sdf_directional_light;

typedef struct se_sdf_point_light {
    s_vec3 position;
    s_vec3 color;
	f32 radius;
} se_sdf_point_light;

typedef struct se_sdf_shading {
    s_vec3 ambient;
    s_vec3 diffuse;
    s_vec3 specular;
    f32 roughness;
	f32 bias;
	f32 smoothness;
} se_sdf_shading;

typedef struct se_sdf_shadow {
	f32 softness;
	f32 bias;
	u16 samples;
} se_sdf_shadow;

typedef struct se_sdf_lod {
	f32 distance;
	u16 steps;
	b8 noise;
	b8 point_lights;
	b8 shadows;
} se_sdf_lod;

typedef struct se_sdf_lods {
	se_sdf_lod low;
	se_sdf_lod medium;
	se_sdf_lod high;
} se_sdf_lods;

typedef struct se_sdf {
    s_mat4 transform;
    se_sdf_type type;
	se_sdf_operator operation;
	f32 operation_amount;
	se_sdf_shading shading;
	se_sdf_shadow shadow;
	se_sdf_lods lods;
    union {
        struct { f32 radius; } sphere;
        struct { s_vec3 size; } box;
    };

	// nodes/scene
    // don't set manually
    se_sdf_handle parent;
    s_array(se_sdf_handle, children);
	s_array(se_sdf_noise_handle, noises);
	s_array(se_sdf_point_light_handle, point_lights);
	s_array(se_sdf_directional_light_handle, directional_lights);
    se_quad quad;
    se_shader_handle shader;
	se_framebuffer_handle output;
    se_texture_handle volume;
} se_sdf;

#define se_sdf_create(...) se_sdf_create_internal(&(se_sdf){__VA_ARGS__})
#define SE_SDF_NULL S_HANDLE_NULL
extern se_sdf_handle se_sdf_create_internal(const se_sdf* sdf);
extern void se_sdf_destroy(se_sdf_handle sdf);
extern void se_sdf_add_child(se_sdf_handle parent, se_sdf_handle child);

#define se_sdf_add_noise(sdf, ...) se_sdf_add_noise_internal((sdf), (se_sdf_noise){__VA_ARGS__})
extern se_sdf_noise_handle se_sdf_add_noise_internal(se_sdf_handle sdf, const se_sdf_noise noise);
extern f32 se_sdf_get_noise_frequency(se_sdf_noise_handle noise);
extern void se_sdf_noise_set_frequency(se_sdf_noise_handle noise, f32 frequency);
extern s_vec3 se_sdf_get_noise_offset(se_sdf_noise_handle noise);
extern void se_sdf_noise_set_offset(se_sdf_noise_handle noise, const s_vec3* offset);

#define se_sdf_add_point_light(sdf, ...) se_sdf_add_point_light_internal((sdf), (se_sdf_point_light){__VA_ARGS__})
extern se_sdf_point_light_handle se_sdf_add_point_light_internal(se_sdf_handle sdf, const se_sdf_point_light point_light);
extern void se_sdf_point_light_remove(se_sdf_handle sdf, se_sdf_point_light_handle point_light);
extern s_vec3 se_sdf_get_point_light_position(se_sdf_point_light_handle point_light);
extern void se_sdf_point_light_set_position(se_sdf_point_light_handle point_light, const s_vec3* position);
extern s_vec3 se_sdf_get_point_light_color(se_sdf_point_light_handle point_light);
extern void se_sdf_point_light_set_color(se_sdf_point_light_handle point_light, const s_vec3* color);
extern f32 se_sdf_get_point_light_radius(se_sdf_point_light_handle point_light);
extern void se_sdf_point_light_set_radius(se_sdf_point_light_handle point_light, f32 radius);

#define se_sdf_add_directional_light(sdf, ...) se_sdf_add_directional_light_internal((sdf), (se_sdf_directional_light){__VA_ARGS__})
extern se_sdf_directional_light_handle se_sdf_add_directional_light_internal(se_sdf_handle sdf, const se_sdf_directional_light directional_light);
extern s_vec3 se_sdf_get_directional_light_direction(se_sdf_directional_light_handle directional_light);
extern void se_sdf_directional_light_set_direction(se_sdf_directional_light_handle directional_light, const s_vec3* direction);
extern s_vec3 se_sdf_get_directional_light_color(se_sdf_directional_light_handle directional_light);
extern void se_sdf_directional_light_set_color(se_sdf_directional_light_handle directional_light, const s_vec3* color);

extern se_sdf_lods se_sdf_get_lods(se_sdf_handle sdf);
extern void se_sdf_set_lods(se_sdf_handle sdf, const se_sdf_lods* lods);

extern se_sdf_shading se_sdf_get_shading(se_sdf_handle sdf);
extern void se_sdf_set_shading(se_sdf_handle sdf, const se_sdf_shading* shading);
extern s_vec3 se_sdf_get_shading_ambient(se_sdf_handle sdf);
extern void se_sdf_set_shading_ambient(se_sdf_handle sdf, const s_vec3* ambient);
extern s_vec3 se_sdf_get_shading_diffuse(se_sdf_handle sdf);
extern void se_sdf_set_shading_diffuse(se_sdf_handle sdf, const s_vec3* diffuse);
extern s_vec3 se_sdf_get_shading_specular(se_sdf_handle sdf);
extern void se_sdf_set_shading_specular(se_sdf_handle sdf, const s_vec3* specular);
extern f32 se_sdf_get_shading_roughness(se_sdf_handle sdf);
extern void se_sdf_set_shading_roughness(se_sdf_handle sdf, f32 roughness);
extern f32 se_sdf_get_shading_bias(se_sdf_handle sdf);
extern void se_sdf_set_shading_bias(se_sdf_handle sdf, f32 bias);
extern f32 se_sdf_get_shading_smoothness(se_sdf_handle sdf);
extern void se_sdf_set_shading_smoothness(se_sdf_handle sdf, f32 smoothness);

extern se_sdf_shadow se_sdf_get_shadow(se_sdf_handle sdf);
extern void se_sdf_set_shadow(se_sdf_handle sdf, const se_sdf_shadow* shadow);
extern f32 se_sdf_get_shadow_softness(se_sdf_handle sdf);
extern void se_sdf_set_shadow_softness(se_sdf_handle sdf, f32 softness);
extern f32 se_sdf_get_shadow_bias(se_sdf_handle sdf);
extern void se_sdf_set_shadow_bias(se_sdf_handle sdf, f32 bias);
extern u16 se_sdf_get_shadow_samples(se_sdf_handle sdf);
extern void se_sdf_set_shadow_samples(se_sdf_handle sdf, u16 samples);

extern void se_sdf_render(se_sdf_handle sdf, se_camera_handle camera); 
extern void se_sdf_bake(se_sdf_handle sdf);
extern void se_sdf_set_position(se_sdf_handle sdf, const s_vec3* position);

#endif // SE_SDF_H
