// Syphax-Engine - Ougi Washi

#include "se_graphics.h"
#include "se_scene.h"

#include <math.h>
#include <stdio.h>

// What you will learn: create many 2D instances and animate them with one loop.
// Try this: change count, spacing, or wave speed to make new patterns.
#define INSTANCE_COUNT 18

int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Scene2D Instancing", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("scene2d_instancing :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.05f, 0.07f, 0.10f, 1.0f));

	se_scene_2d_handle scene = se_scene_2d_create(&s_vec2(1280.0f, 720.0f), 2);
	se_scene_2d_set_auto_resize(scene, window, &s_vec2(1.0f, 1.0f));
	se_object_2d_handle dots = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d_inst/instance.glsl"), &s_mat3_identity, INSTANCE_COUNT);
	se_scene_2d_add_object(scene, dots);

	se_instance_ids ids = {0};
	s_array_init(&ids);
	for (i32 i = 0; i < INSTANCE_COUNT; ++i) {
		s_mat3 transform = s_mat3_identity;
		s_mat3_set_translation(&transform, &s_vec2(-0.85f + i * 0.10f, 0.0f));
		se_instance_id id = se_object_2d_add_instance(dots, &transform, &s_mat4_identity);
		s_array_add(&ids, id);
	}

	b8 wave_enabled = true;
	printf("scene2d_instancing controls:\n");
	printf("  Space: pause/resume wave motion\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		if (se_window_is_key_pressed(window, SE_KEY_SPACE)) {
			wave_enabled = !wave_enabled;
		}

		const f32 time = (f32)se_window_get_time(window);
		for (sz i = 0; i < s_array_get_size(&ids); ++i) {
			const se_instance_id* id = s_array_get(&ids, s_array_handle(&ids, (u32)i));
			if (!id) {
				continue;
			}
			const f32 x = -0.85f + (f32)i * 0.10f;
			const f32 y = wave_enabled ? sinf(time * 2.2f + (f32)i * 0.45f) * 0.24f : 0.0f;
			s_mat3 transform = s_mat3_identity;
			s_mat3_set_translation(&transform, &s_vec2(x, y));
			se_object_2d_set_instance_transform(dots, *id, &transform);
		}

		se_render_clear();
		se_scene_2d_draw(scene, window);
		se_window_end_frame(window);
	}

	s_array_clear(&ids);
	se_scene_2d_destroy_full(scene, true);
	se_context_destroy(context);
	return 0;
}
