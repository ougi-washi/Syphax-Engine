// Syphax-Engine - Ougi Washi

#include <stdio.h>
#include <stdlib.h>

#include "se.h"
#include "se_window.h"
#include "se_graphics.h"
#include "se_camera.h"
#include "se_scene.h"
#include "se_physics.h"
#include "se_sdf.h"

i32 main(void) {
	se_context *ctx = se_context_create();
    se_window_handle window = se_window_create("Syphax - Civilization", 1280, 720);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);

    se_camera_handle camera = se_camera_create();

    se_sdf_handle ground = se_sdf_create( 
        .transform = s_mat4_identity,
        .type = SE_SDF_BOX,
        .box.size = s_vec3(10., .1, 10.),
        .noise_0 = {
           .type = SE_SDF_NOISE_PERLIN,
           .frequency = 1.,
        },
        .noise_1 = {
            .type = SE_SDF_NOISE_PERLIN,
            .frequency = 0.4,
            .offset = s_vec3(.3, .5, .1)
        }
    );

    se_sdf_handle sphere = se_sdf_create(
        .transform = s_mat4_identity,
        .type = SE_SDF_SPHERE,
        .sphere.radius = 1.
    );

    se_sdf_handle root = se_sdf_create();
    se_sdf_add_child(root, ground);
    // add 100 spheres
    for (u16 i = 0; i < 10; i++) {
        for (u16 j = 0; j < 10; j++) {
            se_sdf_set_position(sphere, &s_vec3(i, 0, j));
            se_sdf_add_child(root, sphere);
        }
    }
    se_sdf_bake(root);

    se_window_loop(window, 
        se_render_clear();
	    se_render_set_background(s_vec4(0.0f, 0.0f, 0.0f, 0.0f));
        se_sdf_render(root);
    );

    se_context_destroy(ctx);

}
