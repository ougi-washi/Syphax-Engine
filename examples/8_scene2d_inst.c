// Syphax-Engine - Ougi Washi

#include "se_scene.h"

#define WIDTH 1920
#define HEIGHT 1080

#define INSTANCE_COUNT 16

i32 main(void) {
	se_context *ctx = se_context_create();
	se_window_handle window = se_window_create("Syphax-Engine - Scene 2D Example", WIDTH, HEIGHT);
	se_scene_2d_handle scene_2d = se_scene_2d_create(&s_vec2(WIDTH, HEIGHT), 4);
	se_object_2d_handle button = S_HANDLE_NULL;

	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	s_mat3 transform = s_mat3_identity;
	button = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d_inst/instance.glsl"), &transform, 16);
	se_object_2d_set_position(button, &s_vec2(0.15, 0.));
	se_object_2d_set_scale(button, &s_vec2(0.1, 0.1));
	s_mat3 instance_transform = s_mat3_identity;
	for (i32 i = 0; i < 16; i++) {
		s_mat3_set_translation(&instance_transform, &s_vec2(-0.8f + i * 0.1f, 0.0f));
		se_object_2d_add_instance(button, &instance_transform, &s_mat4_identity);
	}

	se_object_2d *button_ptr = s_array_get(&ctx->objects_2d, button);
	if (button_ptr) {
		se_shader_set_vec3(button_ptr->shader, "u_color", &s_vec3(0, 1, 0));
	}
	se_scene_2d_add_object(scene_2d, button);
	se_scene_2d_set_auto_resize(scene_2d, window, &s_vec2(1., 1.));

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_context_reload_changed_shaders();
		// Update position using setter
		s_vec2 current_pos = se_object_2d_get_position(button);
		se_object_2d_set_position(button, &s_vec2(current_pos.x + 0.001, current_pos.y));
		se_scene_2d_draw(scene_2d, window);
	}

	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
