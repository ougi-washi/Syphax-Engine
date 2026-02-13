// Syphax-Engine - Ougi Washi

#include "se_debug.h"
#include "se_text.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef s_array(se_debug_trace_event, se_debug_trace_events);

static se_debug_level g_debug_level = SE_DEBUG_LEVEL_INFO;
static u32 g_debug_category_mask = SE_DEBUG_CATEGORY_ALL;
static se_debug_log_callback g_log_callback = NULL;
static void* g_log_user_data = NULL;
static b8 g_overlay_enabled = false;
static se_debug_trace_events g_trace_events = {0};
static se_text_handle* g_overlay_text_handle = NULL;
static se_context* g_overlay_context = NULL;
static se_debug_frame_timing g_frame_timing_current = {0};
static se_debug_frame_timing g_frame_timing_last = {0};
static b8 g_frame_active = false;
static f64 g_frame_begin_time = 0.0;
static u64 g_frame_counter = 0;

static f64 se_debug_now_seconds(void) {
	struct timespec ts = {0};
	timespec_get(&ts, TIME_UTC);
	return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1000000000.0;
}

static const c8* se_debug_level_name(const se_debug_level level) {
	switch (level) {
		case SE_DEBUG_LEVEL_TRACE: return "TRACE";
		case SE_DEBUG_LEVEL_DEBUG: return "DEBUG";
		case SE_DEBUG_LEVEL_INFO: return "INFO";
		case SE_DEBUG_LEVEL_WARN: return "WARN";
		case SE_DEBUG_LEVEL_ERROR: return "ERROR";
		default: return "UNKNOWN";
	}
}

static const c8* se_debug_category_name(const se_debug_category category) {
	switch (category) {
		case SE_DEBUG_CATEGORY_WINDOW: return "WINDOW";
		case SE_DEBUG_CATEGORY_INPUT: return "INPUT";
		case SE_DEBUG_CATEGORY_CAMERA: return "CAMERA";
		case SE_DEBUG_CATEGORY_UI: return "UI";
		case SE_DEBUG_CATEGORY_RENDER: return "RENDER";
		case SE_DEBUG_CATEGORY_NAVIGATION: return "NAVIGATION";
		case SE_DEBUG_CATEGORY_SCENE: return "SCENE";
		case SE_DEBUG_CATEGORY_CORE: return "CORE";
		default: return "GENERIC";
	}
}

static void se_debug_trace_events_init(void) {
	if (s_array_get_capacity(&g_trace_events) == 0) {
		s_array_init(&g_trace_events);
		s_array_reserve(&g_trace_events, 4096);
	}
}

static void se_debug_frame_timing_reset(se_debug_frame_timing* timing) {
	if (!timing) {
		return;
	}
	memset(timing, 0, sizeof(*timing));
}

static void se_debug_frame_timing_add_duration(const c8* name, const f64 duration_seconds) {
	if (!g_frame_active || !name || duration_seconds <= 0.0) {
		return;
	}
	const f64 duration_ms = duration_seconds * 1000.0;
	if (strcmp(name, "window_update") == 0) {
		g_frame_timing_current.window_update_ms += duration_ms;
		return;
	}
	if (strcmp(name, "input_tick") == 0) {
		g_frame_timing_current.input_ms += duration_ms;
		return;
	}
	if (strcmp(name, "scene2d_render") == 0) {
		g_frame_timing_current.scene2d_ms += duration_ms;
		return;
	}
	if (strcmp(name, "scene3d_render") == 0) {
		g_frame_timing_current.scene3d_ms += duration_ms;
		return;
	}
	if (strcmp(name, "text_render") == 0) {
		g_frame_timing_current.text_ms += duration_ms;
		return;
	}
	if (strcmp(name, "ui_render") == 0) {
		g_frame_timing_current.ui_ms += duration_ms;
		return;
	}
	if (strcmp(name, "navigation_astar") == 0) {
		g_frame_timing_current.navigation_ms += duration_ms;
		return;
	}
	if (strcmp(name, "window_present") == 0) {
		g_frame_timing_current.window_present_ms += duration_ms;
		return;
	}
	g_frame_timing_current.other_ms += duration_ms;
}

static void se_debug_frame_timing_finalize(const f64 now_seconds) {
	if (!g_frame_active) {
		return;
	}
	g_frame_timing_current.frame_ms = s_max(0.0, (now_seconds - g_frame_begin_time) * 1000.0);
	const f64 accounted_ms =
		g_frame_timing_current.window_update_ms +
		g_frame_timing_current.input_ms +
		g_frame_timing_current.scene2d_ms +
		g_frame_timing_current.scene3d_ms +
		g_frame_timing_current.text_ms +
		g_frame_timing_current.ui_ms +
		g_frame_timing_current.navigation_ms +
		g_frame_timing_current.window_present_ms +
		g_frame_timing_current.other_ms;
	if (g_frame_timing_current.frame_ms > accounted_ms) {
		g_frame_timing_current.other_ms += (g_frame_timing_current.frame_ms - accounted_ms);
	}
	g_frame_timing_current.frame_index = ++g_frame_counter;
	g_frame_timing_last = g_frame_timing_current;
	se_debug_frame_timing_reset(&g_frame_timing_current);
	g_frame_active = false;
	g_frame_begin_time = 0.0;
}

static void se_debug_log_default(const se_debug_level level, const se_debug_category category, const c8* message, void* user_data) {
	(void)user_data;
	fprintf(stderr, "[%s][%s] %s\n", se_debug_level_name(level), se_debug_category_name(category), message ? message : "");
}

void se_debug_set_level(const se_debug_level level) {
	g_debug_level = level;
}

se_debug_level se_debug_get_level(void) {
	return g_debug_level;
}

void se_debug_set_category_mask(const u32 category_mask) {
	g_debug_category_mask = category_mask;
}

u32 se_debug_get_category_mask(void) {
	return g_debug_category_mask;
}

void se_debug_set_log_callback(se_debug_log_callback callback, void* user_data) {
	g_log_callback = callback;
	g_log_user_data = user_data;
}

void se_debug_log(const se_debug_level level, const se_debug_category category, const c8* fmt, ...) {
	if (level < g_debug_level || ((u32)category & g_debug_category_mask) == 0u) {
		return;
	}
	c8 message[1024] = {0};
	va_list args;
	va_start(args, fmt);
	vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);
	if (g_log_callback) {
		g_log_callback(level, category, message, g_log_user_data);
	} else {
		se_debug_log_default(level, category, message, NULL);
	}
}

b8 se_debug_validate(const b8 condition, const se_result error_code, const c8* expression, const c8* file, const i32 line) {
	if (condition) {
		return true;
	}
	se_set_last_error(error_code);
	se_debug_log(SE_DEBUG_LEVEL_ERROR, SE_DEBUG_CATEGORY_CORE, "Validation failed: `%s` at %s:%d -> %s", expression, file, line, se_result_str(error_code));
	return false;
}

void se_debug_trace_begin(const c8* name) {
	if (!name) {
		return;
	}
	se_debug_trace_events_init();
	se_debug_trace_event event = {0};
	strncpy(event.name, name, sizeof(event.name) - 1);
	event.begin_time = se_debug_now_seconds();
	event.active = true;
	s_array_add(&g_trace_events, event);
}

void se_debug_trace_end(const c8* name) {
	if (!name || s_array_get_size(&g_trace_events) == 0) {
		return;
	}
	const f64 now = se_debug_now_seconds();
	for (sz i = s_array_get_size(&g_trace_events); i > 0; --i) {
		se_debug_trace_event* event = s_array_get(&g_trace_events, s_array_handle(&g_trace_events, (u32)(i - 1)));
		if (!event || !event->active || strcmp(event->name, name) != 0) {
			continue;
		}
		event->end_time = now;
		event->duration = s_max(0.0, event->end_time - event->begin_time);
		event->active = false;
		se_debug_frame_timing_add_duration(name, event->duration);
		return;
	}
}

b8 se_debug_get_trace_events(const se_debug_trace_event** out_events, sz* out_count) {
	if (!out_events || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_debug_trace_events_init();
	*out_events = s_array_get_data(&g_trace_events);
	*out_count = s_array_get_size(&g_trace_events);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_debug_clear_trace_events(void) {
	se_debug_trace_events_init();
	while (s_array_get_size(&g_trace_events) > 0) {
		s_array_remove(&g_trace_events, s_array_handle(&g_trace_events, (u32)(s_array_get_size(&g_trace_events) - 1)));
	}
}

void se_debug_frame_begin(void) {
	const f64 now = se_debug_now_seconds();
	if (g_frame_active) {
		se_debug_frame_timing_finalize(now);
	}
	se_debug_frame_timing_reset(&g_frame_timing_current);
	g_frame_active = true;
	g_frame_begin_time = now;
}

void se_debug_frame_end(void) {
	const f64 now = se_debug_now_seconds();
	se_debug_frame_timing_finalize(now);
}

b8 se_debug_get_last_frame_timing(se_debug_frame_timing* out_timing) {
	if (!out_timing) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_timing = g_frame_timing_last;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_debug_dump_last_frame_timing(c8* out_buffer, const sz out_buffer_size) {
	if (!out_buffer || out_buffer_size == 0) {
		return;
	}
	const se_debug_frame_timing* t = &g_frame_timing_last;
	snprintf(
		out_buffer,
		out_buffer_size,
		"frame=%llu ms=%.3f win_upd=%.3f input=%.3f scene2d=%.3f scene3d=%.3f text=%.3f ui=%.3f nav=%.3f present=%.3f other=%.3f",
		(unsigned long long)t->frame_index,
		t->frame_ms,
		t->window_update_ms,
		t->input_ms,
		t->scene2d_ms,
		t->scene3d_ms,
		t->text_ms,
		t->ui_ms,
		t->navigation_ms,
		t->window_present_ms,
		t->other_ms);
}

void se_debug_dump_last_frame_timing_lines(c8* out_buffer, const sz out_buffer_size) {
	if (!out_buffer || out_buffer_size == 0) {
		return;
	}
	const se_debug_frame_timing* t = &g_frame_timing_last;
	snprintf(
		out_buffer,
		out_buffer_size,
		"frame=%llu total=%.3fms\n"
		"window_update=%.3fms input=%.3fms\n"
		"scene2d=%.3fms scene3d=%.3fms\n"
		"text=%.3fms ui=%.3fms\n"
		"navigation=%.3fms present=%.3fms\n"
		"other=%.3fms",
		(unsigned long long)t->frame_index,
		t->frame_ms,
		t->window_update_ms,
		t->input_ms,
		t->scene2d_ms,
		t->scene3d_ms,
		t->text_ms,
		t->ui_ms,
		t->navigation_ms,
		t->window_present_ms,
		t->other_ms);
}

b8 se_debug_collect_stats(const se_window_handle window, se_input_handle* input, se_debug_system_stats* out_stats) {
	if (!out_stats) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	memset(out_stats, 0, sizeof(*out_stats));
	se_context* ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (window != S_HANDLE_NULL) {
		se_window_get_diagnostics(window, &out_stats->window);
	}
	if (input) {
		se_input_get_diagnostics(input, &out_stats->input);
	}
	out_stats->window_count = (u32)s_array_get_size(&ctx->windows);
	out_stats->camera_count = (u32)s_array_get_size(&ctx->cameras);
	out_stats->scene2d_count = (u32)s_array_get_size(&ctx->scenes_2d);
	out_stats->scene3d_count = (u32)s_array_get_size(&ctx->scenes_3d);
	out_stats->object2d_count = (u32)s_array_get_size(&ctx->objects_2d);
	out_stats->object3d_count = (u32)s_array_get_size(&ctx->objects_3d);
	out_stats->ui_element_count = (u32)s_array_get_size(&ctx->ui_elements);
	out_stats->navigation_trace_count = (u32)s_array_get_size(&g_trace_events);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_debug_set_overlay_enabled(const b8 enabled) {
	g_overlay_enabled = enabled;
}

b8 se_debug_is_overlay_enabled(void) {
	return g_overlay_enabled;
}

void se_debug_render_overlay(const se_window_handle window, se_input_handle* input) {
	if (!g_overlay_enabled || window == S_HANDLE_NULL) {
		return;
	}
	se_context* ctx = se_current_context();
	if (!ctx) {
		return;
	}

	if (g_overlay_context != ctx) {
		if (g_overlay_text_handle) {
			se_text_handle_destroy(g_overlay_text_handle);
		}
		g_overlay_text_handle = se_text_handle_create(0);
		g_overlay_context = ctx;
	}
	if (!g_overlay_text_handle) {
		return;
	}

	se_debug_system_stats stats = {0};
	if (!se_debug_collect_stats(window, input, &stats)) {
		return;
	}

	se_font_handle font = se_font_load(g_overlay_text_handle, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 16.0f);
	if (font == S_HANDLE_NULL) {
		return;
	}

	c8 timing_text[512] = {0};
	se_debug_dump_last_frame_timing_lines(timing_text, sizeof(timing_text));

	c8 text[1536] = {0};
	snprintf(
		text,
		sizeof(text),
		"dbg win:%u cam:%u sc2:%u sc3:%u obj2:%u obj3:%u ui:%u fps:%u traces:%u\n%s",
		stats.window_count,
		stats.camera_count,
		stats.scene2d_count,
		stats.scene3d_count,
		stats.object2d_count,
		stats.object3d_count,
		stats.ui_element_count,
		(u32)(stats.window.last_present_duration > 0.0 ? (1.0 / stats.window.last_present_duration) : 0.0),
		stats.navigation_trace_count,
		timing_text);
	se_text_render(g_overlay_text_handle, font, text, &s_vec2(-0.98f, 0.96f), &s_vec2(1.0f, 1.0f), 0.017f);
}
