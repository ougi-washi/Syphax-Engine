// Syphax-Engine - Ougi Washi

#include "se_audio.h"
#include "se_camera.h"
#include "se_editor.h"
#include "se_graphics.h"
#include "se_input.h"
#include "se_navigation.h"
#include "se_physics.h"
#include "se_scene.h"
#include "se_simulation.h"
#include "se_text.h"
#include "se_texture.h"
#include "se_ui.h"
#include "se_vfx.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SHOWCASE_WIDTH 1400
#define SHOWCASE_HEIGHT 860

typedef struct {
	se_context* context;
	se_window_handle window;
	se_input_handle* input;
	se_editor* editor;

	se_scene_2d_handle scene_2d;
	se_object_2d_handle object_2d;

	se_scene_3d_handle scene_3d;
	se_object_3d_handle object_3d;
	se_instance_id object_3d_instance;
	se_camera_handle camera;
	se_model_handle model;
	se_shader_handle shader;

	se_ui_handle ui;
	se_ui_widget_handle ui_panel;
	se_ui_widget_handle ui_label;
	se_ui_widget_handle ui_subtitle;
	se_ui_widget_handle ui_metrics;
	se_ui_widget_handle ui_controls_row_a;
	se_ui_widget_handle ui_controls_row_b;
	se_ui_widget_handle ui_button_apply;
	se_ui_widget_handle ui_button_queue;
	se_ui_widget_handle ui_button_counts;
	se_ui_widget_handle ui_button_diag;
	se_ui_widget_handle ui_button_play;

	se_text_handle* text;
	se_font_handle font;

	se_vfx_2d_handle vfx_2d;
	se_vfx_emitter_2d_handle emitter_2d;
	se_vfx_3d_handle vfx_3d;
	se_vfx_emitter_3d_handle emitter_3d;

	se_texture_handle texture;
	se_framebuffer_handle framebuffer;
	se_render_buffer_handle render_buffer;

	se_audio_engine* audio;
	se_audio_clip* clip;
	se_audio_stream* stream;
	se_audio_capture* capture;

	se_navigation_grid navigation;
	b8 navigation_ready;

	se_physics_world_2d* physics_2d;
	se_physics_body_2d* body_2d;
	se_physics_world_3d* physics_3d;
	se_physics_body_3d* body_3d;

	se_simulation_handle simulation;

	u64 frame_index;
	f64 next_diagnostics_time;
	f64 next_ui_metrics_time;
	b8 batch_applied;
	b8 ui_request_apply_batch;
	b8 ui_request_queue_batch;
	b8 ui_request_print_counts;
	b8 ui_request_print_diagnostics;
	b8 ui_request_play_clip;
} editor_showcase_app;

static const u8 se_example_png_1x1_rgba[] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
	0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
	0x89, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x44, 0x41,
	0x54, 0x78, 0x9C, 0x63, 0xF8, 0xCF, 0xC0, 0xF0,
	0x1F, 0x00, 0x05, 0x00, 0x01, 0xFF, 0x89, 0x99,
	0x3D, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
	0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

static s_mat4 showcase_make_box_transform(const s_vec3* position, const s_vec3* half_extents) {
	s_mat4 transform = s_mat4_identity;
	transform = s_mat4_scale(&transform, &s_vec3(half_extents->x * 2.0f, half_extents->y * 2.0f, half_extents->z * 2.0f));
	s_mat4_set_translation(&transform, position);
	return transform;
}

static void showcase_report_editor_failure(const c8* command) {
	printf("editor_showcase :: command failed: %s (%s)\n", command, se_result_str(se_get_last_error()));
}

#define SHOWCASE_TRY_EDITOR(_command_expr) \
	do { \
		if (!(_command_expr)) { \
			showcase_report_editor_failure(#_command_expr); \
		} \
	} while (0)

static void showcase_print_counts(se_editor* editor) {
	se_editor_counts counts = {0};
	if (!editor || !se_editor_collect_counts(editor, &counts)) {
		printf("editor_showcase :: collect_counts failed (%s)\n", se_result_str(se_get_last_error()));
		return;
	}
	printf(
		"editor_showcase :: counts windows=%u cameras=%u scene2d=%u scene3d=%u ui=%u vfx2d=%u vfx3d=%u sim=%u audio(clips=%u streams=%u captures=%u) physics(2d=%u 3d=%u) queued=%u\n",
		counts.windows,
		counts.cameras,
		counts.scenes_2d,
		counts.scenes_3d,
		counts.uis,
		counts.vfx_2d,
		counts.vfx_3d,
		counts.simulations,
		counts.audio_clips,
		counts.audio_streams,
		counts.audio_captures,
		counts.physics_2d_bodies,
		counts.physics_3d_bodies,
		counts.queued_commands);
}

static b8 showcase_find_item_by_category_name(se_editor* editor, const c8* category_name, s_handle handle, se_editor_item* out_item) {
	const se_editor_category category = se_editor_category_from_name(category_name);
	if (!editor || !category_name || !out_item || category == SE_EDITOR_CATEGORY_COUNT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	return se_editor_find_item(editor, category, handle, out_item);
}

static void showcase_print_category_parse_demo(void) {
	printf(
		"editor_showcase :: category parser scene_2d=%s scene-3d=%s ui_widget=%s physics-2d-body=%s\n",
		se_editor_category_name(se_editor_category_from_name("scene_2d")),
		se_editor_category_name(se_editor_category_from_name("Scene-3D")),
		se_editor_category_name(se_editor_category_from_name("ui_widget")),
		se_editor_category_name(se_editor_category_from_name("Physics-2D-Body")));
}

static f32 showcase_clamp01(f32 value) {
	if (value < 0.0f) {
		return 0.0f;
	}
	if (value > 1.0f) {
		return 1.0f;
	}
	return value;
}

static s_vec4 showcase_color_shift(const s_vec4 color, f32 delta) {
	return s_vec4(
		showcase_clamp01(color.x + delta),
		showcase_clamp01(color.y + delta),
		showcase_clamp01(color.z + delta),
		color.w);
}

static void showcase_style_panel(editor_showcase_app* app) {
	se_ui_style style = SE_UI_STYLE_DEFAULT;
	if (!app || app->ui == S_HANDLE_NULL || app->ui_panel == S_HANDLE_NULL) {
		return;
	}
	style.normal.background_color = s_vec4(0.02f, 0.04f, 0.07f, 1.00f);
	style.normal.border_color = s_vec4(0.15f, 0.60f, 0.82f, 1.00f);
	style.normal.text_color = s_vec4(0.96f, 0.98f, 1.00f, 1.00f);
	style.normal.border_width = 0.005f;
	style.hovered = style.normal;
	style.pressed = style.normal;
	style.disabled = style.normal;
	style.focused = style.normal;
	(void)se_ui_widget_set_style(app->ui, app->ui_panel, &style);
}

static void showcase_style_button(se_ui_handle ui, se_ui_widget_handle widget, s_vec4 base_bg, s_vec4 border, s_vec4 text_color) {
	se_ui_style style = SE_UI_STYLE_DEFAULT;
	if (ui == S_HANDLE_NULL || widget == S_HANDLE_NULL) {
		return;
	}
	style.normal.background_color = base_bg;
	style.normal.border_color = border;
	style.normal.text_color = text_color;
	style.normal.border_width = 0.0040f;
	style.hovered = style.normal;
	style.hovered.background_color = showcase_color_shift(base_bg, 0.06f);
	style.hovered.border_color = showcase_color_shift(border, 0.04f);
	style.pressed = style.normal;
	style.pressed.background_color = showcase_color_shift(base_bg, -0.06f);
	style.pressed.border_color = showcase_color_shift(border, -0.04f);
	style.disabled = style.normal;
	style.disabled.background_color = showcase_color_shift(base_bg, -0.10f);
	style.disabled.background_color.w = 1.00f;
	style.disabled.border_color = showcase_color_shift(border, -0.08f);
	style.disabled.border_color.w = 0.70f;
	style.disabled.text_color = showcase_color_shift(text_color, -0.30f);
	style.focused = style.normal;
	style.focused.border_color = showcase_color_shift(border, 0.10f);
	style.focused.border_width = 0.0052f;
	(void)se_ui_widget_set_style(ui, widget, &style);
}

static void showcase_on_click_apply(const se_ui_click_event* event, void* user_data) {
	editor_showcase_app* app = (editor_showcase_app*)user_data;
	(void)event;
	if (app) {
		app->ui_request_apply_batch = true;
	}
}

static void showcase_on_click_queue(const se_ui_click_event* event, void* user_data) {
	editor_showcase_app* app = (editor_showcase_app*)user_data;
	(void)event;
	if (app) {
		app->ui_request_queue_batch = true;
	}
}

static void showcase_on_click_counts(const se_ui_click_event* event, void* user_data) {
	editor_showcase_app* app = (editor_showcase_app*)user_data;
	(void)event;
	if (app) {
		app->ui_request_print_counts = true;
	}
}

static void showcase_on_click_diagnostics(const se_ui_click_event* event, void* user_data) {
	editor_showcase_app* app = (editor_showcase_app*)user_data;
	(void)event;
	if (app) {
		app->ui_request_print_diagnostics = true;
	}
}

static void showcase_on_click_play(const se_ui_click_event* event, void* user_data) {
	editor_showcase_app* app = (editor_showcase_app*)user_data;
	(void)event;
	if (app) {
		app->ui_request_play_clip = true;
	}
}

static void showcase_style_ui(editor_showcase_app* app) {
	if (!app || app->ui == S_HANDLE_NULL) {
		return;
	}
	showcase_style_panel(app);
	if (app->ui_subtitle != S_HANDLE_NULL) {
		(void)se_ui_widget_use_style_preset(app->ui, app->ui_subtitle, SE_UI_STYLE_PRESET_TEXT_MUTED);
	}
	if (app->ui_metrics != S_HANDLE_NULL) {
		(void)se_ui_widget_use_style_preset(app->ui, app->ui_metrics, SE_UI_STYLE_PRESET_TEXT_MUTED);
	}
	showcase_style_button(app->ui, app->ui_button_apply, s_vec4(0.08f, 0.38f, 0.46f, 0.94f), s_vec4(0.22f, 0.72f, 0.78f, 1.00f), s_vec4(0.96f, 0.99f, 1.00f, 1.00f));
	showcase_style_button(app->ui, app->ui_button_queue, s_vec4(0.05f, 0.26f, 0.42f, 1.00f), s_vec4(0.18f, 0.56f, 0.78f, 1.00f), s_vec4(0.92f, 0.98f, 1.00f, 1.00f));
	showcase_style_button(app->ui, app->ui_button_counts, s_vec4(0.13f, 0.16f, 0.30f, 1.00f), s_vec4(0.32f, 0.42f, 0.72f, 1.00f), s_vec4(0.90f, 0.92f, 0.98f, 1.00f));
	showcase_style_button(app->ui, app->ui_button_diag, s_vec4(0.18f, 0.14f, 0.33f, 1.00f), s_vec4(0.42f, 0.34f, 0.74f, 1.00f), s_vec4(0.95f, 0.92f, 1.00f, 1.00f));
	showcase_style_button(app->ui, app->ui_button_play, s_vec4(0.34f, 0.17f, 0.08f, 1.00f), s_vec4(0.78f, 0.44f, 0.18f, 1.00f), s_vec4(1.00f, 0.96f, 0.90f, 1.00f));

	if (app->ui_button_apply != S_HANDLE_NULL) {
		(void)se_ui_widget_set_alignment(app->ui, app->ui_button_apply, SE_UI_ALIGN_START, SE_UI_ALIGN_CENTER);
	}
	if (app->ui_button_queue != S_HANDLE_NULL) {
		(void)se_ui_widget_set_alignment(app->ui, app->ui_button_queue, SE_UI_ALIGN_START, SE_UI_ALIGN_CENTER);
	}
	if (app->ui_button_counts != S_HANDLE_NULL) {
		(void)se_ui_widget_set_alignment(app->ui, app->ui_button_counts, SE_UI_ALIGN_START, SE_UI_ALIGN_CENTER);
	}
	if (app->ui_button_diag != S_HANDLE_NULL) {
		(void)se_ui_widget_set_alignment(app->ui, app->ui_button_diag, SE_UI_ALIGN_START, SE_UI_ALIGN_CENTER);
	}
	if (app->ui_button_play != S_HANDLE_NULL) {
		(void)se_ui_widget_set_alignment(app->ui, app->ui_button_play, SE_UI_ALIGN_START, SE_UI_ALIGN_CENTER);
	}
}

static void showcase_update_ui_status(editor_showcase_app* app) {
	c8 title_text[SE_MAX_PATH_LENGTH] = {0};
	c8 metrics_text[SE_MAX_PATH_LENGTH] = {0};
	f64 now = 0.0;
	se_editor_counts counts = {0};
	se_editor_diagnostics diagnostics = {0};
	b8 has_counts = false;
	b8 has_diagnostics = false;
	if (!app || app->ui == S_HANDLE_NULL || app->ui_label == S_HANDLE_NULL) {
		return;
	}
	now = se_window_get_time(app->window);
	snprintf(
		title_text,
		sizeof(title_text),
		"Syphax Editor  |  %.0f FPS  |  frame %llu",
		se_window_get_fps(app->window),
		(unsigned long long)app->frame_index);
	(void)se_ui_widget_set_text(app->ui, app->ui_label, title_text);

	if (app->ui_metrics == S_HANDLE_NULL || now < app->next_ui_metrics_time) {
		return;
	}
	if (app->editor) {
		has_counts = se_editor_collect_counts(app->editor, &counts);
		has_diagnostics = se_editor_collect_diagnostics(app->editor, &diagnostics);
	}
	if (has_counts && has_diagnostics) {
		snprintf(
			metrics_text,
			sizeof(metrics_text),
			"CPU %.2f ms  GPU %.2f ms  |  obj2d %u  obj3d %u  sim %u  queue %u",
			diagnostics.trace_cpu_ms_last_frame,
			diagnostics.trace_gpu_ms_last_frame,
			counts.objects_2d,
			counts.objects_3d,
			counts.simulations,
			counts.queued_commands);
	} else {
		snprintf(metrics_text, sizeof(metrics_text), "Press buttons or keys (E/Q/C/D/Space) to drive the editor");
	}
	(void)se_ui_widget_set_text(app->ui, app->ui_metrics, metrics_text);
	app->next_ui_metrics_time = now + 0.20;
}

static void showcase_play_clip_via_editor(editor_showcase_app* app) {
	se_editor_item clip_item = {0};
	se_editor_value play_value = se_editor_value_vec4(s_vec4(0.40f, 0.0f, 1.0f, (f32)SE_AUDIO_BUS_SFX));
	if (!app || !app->editor) {
		return;
	}
	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_AUDIO_CLIP, S_HANDLE_NULL, &clip_item)) {
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &clip_item, "play", &play_value, NULL));
	}
}

static void showcase_print_category_property_counts(se_editor* editor) {
	if (!editor) {
		return;
	}
	for (u32 category = 0; category < (u32)SE_EDITOR_CATEGORY_COUNT; ++category) {
		const se_editor_property* properties = NULL;
		se_editor_item item = {0};
		sz property_count = 0;
		if (!se_editor_find_item(editor, (se_editor_category)category, S_HANDLE_NULL, &item)) {
			continue;
		}
		if (!se_editor_collect_properties(editor, &item, &properties, &property_count)) {
			continue;
		}
		printf("editor_showcase :: category=%s properties=%zu\n", se_editor_category_name((se_editor_category)category), property_count);
		(void)properties;
	}
}

static void showcase_print_diagnostics(se_editor* editor) {
	se_editor_diagnostics diagnostics = {0};
	if (!editor || !se_editor_collect_diagnostics(editor, &diagnostics)) {
		printf("editor_showcase :: collect_diagnostics failed (%s)\n", se_result_str(se_get_last_error()));
		return;
	}
	printf(
		"editor_showcase :: frame_ms=%.3f trace(events=%llu total_stats=%u last_stats=%u threads=%u cpu_ms=%.3f gpu_ms=%.3f)\n",
		diagnostics.frame_timing.frame_ms,
		(unsigned long long)diagnostics.trace_event_count,
		diagnostics.trace_stat_count_total,
		diagnostics.trace_stat_count_last_frame,
		diagnostics.trace_thread_count_last_frame,
		diagnostics.trace_cpu_ms_last_frame,
		diagnostics.trace_gpu_ms_last_frame);
	if (diagnostics.has_audio_clip_bands) {
		printf(
			"editor_showcase :: audio_clip_bands low=%.3f mid=%.3f high=%.3f\n",
			diagnostics.audio_clip_bands.low,
			diagnostics.audio_clip_bands.mid,
			diagnostics.audio_clip_bands.high);
	}
	if (diagnostics.has_audio_capture_bands) {
		printf(
			"editor_showcase :: audio_capture_bands low=%.3f mid=%.3f high=%.3f\n",
			diagnostics.audio_capture_bands.low,
			diagnostics.audio_capture_bands.mid,
			diagnostics.audio_capture_bands.high);
	}
}

static void showcase_apply_queue_demo(editor_showcase_app* app) {
	se_editor_command command = {0};
	const se_editor_command* queued_commands = NULL;
	se_editor_item item = {0};
	se_editor_value value = se_editor_value_none();
	sz queue_count = 0;
	u32 applied = 0;

	if (!app || !app->editor) {
		return;
	}

	se_editor_clear_queue(app->editor);

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_DEBUG, S_HANDLE_NULL, &item)) {
		SHOWCASE_TRY_EDITOR(se_editor_validate_item(app->editor, &item));
		value = se_editor_value_bool(false);
		command = se_editor_command_make_set(&item, "overlay_enabled", &value);
		SHOWCASE_TRY_EDITOR(se_editor_queue_command(app->editor, &command));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_WINDOW, app->window, &item)) {
		SHOWCASE_TRY_EDITOR(se_editor_validate_item(app->editor, &item));
		value = se_editor_value_uint(60u);
		command = se_editor_command_make_set(&item, "target_fps", &value);
		SHOWCASE_TRY_EDITOR(se_editor_queue_command(app->editor, &command));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_AUDIO, S_HANDLE_NULL, &item)) {
		SHOWCASE_TRY_EDITOR(se_editor_validate_item(app->editor, &item));
		value = se_editor_value_float(0.92f);
		command = se_editor_command_make_set(&item, "master_volume", &value);
		SHOWCASE_TRY_EDITOR(se_editor_queue_command(app->editor, &command));
	}

	if (se_editor_get_queue(app->editor, &queued_commands, &queue_count)) {
		printf("editor_showcase :: queue demo queued=%zu\n", queue_count);
	}
	(void)queued_commands;

	if (se_editor_apply_queue(app->editor, &applied, true)) {
		printf("editor_showcase :: queue demo applied=%u\n", applied);
	}
}

static void showcase_apply_full_batch(editor_showcase_app* app) {
	se_editor_item item = {0};
	se_editor_value value = se_editor_value_none();
	se_editor_value aux_value = se_editor_value_none();

	if (!app || !app->editor) {
		return;
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_BACKEND, S_HANDLE_NULL, &item)) {
		value = se_editor_value_vec4(s_vec4(0.07f, 0.10f, 0.14f, 1.0f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "render_set_background_color", &value));
		value = se_editor_value_bool(true);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "render_set_blending", &value));
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "reload_changed_shaders", NULL, NULL));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_DEBUG, S_HANDLE_NULL, &item)) {
		value = se_editor_value_int(SE_DEBUG_LEVEL_WARN);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "level", &value));
		value = se_editor_value_bool(false);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "overlay_enabled", &value));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_WINDOW, app->window, &item)) {
		value = se_editor_value_uint(60u);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "target_fps", &value));
		value = se_editor_value_bool(false);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "raw_mouse_motion_enabled", &value));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_INPUT, S_HANDLE_NULL, &item)) {
		value = se_editor_value_bool(true);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "enabled", &value));
		value = se_editor_value_text("editor_showcase");
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "emit_text", &value, NULL));
	}

	if (app->camera != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_CAMERA, app->camera, &item)) {
		value = se_editor_value_float(56.0f);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "fov", &value));
		value = se_editor_value_handle(app->window);
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "set_aspect_from_window", &value, NULL));
	}

	if (app->scene_2d != S_HANDLE_NULL && showcase_find_item_by_category_name(app->editor, "scene_2d", app->scene_2d, &item)) {
		SHOWCASE_TRY_EDITOR(se_editor_validate_item(app->editor, &item));
		value = se_editor_value_vec2(s_vec2(640.0f, 360.0f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "output_size", &value));
		if (app->object_2d != S_HANDLE_NULL) {
			value = se_editor_value_handle(app->object_2d);
			SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "add_object", &value, NULL));
		}
	}

	if (app->object_2d != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_OBJECT_2D, app->object_2d, &item)) {
		value = se_editor_value_vec2(s_vec2(-0.58f, -0.30f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "position", &value));
		value = se_editor_value_vec2(s_vec2(0.20f, 0.12f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "scale", &value));
	}

	if (app->scene_3d != S_HANDLE_NULL && showcase_find_item_by_category_name(app->editor, "Scene-3D", app->scene_3d, &item)) {
		SHOWCASE_TRY_EDITOR(se_editor_validate_item(app->editor, &item));
		value = se_editor_value_bool(true);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "culling", &value));
		if (app->render_buffer != S_HANDLE_NULL) {
			value = se_editor_value_handle(app->render_buffer);
			SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "add_post_process_buffer", &value, NULL));
		}
	}

	if (app->object_3d != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_OBJECT_3D, app->object_3d, &item)) {
		value = se_editor_value_bool(true);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "visible", &value));
	}

	if (app->ui != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_UI, app->ui, &item)) {
		value = se_editor_value_bool(false);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "debug_overlay", &value));
		value = se_editor_value_int(SE_UI_THEME_PRESET_DEFAULT);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "theme_preset", &value));
	}

	if (app->ui_label != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_UI_WIDGET, app->ui_label, &item)) {
		value = se_editor_value_text("Editor is now driving all systems");
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "text", &value));
		value = se_editor_value_int(4);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "z_order", &value));
	}

	if (app->vfx_2d != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_VFX_2D, app->vfx_2d, &item)) {
		value = se_editor_value_bool(true);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "focused", &value));
		if (app->emitter_2d != S_HANDLE_NULL) {
			value = se_editor_value_handle(app->emitter_2d);
			aux_value = se_editor_value_uint(24u);
			SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "emitter_burst", &value, &aux_value));
		}
	}

	if (app->vfx_3d != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_VFX_3D, app->vfx_3d, &item)) {
		value = se_editor_value_bool(true);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "focused", &value));
		if (app->emitter_3d != S_HANDLE_NULL) {
			value = se_editor_value_handle(app->emitter_3d);
			aux_value = se_editor_value_uint(16u);
			SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "emitter_burst", &value, &aux_value));
		}
	}

	if (app->simulation != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_SIMULATION, app->simulation, &item)) {
		value = se_editor_value_bool(true);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "focused", &value));
		value = se_editor_value_uint(1u);
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "step", &value, NULL));
	}

	if (app->model != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_MODEL, app->model, &item)) {
		value = se_editor_value_vec3(s_vec3(1.005f, 1.0f, 1.0f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "scale", &value, NULL));
	}

	if (app->shader != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_SHADER, app->shader, &item)) {
		value = se_editor_value_bool(false);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "needs_reload", &value));
		value = se_editor_value_float(0.5f);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "uniform:u_time", &value));
	}

	if (app->framebuffer != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_FRAMEBUFFER, app->framebuffer, &item)) {
		value = se_editor_value_vec2(s_vec2(420.0f, 240.0f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "size", &value));
		value = se_editor_value_bool(false);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "auto_resize", &value));
	}

	if (app->render_buffer != S_HANDLE_NULL && se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_RENDER_BUFFER, app->render_buffer, &item)) {
		value = se_editor_value_vec2(s_vec2(0.96f, 0.96f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "scale", &value));
		value = se_editor_value_vec2(s_vec2(0.01f, 0.01f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "position", &value));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_AUDIO, S_HANDLE_NULL, &item)) {
		value = se_editor_value_float(0.90f);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "master_volume", &value));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_AUDIO_CLIP, S_HANDLE_NULL, &item)) {
		value = se_editor_value_float(0.70f);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "volume", &value));
		value = se_editor_value_vec4(s_vec4(0.45f, 0.0f, 1.0f, (f32)SE_AUDIO_BUS_SFX));
		aux_value = se_editor_value_bool(false);
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "play", &value, &aux_value));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_AUDIO_STREAM, S_HANDLE_NULL, &item)) {
		value = se_editor_value_float(0.28f);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "volume", &value));
		value = se_editor_value_double(0.0);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "seek", &value));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_AUDIO_CAPTURE, S_HANDLE_NULL, &item)) {
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "stop", NULL, NULL));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_NAVIGATION, S_HANDLE_NULL, &item)) {
		value = se_editor_value_float(0.55f);
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "cell_size", &value));
		value = se_editor_value_vec3(s_vec3(4.0f, 4.0f, 1.0f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "set_cell_blocked", &value, NULL));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_PHYSICS_2D, S_HANDLE_NULL, &item)) {
		value = se_editor_value_vec2(s_vec2(0.0f, -2.8f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "gravity", &value));
		value = se_editor_value_float(1.0f / 120.0f);
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "step", &value, NULL));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_PHYSICS_2D_BODY, S_HANDLE_NULL, &item)) {
		value = se_editor_value_vec2(s_vec2(0.0f, 1.6f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "apply_impulse", &value, NULL));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_PHYSICS_3D, S_HANDLE_NULL, &item)) {
		value = se_editor_value_vec3(s_vec3(0.0f, -4.0f, 0.0f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_set(app->editor, &item, "gravity", &value));
		value = se_editor_value_float(1.0f / 120.0f);
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "step", &value, NULL));
	}

	if (se_editor_find_item(app->editor, SE_EDITOR_CATEGORY_PHYSICS_3D_BODY, S_HANDLE_NULL, &item)) {
		value = se_editor_value_vec3(s_vec3(0.0f, 2.5f, 0.0f));
		SHOWCASE_TRY_EDITOR(se_editor_apply_action(app->editor, &item, "apply_impulse", &value, NULL));
	}

	showcase_style_ui(app);
	showcase_apply_queue_demo(app);
}

int main(void) {
	printf("advanced/editor_showcase :: Advanced editor integration example\n");
	se_debug_set_level(SE_DEBUG_LEVEL_WARN);

	editor_showcase_app app = {0};
	s_vec3 box_half_extents = s_vec3(0.35f, 0.35f, 0.35f);
	se_audio_config audio_config = SE_AUDIO_CONFIG_DEFAULTS;
	se_audio_play_params stream_params = {
		.volume = 0.30f,
		.pan = 0.0f,
		.pitch = 1.0f,
		.looping = true,
		.bus = SE_AUDIO_BUS_MUSIC
	};
	se_physics_world_params_2d physics_params_2d = SE_PHYSICS_WORLD_PARAMS_2D_DEFAULTS;
	se_physics_world_params_3d physics_params_3d = SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS;
	se_physics_body_params_2d body_params_2d = SE_PHYSICS_BODY_PARAMS_2D_DEFAULTS;
	se_physics_body_params_3d body_params_3d = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
	se_vfx_2d_params vfx_params_2d = SE_VFX_2D_PARAMS_DEFAULTS;
	se_vfx_3d_params vfx_params_3d = SE_VFX_3D_PARAMS_DEFAULTS;
	se_vfx_emitter_2d_params emitter_params_2d = SE_VFX_EMITTER_2D_PARAMS_DEFAULTS;
	se_vfx_emitter_3d_params emitter_params_3d = SE_VFX_EMITTER_3D_PARAMS_DEFAULTS;
	se_simulation_config simulation_config = SE_SIMULATION_CONFIG_DEFAULTS;
	se_editor_config editor_config = SE_EDITOR_CONFIG_DEFAULTS;

	app.context = se_context_create();
	if (!app.context) {
		printf("editor_showcase :: failed to create context\n");
		return 1;
	}

	app.window = se_window_create("Syphax - Advanced Editor Showcase", SHOWCASE_WIDTH, SHOWCASE_HEIGHT);
	if (app.window == S_HANDLE_NULL) {
		printf("editor_showcase :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(app.context);
		return 0;
	}
	se_window_set_exit_key(app.window, SE_KEY_ESCAPE);
	se_window_set_target_fps(app.window, 60);
	se_render_set_background_color(s_vec4(0.01f, 0.02f, 0.04f, 1.0f));

	app.input = se_input_create(app.window, 128);
	if (app.input) {
		se_input_attach_window_text(app.input);
	}

	app.scene_2d = se_scene_2d_create(&s_vec2((f32)SHOWCASE_WIDTH, (f32)SHOWCASE_HEIGHT), 16);
	if (app.scene_2d != S_HANDLE_NULL) {
		se_scene_2d_set_auto_resize(app.scene_2d, app.window, &s_vec2(1.0f, 1.0f));
		app.object_2d = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d_inst/instance.glsl"), &s_mat3_identity, 8);
		if (app.object_2d != S_HANDLE_NULL) {
			s_mat3 transform = s_mat3_identity;
			s_mat3_set_translation(&transform, &s_vec2(-0.60f, -0.26f));
			se_object_2d_set_transform(app.object_2d, &transform);
			app.shader = se_object_2d_get_shader(app.object_2d);
			if (app.shader != S_HANDLE_NULL) {
				se_shader_set_vec3(app.shader, "u_color", &s_vec3(0.2f, 0.8f, 0.7f));
			}
			se_scene_2d_add_object(app.scene_2d, app.object_2d);
		}
	}

	app.scene_3d = se_scene_3d_create_for_window(app.window, 64);
	if (app.scene_3d != S_HANDLE_NULL) {
		app.model = se_model_load_obj_simple(
			SE_RESOURCE_PUBLIC("models/cube.obj"),
			SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
			SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"));
		if (app.model != S_HANDLE_NULL) {
			app.object_3d = se_scene_3d_add_model(app.scene_3d, app.model, &s_mat4_identity);
			if (app.object_3d != S_HANDLE_NULL) {
				s_mat4 transform = showcase_make_box_transform(&s_vec3(0.0f, 0.35f, 0.0f), &box_half_extents);
				app.object_3d_instance = se_object_3d_add_instance(app.object_3d, &transform, &s_mat4_identity);
			}
			if (app.shader == S_HANDLE_NULL && se_model_get_mesh_count(app.model) > 0u) {
				app.shader = se_model_get_mesh_shader(app.model, 0u);
			}
		}
		app.camera = se_scene_3d_get_camera(app.scene_3d);
		if (app.camera == S_HANDLE_NULL) {
			app.camera = se_camera_create();
			if (app.camera != S_HANDLE_NULL) {
				se_scene_3d_set_camera(app.scene_3d, app.camera);
			}
		}
		if (app.camera != S_HANDLE_NULL) {
			se_camera_set_aspect_from_window(app.camera, app.window);
			se_camera* camera_ptr = se_camera_get(app.camera);
			if (camera_ptr) {
				camera_ptr->position = s_vec3(3.2f, 2.4f, 4.4f);
				camera_ptr->target = s_vec3(0.0f, 0.35f, 0.0f);
			}
		}
	}

	app.ui = se_ui_create(app.window, 96);
	if (app.ui != S_HANDLE_NULL) {
		se_ui_widget_handle root = se_ui_create_root(app.ui);
		se_ui_style root_style = SE_UI_STYLE_DEFAULT;
		root_style.normal.background_color = s_vec4(0.0f, 0.0f, 0.0f, 0.0f);
		root_style.normal.border_color = s_vec4(0.0f, 0.0f, 0.0f, 0.0f);
		root_style.normal.border_width = 0.0f;
		root_style.hovered = root_style.normal;
		root_style.pressed = root_style.normal;
		root_style.disabled = root_style.normal;
		root_style.focused = root_style.normal;
		(void)se_ui_widget_set_style(app.ui, root, &root_style);

		app.ui_panel = se_ui_add_panel(root, {
			.id = "hud_panel",
			.position = s_vec2(0.02f, 0.02f),
			.size = s_vec2(0.44f, 0.18f)
		});
		(void)se_ui_widget_set_alignment(app.ui, app.ui_panel, SE_UI_ALIGN_START, SE_UI_ALIGN_START);
		se_ui_vstack(app.ui, app.ui_panel, 0.006f, se_ui_edge_all(0.010f));
		app.ui_label = se_ui_add_text(app.ui_panel, {
			.id = "hud_title",
			.text = "Syphax Editor Showcase",
			.size = s_vec2(0.40f, 0.045f),
			.font_size = 22.0f
		});
		app.ui_controls_row_a = se_ui_add_panel(app.ui_panel, {
			.id = "hud_controls_row_a",
			.size = s_vec2(0.40f, 0.046f)
		});
		(void)se_ui_widget_set_alignment(app.ui, app.ui_controls_row_a, SE_UI_ALIGN_START, SE_UI_ALIGN_START);
		se_ui_hstack(app.ui, app.ui_controls_row_a, 0.010f, SE_UI_EDGE_ZERO);
		app.ui_button_apply = se_ui_add_button(app.ui_controls_row_a, {
			.id = "hud_apply",
			.text = "Apply Batch",
			.size = s_vec2(0.195f, 0.042f),
			.font_size = 14.0f,
			.on_click_fn = showcase_on_click_apply,
			.on_click_data = &app
		});
		app.ui_button_queue = se_ui_add_button(app.ui_controls_row_a, {
			.id = "hud_queue",
			.text = "Queue Demo",
			.size = s_vec2(0.195f, 0.042f),
			.font_size = 14.0f,
			.on_click_fn = showcase_on_click_queue,
			.on_click_data = &app
		});
		app.ui_controls_row_b = se_ui_add_panel(app.ui_panel, {
			.id = "hud_controls_row_b",
			.size = s_vec2(0.40f, 0.046f)
		});
		(void)se_ui_widget_set_alignment(app.ui, app.ui_controls_row_b, SE_UI_ALIGN_START, SE_UI_ALIGN_START);
		se_ui_hstack(app.ui, app.ui_controls_row_b, 0.010f, SE_UI_EDGE_ZERO);
		app.ui_button_counts = se_ui_add_button(app.ui_controls_row_b, {
			.id = "hud_counts",
			.text = "Counts",
			.size = s_vec2(0.125f, 0.042f),
			.font_size = 14.0f,
			.on_click_fn = showcase_on_click_counts,
			.on_click_data = &app
		});
		app.ui_button_diag = se_ui_add_button(app.ui_controls_row_b, {
			.id = "hud_diag",
			.text = "Diagnostics",
			.size = s_vec2(0.135f, 0.042f),
			.font_size = 14.0f,
			.on_click_fn = showcase_on_click_diagnostics,
			.on_click_data = &app
		});
		app.ui_button_play = se_ui_add_button(app.ui_controls_row_b, {
			.id = "hud_play",
			.text = "Play Clip",
			.size = s_vec2(0.125f, 0.042f),
			.font_size = 14.0f,
			.on_click_fn = showcase_on_click_play,
			.on_click_data = &app
		});
		showcase_style_ui(&app);
	}

	app.text = se_text_handle_create(4u);
	app.font = se_font_load(SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 30.0f);

	audio_config.enable_capture = true;
	app.audio = se_audio_init(&audio_config);
	if (app.audio) {
		app.clip = se_audio_clip_load(app.audio, SE_RESOURCE_PUBLIC("audio/chime.wav"));
		app.stream = se_audio_stream_open(app.audio, SE_RESOURCE_PUBLIC("audio/loop.wav"), &stream_params);
	}

	if (app.audio) {
		se_audio_capture_config capture_config = {0};
		capture_config.device_index = 0u;
		capture_config.sample_rate = 48000u;
		capture_config.channels = 2u;
		capture_config.frames_per_buffer = 512u;
		app.capture = se_audio_capture_start(app.audio, &capture_config);
	}

	if (se_navigation_grid_create(&app.navigation, 28, 28, 0.5f, &s_vec2(-7.0f, -7.0f))) {
		app.navigation_ready = true;
	}

	physics_params_2d.gravity = s_vec2(0.0f, -2.2f);
	app.physics_2d = se_physics_world_2d_create(&physics_params_2d);
	if (app.physics_2d) {
		body_params_2d.type = SE_PHYSICS_BODY_DYNAMIC;
		body_params_2d.position = s_vec2(-0.58f, -0.22f);
		app.body_2d = se_physics_body_2d_create(app.physics_2d, &body_params_2d);
		if (app.body_2d) {
			se_physics_body_2d_add_box(app.body_2d, &s_vec2(0.0f, 0.0f), &s_vec2(0.12f, 0.08f), 0.0f, false);
		}
	}

	physics_params_3d.gravity = s_vec3(0.0f, -3.0f, 0.0f);
	app.physics_3d = se_physics_world_3d_create(&physics_params_3d);
	if (app.physics_3d) {
		body_params_3d.type = SE_PHYSICS_BODY_DYNAMIC;
		body_params_3d.position = s_vec3(0.0f, 0.35f, 0.0f);
		app.body_3d = se_physics_body_3d_create(app.physics_3d, &body_params_3d);
		if (app.body_3d) {
			se_physics_body_3d_add_box(app.body_3d, &s_vec3(0.0f, 0.0f, 0.0f), &box_half_extents, &s_vec3(0.0f, 0.0f, 0.0f), false);
		}
	}

	simulation_config.max_entities = 128u;
	simulation_config.max_events = 128u;
	app.simulation = se_simulation_create(&simulation_config);

	app.framebuffer = se_framebuffer_create(&s_vec2(512.0f, 288.0f));
	app.render_buffer = se_render_buffer_create(512u, 288u, SE_RESOURCE_INTERNAL("shaders/render_quad_frag.glsl"));

	app.texture = se_texture_load_from_memory(se_example_png_1x1_rgba, sizeof(se_example_png_1x1_rgba), SE_CLAMP);

	vfx_params_2d.auto_resize_with_window = true;
	vfx_params_3d.auto_resize_with_window = true;
	app.vfx_2d = se_vfx_2d_create(&vfx_params_2d);
	app.vfx_3d = se_vfx_3d_create(&vfx_params_3d);

	if (app.vfx_2d != S_HANDLE_NULL) {
		emitter_params_2d.position = s_vec2(0.55f, -0.70f);
		emitter_params_2d.velocity = s_vec2(0.0f, 0.7f);
		emitter_params_2d.spawn_rate = 20.0f;
		emitter_params_2d.max_particles = 512u;
		emitter_params_2d.texture = app.texture;
		emitter_params_2d.size = 0.03f;
		app.emitter_2d = se_vfx_2d_add_emitter(app.vfx_2d, &emitter_params_2d);
	}

	if (app.vfx_3d != S_HANDLE_NULL) {
		emitter_params_3d.position = s_vec3(0.0f, 0.55f, 0.0f);
		emitter_params_3d.velocity = s_vec3(0.0f, 0.8f, 0.0f);
		emitter_params_3d.spawn_rate = 12.0f;
		emitter_params_3d.max_particles = 384u;
		emitter_params_3d.model = app.model;
		emitter_params_3d.size = 0.10f;
		app.emitter_3d = se_vfx_3d_add_emitter(app.vfx_3d, &emitter_params_3d);
	}

	editor_config.context = app.context;
	editor_config.window = app.window;
	editor_config.input = app.input;
	editor_config.audio = app.audio;
	editor_config.navigation = app.navigation_ready ? &app.navigation : NULL;
	editor_config.physics_2d = app.physics_2d;
	editor_config.physics_3d = app.physics_3d;
	editor_config.focused_simulation = app.simulation;
	editor_config.focused_vfx_2d = app.vfx_2d;
	editor_config.focused_vfx_3d = app.vfx_3d;
	app.editor = se_editor_create(&editor_config);
	if (app.editor) {
		if (app.clip) {
			se_editor_track_audio_clip(app.editor, app.clip, "chime_clip");
		}
		if (app.stream) {
			se_editor_track_audio_stream(app.editor, app.stream, "loop_stream");
		}
		if (app.capture) {
			se_editor_track_audio_capture(app.editor, app.capture, "mic_capture");
		}
		showcase_print_category_parse_demo();
	}

	printf("editor_showcase controls:\n");
	printf("  Mouse UI buttons: Apply Batch, Queue Demo, Counts, Diagnostics, Play Clip\n");
	printf("  E: apply full editor command batch (all systems)\n");
	printf("  Q: queue + apply command batch\n");
	printf("  C: print editor counts + property coverage\n");
	printf("  D: print editor diagnostics\n");
	printf("  Space: play audio clip through editor API\n");
	printf("  Esc: quit\n");

	app.next_diagnostics_time = 0.0;
	while (!se_window_should_close(app.window)) {
		se_window_begin_frame(app.window);
		app.frame_index++;
		showcase_update_ui_status(&app);

		if (app.input) {
			se_input_tick(app.input);
		}
		if (app.audio) {
			se_audio_update(app.audio);
		}

		const f32 dt = (f32)se_window_get_delta_time(app.window);
		const f32 time_s = (f32)se_window_get_time(app.window);

		if (app.camera != S_HANDLE_NULL) {
			se_camera_set_aspect_from_window(app.camera, app.window);
			se_camera* camera_ptr = se_camera_get(app.camera);
			if (camera_ptr) {
				camera_ptr->position = s_vec3(cosf(time_s * 0.35f) * 4.0f, 2.3f, sinf(time_s * 0.35f) * 4.0f);
				camera_ptr->target = s_vec3(0.0f, 0.3f, 0.0f);
			}
		}

		if (app.physics_2d) {
			se_physics_world_2d_step(app.physics_2d, dt);
			if (app.body_2d && app.object_2d != S_HANDLE_NULL) {
				se_object_2d_set_position(app.object_2d, &app.body_2d->position);
			}
		}

		if (app.physics_3d) {
			se_physics_world_3d_step(app.physics_3d, dt);
			if (app.body_3d && app.object_3d != S_HANDLE_NULL) {
				s_mat4 transform = showcase_make_box_transform(&app.body_3d->position, &box_half_extents);
				se_object_3d_set_instance_transform(app.object_3d, app.object_3d_instance, &transform);
			}
		}

		if (app.simulation != S_HANDLE_NULL && (app.frame_index % 6u) == 0u) {
			se_simulation_step(app.simulation, 1u);
		}

		if (app.vfx_2d != S_HANDLE_NULL) {
			se_vfx_2d_tick(app.vfx_2d, dt);
			se_vfx_2d_render(app.vfx_2d, app.window);
		}
		if (app.vfx_3d != S_HANDLE_NULL && app.camera != S_HANDLE_NULL) {
			se_vfx_3d_tick(app.vfx_3d, dt);
			se_vfx_3d_render(app.vfx_3d, app.window, app.camera);
		}

		if (app.editor && !app.batch_applied && app.frame_index > 3u) {
			showcase_apply_full_batch(&app);
			app.batch_applied = true;
			showcase_print_counts(app.editor);
		}

		if (se_window_is_key_pressed(app.window, SE_KEY_E)) {
			app.ui_request_apply_batch = true;
		}
		if (se_window_is_key_pressed(app.window, SE_KEY_Q)) {
			app.ui_request_queue_batch = true;
		}
		if (se_window_is_key_pressed(app.window, SE_KEY_C)) {
			app.ui_request_print_counts = true;
		}
		if (se_window_is_key_pressed(app.window, SE_KEY_D)) {
			app.ui_request_print_diagnostics = true;
		}
		if (se_window_is_key_pressed(app.window, SE_KEY_SPACE)) {
			app.ui_request_play_clip = true;
		}

		if (app.editor && se_window_get_time(app.window) >= app.next_diagnostics_time) {
			showcase_print_diagnostics(app.editor);
			app.next_diagnostics_time = se_window_get_time(app.window) + 4.0;
		}

		if (app.ui != S_HANDLE_NULL) {
			se_ui_tick(app.ui);
		}
		if (app.editor && app.ui_request_apply_batch) {
			showcase_apply_full_batch(&app);
		}
		if (app.editor && app.ui_request_queue_batch) {
			showcase_apply_queue_demo(&app);
		}
		if (app.editor && app.ui_request_print_counts) {
			showcase_print_counts(app.editor);
			showcase_print_category_property_counts(app.editor);
		}
		if (app.editor && app.ui_request_print_diagnostics) {
			showcase_print_diagnostics(app.editor);
		}
		if (app.editor && app.ui_request_play_clip) {
			showcase_play_clip_via_editor(&app);
		}
		app.ui_request_apply_batch = false;
		app.ui_request_queue_batch = false;
		app.ui_request_print_counts = false;
		app.ui_request_print_diagnostics = false;
		app.ui_request_play_clip = false;

		se_render_clear();
		if (app.scene_3d != S_HANDLE_NULL) {
			se_scene_3d_draw(app.scene_3d, app.window);
		}
		if (app.vfx_3d != S_HANDLE_NULL) {
			se_vfx_3d_draw(app.vfx_3d, app.window);
		}
		if (app.scene_2d != S_HANDLE_NULL) {
			se_scene_2d_draw(app.scene_2d, app.window);
		}
		if (app.vfx_2d != S_HANDLE_NULL) {
			se_vfx_2d_draw(app.vfx_2d, app.window);
		}
		if (app.ui != S_HANDLE_NULL) {
			se_ui_draw(app.ui);
		}
		if (app.text && app.font != S_HANDLE_NULL) {
			se_text_render(
				app.text,
				app.font,
				"syphax-engine :: advanced/editor_showcase",
				&s_vec2(-0.98f, -0.965f),
				&s_vec2(0.018f, 0.018f),
				0.018f);
		}

		se_window_end_frame(app.window);
	}

	if (app.editor) {
		se_editor_destroy(app.editor);
		app.editor = NULL;
	}

	if (app.vfx_2d != S_HANDLE_NULL) {
		se_vfx_2d_destroy(app.vfx_2d);
		app.vfx_2d = S_HANDLE_NULL;
	}
	if (app.vfx_3d != S_HANDLE_NULL) {
		se_vfx_3d_destroy(app.vfx_3d);
		app.vfx_3d = S_HANDLE_NULL;
	}

	if (app.ui != S_HANDLE_NULL) {
		se_ui_destroy(app.ui);
		app.ui = S_HANDLE_NULL;
	}

	if (app.text) {
		se_text_handle_destroy(app.text);
		app.text = NULL;
	}
	if (app.font != S_HANDLE_NULL) {
		se_font_destroy(app.font);
		app.font = S_HANDLE_NULL;
	}

	if (app.render_buffer != S_HANDLE_NULL) {
		se_render_buffer_destroy(app.render_buffer);
		app.render_buffer = S_HANDLE_NULL;
	}
	if (app.framebuffer != S_HANDLE_NULL) {
		se_framebuffer_destroy(app.framebuffer);
		app.framebuffer = S_HANDLE_NULL;
	}

	if (app.simulation != S_HANDLE_NULL) {
		se_simulation_destroy(app.simulation);
		app.simulation = S_HANDLE_NULL;
	}

	if (app.physics_2d) {
		se_physics_world_2d_destroy(app.physics_2d);
		app.physics_2d = NULL;
	}
	if (app.physics_3d) {
		se_physics_world_3d_destroy(app.physics_3d);
		app.physics_3d = NULL;
	}

	if (app.navigation_ready) {
		se_navigation_grid_destroy(&app.navigation);
		app.navigation_ready = false;
	}

	if (app.capture) {
		se_audio_capture_stop(app.capture);
		app.capture = NULL;
	}
	if (app.stream) {
		se_audio_stream_close(app.stream);
		app.stream = NULL;
	}
	if (app.clip && app.audio) {
		se_audio_clip_unload(app.audio, app.clip);
		app.clip = NULL;
	}
	if (app.audio) {
		se_audio_shutdown(app.audio);
		app.audio = NULL;
	}

	if (app.input) {
		se_input_destroy(app.input);
		app.input = NULL;
	}

	se_context_destroy(app.context);
	return 0;
}
