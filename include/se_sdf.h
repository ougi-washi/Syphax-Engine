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

typedef struct se_sdf {
    s_mat4 transform;
    se_sdf_type type;
    union {
        struct { f32 radius; } sphere;
        struct { s_vec3 size; } box;
    };
    se_sdf_noise noise_0;
    se_sdf_noise noise_1;
    se_sdf_noise noise_2;
    se_sdf_noise noise_3;

    // nodes/scene
    // don't set manually
    se_sdf_handle parent;
    s_array(se_sdf_handle, children);
    se_quad quad;
    se_shader_handle shader;
	se_framebuffer_handle output;
    se_texture_handle volume;
} se_sdf;

#define se_sdf_create(...) se_sdf_create_internal(&(se_sdf){__VA_ARGS__})
extern se_sdf_handle se_sdf_create_internal(const se_sdf* sdf);
extern void se_sdf_add_child(se_sdf_handle parent, se_sdf_handle child);
extern void se_sdf_render(se_sdf_handle sdf, se_camera_handle camera); 
extern void se_sdf_bake(se_sdf_handle sdf);
extern void se_sdf_set_position(se_sdf_handle sdf, const s_vec3* position);

#endif // SE_SDF_H
