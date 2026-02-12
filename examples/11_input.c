// Syphax-Engine - Ougi Washi

#include "se_input.h"
#include "se_scene.h"

#include <math.h>
#include <stdio.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

typedef struct {
	se_input_handle* input;
	se_window* window;
	se_scene_3d* scene;
	s_vec3 target;
	f32 yaw;
	f32 pitch;
	f32 distance;
	f32 mouse_x_speed;
	f32 mouse_y_speed;
	f32 key_orbit_speed;
	f32 zoom_speed;
} input_viewer_state;

static void input_viewer_apply_camera(input_viewer_state* state) {
	if (!state || !state->scene || !state->scene->camera) {
		return;
	}

	if (state->pitch > 1.45f) state->pitch = 1.45f;
	if (state->pitch < -1.45f) state->pitch = -1.45f;
	if (state->distance < 2.0f) state->distance = 2.0f;
	if (state->distance > 20.0f) state->distance = 20.0f;

	s_vec3 orbit_offset = s_vec3(
		sinf(state->yaw) * cosf(state->pitch) * state->distance,
		sinf(state->pitch) * state->distance,
		cosf(state->yaw) * cosf(state->pitch) * state->distance);

	state->scene->camera->position = s_vec3_add(&state->target, &orbit_offset);
	state->scene->camera->target = state->target;
	state->scene->camera->up = s_vec3(0.0f, 1.0f, 0.0f);
}

static void input_viewer_on_key_w(void* user_data) {
	input_viewer_state* state = (input_viewer_state*)user_data;
	const f32 dt = (f32)se_window_get_delta_time(state->window);
	state->pitch += state->key_orbit_speed * dt;
	input_viewer_apply_camera(state);
}

static void input_viewer_on_key_s(void* user_data) {
	input_viewer_state* state = (input_viewer_state*)user_data;
	const f32 dt = (f32)se_window_get_delta_time(state->window);
	state->pitch -= state->key_orbit_speed * dt;
	input_viewer_apply_camera(state);
}

static void input_viewer_on_key_a(void* user_data) {
	input_viewer_state* state = (input_viewer_state*)user_data;
	const f32 dt = (f32)se_window_get_delta_time(state->window);
	state->yaw -= state->key_orbit_speed * dt;
	input_viewer_apply_camera(state);
}

static void input_viewer_on_key_d(void* user_data) {
	input_viewer_state* state = (input_viewer_state*)user_data;
	const f32 dt = (f32)se_window_get_delta_time(state->window);
	state->yaw += state->key_orbit_speed * dt;
	input_viewer_apply_camera(state);
}

static void input_viewer_on_reset(void* user_data) {
	input_viewer_state* state = (input_viewer_state*)user_data;
	state->yaw = 0.0f;
	state->pitch = -0.2f;
	state->distance = 6.0f;
	input_viewer_apply_camera(state);
}

static void input_viewer_on_mouse_delta_x(void* user_data) {
	input_viewer_state* state = (input_viewer_state*)user_data;
	const f32 mouse_dx = se_input_get_value(state->input, SE_INPUT_MOUSE_DELTA_X, SE_INPUT_AXIS);
	state->yaw += mouse_dx * state->mouse_x_speed;
	input_viewer_apply_camera(state);
}

static void input_viewer_on_mouse_delta_y(void* user_data) {
	input_viewer_state* state = (input_viewer_state*)user_data;
	const f32 mouse_dy = se_input_get_value(state->input, SE_INPUT_MOUSE_DELTA_Y, SE_INPUT_AXIS);
	state->pitch -= mouse_dy * state->mouse_y_speed;
	input_viewer_apply_camera(state);
}

static void input_viewer_on_scroll_y(void* user_data) {
	input_viewer_state* state = (input_viewer_state*)user_data;
	const f32 scroll_y = se_input_get_value(state->input, SE_INPUT_MOUSE_SCROLL_Y, SE_INPUT_AXIS);
	state->distance -= scroll_y * state->zoom_speed;
	input_viewer_apply_camera(state);
}

int main(void) {
	se_context* ctx = se_context_create();
	se_window* window = se_window_create(ctx, "Syphax-Engine - Input Viewer", WINDOW_WIDTH, WINDOW_HEIGHT);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.07f, 0.09f, 0.11f, 1.0f));

	se_scene_3d* scene = se_scene_3d_create_for_window(ctx, window, 1);

	se_model* cube_model = se_model_load_obj_simple(
		ctx,
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("input/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("input/scene3d_fragment.glsl"));
	se_scene_3d_add_model(ctx, scene, cube_model, &s_mat4_identity);

	se_input_handle* input = se_input_create(window, 8);
	input_viewer_state viewer = {
		.input = input,
		.window = window,
		.scene = scene,
		.target = s_vec3(0.0f, 0.0f, 0.0f),
		.yaw = 0.0f,
		.pitch = -0.2f,
		.distance = 6.0f,
		.mouse_x_speed = 0.01f,
		.mouse_y_speed = 0.01f,
		.key_orbit_speed = 1.8f,
		.zoom_speed = 0.45f
	};

	se_input_bind(input, SE_KEY_W, SE_INPUT_DOWN, input_viewer_on_key_w, &viewer);
	se_input_bind(input, SE_KEY_A, SE_INPUT_DOWN, input_viewer_on_key_a, &viewer);
	se_input_bind(input, SE_KEY_S, SE_INPUT_DOWN, input_viewer_on_key_s, &viewer);
	se_input_bind(input, SE_KEY_D, SE_INPUT_DOWN, input_viewer_on_key_d, &viewer);
	se_input_bind(input, SE_KEY_R, SE_INPUT_PRESS, input_viewer_on_reset, &viewer);
	se_input_bind(input, SE_INPUT_MOUSE_DELTA_X, SE_INPUT_AXIS, input_viewer_on_mouse_delta_x, &viewer);
	se_input_bind(input, SE_INPUT_MOUSE_DELTA_Y, SE_INPUT_AXIS, input_viewer_on_mouse_delta_y, &viewer);
	se_input_bind(input, SE_INPUT_MOUSE_SCROLL_Y, SE_INPUT_AXIS, input_viewer_on_scroll_y, &viewer);

	printf("11_input :: small viewer controls\n");
	printf("  mouse move: orbit around cube\n");
	printf("  scroll: zoom\n");
	printf("  W/A/S/D: move around cube\n");
	printf("  R: reset view\n");

	input_viewer_apply_camera(&viewer);

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_context_reload_changed_shaders(ctx);
		se_input_tick(input);
		se_scene_3d_draw(scene, ctx, window);
	}

	se_input_destroy(input);
	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
