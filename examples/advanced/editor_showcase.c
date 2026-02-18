// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_editor.h"
#include "se_graphics.h"
#include "se_scene.h"
#include "se_shader.h"
#include "se_ui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SHOWCASE_WIDTH 1400
#define SHOWCASE_HEIGHT 860

#define EDITOR_2D_GIZMO_HANDLE_OFFSET 0.14f
#define EDITOR_2D_GIZMO_HANDLE_SIZE 0.024f
#define EDITOR_2D_GIZMO_PICK_RADIUS 0.050f
#define EDITOR_2D_PICK_FALLBACK_RADIUS 0.12f
#define EDITOR_3D_GIZMO_AXIS_LENGTH 1.25f
#define EDITOR_3D_GIZMO_PICK_THRESHOLD_PX 12.0f
#define EDITOR_3D_PICK_RADIUS 1.10f
#define EDITOR_3D_PICK_FALLBACK_RADIUS_MIN 0.70f

typedef enum {
	EDITOR_TARGET_SCENE_2D = 0,
	EDITOR_TARGET_SCENE_3D
} editor_target;

typedef enum {
	EDITOR_GIZMO_AXIS_NONE = 0,
	EDITOR_GIZMO_AXIS_FREE,
	EDITOR_GIZMO_AXIS_X,
	EDITOR_GIZMO_AXIS_Y,
	EDITOR_GIZMO_AXIS_Z
} editor_gizmo_axis;

typedef struct {
	se_object_2d_handle handle;
	s_vec3 base_color;
} editor_object_2d_slot;
typedef s_array(editor_object_2d_slot, editor_object_2d_slots);

typedef struct {
	se_object_3d_handle handle;
} editor_object_3d_slot;
typedef s_array(editor_object_3d_slot, editor_object_3d_slots);

typedef struct {
	se_context* context;
	se_window_handle window;
	se_editor* editor;

	se_scene_2d_handle scene_2d;
	se_scene_3d_handle scene_3d;
	se_camera_handle camera_3d;
	se_camera_controller* camera_controller;
	se_model_handle cube_model;

	se_ui_handle ui;
	se_ui_widget_handle ui_panel;
	se_ui_widget_handle ui_title;
	se_ui_widget_handle ui_status;

	editor_object_2d_slots objects_2d;
	editor_object_3d_slots objects_3d;

	se_object_2d_handle gizmo_2d_center;
	se_object_2d_handle gizmo_2d_x;
	se_object_2d_handle gizmo_2d_y;

	se_object_2d_handle selected_2d;
	se_object_3d_handle selected_3d;

	editor_target target;
	editor_gizmo_axis active_axis;
	b8 dragging;

	s_vec2 drag_start_mouse_ndc;
	s_vec2 drag_start_pos_2d;
	s_vec3 drag_start_pos_3d;
	s_vec3 drag_axis_world;
	s_vec3 drag_plane_point;
	s_vec3 drag_plane_normal;
	s_vec3 drag_start_hit_world;
	f32 drag_axis_param_start;

	f64 next_status_time;
	u64 frame_index;
} editor_showcase_app;

static f32 editor_clampf(const f32 value, const f32 min_value, const f32 max_value) {
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}

static f32 editor_distance_vec2(const s_vec2* a, const s_vec2* b) {
	const s_vec2 delta = s_vec2_sub(a, b);
	return s_vec2_length(&delta);
}

static f32 editor_distance_point_segment(const s_vec2* point, const s_vec2* a, const s_vec2* b) {
	const s_vec2 ab = s_vec2_sub(b, a);
	const f32 ab_len2 = s_vec2_dot(&ab, &ab);
	if (ab_len2 <= 0.000001f) {
		return editor_distance_vec2(point, a);
	}
	const s_vec2 ap = s_vec2_sub(point, a);
	f32 t = s_vec2_dot(&ap, &ab) / ab_len2;
	t = editor_clampf(t, 0.0f, 1.0f);
	const s_vec2 projected = s_vec2(a->x + ab.x * t, a->y + ab.y * t);
	return editor_distance_vec2(point, &projected);
}

static s_vec3 editor_color_from_index(const u32 index) {
	const f32 i = (f32)index;
	const f32 r = 0.28f + 0.58f * fabsf(sinf(i * 1.11f + 0.23f));
	const f32 g = 0.24f + 0.62f * fabsf(sinf(i * 1.47f + 1.19f));
	const f32 b = 0.30f + 0.55f * fabsf(sinf(i * 1.73f + 2.41f));
	return s_vec3(r, g, b);
}

static b8 editor_get_mouse_framebuffer(editor_showcase_app* app, s_vec2* out_mouse, f32* out_width, f32* out_height) {
	u32 framebuffer_width = 0;
	u32 framebuffer_height = 0;
	f32 mouse_x = 0.0f;
	f32 mouse_y = 0.0f;
	s_vec2 mouse_framebuffer = s_vec2(0.0f, 0.0f);
	if (!app || app->window == S_HANDLE_NULL) {
		return false;
	}
	se_window_get_framebuffer_size(app->window, &framebuffer_width, &framebuffer_height);
	if (framebuffer_width <= 1u || framebuffer_height <= 1u) {
		return false;
	}
	mouse_x = se_window_get_mouse_position_x(app->window);
	mouse_y = se_window_get_mouse_position_y(app->window);
	if (!se_window_window_to_framebuffer(app->window, mouse_x, mouse_y, &mouse_framebuffer)) {
		mouse_framebuffer = s_vec2(mouse_x, mouse_y);
	}
	if (out_mouse) {
		*out_mouse = mouse_framebuffer;
	}
	if (out_width) {
		*out_width = (f32)framebuffer_width;
	}
	if (out_height) {
		*out_height = (f32)framebuffer_height;
	}
	return true;
}

static void editor_log_error(const c8* label) {
	printf("editor_showcase :: %s failed (%s)\n", label, se_result_str(se_get_last_error()));
}

static b8 editor_apply_scene_2d_action(editor_showcase_app* app, const c8* action, se_object_2d_handle object) {
	se_editor_item scene_item = {0};
	se_editor_value value = se_editor_value_handle(object);
	if (!app || app->scene_2d == S_HANDLE_NULL) {
		return false;
	}
	if (app->editor && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_SCENE_2D, app->scene_2d, &scene_item)) {
		if (se_editor_apply_action(app->editor, &scene_item, action, &value, NULL)) {
			return true;
		}
		editor_log_error(action);
	}
	if (strcmp(action, "add_object") == 0) {
		se_scene_2d_add_object(app->scene_2d, object);
		return true;
	}
	if (strcmp(action, "remove_object") == 0) {
		se_scene_2d_remove_object(app->scene_2d, object);
		return true;
	}
	return false;
}

static b8 editor_apply_scene_3d_action(editor_showcase_app* app, const c8* action, se_object_3d_handle object) {
	se_editor_item scene_item = {0};
	se_editor_value value = se_editor_value_handle(object);
	if (!app || app->scene_3d == S_HANDLE_NULL) {
		return false;
	}
	if (app->editor && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_SCENE_3D, app->scene_3d, &scene_item)) {
		if (se_editor_apply_action(app->editor, &scene_item, action, &value, NULL)) {
			return true;
		}
		editor_log_error(action);
	}
	if (strcmp(action, "add_object") == 0) {
		se_scene_3d_add_object(app->scene_3d, object);
		return true;
	}
	if (strcmp(action, "remove_object") == 0) {
		se_scene_3d_remove_object(app->scene_3d, object);
		return true;
	}
	return false;
}

static b8 editor_apply_object_2d_position(editor_showcase_app* app, const se_object_2d_handle object, const s_vec2* position) {
	se_editor_item item = {0};
	se_editor_value value = se_editor_value_vec2(*position);
	if (!app || object == S_HANDLE_NULL || !position) {
		return false;
	}
	if (app->editor && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_OBJECT_2D, object, &item)) {
		if (se_editor_apply_set(app->editor, &item, "position", &value)) {
			return true;
		}
		editor_log_error("set object_2d position");
	}
	se_object_2d_set_position(object, position);
	return true;
}

static b8 editor_apply_object_3d_transform(editor_showcase_app* app, const se_object_3d_handle object, const s_mat4* transform) {
	se_editor_item item = {0};
	se_editor_value value = se_editor_value_mat4(*transform);
	if (!app || object == S_HANDLE_NULL || !transform) {
		return false;
	}
	if (app->editor && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_OBJECT_3D, object, &item)) {
		if (se_editor_apply_set(app->editor, &item, "transform", &value)) {
			return true;
		}
		editor_log_error("set object_3d transform");
	}
	se_object_3d_set_transform(object, transform);
	return true;
}

static void editor_remove_object_2d_slot(editor_showcase_app* app, const se_object_2d_handle handle) {
	if (!app || handle == S_HANDLE_NULL) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&app->objects_2d); ++i) {
		editor_object_2d_slot* slot = s_array_get(&app->objects_2d, s_array_handle(&app->objects_2d, (u32)i));
		if (slot && slot->handle == handle) {
			s_array_remove(&app->objects_2d, s_array_handle(&app->objects_2d, (u32)i));
			return;
		}
	}
}

static void editor_remove_object_3d_slot(editor_showcase_app* app, const se_object_3d_handle handle) {
	if (!app || handle == S_HANDLE_NULL) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&app->objects_3d); ++i) {
		editor_object_3d_slot* slot = s_array_get(&app->objects_3d, s_array_handle(&app->objects_3d, (u32)i));
		if (slot && slot->handle == handle) {
			s_array_remove(&app->objects_3d, s_array_handle(&app->objects_3d, (u32)i));
			return;
		}
	}
}

static b8 editor_contains_object_3d(const editor_showcase_app* app, const se_object_3d_handle handle) {
	if (!app || handle == S_HANDLE_NULL) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&app->objects_3d); ++i) {
		const editor_object_3d_slot* slot = s_array_get((editor_object_3d_slots*)&app->objects_3d, s_array_handle((editor_object_3d_slots*)&app->objects_3d, (u32)i));
		if (slot && slot->handle == handle) {
			return true;
		}
	}
	return false;
}

static b8 editor_ray_hits_sphere(const s_vec3* ray_origin, const s_vec3* ray_dir, const s_vec3* center, const f32 radius, f32* out_t) {
	s_vec3 offset = s_vec3(0.0f, 0.0f, 0.0f);
	f32 b = 0.0f;
	f32 c = 0.0f;
	f32 discriminant = 0.0f;
	f32 t = 0.0f;
	if (!ray_origin || !ray_dir || !center || radius <= 0.0f) {
		return false;
	}
	offset = s_vec3_sub(ray_origin, center);
	b = s_vec3_dot(&offset, ray_dir);
	c = s_vec3_dot(&offset, &offset) - radius * radius;
	discriminant = b * b - c;
	if (discriminant < 0.0f) {
		return false;
	}
	t = -b - sqrtf(discriminant);
	if (t < 0.0f) {
		t = -b + sqrtf(discriminant);
	}
	if (t < 0.0f) {
		return false;
	}
	if (out_t) {
		*out_t = t;
	}
	return true;
}

static void editor_delete_object_2d(editor_showcase_app* app, const se_object_2d_handle handle) {
	se_shader_handle shader = S_HANDLE_NULL;
	if (!app || handle == S_HANDLE_NULL) {
		return;
	}
	shader = se_object_2d_get_shader(handle);
	(void)editor_apply_scene_2d_action(app, "remove_object", handle);
	se_object_2d_destroy(handle);
	if (shader != S_HANDLE_NULL) {
		se_shader_destroy(shader);
	}
	editor_remove_object_2d_slot(app, handle);
	if (app->selected_2d == handle) {
		app->selected_2d = S_HANDLE_NULL;
	}
}

static void editor_delete_object_3d(editor_showcase_app* app, const se_object_3d_handle handle) {
	if (!app || handle == S_HANDLE_NULL) {
		return;
	}
	(void)editor_apply_scene_3d_action(app, "remove_object", handle);
	se_object_3d_destroy(handle);
	editor_remove_object_3d_slot(app, handle);
	if (app->selected_3d == handle) {
		app->selected_3d = S_HANDLE_NULL;
	}
}

static se_object_2d_handle editor_spawn_object_2d(editor_showcase_app* app, const s_vec2* position) {
	editor_object_2d_slot slot = {0};
	se_object_2d_handle handle = S_HANDLE_NULL;
	se_shader_handle shader = S_HANDLE_NULL;
	s_vec2 object_scale = s_vec2(0.10f, 0.10f);
	if (!app || app->scene_2d == S_HANDLE_NULL || !position) {
		return S_HANDLE_NULL;
	}
	handle = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/button.glsl"), &s_mat3_identity, 0);
	if (handle == S_HANDLE_NULL) {
		editor_log_error("create object_2d");
		return S_HANDLE_NULL;
	}
	se_object_2d_set_scale(handle, &object_scale);
	(void)editor_apply_object_2d_position(app, handle, position);
	if (!editor_apply_scene_2d_action(app, "add_object", handle)) {
		se_object_2d_destroy(handle);
		return S_HANDLE_NULL;
	}
	slot.handle = handle;
	slot.base_color = editor_color_from_index((u32)s_array_get_size(&app->objects_2d));
	s_array_add(&app->objects_2d, slot);
	shader = se_object_2d_get_shader(handle);
	if (shader != S_HANDLE_NULL) {
		se_shader_set_vec3(shader, "u_color", &slot.base_color);
	}
	return handle;
}

static se_object_3d_handle editor_spawn_object_3d(editor_showcase_app* app, const s_vec3* position) {
	editor_object_3d_slot slot = {0};
	se_object_3d_handle handle = S_HANDLE_NULL;
	s_mat4 transform = s_mat4_identity;
	const s_vec3 object_scale = s_vec3(0.36f, 0.36f, 0.36f);
	if (!app || app->scene_3d == S_HANDLE_NULL || app->cube_model == S_HANDLE_NULL || !position) {
		return S_HANDLE_NULL;
	}
	transform = s_mat4_scale(&transform, &object_scale);
	s_mat4_set_translation(&transform, position);
	handle = se_object_3d_create(app->cube_model, &transform, 0);
	if (handle == S_HANDLE_NULL) {
		editor_log_error("create object_3d");
		return S_HANDLE_NULL;
	}
	if (!editor_apply_scene_3d_action(app, "add_object", handle)) {
		se_object_3d_destroy(handle);
		return S_HANDLE_NULL;
	}
	slot.handle = handle;
	s_array_add(&app->objects_3d, slot);
	return handle;
}

static s_vec3 editor_get_selected_3d_position(editor_showcase_app* app) {
	s_mat4 transform = s_mat4_identity;
	if (!app || app->selected_3d == S_HANDLE_NULL) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	transform = se_object_3d_get_transform(app->selected_3d);
	return s_mat4_get_translation(&transform);
}

static b8 editor_pick_filter_scene_2d(const se_object_2d_handle object, void* user_data) {
	editor_showcase_app* app = (editor_showcase_app*)user_data;
	if (!app) {
		return false;
	}
	if (object == app->gizmo_2d_center || object == app->gizmo_2d_x || object == app->gizmo_2d_y) {
		return false;
	}
	return true;
}

static b8 editor_pick_filter_scene_3d(const se_object_3d_handle object, void* user_data) {
	editor_showcase_app* app = (editor_showcase_app*)user_data;
	if (!app) {
		return false;
	}
	return editor_contains_object_3d(app, object);
}

static se_object_2d_handle editor_pick_scene_object_2d(editor_showcase_app* app) {
	s_vec2 mouse_ndc = s_vec2(0.0f, 0.0f);
	se_object_2d_handle picked = S_HANDLE_NULL;
	f32 best_distance = EDITOR_2D_PICK_FALLBACK_RADIUS;
	if (!app || app->scene_2d == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_window_get_mouse_position_normalized(app->window, &mouse_ndc);
	if (se_scene_2d_pick_object(app->scene_2d, &mouse_ndc, editor_pick_filter_scene_2d, app, &picked)) {
		return picked;
	}
	for (sz i = 0; i < s_array_get_size(&app->objects_2d); ++i) {
		const editor_object_2d_slot* slot = s_array_get(&app->objects_2d, s_array_handle(&app->objects_2d, (u32)i));
		if (!slot || slot->handle == S_HANDLE_NULL) {
			continue;
		}
		const s_vec2 position = se_object_2d_get_position(slot->handle);
		const f32 distance = editor_distance_vec2(&mouse_ndc, &position);
		if (distance <= best_distance) {
			best_distance = distance;
			picked = slot->handle;
		}
	}
	if (picked != S_HANDLE_NULL) {
		return picked;
	}
	return S_HANDLE_NULL;
}

static se_object_3d_handle editor_pick_scene_object_3d(editor_showcase_app* app) {
	s_vec2 mouse_framebuffer = s_vec2(0.0f, 0.0f);
	se_object_3d_handle picked = S_HANDLE_NULL;
	f32 viewport_width = 0.0f;
	f32 viewport_height = 0.0f;
	if (!app || app->scene_3d == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	if (!editor_get_mouse_framebuffer(app, &mouse_framebuffer, &viewport_width, &viewport_height)) {
		return S_HANDLE_NULL;
	}
	if (se_scene_3d_pick_object_screen(
			app->scene_3d,
			mouse_framebuffer.x,
			mouse_framebuffer.y,
			viewport_width,
			viewport_height,
			EDITOR_3D_PICK_RADIUS,
				editor_pick_filter_scene_3d,
				app,
				&picked,
				NULL)) {
		return picked;
	}
	{
		s_vec3 ray_origin = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 ray_dir = s_vec3(0.0f, 0.0f, -1.0f);
		f32 best_t = 1e30f;
		if (se_camera_screen_to_ray(
				app->camera_3d,
				mouse_framebuffer.x,
				mouse_framebuffer.y,
				viewport_width,
				viewport_height,
				&ray_origin,
				&ray_dir)) {
			for (sz i = 0; i < s_array_get_size(&app->objects_3d); ++i) {
				const editor_object_3d_slot* slot = s_array_get(&app->objects_3d, s_array_handle(&app->objects_3d, (u32)i));
				s_mat4 transform = s_mat4_identity;
				s_vec3 center = s_vec3(0.0f, 0.0f, 0.0f);
				s_vec3 scale = s_vec3(1.0f, 1.0f, 1.0f);
				f32 scale_max = 1.0f;
				f32 radius = 1.0f;
				f32 t = 0.0f;
				if (!slot || slot->handle == S_HANDLE_NULL) {
					continue;
				}
				transform = se_object_3d_get_transform(slot->handle);
				center = s_mat4_get_translation(&transform);
				scale = s_mat4_get_scale(&transform);
				scale_max = s_max(scale.x, s_max(scale.y, scale.z));
				radius = s_max(EDITOR_3D_PICK_FALLBACK_RADIUS_MIN, scale_max * 1.65f);
				if (editor_ray_hits_sphere(&ray_origin, &ray_dir, &center, radius, &t) && t < best_t) {
					best_t = t;
					picked = slot->handle;
				}
			}
		}
	}
	if (picked != S_HANDLE_NULL) {
		return picked;
	}
	return S_HANDLE_NULL;
}

static editor_gizmo_axis editor_pick_2d_gizmo_axis(editor_showcase_app* app, const s_vec2* mouse_ndc) {
	s_vec2 selected_pos = s_vec2(0.0f, 0.0f);
	s_vec2 x_handle_pos = s_vec2(0.0f, 0.0f);
	s_vec2 y_handle_pos = s_vec2(0.0f, 0.0f);
	if (!app || app->selected_2d == S_HANDLE_NULL || !mouse_ndc) {
		return EDITOR_GIZMO_AXIS_NONE;
	}
	selected_pos = se_object_2d_get_position(app->selected_2d);
	x_handle_pos = s_vec2(selected_pos.x + EDITOR_2D_GIZMO_HANDLE_OFFSET, selected_pos.y);
	y_handle_pos = s_vec2(selected_pos.x, selected_pos.y + EDITOR_2D_GIZMO_HANDLE_OFFSET);
	if (editor_distance_vec2(mouse_ndc, &x_handle_pos) <= EDITOR_2D_GIZMO_PICK_RADIUS) {
		return EDITOR_GIZMO_AXIS_X;
	}
	if (editor_distance_vec2(mouse_ndc, &y_handle_pos) <= EDITOR_2D_GIZMO_PICK_RADIUS) {
		return EDITOR_GIZMO_AXIS_Y;
	}
	if (editor_distance_vec2(mouse_ndc, &selected_pos) <= EDITOR_2D_GIZMO_PICK_RADIUS) {
		return EDITOR_GIZMO_AXIS_FREE;
	}
	return EDITOR_GIZMO_AXIS_NONE;
}

static editor_gizmo_axis editor_pick_3d_gizmo_axis(editor_showcase_app* app, const s_vec2* mouse_framebuffer, const f32 viewport_width, const f32 viewport_height) {
	s_vec3 object_pos = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec2 center_screen = s_vec2(0.0f, 0.0f);
	editor_gizmo_axis best_axis = EDITOR_GIZMO_AXIS_NONE;
	f32 best_distance = 1e30f;
	if (!app || app->selected_3d == S_HANDLE_NULL || app->camera_3d == S_HANDLE_NULL || !mouse_framebuffer) {
		return EDITOR_GIZMO_AXIS_NONE;
	}
	object_pos = editor_get_selected_3d_position(app);
	if (!se_camera_world_to_screen(app->camera_3d, &object_pos, viewport_width, viewport_height, &center_screen)) {
		return EDITOR_GIZMO_AXIS_NONE;
	}
	if (editor_distance_vec2(mouse_framebuffer, &center_screen) <= EDITOR_3D_GIZMO_PICK_THRESHOLD_PX) {
		return EDITOR_GIZMO_AXIS_FREE;
	}

	{
		const s_vec3 axis_dirs[3] = {
			s_vec3(1.0f, 0.0f, 0.0f),
			s_vec3(0.0f, 1.0f, 0.0f),
			s_vec3(0.0f, 0.0f, 1.0f)
		};
		const editor_gizmo_axis axes[3] = {
			EDITOR_GIZMO_AXIS_X,
			EDITOR_GIZMO_AXIS_Y,
			EDITOR_GIZMO_AXIS_Z
		};
		for (u32 i = 0; i < 3u; ++i) {
			s_vec2 endpoint_screen = s_vec2(0.0f, 0.0f);
			s_vec3 endpoint_world = s_vec3_add(&object_pos, &s_vec3_muls(&axis_dirs[i], EDITOR_3D_GIZMO_AXIS_LENGTH));
			if (!se_camera_world_to_screen(app->camera_3d, &endpoint_world, viewport_width, viewport_height, &endpoint_screen)) {
				continue;
			}
			const f32 distance = editor_distance_point_segment(mouse_framebuffer, &center_screen, &endpoint_screen);
			if (distance < best_distance) {
				best_distance = distance;
				best_axis = axes[i];
			}
		}
	}

	if (best_distance <= EDITOR_3D_GIZMO_PICK_THRESHOLD_PX) {
		return best_axis;
	}
	return EDITOR_GIZMO_AXIS_NONE;
}

static void editor_begin_drag_2d(editor_showcase_app* app, const editor_gizmo_axis axis) {
	if (!app || app->selected_2d == S_HANDLE_NULL) {
		return;
	}
	app->active_axis = axis;
	app->dragging = true;
	se_window_get_mouse_position_normalized(app->window, &app->drag_start_mouse_ndc);
	app->drag_start_pos_2d = se_object_2d_get_position(app->selected_2d);
}

static void editor_update_drag_2d(editor_showcase_app* app) {
	s_vec2 mouse_ndc = s_vec2(0.0f, 0.0f);
	s_vec2 delta = s_vec2(0.0f, 0.0f);
	s_vec2 new_position = s_vec2(0.0f, 0.0f);
	if (!app || !app->dragging || app->selected_2d == S_HANDLE_NULL) {
		return;
	}
	se_window_get_mouse_position_normalized(app->window, &mouse_ndc);
	delta = s_vec2_sub(&mouse_ndc, &app->drag_start_mouse_ndc);
	new_position = s_vec2_add(&app->drag_start_pos_2d, &delta);
	if (app->active_axis == EDITOR_GIZMO_AXIS_X) {
		new_position.y = app->drag_start_pos_2d.y;
	} else if (app->active_axis == EDITOR_GIZMO_AXIS_Y) {
		new_position.x = app->drag_start_pos_2d.x;
	}
	new_position.x = editor_clampf(new_position.x, -0.94f, 0.94f);
	new_position.y = editor_clampf(new_position.y, -0.92f, 0.92f);
	(void)editor_apply_object_2d_position(app, app->selected_2d, &new_position);
}

static void editor_begin_drag_3d(editor_showcase_app* app, const editor_gizmo_axis axis, const s_vec2* mouse_framebuffer, const f32 viewport_width, const f32 viewport_height) {
	se_camera* camera_ptr = NULL;
	s_vec3 camera_forward = s_vec3(0.0f, 0.0f, -1.0f);
	s_vec3 side = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 plane_hit = s_vec3(0.0f, 0.0f, 0.0f);
	if (!app || app->selected_3d == S_HANDLE_NULL || !mouse_framebuffer) {
		return;
	}
	app->dragging = true;
	app->active_axis = axis;
	app->drag_start_pos_3d = editor_get_selected_3d_position(app);
	app->drag_plane_point = app->drag_start_pos_3d;
	app->drag_plane_normal = s_vec3(0.0f, 1.0f, 0.0f);
	app->drag_start_hit_world = app->drag_start_pos_3d;
	app->drag_axis_param_start = 0.0f;
	app->drag_axis_world = s_vec3(0.0f, 0.0f, 0.0f);

	camera_ptr = se_camera_get(app->camera_3d);
	if (camera_ptr) {
		camera_forward = s_vec3_sub(&camera_ptr->target, &camera_ptr->position);
		camera_forward = s_vec3_normalize(&camera_forward);
	}

	if (axis == EDITOR_GIZMO_AXIS_X) {
		app->drag_axis_world = s_vec3(1.0f, 0.0f, 0.0f);
	} else if (axis == EDITOR_GIZMO_AXIS_Y) {
		app->drag_axis_world = s_vec3(0.0f, 1.0f, 0.0f);
	} else if (axis == EDITOR_GIZMO_AXIS_Z) {
		app->drag_axis_world = s_vec3(0.0f, 0.0f, 1.0f);
	}

	if (axis == EDITOR_GIZMO_AXIS_FREE) {
		if (se_camera_screen_to_plane(
				app->camera_3d,
				mouse_framebuffer->x,
				mouse_framebuffer->y,
				viewport_width,
				viewport_height,
				&app->drag_plane_point,
				&app->drag_plane_normal,
				&plane_hit)) {
			app->drag_start_hit_world = plane_hit;
		}
		return;
	}

	side = s_vec3_cross(&app->drag_axis_world, &camera_forward);
	if (s_vec3_length(&side) < 0.0001f) {
		side = s_vec3_cross(&app->drag_axis_world, &s_vec3(0.0f, 1.0f, 0.0f));
	}
	if (s_vec3_length(&side) < 0.0001f) {
		side = s_vec3_cross(&app->drag_axis_world, &s_vec3(1.0f, 0.0f, 0.0f));
	}
	side = s_vec3_normalize(&side);
	app->drag_plane_normal = s_vec3_cross(&side, &app->drag_axis_world);
	app->drag_plane_normal = s_vec3_normalize(&app->drag_plane_normal);
	if (s_vec3_length(&app->drag_plane_normal) < 0.0001f) {
		app->drag_plane_normal = s_vec3(0.0f, 1.0f, 0.0f);
	}
	if (se_camera_screen_to_plane(
			app->camera_3d,
			mouse_framebuffer->x,
			mouse_framebuffer->y,
			viewport_width,
			viewport_height,
			&app->drag_plane_point,
			&app->drag_plane_normal,
			&plane_hit)) {
		const s_vec3 rel = s_vec3_sub(&plane_hit, &app->drag_plane_point);
		app->drag_axis_param_start = s_vec3_dot(&rel, &app->drag_axis_world);
	}
}

static void editor_update_drag_3d(editor_showcase_app* app, const s_vec2* mouse_framebuffer, const f32 viewport_width, const f32 viewport_height) {
	s_vec3 plane_hit = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 new_position = s_vec3(0.0f, 0.0f, 0.0f);
	s_mat4 transform = s_mat4_identity;
	if (!app || !app->dragging || app->selected_3d == S_HANDLE_NULL || !mouse_framebuffer) {
		return;
	}
	if (!se_camera_screen_to_plane(
			app->camera_3d,
			mouse_framebuffer->x,
			mouse_framebuffer->y,
			viewport_width,
			viewport_height,
			&app->drag_plane_point,
			&app->drag_plane_normal,
			&plane_hit)) {
		return;
	}

	if (app->active_axis == EDITOR_GIZMO_AXIS_FREE) {
		const s_vec3 delta = s_vec3_sub(&plane_hit, &app->drag_start_hit_world);
		new_position = s_vec3_add(&app->drag_start_pos_3d, &delta);
	} else {
		const s_vec3 rel = s_vec3_sub(&plane_hit, &app->drag_plane_point);
		const f32 axis_param = s_vec3_dot(&rel, &app->drag_axis_world);
		const f32 axis_delta = axis_param - app->drag_axis_param_start;
		new_position = s_vec3_add(&app->drag_start_pos_3d, &s_vec3_muls(&app->drag_axis_world, axis_delta));
	}
	transform = se_object_3d_get_transform(app->selected_3d);
	s_mat4_set_translation(&transform, &new_position);
	(void)editor_apply_object_3d_transform(app, app->selected_3d, &transform);
}

static void editor_end_drag(editor_showcase_app* app) {
	if (!app) {
		return;
	}
	app->dragging = false;
	app->active_axis = EDITOR_GIZMO_AXIS_NONE;
}

static void editor_set_2d_object_color(const se_object_2d_handle object, const s_vec3* color) {
	se_shader_handle shader = S_HANDLE_NULL;
	if (object == S_HANDLE_NULL || !color) {
		return;
	}
	shader = se_object_2d_get_shader(object);
	if (shader != S_HANDLE_NULL) {
		se_shader_set_vec3(shader, "u_color", color);
	}
}

static void editor_update_2d_object_colors(editor_showcase_app* app) {
	if (!app) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&app->objects_2d); ++i) {
		editor_object_2d_slot* slot = s_array_get(&app->objects_2d, s_array_handle(&app->objects_2d, (u32)i));
		s_vec3 color = s_vec3(0.9f, 0.9f, 0.9f);
		if (!slot || slot->handle == S_HANDLE_NULL) {
			continue;
		}
		color = slot->base_color;
		if (slot->handle == app->selected_2d) {
			color = s_vec3(
				editor_clampf(color.x + 0.18f, 0.0f, 1.0f),
				editor_clampf(color.y + 0.18f, 0.0f, 1.0f),
				editor_clampf(color.z + 0.18f, 0.0f, 1.0f));
		}
		editor_set_2d_object_color(slot->handle, &color);
	}
}

static void editor_update_2d_gizmo_visuals(editor_showcase_app* app) {
	s_vec2 hidden = s_vec2(-3.0f, -3.0f);
	s_vec2 selected_pos = s_vec2(0.0f, 0.0f);
	s_vec2 x_handle_pos = s_vec2(0.0f, 0.0f);
	s_vec2 y_handle_pos = s_vec2(0.0f, 0.0f);
	s_vec2 center_scale = s_vec2(EDITOR_2D_GIZMO_HANDLE_SIZE, EDITOR_2D_GIZMO_HANDLE_SIZE);
	s_vec2 axis_scale = s_vec2(EDITOR_2D_GIZMO_HANDLE_SIZE * 0.9f, EDITOR_2D_GIZMO_HANDLE_SIZE * 0.9f);
	s_vec3 color_center = s_vec3(0.95f, 0.88f, 0.15f);
	s_vec3 color_x = s_vec3(0.92f, 0.24f, 0.21f);
	s_vec3 color_y = s_vec3(0.20f, 0.88f, 0.38f);
	if (!app) {
		return;
	}
	if (app->selected_2d == S_HANDLE_NULL || app->target != EDITOR_TARGET_SCENE_2D) {
		se_object_2d_set_position(app->gizmo_2d_center, &hidden);
		se_object_2d_set_position(app->gizmo_2d_x, &hidden);
		se_object_2d_set_position(app->gizmo_2d_y, &hidden);
		return;
	}
	selected_pos = se_object_2d_get_position(app->selected_2d);
	x_handle_pos = s_vec2(selected_pos.x + EDITOR_2D_GIZMO_HANDLE_OFFSET, selected_pos.y);
	y_handle_pos = s_vec2(selected_pos.x, selected_pos.y + EDITOR_2D_GIZMO_HANDLE_OFFSET);
	if (app->active_axis == EDITOR_GIZMO_AXIS_FREE && app->dragging) {
		center_scale = s_vec2(center_scale.x * 1.35f, center_scale.y * 1.35f);
		color_center = s_vec3(1.0f, 0.98f, 0.38f);
	}
	if (app->active_axis == EDITOR_GIZMO_AXIS_X && app->dragging) {
		axis_scale = s_vec2(axis_scale.x * 1.35f, axis_scale.y * 1.35f);
		color_x = s_vec3(1.0f, 0.50f, 0.42f);
	}
	if (app->active_axis == EDITOR_GIZMO_AXIS_Y && app->dragging) {
		axis_scale = s_vec2(axis_scale.x * 1.35f, axis_scale.y * 1.35f);
		color_y = s_vec3(0.45f, 1.0f, 0.56f);
	}
	se_object_2d_set_position(app->gizmo_2d_center, &selected_pos);
	se_object_2d_set_position(app->gizmo_2d_x, &x_handle_pos);
	se_object_2d_set_position(app->gizmo_2d_y, &y_handle_pos);
	se_object_2d_set_scale(app->gizmo_2d_center, &center_scale);
	se_object_2d_set_scale(app->gizmo_2d_x, &axis_scale);
	se_object_2d_set_scale(app->gizmo_2d_y, &axis_scale);
	editor_set_2d_object_color(app->gizmo_2d_center, &color_center);
	editor_set_2d_object_color(app->gizmo_2d_x, &color_x);
	editor_set_2d_object_color(app->gizmo_2d_y, &color_y);
}

static void editor_draw_3d_helpers(editor_showcase_app* app) {
	if (!app || app->scene_3d == S_HANDLE_NULL || app->target != EDITOR_TARGET_SCENE_3D) {
		return;
	}
	se_scene_3d_clear_debug_markers(app->scene_3d);

	for (i32 i = -8; i <= 8; ++i) {
		s_vec4 line_color = s_vec4(0.22f, 0.24f, 0.30f, 1.0f);
		s_vec3 a = s_vec3((f32)i, 0.0f, -8.0f);
		s_vec3 b = s_vec3((f32)i, 0.0f, 8.0f);
		s_vec3 c = s_vec3(-8.0f, 0.0f, (f32)i);
		s_vec3 d = s_vec3(8.0f, 0.0f, (f32)i);
		if (i == 0) {
			line_color = s_vec4(0.34f, 0.38f, 0.46f, 1.0f);
		}
		se_scene_3d_debug_line(app->scene_3d, &a, &b, &line_color);
		se_scene_3d_debug_line(app->scene_3d, &c, &d, &line_color);
	}

	if (app->selected_3d != S_HANDLE_NULL) {
		const s_vec3 origin = editor_get_selected_3d_position(app);
		const s_vec4 color_x = s_vec4(0.95f, 0.28f, 0.26f, 1.0f);
		const s_vec4 color_y = s_vec4(0.24f, 0.92f, 0.38f, 1.0f);
		const s_vec4 color_z = s_vec4(0.34f, 0.62f, 0.95f, 1.0f);
		s_vec4 color_center = s_vec4(0.98f, 0.88f, 0.25f, 1.0f);
		s_vec3 x_end = s_vec3_add(&origin, &s_vec3(EDITOR_3D_GIZMO_AXIS_LENGTH, 0.0f, 0.0f));
		s_vec3 y_end = s_vec3_add(&origin, &s_vec3(0.0f, EDITOR_3D_GIZMO_AXIS_LENGTH, 0.0f));
		s_vec3 z_end = s_vec3_add(&origin, &s_vec3(0.0f, 0.0f, EDITOR_3D_GIZMO_AXIS_LENGTH));
		if (app->active_axis == EDITOR_GIZMO_AXIS_X && app->dragging) {
			color_center = s_vec4(1.0f, 0.60f, 0.42f, 1.0f);
		}
		if (app->active_axis == EDITOR_GIZMO_AXIS_Y && app->dragging) {
			color_center = s_vec4(0.56f, 1.0f, 0.62f, 1.0f);
		}
		if (app->active_axis == EDITOR_GIZMO_AXIS_Z && app->dragging) {
			color_center = s_vec4(0.58f, 0.76f, 1.0f, 1.0f);
		}
		if (app->active_axis == EDITOR_GIZMO_AXIS_FREE && app->dragging) {
			color_center = s_vec4(1.0f, 0.95f, 0.55f, 1.0f);
		}
		se_scene_3d_debug_line(app->scene_3d, &origin, &x_end, &color_x);
		se_scene_3d_debug_line(app->scene_3d, &origin, &y_end, &color_y);
		se_scene_3d_debug_line(app->scene_3d, &origin, &z_end, &color_z);
		se_scene_3d_debug_sphere(app->scene_3d, &origin, 0.10f, &color_center);
	}
}

static void editor_update_ui_status(editor_showcase_app* app) {
	c8 status[512] = {0};
	const c8* target_name = "scene_2d";
	const c8* axis_name = "none";
	f64 now = 0.0;
	if (!app || app->ui == S_HANDLE_NULL || app->ui_status == S_HANDLE_NULL) {
		return;
	}
	now = se_window_get_time(app->window);
	if (now < app->next_status_time) {
		return;
	}
	if (app->target == EDITOR_TARGET_SCENE_3D) {
		target_name = "scene_3d";
	}
	if (app->active_axis == EDITOR_GIZMO_AXIS_FREE) {
		axis_name = "free";
	} else if (app->active_axis == EDITOR_GIZMO_AXIS_X) {
		axis_name = "x";
	} else if (app->active_axis == EDITOR_GIZMO_AXIS_Y) {
		axis_name = "y";
	} else if (app->active_axis == EDITOR_GIZMO_AXIS_Z) {
		axis_name = "z";
	}
	snprintf(
		status,
		sizeof(status),
		"Mode=%s | Drag=%s axis=%s | selected2d=%llu selected3d=%llu | objects2d=%zu objects3d=%zu | Tab switch mode | N add | Delete remove | LMB select/drag gizmo",
		target_name,
		app->dragging ? "on" : "off",
		axis_name,
		(unsigned long long)app->selected_2d,
		(unsigned long long)app->selected_3d,
		s_array_get_size(&app->objects_2d),
		s_array_get_size(&app->objects_3d));
	(void)se_ui_widget_set_text(app->ui, app->ui_status, status);
	app->next_status_time = now + 0.08;
}

static b8 editor_setup_ui(editor_showcase_app* app) {
	se_ui_style panel_style = SE_UI_STYLE_DEFAULT;
	if (!app || app->window == S_HANDLE_NULL) {
		return false;
	}
	app->ui = se_ui_create(app->window, 64);
	if (app->ui == S_HANDLE_NULL) {
		return false;
	}

	se_ui_widget_handle root = se_ui_create_root(app->ui);
	app->ui_panel = se_ui_add_panel(root, {
		.id = "editor_hud",
		.position = s_vec2(0.012f, 0.012f),
		.size = s_vec2(0.976f, 0.095f)
	});
	se_ui_vstack(app->ui, app->ui_panel, 0.005f, se_ui_edge_all(0.008f));

	app->ui_title = se_ui_add_text(app->ui_panel, {
		.id = "editor_title",
		.text = "Advanced Editor Showcase | 2D+3D Scene Editing",
		.size = s_vec2(0.95f, 0.038f),
		.font_size = 20.0f
	});

	app->ui_status = se_ui_add_text(app->ui_panel, {
		.id = "editor_status",
		.text = "Initializing editor state...",
		.size = s_vec2(0.95f, 0.032f),
		.font_size = 14.0f
	});

	panel_style.normal.background_color = s_vec4(0.03f, 0.06f, 0.10f, 0.92f);
	panel_style.normal.border_color = s_vec4(0.21f, 0.63f, 0.84f, 1.0f);
	panel_style.normal.text_color = s_vec4(0.94f, 0.98f, 1.0f, 1.0f);
	panel_style.normal.border_width = 0.004f;
	panel_style.hovered = panel_style.normal;
	panel_style.pressed = panel_style.normal;
	panel_style.disabled = panel_style.normal;
	panel_style.focused = panel_style.normal;
	(void)se_ui_widget_set_style(app->ui, app->ui_panel, &panel_style);
	return true;
}

static b8 editor_setup_scenes(editor_showcase_app* app) {
	se_camera_controller_params camera_params = SE_CAMERA_CONTROLLER_PARAMS_DEFAULTS;
	se_editor_config editor_config = SE_EDITOR_CONFIG_DEFAULTS;
	if (!app) {
		return false;
	}
	app->scene_2d = se_scene_2d_create(&s_vec2((f32)SHOWCASE_WIDTH, (f32)SHOWCASE_HEIGHT), 128);
	if (app->scene_2d == S_HANDLE_NULL) {
		editor_log_error("create scene_2d");
		return false;
	}
	se_scene_2d_set_auto_resize(app->scene_2d, app->window, &s_vec2(1.0f, 1.0f));

	app->scene_3d = se_scene_3d_create_for_window(app->window, 128);
	if (app->scene_3d == S_HANDLE_NULL) {
		editor_log_error("create scene_3d");
		return false;
	}
	app->camera_3d = se_scene_3d_get_camera(app->scene_3d);
	if (app->camera_3d == S_HANDLE_NULL) {
		editor_log_error("scene_3d camera");
		return false;
	}
	se_camera_set_orbit_defaults(app->camera_3d, app->window, &s_vec3(0.0f, 0.5f, 0.0f), 7.5f);

	camera_params.window = app->window;
	camera_params.camera = app->camera_3d;
	camera_params.movement_speed = 7.0f;
	camera_params.mouse_x_speed = 0.0032f;
	camera_params.mouse_y_speed = 0.0032f;
	camera_params.preset = SE_CAMERA_CONTROLLER_PRESET_UE;
	camera_params.look_toggle = false;
	camera_params.lock_cursor_while_active = true;
	app->camera_controller = se_camera_controller_create(&camera_params);

	app->cube_model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"));
	if (app->cube_model == S_HANDLE_NULL) {
		editor_log_error("load cube model");
		return false;
	}

	editor_config.context = app->context;
	editor_config.window = app->window;
	app->editor = se_editor_create(&editor_config);
	if (!app->editor) {
		editor_log_error("create editor");
		return false;
	}

	app->gizmo_2d_center = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/panel.glsl"), &s_mat3_identity, 0);
	app->gizmo_2d_x = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/panel.glsl"), &s_mat3_identity, 0);
	app->gizmo_2d_y = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/panel.glsl"), &s_mat3_identity, 0);
	if (app->gizmo_2d_center == S_HANDLE_NULL || app->gizmo_2d_x == S_HANDLE_NULL || app->gizmo_2d_y == S_HANDLE_NULL) {
		editor_log_error("create 2d gizmo objects");
		return false;
	}
	se_object_2d_set_scale(app->gizmo_2d_center, &s_vec2(EDITOR_2D_GIZMO_HANDLE_SIZE, EDITOR_2D_GIZMO_HANDLE_SIZE));
	se_object_2d_set_scale(app->gizmo_2d_x, &s_vec2(EDITOR_2D_GIZMO_HANDLE_SIZE, EDITOR_2D_GIZMO_HANDLE_SIZE));
	se_object_2d_set_scale(app->gizmo_2d_y, &s_vec2(EDITOR_2D_GIZMO_HANDLE_SIZE, EDITOR_2D_GIZMO_HANDLE_SIZE));
	se_scene_2d_add_object(app->scene_2d, app->gizmo_2d_center);
	se_scene_2d_add_object(app->scene_2d, app->gizmo_2d_x);
	se_scene_2d_add_object(app->scene_2d, app->gizmo_2d_y);
	se_object_2d_set_position(app->gizmo_2d_center, &s_vec2(-3.0f, -3.0f));
	se_object_2d_set_position(app->gizmo_2d_x, &s_vec2(-3.0f, -3.0f));
	se_object_2d_set_position(app->gizmo_2d_y, &s_vec2(-3.0f, -3.0f));

	app->selected_2d = editor_spawn_object_2d(app, &s_vec2(-0.60f, -0.30f));
	(void)editor_spawn_object_2d(app, &s_vec2(-0.10f, -0.05f));
	(void)editor_spawn_object_2d(app, &s_vec2(0.44f, 0.26f));

	app->selected_3d = editor_spawn_object_3d(app, &s_vec3(-1.1f, 0.35f, -0.7f));
	(void)editor_spawn_object_3d(app, &s_vec3(0.0f, 0.35f, 0.0f));
	(void)editor_spawn_object_3d(app, &s_vec3(1.25f, 0.35f, 0.55f));

	return true;
}

static b8 editor_screen_to_horizontal_plane(editor_showcase_app* app, const f32 plane_y, s_vec3* out_world) {
	s_vec2 mouse_framebuffer = s_vec2(0.0f, 0.0f);
	f32 viewport_width = 0.0f;
	f32 viewport_height = 0.0f;
	s_vec3 plane_point = s_vec3(0.0f, plane_y, 0.0f);
	s_vec3 plane_normal = s_vec3(0.0f, 1.0f, 0.0f);
	if (!app || app->camera_3d == S_HANDLE_NULL || !out_world) {
		return false;
	}
	if (!editor_get_mouse_framebuffer(app, &mouse_framebuffer, &viewport_width, &viewport_height)) {
		return false;
	}
	return se_camera_screen_to_plane(
		app->camera_3d,
		mouse_framebuffer.x,
		mouse_framebuffer.y,
		viewport_width,
		viewport_height,
		&plane_point,
		&plane_normal,
		out_world);
}

static void editor_add_object_from_cursor(editor_showcase_app* app) {
	if (!app) {
		return;
	}
	if (app->target == EDITOR_TARGET_SCENE_2D) {
		s_vec2 mouse_ndc = s_vec2(0.0f, 0.0f);
		se_window_get_mouse_position_normalized(app->window, &mouse_ndc);
		app->selected_2d = editor_spawn_object_2d(app, &mouse_ndc);
		return;
	}
	if (app->target == EDITOR_TARGET_SCENE_3D) {
		s_vec3 world = s_vec3(0.0f, 0.35f, 0.0f);
		if (editor_screen_to_horizontal_plane(app, 0.35f, &world)) {
			world.y = 0.35f;
		}
		app->selected_3d = editor_spawn_object_3d(app, &world);
	}
}

static void editor_remove_selected_object(editor_showcase_app* app) {
	if (!app) {
		return;
	}
	if (app->target == EDITOR_TARGET_SCENE_2D) {
		editor_delete_object_2d(app, app->selected_2d);
		return;
	}
	editor_delete_object_3d(app, app->selected_3d);
}

static b8 editor_is_alt_down(editor_showcase_app* app) {
	if (!app) {
		return false;
	}
	return se_window_is_key_down(app->window, SE_KEY_LEFT_ALT) || se_window_is_key_down(app->window, SE_KEY_RIGHT_ALT);
}

static void editor_update_scene_2d_interaction(editor_showcase_app* app) {
	s_vec2 mouse_ndc = s_vec2(0.0f, 0.0f);
	if (!app || app->target != EDITOR_TARGET_SCENE_2D) {
		return;
	}
	if (se_window_is_mouse_pressed(app->window, SE_MOUSE_LEFT)) {
		editor_gizmo_axis axis = EDITOR_GIZMO_AXIS_NONE;
		se_object_2d_handle picked = S_HANDLE_NULL;
		se_window_get_mouse_position_normalized(app->window, &mouse_ndc);
		axis = editor_pick_2d_gizmo_axis(app, &mouse_ndc);
		if (axis != EDITOR_GIZMO_AXIS_NONE) {
			editor_begin_drag_2d(app, axis);
			return;
		}
		picked = editor_pick_scene_object_2d(app);
		app->selected_2d = picked;
		if (picked != S_HANDLE_NULL) {
			editor_begin_drag_2d(app, EDITOR_GIZMO_AXIS_FREE);
		}
	}
	if (app->dragging && se_window_is_mouse_down(app->window, SE_MOUSE_LEFT)) {
		editor_update_drag_2d(app);
	}
	if (app->dragging && se_window_is_mouse_released(app->window, SE_MOUSE_LEFT)) {
		editor_end_drag(app);
	}
}

static void editor_update_scene_3d_interaction(editor_showcase_app* app) {
	s_vec2 mouse_framebuffer = s_vec2(0.0f, 0.0f);
	f32 viewport_width = 0.0f;
	f32 viewport_height = 0.0f;
	if (!app || app->target != EDITOR_TARGET_SCENE_3D) {
		return;
	}
	if (!editor_get_mouse_framebuffer(app, &mouse_framebuffer, &viewport_width, &viewport_height)) {
		return;
	}

	if (se_window_is_mouse_pressed(app->window, SE_MOUSE_LEFT) && !editor_is_alt_down(app)) {
		editor_gizmo_axis axis = editor_pick_3d_gizmo_axis(app, &mouse_framebuffer, viewport_width, viewport_height);
		if (axis != EDITOR_GIZMO_AXIS_NONE) {
			editor_begin_drag_3d(app, axis, &mouse_framebuffer, viewport_width, viewport_height);
			return;
		}
		app->selected_3d = editor_pick_scene_object_3d(app);
		if (app->selected_3d != S_HANDLE_NULL) {
			editor_begin_drag_3d(app, EDITOR_GIZMO_AXIS_FREE, &mouse_framebuffer, viewport_width, viewport_height);
		}
	}
	if (app->dragging && se_window_is_mouse_down(app->window, SE_MOUSE_LEFT)) {
		editor_update_drag_3d(app, &mouse_framebuffer, viewport_width, viewport_height);
	}
	if (app->dragging && se_window_is_mouse_released(app->window, SE_MOUSE_LEFT)) {
		editor_end_drag(app);
	}
}

static void editor_cleanup(editor_showcase_app* app) {
	if (!app) {
		return;
	}
	if (app->camera_controller) {
		se_camera_controller_destroy(app->camera_controller);
		app->camera_controller = NULL;
	}
	if (app->editor) {
		se_editor_destroy(app->editor);
		app->editor = NULL;
	}
	if (app->ui != S_HANDLE_NULL) {
		se_ui_destroy(app->ui);
		app->ui = S_HANDLE_NULL;
	}
	if (app->scene_2d != S_HANDLE_NULL) {
		se_scene_2d_destroy_full(app->scene_2d, true);
		app->scene_2d = S_HANDLE_NULL;
	}
	if (app->scene_3d != S_HANDLE_NULL) {
		se_scene_3d_destroy_full(app->scene_3d, true, true);
		app->scene_3d = S_HANDLE_NULL;
	}
	s_array_clear(&app->objects_2d);
	s_array_clear(&app->objects_3d);
	if (app->context) {
		se_context_destroy(app->context);
		app->context = NULL;
	}
}

int main(void) {
	editor_showcase_app app = {0};
	printf("advanced/editor_showcase :: Interactive scene editor example\n");
	se_debug_set_level(SE_DEBUG_LEVEL_WARN);

	s_array_init(&app.objects_2d);
	s_array_init(&app.objects_3d);
	app.target = EDITOR_TARGET_SCENE_2D;
	app.active_axis = EDITOR_GIZMO_AXIS_NONE;

	app.context = se_context_create();
	if (!app.context) {
		printf("editor_showcase :: failed to create context\n");
		return 1;
	}

	app.window = se_window_create("Syphax - Advanced Editor Showcase", SHOWCASE_WIDTH, SHOWCASE_HEIGHT);
	if (app.window == S_HANDLE_NULL) {
		printf("editor_showcase :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		editor_cleanup(&app);
		return 0;
	}
	se_window_set_exit_key(app.window, SE_KEY_ESCAPE);
	se_window_set_target_fps(app.window, 60);
	se_render_set_background_color(s_vec4(0.02f, 0.03f, 0.05f, 1.0f));

	if (!editor_setup_ui(&app)) {
		printf("editor_showcase :: ui setup failed (%s)\n", se_result_str(se_get_last_error()));
		editor_cleanup(&app);
		return 1;
	}
	if (!editor_setup_scenes(&app)) {
		printf("editor_showcase :: scene setup failed (%s)\n", se_result_str(se_get_last_error()));
		editor_cleanup(&app);
		return 1;
	}

	printf("editor_showcase controls:\n");
	printf("  Tab: switch active editor target (scene_2d / scene_3d)\n");
	printf("  Left Mouse: select and drag with gizmo handles\n");
	printf("  N: add object at cursor\n");
	printf("  Delete/Backspace: remove selected object\n");
	printf("  3D camera: RMB fly + WASD, Alt+LMB orbit, Alt+MMB pan, Alt+RMB dolly\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(app.window)) {
		const f32 dt = (f32)se_window_get_delta_time(app.window);
		se_window_begin_frame(app.window);
		app.frame_index++;

		if (se_window_is_key_pressed(app.window, SE_KEY_TAB)) {
			if (app.dragging) {
				editor_end_drag(&app);
			}
			app.target = (app.target == EDITOR_TARGET_SCENE_2D) ? EDITOR_TARGET_SCENE_3D : EDITOR_TARGET_SCENE_2D;
		}
		if (se_window_is_key_pressed(app.window, SE_KEY_N)) {
			editor_add_object_from_cursor(&app);
		}
		if (se_window_is_key_pressed(app.window, SE_KEY_DELETE) || se_window_is_key_pressed(app.window, SE_KEY_BACKSPACE)) {
			editor_remove_selected_object(&app);
		}

		if (app.target == EDITOR_TARGET_SCENE_3D && app.camera_controller && !app.dragging) {
			se_camera_controller_set_enabled(app.camera_controller, true);
			se_camera_controller_tick(app.camera_controller, dt);
		} else if (app.camera_controller) {
			se_camera_controller_set_enabled(app.camera_controller, false);
		}

		editor_update_scene_2d_interaction(&app);
		editor_update_scene_3d_interaction(&app);

		editor_update_2d_object_colors(&app);
		editor_update_2d_gizmo_visuals(&app);
		editor_draw_3d_helpers(&app);
		editor_update_ui_status(&app);

		if (app.ui != S_HANDLE_NULL) {
			se_ui_tick(app.ui);
		}

		se_render_clear();
		if (app.target == EDITOR_TARGET_SCENE_2D) {
			se_scene_2d_draw(app.scene_2d, app.window);
		} else {
			se_scene_3d_draw(app.scene_3d, app.window);
		}
		if (app.ui != S_HANDLE_NULL) {
			se_ui_draw(app.ui);
		}
		se_window_end_frame(app.window);
	}

	editor_cleanup(&app);
	return 0;
}
