// Syphax-Engine - Ougi Washi

#include "se_sdf.h"
#include "se_camera.h"
#include "se_framebuffer.h"
#include "se_texture.h"

se_sdf_handle se_sdf_create_internal(const se_sdf* sdf) {
    se_context *ctx = se_current_context();
    se_sdf_handle new_sdf = s_array_increment(&ctx->sdfs);
    se_sdf *sdf_ptr = s_array_get(&ctx->sdfs, new_sdf);
    if (sdf_ptr) {
        memcpy(sdf_ptr, sdf, sizeof(se_sdf));
        sdf_ptr->parent = S_HANDLE_NULL;
        s_array_clear(&sdf_ptr->children);
    }
    return new_sdf;
}

void se_sdf_add_child(se_sdf_handle parent, se_sdf_handle child) {
    se_context *ctx = se_current_context();
    se_sdf *parent_ptr = s_array_get(&ctx->sdfs, parent);
    se_sdf *child_ptr = s_array_get(&ctx->sdfs, child);
    child_ptr->parent = parent;
    s_array_add(&parent_ptr->children, child);
}

void se_sdf_gen_uniform(c8 *out, se_sdf_handle sdf) {
    se_context *ctx = se_current_context();
    se_sdf *sdf_ptr = s_array_get(&ctx->sdfs, sdf);
    switch (sdf_ptr->type) {
    case SE_SDF_SPHERE:
        sprintf(out, "uniform float _%u_radius;", sdf);
        break;
    case SE_SDF_BOX:
        sprintf(out, "uniform vec3 _%u_size;", sdf);
    default:
        break;
    }
}

void se_sdf_gen_function(c8 *out, se_sdf_handle sdf) {
    se_context* ctx = se_current_context();
    se_sdf *sdf_ptr = s_array_get(&ctx->sdfs, sdf);
    switch (sdf_ptr->type) {
    case SE_SDF_SPHERE:
        sprintf(out, "length(p) - _%u_radius", sdf);
        break;
    case SE_SDF_BOX:
        // TODO: implement 
        break;
    default:
        break;
    }
}

void se_sdf_render_raw(se_sdf_handle sdf, se_camera_handle camera) {
    se_context *ctx = se_current_context();
    se_sdf *sdf_ptr = s_array_get(&ctx->sdfs, sdf);
    se_camera *camera_ptr = s_array_get(&ctx->cameras, camera);

    if (sdf_ptr->volume != S_HANDLE_NULL) {
        // use baked volume
    }
    else {
        // render normally
    }

    se_sdf_handle *child_handle = NULL;
    s_foreach(&sdf_ptr->children, child_handle) {
        se_sdf_render_raw(*child_handle, camera);
    }
}

void se_sdf_render(se_sdf_handle sdf, se_camera_handle camera) {
    se_context *ctx = se_current_context();
    se_sdf *sdf_ptr = s_array_get(&ctx->sdfs, sdf);
    if (sdf_ptr->output == S_HANDLE_NULL) {
        se_framebuffer_create(&s_vec2(1920, 1080));
    }
    se_sdf_render_raw(sdf, camera);
}

void se_sdf_bake(se_sdf_handle sdf) {
    // TODO: implement in the future, not now
}

