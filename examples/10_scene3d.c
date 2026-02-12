// Syphax-Engine - Ougi Washi

#include "se_scene.h"
#include <math.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define GRID_DIM 4
#define INSTANCE_TOTAL (GRID_DIM * GRID_DIM * GRID_DIM)

typedef struct {
	se_instance_id id;
	s_vec3 base_position;
} se_instance_slot;

i32 main(void) {
	se_context *ctx = se_context_create();
	se_window *window = se_window_create(ctx, "Syphax-Engine - Scene 3D", WINDOW_WIDTH, WINDOW_HEIGHT);
	se_scene_3d *scene = se_scene_3d_create_for_window(ctx, window, INSTANCE_TOTAL + 4);
	se_model *cube_model = NULL;
	se_object_3d *cube_field = NULL;

	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	cube_model = se_model_load_obj_simple(
		ctx,
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"));

	s_mat4 identity = s_mat4_identity;
	cube_field = se_object_3d_create(ctx, cube_model, &identity, INSTANCE_TOTAL);
	se_scene_3d_add_object(scene, cube_field);

	se_instance_slot slots[INSTANCE_TOTAL] = {0};
	sz slot_idx = 0;
	const f32 spacing = 1.25f;
	for (i32 x = 0; x < GRID_DIM; x++) {
		for (i32 y = 0; y < GRID_DIM; y++) {
			for (i32 z = 0; z < GRID_DIM; z++) {
				s_vec3 position = s_vec3(
					(x - GRID_DIM / 2) * spacing,
					(y - GRID_DIM / 2) * spacing,
					(z - GRID_DIM / 2) * spacing);
				s_mat4 transform = s_mat4_identity;
				s_mat4_set_translation(&transform, &position);
				se_instance_id id = se_object_3d_add_instance(cube_field, &transform, &identity);
				slots[slot_idx].id = id;
				slots[slot_idx].base_position = position;
				slot_idx++;
			}
		}
	}

	scene->camera->position = s_vec3(6.0f, 5.0f, 10.0f);
	scene->camera->target = s_vec3(0.0f, 0.0f, 0.0f);
	scene->camera->up = s_vec3(0.0f, 1.0f, 0.0f);

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_context_reload_changed_shaders(ctx);

		const f32 time = (f32)se_window_get_time(window);
		for (sz i = 0; i < INSTANCE_TOTAL; i++) {
			s_mat4 transform = s_mat4_identity;
			s_mat4_set_translation(&transform, &slots[i].base_position);
			s_mat4_rotate_y(&transform, time * 0.5f + (f32)i * 0.1f);
			s_mat4_rotate_x(&transform, time * 0.3f + (f32)i * 0.05f);
			se_object_3d_set_instance_transform(cube_field, slots[i].id, &transform);
		}

		se_scene_3d_draw(scene, ctx, window);
	}

	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
