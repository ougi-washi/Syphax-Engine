// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_editor.h"
#include "se_graphics.h"
#include "se_scene.h"
#include "se_window.h"
#include "syphax/s_json.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define EDITOR_SHOWCASE_SCENE_2D_PATH "scene2d_editor_showcase.json"
#define EDITOR_SHOWCASE_SCENE_3D_PATH "scene3d_editor_showcase.json"

typedef enum {
	EDITOR_SHOWCASE_TARGET_SCENE_2D = 0,
	EDITOR_SHOWCASE_TARGET_SCENE_3D
} editor_showcase_target;

typedef struct {
	se_context* context;
	se_window_handle window;
	se_editor* editor;
	se_scene_2d_handle scene_2d;
	se_scene_3d_handle scene_3d;
	se_camera_handle camera_3d;
	editor_showcase_target target;
	s_vec3 camera_target;
	f32 camera_yaw;
	f32 camera_pitch;
	f32 camera_distance;
} editor_showcase_app;

static const c8* editor_showcase_target_name(const editor_showcase_target target) {
	return target == EDITOR_SHOWCASE_TARGET_SCENE_2D ? "scene_2d" : "scene_3d";
}

static void editor_showcase_apply_camera(editor_showcase_app* app) {
	if (!app || app->camera_3d == S_HANDLE_NULL) {
		return;
	}
	const s_vec3 rotation = s_vec3(app->camera_pitch, app->camera_yaw, 0.0f);
	se_camera_set_rotation(app->camera_3d, &rotation);
	{
		const s_vec3 forward = se_camera_get_forward_vector(app->camera_3d);
		const s_vec3 position = s_vec3_sub(&app->camera_target, &s_vec3_muls(&forward, app->camera_distance));
		se_camera_set_location(app->camera_3d, &position);
	}
	se_camera_set_target(app->camera_3d, &app->camera_target);
	se_camera_set_window_aspect(app->camera_3d, app->window);
}

static b8 editor_showcase_setup_scene_2d(editor_showcase_app* app) {
	se_object_2d_handle panel = S_HANDLE_NULL;
	se_object_2d_handle button = S_HANDLE_NULL;
	if (!app) {
		return false;
	}
	app->scene_2d = se_scene_2d_create(&s_vec2(1280.0f, 720.0f), 4);
	if (app->scene_2d == S_HANDLE_NULL) {
		return false;
	}
	se_scene_2d_set_fit_to_window(app->scene_2d, app->window, &s_vec2(1.0f, 1.0f));

	panel = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/panel.glsl"), &s_mat3_identity, 0);
	button = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/button.glsl"), &s_mat3_identity, 0);
	if (panel == S_HANDLE_NULL || button == S_HANDLE_NULL) {
		return false;
	}

	se_object_2d_set_scale(panel, &s_vec2(0.62f, 0.46f));
	se_object_2d_set_position(button, &s_vec2(0.12f, -0.05f));
	se_object_2d_set_scale(button, &s_vec2(0.20f, 0.12f));
	se_scene_2d_add_object(app->scene_2d, panel);
	se_scene_2d_add_object(app->scene_2d, button);
	return true;
}

static b8 editor_showcase_setup_scene_3d(editor_showcase_app* app) {
	se_model_handle cube_model = S_HANDLE_NULL;
	se_object_3d_handle cubes = S_HANDLE_NULL;
	if (!app) {
		return false;
	}
	app->scene_3d = se_scene_3d_create_for_window(app->window, 32);
	if (app->scene_3d == S_HANDLE_NULL) {
		return false;
	}
	app->camera_3d = se_scene_3d_get_camera(app->scene_3d);
	if (app->camera_3d == S_HANDLE_NULL) {
		return false;
	}

	app->camera_target = s_vec3(0.0f, 0.3f, 0.0f);
	app->camera_yaw = 0.54f;
	app->camera_pitch = 0.30f;
	app->camera_distance = 8.0f;
	se_camera_set_target_mode(app->camera_3d, true);
	se_camera_set_perspective(app->camera_3d, 52.0f, 0.05f, 200.0f);
	editor_showcase_apply_camera(app);

	cube_model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"));
	if (cube_model == S_HANDLE_NULL) {
		return false;
	}
	cubes = se_scene_3d_add_model(app->scene_3d, cube_model, &s_mat4_identity);
	if (cubes == S_HANDLE_NULL) {
		return false;
	}

	for (i32 x = -2; x <= 2; ++x) {
		for (i32 z = -2; z <= 2; ++z) {
			s_mat4 tile = s_mat4_identity;
			tile = s_mat4_scale(&tile, &s_vec3(0.8f, 0.08f, 0.8f));
			s_mat4_set_translation(&tile, &s_vec3((f32)x * 1.1f, -0.55f, (f32)z * 1.1f));
			se_object_3d_add_instance(cubes, &tile, &s_mat4_identity);
		}
	}

	{
		s_mat4 center = s_mat4_identity;
		center = s_mat4_scale(&center, &s_vec3(0.9f, 0.9f, 0.9f));
		s_mat4_set_translation(&center, &s_vec3(0.0f, 0.35f, 0.0f));
		se_object_3d_add_instance(cubes, &center, &s_mat4_identity);
	}
	{
		s_mat4 side = s_mat4_identity;
		side = s_mat4_scale(&side, &s_vec3(0.6f, 1.2f, 0.6f));
		s_mat4_set_translation(&side, &s_vec3(1.6f, 0.65f, -1.0f));
		se_object_3d_add_instance(cubes, &side, &s_mat4_identity);
	}
	return true;
}

static void editor_showcase_print_counts(editor_showcase_app* app) {
	se_editor_counts counts = {0};
	if (!app || !app->editor) {
		return;
	}
	if (se_editor_collect_counts(app->editor, &counts)) {
		printf(
			"editor_showcase :: counts windows=%u scenes2d=%u scenes3d=%u objects2d=%u objects3d=%u queued=%u\n",
			counts.windows,
			counts.scenes_2d,
			counts.scenes_3d,
			counts.objects_2d,
			counts.objects_3d,
			counts.queued_commands);
	}
}

static void editor_showcase_save_active(editor_showcase_app* app) {
	b8 ok = false;
	s_json* root = NULL;
	if (!app || !app->editor) {
		return;
	}
	if (app->target == EDITOR_SHOWCASE_TARGET_SCENE_2D) {
		root = se_scene_2d_to_json(app->scene_2d);
		ok = root && se_editor_json_write_file(EDITOR_SHOWCASE_SCENE_2D_PATH, root);
	} else {
		root = se_scene_3d_to_json(app->scene_3d);
		ok = root && se_editor_json_write_file(EDITOR_SHOWCASE_SCENE_3D_PATH, root);
	}
	s_json_free(root);
	printf("editor_showcase :: %s save %s\n", editor_showcase_target_name(app->target), ok ? "ok" : se_result_str(se_get_last_error()));
}

static void editor_showcase_load_active(editor_showcase_app* app) {
	b8 ok = false;
	s_json* root = NULL;
	if (!app || !app->editor) {
		return;
	}
	if (app->target == EDITOR_SHOWCASE_TARGET_SCENE_2D) {
		ok = se_editor_json_read_file(EDITOR_SHOWCASE_SCENE_2D_PATH, &root) && se_scene_2d_from_json(app->scene_2d, root);
	} else {
		ok = se_editor_json_read_file(EDITOR_SHOWCASE_SCENE_3D_PATH, &root) && se_scene_3d_from_json(app->scene_3d, root);
	}
	s_json_free(root);
	printf("editor_showcase :: %s load %s\n", editor_showcase_target_name(app->target), ok ? "ok" : se_result_str(se_get_last_error()));
}

static void editor_showcase_switch_target(editor_showcase_app* app) {
	if (!app) {
		return;
	}
	app->target = app->target == EDITOR_SHOWCASE_TARGET_SCENE_2D
		? EDITOR_SHOWCASE_TARGET_SCENE_3D
		: EDITOR_SHOWCASE_TARGET_SCENE_2D;
	printf("editor_showcase :: target = %s\n", editor_showcase_target_name(app->target));
}

static void editor_showcase_handle_shortcut(editor_showcase_app* app, const se_editor_shortcut_event* event) {
	if (!app || !event) {
		return;
	}
	if (strcmp(event->action, "save") == 0) {
		editor_showcase_save_active(app);
	} else if (strcmp(event->action, "load") == 0) {
		editor_showcase_load_active(app);
	} else if (strcmp(event->action, "quit") == 0) {
		se_window_set_should_close(app->window, true);
	} else if (strcmp(event->action, "next") == 0 || strcmp(event->action, "previous") == 0) {
		editor_showcase_switch_target(app);
	}
}

static void editor_showcase_update_camera(editor_showcase_app* app, const f32 dt) {
	s_vec2 mouse_delta = s_vec2(0.0f, 0.0f);
	s_vec2 scroll_delta = s_vec2(0.0f, 0.0f);
	s_vec3 move = s_vec3(0.0f, 0.0f, 0.0f);
	if (!app || app->target != EDITOR_SHOWCASE_TARGET_SCENE_3D) {
		return;
	}

	se_window_get_mouse_delta(app->window, &mouse_delta);
	se_window_get_scroll(app->window, &scroll_delta);

	if (se_window_is_mouse_down(app->window, SE_MOUSE_LEFT)) {
		app->camera_yaw += mouse_delta.x * 0.008f;
		app->camera_pitch = s_max(-1.35f, s_min(1.35f, app->camera_pitch - mouse_delta.y * 0.008f));
	}
	if (fabsf(scroll_delta.y) > 0.0001f) {
		app->camera_distance = s_max(2.0f, s_min(32.0f, app->camera_distance - scroll_delta.y * 0.8f));
	}

	{
		const s_vec3 forward_xz = s_vec3(sinf(app->camera_yaw), 0.0f, -cosf(app->camera_yaw));
		const s_vec3 right_xz = s_vec3(cosf(app->camera_yaw), 0.0f, sinf(app->camera_yaw));
		const f32 move_speed = se_window_is_key_down(app->window, SE_KEY_LEFT_SHIFT) ? 8.0f : 4.0f;
		if (se_window_is_key_down(app->window, SE_KEY_W)) move = s_vec3_add(&move, &forward_xz);
		if (se_window_is_key_down(app->window, SE_KEY_S)) move = s_vec3_sub(&move, &forward_xz);
		if (se_window_is_key_down(app->window, SE_KEY_D)) move = s_vec3_add(&move, &right_xz);
		if (se_window_is_key_down(app->window, SE_KEY_A)) move = s_vec3_sub(&move, &right_xz);
		if (se_window_is_key_down(app->window, SE_KEY_E)) move.y += 1.0f;
		if (se_window_is_key_down(app->window, SE_KEY_Q)) move.y -= 1.0f;
		if (s_vec3_length(&move) > 0.0001f) {
			const s_vec3 move_dir = s_vec3_normalize(&move);
			move = s_vec3_muls(&move_dir, move_speed * s_max(dt, 0.0f));
			app->camera_target = s_vec3_add(&app->camera_target, &move);
		}
	}

	if (se_window_is_key_pressed(app->window, SE_KEY_R)) {
		app->camera_target = s_vec3(0.0f, 0.3f, 0.0f);
		app->camera_yaw = 0.54f;
		app->camera_pitch = 0.30f;
		app->camera_distance = 8.0f;
	}

	editor_showcase_apply_camera(app);
}

static void editor_showcase_cleanup(editor_showcase_app* app) {
	if (!app) {
		return;
	}
	if (app->editor) {
		se_editor_destroy(app->editor);
		app->editor = NULL;
	}
	if (app->scene_2d != S_HANDLE_NULL) {
		se_scene_2d_destroy_full(app->scene_2d, true);
		app->scene_2d = S_HANDLE_NULL;
	}
	if (app->scene_3d != S_HANDLE_NULL) {
		se_scene_3d_destroy_full(app->scene_3d, true, true);
		app->scene_3d = S_HANDLE_NULL;
	}
	if (app->context) {
		se_context_destroy(app->context);
		app->context = NULL;
	}
}

int main(void) {
	editor_showcase_app app = {0};
	se_editor_config editor_config = SE_EDITOR_CONFIG_DEFAULTS;

	app.context = se_context_create();
	app.window = se_window_create("Syphax - Editor Showcase", 1280, 720);
	if (app.window == S_HANDLE_NULL) {
		printf("editor_showcase :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		editor_showcase_cleanup(&app);
		return 1;
	}

	se_window_set_exit_key(app.window, SE_KEY_ESCAPE);
	se_window_set_target_fps(app.window, 60);
	se_render_set_background(s_vec4(0.04f, 0.05f, 0.07f, 1.0f));

	if (!editor_showcase_setup_scene_2d(&app) || !editor_showcase_setup_scene_3d(&app)) {
		printf("editor_showcase :: setup failed (%s)\n", se_result_str(se_get_last_error()));
		editor_showcase_cleanup(&app);
		return 1;
	}

	editor_config.context = app.context;
	editor_config.window = app.window;
	app.editor = se_editor_create(&editor_config);
	if (!app.editor) {
		printf("editor_showcase :: editor create failed (%s)\n", se_result_str(se_get_last_error()));
		editor_showcase_cleanup(&app);
		return 1;
	}
	if (!se_editor_bind_default_shortcuts(app.editor)) {
		printf("editor_showcase :: shortcut bind failed (%s)\n", se_result_str(se_get_last_error()));
		editor_showcase_cleanup(&app);
		return 1;
	}

	editor_showcase_print_counts(&app);
	printf("editor_showcase controls:\n");
	printf("  Tab: switch between scene_2d and scene_3d\n");
	printf("  Ctrl+S: save active scene json\n");
	printf("  Ctrl+O: load active scene json\n");
	printf("  Ctrl+Q: quit\n");
	printf("  3D mode: left mouse orbit, wheel zoom, WASD + QE move, R reset\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(app.window)) {
		const f32 dt = (f32)se_window_get_delta_time(app.window);
		se_editor_shortcut_event event = {0};
		se_window_begin_frame(app.window);

		se_editor_update_shortcuts(app.editor);
		while (se_editor_poll_shortcut(app.editor, &event)) {
			editor_showcase_handle_shortcut(&app, &event);
		}

		editor_showcase_update_camera(&app, dt);

		se_render_clear();
		if (app.target == EDITOR_SHOWCASE_TARGET_SCENE_2D) {
			se_scene_2d_draw(app.scene_2d, app.window);
		} else {
			se_scene_3d_draw(app.scene_3d, app.window);
		}
		se_window_end_frame(app.window);
	}

	editor_showcase_cleanup(&app);
	return 0;
}
