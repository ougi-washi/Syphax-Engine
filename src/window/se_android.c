// Syphax-Engine - Ougi Washi

#if defined(SE_WINDOW_BACKEND_ANDROID)

#include "se_window.h"
#include "se_debug.h"
#include "se_graphics.h"
#include "render/se_gl.h"
#include "window/se_android_internal.h"
#include "window/se_window_backend_internal.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <android/configuration.h>
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "syphax/s_files.h"

#define SE_MAX_INPUT_EVENTS 1024
#define SE_MAX_RESIZE_HANDLE 1024

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x00000040
#endif

typedef struct {
	se_context* ctx;
	se_window_handle window;
} se_window_registry_entry;

typedef s_array(se_window_registry_entry, se_windows_registry);

typedef struct {
	struct android_app* app;
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
	EGLSurface fallback_surface;
	ANativeWindow* native_window;
	u32 surface_width;
	u32 surface_height;
	f32 content_scale_x;
	f32 content_scale_y;
	b8 display_initialized;
	b8 context_initialized;
	b8 surface_initialized;
	b8 fallback_surface_initialized;
	b8 resumed;
	b8 has_focus;
	b8 quit_requested;
	i32 primary_pointer_id;
} se_android_backend_state;

static se_windows_registry windows_registry = {0};
static se_context* windows_owner_context = NULL;
static se_android_backend_state g_android = {
	.display = EGL_NO_DISPLAY,
	.context = EGL_NO_CONTEXT,
	.surface = EGL_NO_SURFACE,
	.fallback_surface = EGL_NO_SURFACE,
	.primary_pointer_id = -1
};

static se_window* se_window_from_handle(se_context* context, const se_window_handle window) {
	if (!context) {
		return NULL;
	}
	return s_array_get(&context->windows, window);
}

static f64 se_android_now_seconds(void) {
	struct timespec ts = {0};
#if defined(CLOCK_MONOTONIC)
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1000000000.0;
	}
#endif
	timespec_get(&ts, TIME_UTC);
	return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1000000000.0;
}

static void se_android_sleep_seconds(const f64 seconds) {
	if (seconds <= 0.0) {
		return;
	}

	f64 remaining = seconds;
	while (remaining > 0.0) {
		struct timespec request = {0};
		struct timespec leftover = {0};
		request.tv_sec = (time_t)remaining;
		request.tv_nsec = (long)((remaining - (f64)request.tv_sec) * 1000000000.0);
		if (request.tv_nsec >= 1000000000L) {
			request.tv_sec += 1;
			request.tv_nsec -= 1000000000L;
		}
		if (request.tv_sec == 0 && request.tv_nsec == 0) {
			break;
		}
		if (nanosleep(&request, &leftover) == 0) {
			break;
		}
		remaining = (f64)leftover.tv_sec + (f64)leftover.tv_nsec / 1000000000.0;
	}
}

static void se_android_log(const i32 priority, const c8* message) {
	if (!message) {
		return;
	}
	__android_log_print(priority, "syphax-engine", "%s", message);
}

static void se_window_registry_refresh_owner_context(void) {
	if (s_array_get_size(&windows_registry) == 0) {
		windows_owner_context = NULL;
		return;
	}
	if (windows_owner_context != NULL) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (entry && entry->ctx) {
			windows_owner_context = entry->ctx;
			return;
		}
	}
}

static se_window_registry_entry* se_window_registry_find(const se_context* preferred_ctx, const se_window_handle window, sz* out_index) {
	if (out_index) {
		*out_index = 0;
	}
	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (!entry || entry->window != window || entry->ctx != preferred_ctx) {
			continue;
		}
		if (out_index) {
			*out_index = i;
		}
		return entry;
	}
	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (!entry || entry->window != window) {
			continue;
		}
		if (out_index) {
			*out_index = i;
		}
		return entry;
	}
	return NULL;
}

static b8 se_window_context_has_live_resources(const se_context* context) {
	if (context == NULL) {
		return false;
	}
	return
		context->ui_text_handle != NULL ||
		s_array_get_size(&context->fonts) > 0 ||
		s_array_get_size(&context->models) > 0 ||
		s_array_get_size(&context->cameras) > 0 ||
		s_array_get_size(&context->framebuffers) > 0 ||
		s_array_get_size(&context->render_buffers) > 0 ||
		s_array_get_size(&context->textures) > 0 ||
		s_array_get_size(&context->shaders) > 0 ||
		s_array_get_size(&context->objects_2d) > 0 ||
		s_array_get_size(&context->scenes_2d) > 0 ||
		s_array_get_size(&context->objects_3d) > 0 ||
		s_array_get_size(&context->scenes_3d) > 0 ||
		s_array_get_size(&context->vfx_2ds) > 0 ||
		s_array_get_size(&context->vfx_3ds) > 0 ||
		s_array_get_size(&context->ui_roots) > 0 ||
		s_array_get_size(&context->ui_elements) > 0 ||
		s_array_get_size(&context->ui_texts) > 0;
}

static void se_android_backend_update_content_scale(void) {
	g_android.content_scale_x = 1.0f;
	g_android.content_scale_y = 1.0f;
	if (!g_android.app || !g_android.app->config) {
		return;
	}

	const i32 density = AConfiguration_getDensity(g_android.app->config);
	if (density <= 0 ||
		density == ACONFIGURATION_DENSITY_ANY ||
		density == ACONFIGURATION_DENSITY_NONE) {
		return;
	}

	const f32 scale = (f32)density / 160.0f;
	g_android.content_scale_x = scale;
	g_android.content_scale_y = scale;
}

static void se_android_backend_emit_resize(const se_window_handle window, se_window* window_ptr) {
	if (!window_ptr) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&window_ptr->resize_handles); ++i) {
		se_resize_handle* current = s_array_get(&window_ptr->resize_handles, s_array_handle(&window_ptr->resize_handles, (u32)i));
		if (current && current->callback) {
			current->callback(window, current->data);
		}
	}
}

static void se_android_backend_update_window_metrics(const se_window_handle window, se_window* window_ptr) {
	if (!window_ptr || !g_android.native_window) {
		return;
	}

	const u32 next_width = (u32)s_max(ANativeWindow_getWidth(g_android.native_window), 1);
	const u32 next_height = (u32)s_max(ANativeWindow_getHeight(g_android.native_window), 1);
	const b8 changed = next_width != window_ptr->width || next_height != window_ptr->height;
	window_ptr->handle = g_android.native_window;
	window_ptr->width = next_width;
	window_ptr->height = next_height;
	g_android.surface_width = next_width;
	g_android.surface_height = next_height;
	if (changed) {
		se_android_backend_emit_resize(window, window_ptr);
	}
}

static void se_android_backend_sync_all_windows(void) {
	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (!entry || !entry->ctx || entry->window == S_HANDLE_NULL) {
			continue;
		}
		se_window* window_ptr = s_array_get(&entry->ctx->windows, entry->window);
		if (!window_ptr) {
			continue;
		}
		se_android_backend_update_window_metrics(entry->window, window_ptr);
	}
}

static b8 se_android_backend_make_current(const b8 allow_fallback_surface) {
	if (!g_android.display_initialized || !g_android.context_initialized) {
		return false;
	}

	EGLSurface draw_surface = EGL_NO_SURFACE;
	if (g_android.surface_initialized && g_android.surface != EGL_NO_SURFACE) {
		draw_surface = g_android.surface;
	} else if (allow_fallback_surface &&
		g_android.fallback_surface_initialized &&
		g_android.fallback_surface != EGL_NO_SURFACE) {
		draw_surface = g_android.fallback_surface;
	}

	if (draw_surface == EGL_NO_SURFACE) {
		return false;
	}

	return eglMakeCurrent(g_android.display, draw_surface, draw_surface, g_android.context) == EGL_TRUE;
}

static b8 se_android_backend_create_fallback_surface(void) {
	if (g_android.fallback_surface_initialized && g_android.fallback_surface != EGL_NO_SURFACE) {
		return true;
	}

	const EGLint pbuffer_attribs[] = {
		EGL_WIDTH, 1,
		EGL_HEIGHT, 1,
		EGL_NONE
	};

	g_android.fallback_surface = eglCreatePbufferSurface(g_android.display, g_android.config, pbuffer_attribs);
	if (g_android.fallback_surface == EGL_NO_SURFACE) {
		se_android_log(ANDROID_LOG_ERROR, "se_android_backend_create_fallback_surface :: failed to create pbuffer surface");
		return false;
	}
	g_android.fallback_surface_initialized = true;
	return true;
}

static b8 se_android_backend_init_display(void) {
	if (g_android.display_initialized && g_android.context_initialized) {
		return true;
	}

	g_android.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (g_android.display == EGL_NO_DISPLAY) {
		se_android_log(ANDROID_LOG_ERROR, "se_android_backend_init_display :: eglGetDisplay failed");
		return false;
	}

	if (eglInitialize(g_android.display, NULL, NULL) != EGL_TRUE) {
		se_android_log(ANDROID_LOG_ERROR, "se_android_backend_init_display :: eglInitialize failed");
		return false;
	}

	const EGLint config_attribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};

	EGLint config_count = 0;
	if (eglChooseConfig(g_android.display, config_attribs, &g_android.config, 1, &config_count) != EGL_TRUE || config_count <= 0) {
		se_android_log(ANDROID_LOG_ERROR, "se_android_backend_init_display :: eglChooseConfig failed");
		eglTerminate(g_android.display);
		g_android.display = EGL_NO_DISPLAY;
		return false;
	}

	const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
	g_android.context = eglCreateContext(g_android.display, g_android.config, EGL_NO_CONTEXT, context_attribs);
	if (g_android.context == EGL_NO_CONTEXT) {
		se_android_log(ANDROID_LOG_ERROR, "se_android_backend_init_display :: eglCreateContext failed");
		eglTerminate(g_android.display);
		g_android.display = EGL_NO_DISPLAY;
		return false;
	}

	g_android.display_initialized = true;
	g_android.context_initialized = true;
	if (!se_android_backend_create_fallback_surface()) {
		eglDestroyContext(g_android.display, g_android.context);
		eglTerminate(g_android.display);
		g_android.context = EGL_NO_CONTEXT;
		g_android.display = EGL_NO_DISPLAY;
		g_android.display_initialized = false;
		g_android.context_initialized = false;
		return false;
	}
	if (!se_android_backend_make_current(true)) {
		se_android_log(ANDROID_LOG_ERROR, "se_android_backend_init_display :: failed to make fallback surface current");
		return false;
	}
	return true;
}

static void se_android_backend_destroy_surface(void) {
	if (!g_android.display_initialized || !g_android.surface_initialized || g_android.surface == EGL_NO_SURFACE) {
		g_android.native_window = NULL;
		return;
	}

	if (g_android.context_initialized && g_android.context != EGL_NO_CONTEXT) {
		if (g_android.fallback_surface_initialized && g_android.fallback_surface != EGL_NO_SURFACE) {
			(void)eglMakeCurrent(g_android.display, g_android.fallback_surface, g_android.fallback_surface, g_android.context);
		} else {
			(void)eglMakeCurrent(g_android.display, EGL_NO_SURFACE, EGL_NO_SURFACE, g_android.context);
		}
	}
	eglDestroySurface(g_android.display, g_android.surface);
	g_android.surface = EGL_NO_SURFACE;
	g_android.surface_initialized = false;
	g_android.native_window = NULL;
	g_android.surface_width = 0;
	g_android.surface_height = 0;
	g_android.primary_pointer_id = -1;
}

static void se_android_backend_shutdown_display(void) {
	se_android_backend_destroy_surface();
	if (g_android.fallback_surface_initialized && g_android.fallback_surface != EGL_NO_SURFACE && g_android.display_initialized) {
		eglDestroySurface(g_android.display, g_android.fallback_surface);
	}
	g_android.fallback_surface = EGL_NO_SURFACE;
	g_android.fallback_surface_initialized = false;

	if (g_android.context_initialized && g_android.context != EGL_NO_CONTEXT && g_android.display_initialized) {
		eglDestroyContext(g_android.display, g_android.context);
	}
	g_android.context = EGL_NO_CONTEXT;
	g_android.context_initialized = false;

	if (g_android.display_initialized && g_android.display != EGL_NO_DISPLAY) {
		eglTerminate(g_android.display);
	}
	g_android.display = EGL_NO_DISPLAY;
	g_android.display_initialized = false;
	g_android.config = NULL;
}

static b8 se_android_backend_create_surface(void) {
	if (!g_android.app || !g_android.app->window) {
		return false;
	}
	if (!se_android_backend_init_display()) {
		return false;
	}
	if (g_android.surface_initialized &&
		g_android.surface != EGL_NO_SURFACE &&
		g_android.native_window == g_android.app->window) {
		return se_android_backend_make_current(false);
	}

	se_android_backend_destroy_surface();

	EGLint window_format = 0;
	(void)eglGetConfigAttrib(g_android.display, g_android.config, EGL_NATIVE_VISUAL_ID, &window_format);
	ANativeWindow_setBuffersGeometry(g_android.app->window, 0, 0, window_format);

	g_android.surface = eglCreateWindowSurface(g_android.display, g_android.config, g_android.app->window, NULL);
	if (g_android.surface == EGL_NO_SURFACE) {
		se_android_log(ANDROID_LOG_ERROR, "se_android_backend_create_surface :: eglCreateWindowSurface failed");
		return false;
	}

	g_android.surface_initialized = true;
	g_android.native_window = g_android.app->window;
	if (!se_android_backend_make_current(false)) {
		se_android_log(ANDROID_LOG_ERROR, "se_android_backend_create_surface :: eglMakeCurrent failed");
		se_android_backend_destroy_surface();
		return false;
	}
	se_android_backend_update_content_scale();
	se_android_backend_sync_all_windows();
	return true;
}

static void se_android_backend_process_events(i32 timeout_ms) {
	if (!g_android.app) {
		return;
	}

	for (;;) {
		i32 events = 0;
		struct android_poll_source* source = NULL;
		const i32 ident = ALooper_pollOnce(timeout_ms, NULL, &events, (void**)&source);
		if (ident < 0) {
			break;
		}
		if (source) {
			source->process(g_android.app, source);
		}
		if (g_android.app->destroyRequested != 0) {
			g_android.quit_requested = true;
		}
		timeout_ms = 0;
	}
}

static b8 se_android_backend_wait_for_surface(void) {
	while (!g_android.quit_requested) {
		if (g_android.app && g_android.app->window && se_android_backend_create_surface()) {
			return true;
		}
		se_android_backend_process_events(-1);
	}
	return false;
}

static b8 se_android_backend_find_primary_window(se_window_handle* out_handle, se_context** out_context, se_window** out_window) {
	if (out_handle) {
		*out_handle = S_HANDLE_NULL;
	}
	if (out_context) {
		*out_context = NULL;
	}
	if (out_window) {
		*out_window = NULL;
	}

	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (!entry || !entry->ctx || entry->window == S_HANDLE_NULL) {
			continue;
		}
		se_window* window_ptr = s_array_get(&entry->ctx->windows, entry->window);
		if (!window_ptr) {
			continue;
		}
		if (out_handle) {
			*out_handle = entry->window;
		}
		if (out_context) {
			*out_context = entry->ctx;
		}
		if (out_window) {
			*out_window = window_ptr;
		}
		return true;
	}
	return false;
}

static void se_android_backend_update_hover_state(const se_window_handle window, se_window* window_ptr) {
	if (!window_ptr) {
		return;
	}

	s_vec2 mouse_position = {0};
	se_window_get_mouse_normalized(window, &mouse_position);
	se_input_event* out_event = NULL;
	i32 out_depth = INT_MIN;
	b8 interacted = false;

	for (sz j = 0; j < s_array_get_size(&window_ptr->input_events); ++j) {
		se_input_event* current_event = s_array_get(&window_ptr->input_events, s_array_handle(&window_ptr->input_events, (u32)j));
		if (current_event->active &&
			mouse_position.x >= current_event->box.min.x && mouse_position.x <= current_event->box.max.x &&
			mouse_position.y >= current_event->box.min.y && mouse_position.y <= current_event->box.max.y) {
			if (current_event->depth > out_depth) {
				out_event = current_event;
				out_depth = current_event->depth;
				interacted = true;
			}
		}
	}

	if (interacted) {
		if (out_event->on_interact_callback) {
			out_event->on_interact_callback(window, out_event->callback_data);
		}
		out_event->interacted = true;
		return;
	}

	for (sz j = 0; j < s_array_get_size(&window_ptr->input_events); ++j) {
		se_input_event* current_event = s_array_get(&window_ptr->input_events, s_array_handle(&window_ptr->input_events, (u32)j));
		if (!current_event->interacted) {
			continue;
		}
		if (current_event->on_stop_interact_callback) {
			current_event->on_stop_interact_callback(window, current_event->callback_data);
		}
		current_event->interacted = false;
	}
}

static i32 se_hash(const c8* str1, const i32 len1, const c8* str2, const i32 len2) {
	u32 hash = 5381;
	for (i32 i = 0; i < len1; ++i) {
		hash = ((hash << 5) + hash) + (u32)str1[i];
	}
	for (i32 i = 0; i < len2; ++i) {
		hash = ((hash << 5) + hash) + (u32)str2[i];
	}
	return (i32)hash;
}

static se_key se_android_map_keycode(const i32 keycode) {
	if (keycode >= AKEYCODE_A && keycode <= AKEYCODE_Z) {
		return (se_key)(SE_KEY_A + (keycode - AKEYCODE_A));
	}
	if (keycode >= AKEYCODE_0 && keycode <= AKEYCODE_9) {
		return (se_key)(SE_KEY_0 + (keycode - AKEYCODE_0));
	}

	switch (keycode) {
		case AKEYCODE_SPACE: return SE_KEY_SPACE;
		case AKEYCODE_APOSTROPHE: return SE_KEY_APOSTROPHE;
		case AKEYCODE_COMMA: return SE_KEY_COMMA;
		case AKEYCODE_MINUS: return SE_KEY_MINUS;
		case AKEYCODE_PERIOD: return SE_KEY_PERIOD;
		case AKEYCODE_SLASH: return SE_KEY_SLASH;
		case AKEYCODE_SEMICOLON: return SE_KEY_SEMICOLON;
		case AKEYCODE_EQUALS: return SE_KEY_EQUAL;
		case AKEYCODE_LEFT_BRACKET: return SE_KEY_LEFT_BRACKET;
		case AKEYCODE_BACKSLASH: return SE_KEY_BACKSLASH;
		case AKEYCODE_RIGHT_BRACKET: return SE_KEY_RIGHT_BRACKET;
		case AKEYCODE_GRAVE: return SE_KEY_GRAVE_ACCENT;
		case AKEYCODE_ESCAPE: return SE_KEY_ESCAPE;
		case AKEYCODE_BACK: return SE_KEY_ESCAPE;
		case AKEYCODE_ENTER: return SE_KEY_ENTER;
		case AKEYCODE_TAB: return SE_KEY_TAB;
		case AKEYCODE_DEL: return SE_KEY_BACKSPACE;
		case AKEYCODE_FORWARD_DEL: return SE_KEY_DELETE;
		case AKEYCODE_DPAD_RIGHT: return SE_KEY_RIGHT;
		case AKEYCODE_DPAD_LEFT: return SE_KEY_LEFT;
		case AKEYCODE_DPAD_DOWN: return SE_KEY_DOWN;
		case AKEYCODE_DPAD_UP: return SE_KEY_UP;
		case AKEYCODE_MOVE_HOME: return SE_KEY_HOME;
		case AKEYCODE_MOVE_END: return SE_KEY_END;
		case AKEYCODE_PAGE_UP: return SE_KEY_PAGE_UP;
		case AKEYCODE_PAGE_DOWN: return SE_KEY_PAGE_DOWN;
		case AKEYCODE_F1: return SE_KEY_F1;
		case AKEYCODE_F2: return SE_KEY_F2;
		case AKEYCODE_F3: return SE_KEY_F3;
		case AKEYCODE_F4: return SE_KEY_F4;
		case AKEYCODE_F5: return SE_KEY_F5;
		case AKEYCODE_F6: return SE_KEY_F6;
		case AKEYCODE_F7: return SE_KEY_F7;
		case AKEYCODE_F8: return SE_KEY_F8;
		case AKEYCODE_F9: return SE_KEY_F9;
		case AKEYCODE_F10: return SE_KEY_F10;
		case AKEYCODE_F11: return SE_KEY_F11;
		case AKEYCODE_F12: return SE_KEY_F12;
		case AKEYCODE_SHIFT_LEFT: return SE_KEY_LEFT_SHIFT;
		case AKEYCODE_SHIFT_RIGHT: return SE_KEY_RIGHT_SHIFT;
		case AKEYCODE_CTRL_LEFT: return SE_KEY_LEFT_CONTROL;
		case AKEYCODE_CTRL_RIGHT: return SE_KEY_RIGHT_CONTROL;
		case AKEYCODE_ALT_LEFT: return SE_KEY_LEFT_ALT;
		case AKEYCODE_ALT_RIGHT: return SE_KEY_RIGHT_ALT;
		default: return SE_KEY_UNKNOWN;
	}
}

static void se_android_update_mouse_position(se_window* window_ptr, const f64 x, const f64 y) {
	if (!window_ptr) {
		return;
	}

	const f64 max_x = (f64)s_max((u32)1, window_ptr->width) - 1.0;
	const f64 max_y = (f64)s_max((u32)1, window_ptr->height) - 1.0;
	const f64 next_x = s_max(0.0, s_min(x, max_x));
	const f64 next_y = s_max(0.0, s_min(y, max_y));
	window_ptr->mouse_dx = next_x - window_ptr->mouse_x;
	window_ptr->mouse_dy = next_y - window_ptr->mouse_y;
	window_ptr->mouse_x = next_x;
	window_ptr->mouse_y = next_y;
	window_ptr->diagnostics.mouse_move_events++;
}

static void se_android_handle_motion_event(se_window* window_ptr, AInputEvent* event) {
	if (!window_ptr) {
		return;
	}

	const i32 action = AMotionEvent_getAction(event);
	const i32 action_masked = action & AMOTION_EVENT_ACTION_MASK;
	const i32 pointer_index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
	const i32 pointer_id = AMotionEvent_getPointerId(event, pointer_index);

	switch (action_masked) {
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN: {
			if (g_android.primary_pointer_id < 0) {
				g_android.primary_pointer_id = pointer_id;
			}
			if (pointer_id == g_android.primary_pointer_id) {
				const f64 x = AMotionEvent_getX(event, pointer_index);
				const f64 y = AMotionEvent_getY(event, pointer_index);
				se_android_update_mouse_position(window_ptr, x, y);
				window_ptr->mouse_buttons[SE_MOUSE_LEFT] = true;
				window_ptr->diagnostics.mouse_button_events++;
			}
			break;
		}
		case AMOTION_EVENT_ACTION_MOVE: {
			if (g_android.primary_pointer_id < 0) {
				break;
			}
			const size_t pointer_count = AMotionEvent_getPointerCount(event);
			for (size_t i = 0; i < pointer_count; ++i) {
				if (AMotionEvent_getPointerId(event, i) != g_android.primary_pointer_id) {
					continue;
				}
				const f64 x = AMotionEvent_getX(event, i);
				const f64 y = AMotionEvent_getY(event, i);
				se_android_update_mouse_position(window_ptr, x, y);
				break;
			}
			break;
		}
		case AMOTION_EVENT_ACTION_UP:
		case AMOTION_EVENT_ACTION_POINTER_UP:
		case AMOTION_EVENT_ACTION_CANCEL: {
			if (pointer_id == g_android.primary_pointer_id) {
				const f64 x = AMotionEvent_getX(event, pointer_index);
				const f64 y = AMotionEvent_getY(event, pointer_index);
				se_android_update_mouse_position(window_ptr, x, y);
				window_ptr->mouse_buttons[SE_MOUSE_LEFT] = false;
				window_ptr->diagnostics.mouse_button_events++;
				g_android.primary_pointer_id = -1;
			}
			break;
		}
		default:
			break;
	}
}

static int32_t se_android_handle_input(struct android_app* app, AInputEvent* event) {
	(void)app;

	se_window_handle window = S_HANDLE_NULL;
	se_context* context = NULL;
	se_window* window_ptr = NULL;
	if (!se_android_backend_find_primary_window(&window, &context, &window_ptr) || !window_ptr) {
		return 0;
	}

	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		se_android_handle_motion_event(window_ptr, event);
		return 1;
	}

	if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_KEY) {
		return 0;
	}

	(void)context;
	const i32 keycode = AKeyEvent_getKeyCode(event);
	const i32 action = AKeyEvent_getAction(event);
	const se_key mapped = se_android_map_keycode(keycode);
	if (mapped != SE_KEY_UNKNOWN) {
		if (action == AKEY_EVENT_ACTION_DOWN) {
			window_ptr->keys[mapped] = true;
			window_ptr->diagnostics.key_events++;
		} else if (action == AKEY_EVENT_ACTION_UP) {
			window_ptr->keys[mapped] = false;
			window_ptr->diagnostics.key_events++;
		}
	}
	if (keycode == AKEYCODE_BACK && action == AKEY_EVENT_ACTION_UP) {
		window_ptr->should_close = true;
	}
	return mapped != SE_KEY_UNKNOWN || keycode == AKEYCODE_BACK;
}

static void se_android_handle_app_cmd(struct android_app* app, i32 cmd) {
	(void)app;

	switch (cmd) {
		case APP_CMD_RESUME:
			g_android.resumed = true;
			break;
		case APP_CMD_PAUSE:
			g_android.resumed = false;
			break;
		case APP_CMD_GAINED_FOCUS:
			g_android.has_focus = true;
			break;
		case APP_CMD_LOST_FOCUS:
			g_android.has_focus = false;
			break;
		case APP_CMD_INIT_WINDOW:
			if (g_android.app && g_android.app->window) {
				(void)se_android_backend_create_surface();
			}
			break;
		case APP_CMD_TERM_WINDOW:
			se_android_backend_destroy_surface();
			break;
		case APP_CMD_WINDOW_RESIZED:
		case APP_CMD_CONTENT_RECT_CHANGED:
		case APP_CMD_CONFIG_CHANGED:
			se_android_backend_update_content_scale();
			se_android_backend_sync_all_windows();
			break;
		case APP_CMD_DESTROY:
			g_android.quit_requested = true;
			break;
		default:
			break;
	}

	if (g_android.app && g_android.app->destroyRequested != 0) {
		g_android.quit_requested = true;
	}
}

void se_android_set_app(struct android_app* app) {
	memset(&g_android, 0, sizeof(g_android));
	g_android.display = EGL_NO_DISPLAY;
	g_android.context = EGL_NO_CONTEXT;
	g_android.surface = EGL_NO_SURFACE;
	g_android.fallback_surface = EGL_NO_SURFACE;
	g_android.primary_pointer_id = -1;
	g_android.content_scale_x = 1.0f;
	g_android.content_scale_y = 1.0f;
	g_android.app = app;
	if (!app) {
		return;
	}
	app->userData = &g_android;
	app->onAppCmd = se_android_handle_app_cmd;
	app->onInputEvent = se_android_handle_input;
	se_android_backend_update_content_scale();
}

void se_android_clear_app(void) {
	if (g_android.app) {
		g_android.app->userData = NULL;
		g_android.app->onAppCmd = NULL;
		g_android.app->onInputEvent = NULL;
	}
	g_android.app = NULL;
}

static se_result se_window_init_render(se_window* window, se_context* context) {
	s_assertf(window, "se_window_init_render :: window is null");
	if (!context) {
		return SE_RESULT_INVALID_ARGUMENT;
	}
	if (window->context == context && window->shader != S_HANDLE_NULL) {
		return SE_RESULT_OK;
	}
	window->context = context;
	if (window->quad.vao == 0) {
		se_quad_2d_create(&window->quad, 0);
	}
	if (window->shader == S_HANDLE_NULL) {
		se_shader_handle shader = se_shader_load(SE_RESOURCE_INTERNAL("shaders/render_quad_vert.glsl"), SE_RESOURCE_INTERNAL("shaders/render_quad_frag.glsl"));
		if (shader == S_HANDLE_NULL) {
			return se_get_last_error();
		}
		window->shader = shader;
	}
	return SE_RESULT_OK;
}

static void se_window_backend_shutdown_if_unused(void) {
	if (s_array_get_size(&windows_registry) != 0) {
		return;
	}
	s_array_clear(&windows_registry);
	windows_owner_context = NULL;
	se_render_shutdown();
	se_android_backend_shutdown_display();
}

static void se_window_render_screen_internal(const se_window_handle window, se_window* window_ptr) {
	if (!window_ptr) {
		return;
	}

	se_debug_trace_begin("window_present");
	const f64 present_begin = se_android_now_seconds();
	window_ptr->diagnostics.last_sleep_duration = 0.0;
	if (!window_ptr->vsync_enabled && window_ptr->target_fps > 0u) {
		const f64 target_frame_time = 1.0 / (f64)window_ptr->target_fps;
		const f64 now = se_android_now_seconds();
		const f64 elapsed = now - window_ptr->time.frame_start;
		const f64 time_left = target_frame_time - elapsed;
		if (time_left > 0.0) {
			se_debug_trace_begin("window_present_sleep");
			const f64 sleep_begin = se_android_now_seconds();
			se_android_sleep_seconds(time_left);
			const f64 sleep_end = se_android_now_seconds();
			window_ptr->diagnostics.last_sleep_duration = s_max(0.0, sleep_end - sleep_begin);
			se_debug_trace_end("window_present_sleep");
		}
	}
	se_debug_trace_begin("window_present_overlay");
	se_debug_render_overlay(window, NULL);
	se_debug_trace_end("window_present_overlay");
	se_debug_trace_begin("window_present_swap");
	se_debug_trace_begin_channel("window_present_gpu", SE_DEBUG_TRACE_CHANNEL_GPU);
	if (g_android.surface_initialized && g_android.surface != EGL_NO_SURFACE) {
		if (eglSwapBuffers(g_android.display, g_android.surface) != EGL_TRUE) {
			const EGLint egl_error = eglGetError();
			if (egl_error == EGL_BAD_SURFACE || egl_error == EGL_BAD_NATIVE_WINDOW) {
				se_android_backend_destroy_surface();
			}
		}
	}
	se_debug_trace_end_channel("window_present_gpu", SE_DEBUG_TRACE_CHANNEL_GPU);
	se_debug_trace_end("window_present_swap");
	window_ptr->diagnostics.frames_presented++;
	window_ptr->diagnostics.last_present_duration = s_max(0.0, se_android_now_seconds() - present_begin);
	se_debug_trace_end("window_present");
	se_debug_frame_end();
}

se_window_handle se_window_create(const char* title, const u32 width, const u32 height) {
	se_context* context = se_current_context();
	if (!context || !title) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (!g_android.app) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return S_HANDLE_NULL;
	}

	se_window_registry_refresh_owner_context();
	if (windows_owner_context != NULL && windows_owner_context != context) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return S_HANDLE_NULL;
	}
	if (!se_android_backend_wait_for_surface()) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return S_HANDLE_NULL;
	}

	if (s_array_get_capacity(&context->windows) == 0) {
		s_array_init(&context->windows);
	}
	if (s_array_get_capacity(&windows_registry) == 0) {
		s_array_init(&windows_registry);
	}

	se_window_handle window_handle = s_array_increment(&context->windows);
	se_window* new_window = s_array_get(&context->windows, window_handle);
	memset(new_window, 0, sizeof(se_window));
	s_array_init(&new_window->input_events);
	s_array_init(&new_window->resize_handles);

	new_window->context = context;
	new_window->handle = g_android.native_window;
	new_window->width = g_android.surface_width > 0 ? g_android.surface_width : s_max(width, 1u);
	new_window->height = g_android.surface_height > 0 ? g_android.surface_height : s_max(height, 1u);
	new_window->cursor_mode = SE_WINDOW_CURSOR_NORMAL;
	new_window->raw_mouse_motion_supported = false;
	new_window->raw_mouse_motion_enabled = false;
	new_window->vsync_enabled = false;
	new_window->should_close = false;
	new_window->target_fps = 30;

	const f64 now = se_android_now_seconds();
	new_window->time.current = now;
	new_window->time.last_frame = now;
	new_window->time.delta = 0.0;
	new_window->time.frame_start = now;
	new_window->frame_count = 0;

	se_window_registry_entry entry = {
		.ctx = context,
		.window = window_handle
	};
	s_handle entry_handle = s_array_increment(&windows_registry);
	se_window_registry_entry* entry_slot = s_array_get(&windows_registry, entry_handle);
	*entry_slot = entry;
	windows_owner_context = context;

	if (!se_render_init()) {
		s_array_remove(&windows_registry, entry_handle);
		s_array_clear(&new_window->input_events);
		s_array_clear(&new_window->resize_handles);
		s_array_remove(&context->windows, window_handle);
		se_window_backend_shutdown_if_unused();
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return S_HANDLE_NULL;
	}

	glEnable(GL_DEPTH_TEST);
	const se_result render_result = se_window_init_render(new_window, context);
	if (render_result != SE_RESULT_OK) {
		if (new_window->quad.vao != 0) {
			se_quad_destroy(&new_window->quad);
		}
		s_array_remove(&windows_registry, entry_handle);
		s_array_clear(&new_window->input_events);
		s_array_clear(&new_window->resize_handles);
		s_array_remove(&context->windows, window_handle);
		se_window_backend_shutdown_if_unused();
		se_set_last_error(render_result);
		return S_HANDLE_NULL;
	}

	se_set_last_error(SE_RESULT_OK);
	return window_handle;
}

void se_window_attach_render(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	(void)se_window_set_current_context(window);
	(void)se_window_init_render(window_ptr, context);
}

void se_window_update(const se_window_handle window) {
	se_debug_frame_begin();
	se_debug_trace_begin("window_update");
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		se_debug_trace_end("window_update");
		return;
	}

	se_android_backend_update_window_metrics(window, window_ptr);
	se_window_check_exit_keys(window);
	memcpy(window_ptr->keys_prev, window_ptr->keys, sizeof(window_ptr->keys));
	memcpy(window_ptr->mouse_buttons_prev, window_ptr->mouse_buttons, sizeof(window_ptr->mouse_buttons));
	window_ptr->mouse_dx = 0.0;
	window_ptr->mouse_dy = 0.0;
	window_ptr->scroll_dx = 0.0;
	window_ptr->scroll_dy = 0.0;
	window_ptr->time.last_frame = window_ptr->time.current;
	window_ptr->time.current = se_android_now_seconds();
	window_ptr->time.delta = window_ptr->time.current - window_ptr->time.last_frame;
	if (window_ptr->time.delta <= 0.0 && window_ptr->target_fps > 0u) {
		window_ptr->time.delta = 1.0 / (f64)window_ptr->target_fps;
	}
	window_ptr->time.frame_start = window_ptr->time.current;
	window_ptr->frame_count++;
	se_debug_trace_end("window_update");
}

void se_window_tick(const se_window_handle window) {
	se_window_update(window);
	se_context* context = se_current_context();
	if (context) {
		se_window* window_ptr = se_window_from_handle(context, window);
		if (window_ptr) {
			se_uniforms* global_uniforms = se_context_get_global_uniforms();
			if (global_uniforms) {
				se_uniform_set_float(global_uniforms, "u_time", (f32)window_ptr->time.current);
			}
		}
	}
	se_debug_trace_begin("input_tick");
	se_window_poll_events();
	se_debug_trace_end("input_tick");
}

void se_window_set_current_context(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	if (!se_android_backend_wait_for_surface()) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return;
	}
	if (!se_android_backend_make_current(false)) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return;
	}
	se_android_backend_update_window_metrics(window, window_ptr);
	se_set_last_error(SE_RESULT_OK);
}

void se_window_render_quad(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr && window_ptr->shader != S_HANDLE_NULL, "se_window_render_quad :: shader is null");
	se_shader_use(window_ptr->shader, true, false);
	se_quad_render(&window_ptr->quad, 0);
}

void se_window_render_screen(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	se_window_set_current_context(window);
	if (se_get_last_error() != SE_RESULT_OK) {
		return;
	}
	se_window_render_screen_internal(window, window_ptr);
	se_set_last_error(SE_RESULT_OK);
}

void se_window_set_vsync(const se_window_handle window, const b8 enabled) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	window_ptr->vsync_enabled = enabled;
	if (g_android.display_initialized) {
		(void)eglSwapInterval(g_android.display, enabled ? 1 : 0);
	}
	se_set_last_error(SE_RESULT_OK);
}

b8 se_window_is_vsync_enabled(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? window_ptr->vsync_enabled : false;
}

void se_window_present(const se_window_handle window) {
	se_window_render_screen(window);
}

void se_window_present_frame(const se_window_handle window, const s_vec4* clear_color) {
	if (clear_color) {
		se_render_set_background(*clear_color);
	}
	se_render_clear();
	se_window_render_screen(window);
}

void se_window_begin_frame(const se_window_handle window) {
	se_context* context = se_current_context();
	if (!context) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}

	se_window_tick(window);
	se_window_set_current_context(window);
	if (se_get_last_error() != SE_RESULT_OK) {
		return;
	}

	u32 framebuffer_width = 0;
	u32 framebuffer_height = 0;
	se_window_get_framebuffer_size(window, &framebuffer_width, &framebuffer_height);
	if (framebuffer_width > 0 && framebuffer_height > 0) {
		glViewport(0, 0, (GLsizei)framebuffer_width, (GLsizei)framebuffer_height);
	}
	se_set_last_error(SE_RESULT_OK);
}

void se_window_end_frame(const se_window_handle window) {
	se_context* context = se_current_context();
	if (!context) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	se_window_present(window);
	se_set_last_error(SE_RESULT_OK);
}

void se_window_poll_events(void) {
	if (!g_android.app || s_array_get_size(&windows_registry) == 0) {
		return;
	}

	i32 timeout_ms = 0;
	if (!g_android.surface_initialized || !g_android.resumed) {
		timeout_ms = -1;
	}
	se_android_backend_process_events(timeout_ms);

	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (!entry || !entry->ctx || entry->window == S_HANDLE_NULL) {
			continue;
		}
		se_window* window_ptr = s_array_get(&entry->ctx->windows, entry->window);
		if (!window_ptr) {
			continue;
		}
		se_android_backend_update_hover_state(entry->window, window_ptr);
	}
}

b8 se_window_is_key_down(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window_ptr->keys[key];
}

b8 se_window_is_key_pressed(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window_ptr->keys[key] && !window_ptr->keys_prev[key];
}

b8 se_window_is_key_released(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return !window_ptr->keys[key] && window_ptr->keys_prev[key];
}

b8 se_window_is_mouse_down(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window_ptr->mouse_buttons[button];
}

b8 se_window_is_mouse_pressed(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window_ptr->mouse_buttons[button] && !window_ptr->mouse_buttons_prev[button];
}

b8 se_window_is_mouse_released(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return !window_ptr->mouse_buttons[button] && window_ptr->mouse_buttons_prev[button];
}

void se_window_clear_input_state(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	memset(window_ptr->keys, 0, sizeof(window_ptr->keys));
	memset(window_ptr->mouse_buttons, 0, sizeof(window_ptr->mouse_buttons));
	window_ptr->mouse_dx = 0.0;
	window_ptr->mouse_dy = 0.0;
	window_ptr->scroll_dx = 0.0;
	window_ptr->scroll_dy = 0.0;
}

void se_window_inject_key_state(const se_window_handle window, const se_key key, const b8 down) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return;
	}
	window_ptr->keys[key] = down;
}

void se_window_inject_mouse_button_state(const se_window_handle window, const se_mouse_button button, const b8 down) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return;
	}
	window_ptr->mouse_buttons[button] = down;
}

void se_window_inject_mouse_position(const se_window_handle window, const f32 x, const f32 y) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	se_android_update_mouse_position(window_ptr, x, y);
}

void se_window_inject_scroll_delta(const se_window_handle window, const f32 x, const f32 y) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->scroll_dx = x;
	window_ptr->scroll_dy = y;
}

f32 se_window_get_mouse_position_x(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? (f32)window_ptr->mouse_x : 0.0f;
}

f32 se_window_get_mouse_position_y(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? (f32)window_ptr->mouse_y : 0.0f;
}

void se_window_get_size(const se_window_handle window, u32* out_width, u32* out_height) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	if (out_width) {
		*out_width = window_ptr->width;
	}
	if (out_height) {
		*out_height = window_ptr->height;
	}
}

void se_window_get_framebuffer_size(const se_window_handle window, u32* out_width, u32* out_height) {
	se_window_get_size(window, out_width, out_height);
}

void se_window_get_content_scale(const se_window_handle window, f32* out_xscale, f32* out_yscale) {
	(void)window;
	if (out_xscale) {
		*out_xscale = g_android.content_scale_x > 0.0f ? g_android.content_scale_x : 1.0f;
	}
	if (out_yscale) {
		*out_yscale = g_android.content_scale_y > 0.0f ? g_android.content_scale_y : 1.0f;
	}
}

f32 se_window_get_aspect(const se_window_handle window) {
	u32 width = 0;
	u32 height = 0;
	se_window_get_framebuffer_size(window, &width, &height);
	if (height == 0) {
		return 1.0f;
	}
	return (f32)width / (f32)height;
}

b8 se_window_pixel_to_normalized(const se_window_handle window, const f32 x, const f32 y, s_vec2* out_normalized) {
	if (!out_normalized) {
		return false;
	}
	u32 width = 0;
	u32 height = 0;
	se_window_get_size(window, &width, &height);
	if (width <= 1 || height <= 1) {
		return false;
	}
	out_normalized->x = (x / (f32)width) * 2.0f - 1.0f;
	out_normalized->y = 1.0f - (y / (f32)height) * 2.0f;
	return true;
}

b8 se_window_normalized_to_pixel(const se_window_handle window, const f32 nx, const f32 ny, s_vec2* out_pixel) {
	if (!out_pixel) {
		return false;
	}
	u32 width = 0;
	u32 height = 0;
	se_window_get_size(window, &width, &height);
	if (width <= 1 || height <= 1) {
		return false;
	}
	out_pixel->x = (nx * 0.5f + 0.5f) * (f32)width;
	out_pixel->y = (1.0f - (ny * 0.5f + 0.5f)) * (f32)height;
	return true;
}

b8 se_window_window_to_framebuffer(const se_window_handle window, const f32 x, const f32 y, s_vec2* out_framebuffer) {
	if (!out_framebuffer) {
		return false;
	}
	*out_framebuffer = s_vec2(x, y);
	(void)window;
	return true;
}

b8 se_window_framebuffer_to_window(const se_window_handle window, const f32 x, const f32 y, s_vec2* out_window) {
	if (!out_window) {
		return false;
	}
	*out_window = s_vec2(x, y);
	(void)window;
	return true;
}

void se_window_get_diagnostics(const se_window_handle window, se_window_diagnostics* out_diagnostics) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_diagnostics) {
		return;
	}
	*out_diagnostics = window_ptr->diagnostics;
}

void se_window_reset_diagnostics(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	memset(&window_ptr->diagnostics, 0, sizeof(window_ptr->diagnostics));
}

void se_window_get_mouse_normalized(const se_window_handle window, s_vec2* out_mouse_position) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_mouse_position) {
		return;
	}
	if (!se_window_pixel_to_normalized(window, (f32)window_ptr->mouse_x, (f32)window_ptr->mouse_y, out_mouse_position)) {
		*out_mouse_position = s_vec2(0.0f, 0.0f);
	}
}

void se_window_get_mouse_delta(const se_window_handle window, s_vec2* out_mouse_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_mouse_delta) {
		return;
	}
	*out_mouse_delta = s_vec2(window_ptr->mouse_dx, window_ptr->mouse_dy);
}

void se_window_get_mouse_delta_normalized(const se_window_handle window, s_vec2* out_mouse_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_mouse_delta) {
		return;
	}
	*out_mouse_delta = s_vec2((window_ptr->mouse_dx / window_ptr->width), (window_ptr->mouse_dy / window_ptr->height));
}

void se_window_get_scroll(const se_window_handle window, s_vec2* out_scroll_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_scroll_delta) {
		return;
	}
	*out_scroll_delta = s_vec2(window_ptr->scroll_dx, window_ptr->scroll_dy);
}

void se_window_set_cursor_mode(const se_window_handle window, const se_window_cursor_mode mode) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->cursor_mode = mode;
}

se_window_cursor_mode se_window_get_cursor_mode(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? window_ptr->cursor_mode : SE_WINDOW_CURSOR_NORMAL;
}

b8 se_window_is_raw_mouse_supported(const se_window_handle window) {
	(void)window;
	return false;
}

void se_window_set_raw_mouse(const se_window_handle window, const b8 enabled) {
	(void)window;
	(void)enabled;
}

b8 se_window_is_raw_mouse_enabled(const se_window_handle window) {
	(void)window;
	return false;
}

b8 se_window_should_close(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return true;
	}
	return window_ptr->should_close || g_android.quit_requested || (g_android.app && g_android.app->destroyRequested != 0);
}

void se_window_set_should_close(const se_window_handle window, const b8 should_close) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->should_close = should_close;
}

void se_window_set_exit_keys(const se_window_handle window, se_key_combo* keys) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !keys) {
		return;
	}
	window_ptr->exit_keys = keys;
	window_ptr->use_exit_key = false;
}

void se_window_set_exit_key(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->exit_key = key;
	window_ptr->use_exit_key = true;
}

void se_window_check_exit_keys(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	if (window_ptr->use_exit_key) {
		if (window_ptr->exit_key != SE_KEY_UNKNOWN && se_window_is_key_down(window, window_ptr->exit_key)) {
			window_ptr->should_close = true;
			return;
		}
	}
	if (!window_ptr->exit_keys) {
		return;
	}
	se_key_combo* keys = window_ptr->exit_keys;
	if (s_array_get_size(keys) == 0) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(keys); ++i) {
		se_key* current_key = s_array_get(keys, s_array_handle(keys, (u32)i));
		if (!se_window_is_key_down(window, *current_key)) {
			return;
		}
	}
	window_ptr->should_close = true;
}

f64 se_window_get_delta_time(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? window_ptr->time.delta : 0.0;
}

f64 se_window_get_fps(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? (1.0 / window_ptr->time.delta) : 0.0;
}

f64 se_window_get_time(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? window_ptr->time.current : 0.0;
}

void se_window_set_target_fps(const se_window_handle window, const u16 fps) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || fps == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	window_ptr->target_fps = fps;
	se_set_last_error(SE_RESULT_OK);
}

i32 se_window_register_input_event(const se_window_handle window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !box) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return -1;
	}
	if (s_array_get_size(&window_ptr->input_events) >= SE_MAX_INPUT_EVENTS) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return -1;
	}

	s_handle event_handle = s_array_increment(&window_ptr->input_events);
	se_input_event* new_event = s_array_get(&window_ptr->input_events, event_handle);
	c8 event_ptr[16];
	c8 box_ptr[16];
	sprintf(event_ptr, "%p", (void*)new_event);
	sprintf(box_ptr, "%p", (const void*)box);
	new_event->id = se_hash(event_ptr, (i32)strlen(event_ptr), box_ptr, (i32)strlen(box_ptr));
	new_event->box = *box;
	new_event->depth = depth;
	new_event->active = true;
	new_event->on_interact_callback = on_interact_callback;
	new_event->on_stop_interact_callback = on_stop_interact_callback;
	new_event->callback_data = callback_data;
	se_set_last_error(SE_RESULT_OK);
	return new_event->id;
}

void se_window_update_input_event(const i32 input_event_id, const se_window_handle window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !box) {
		return;
	}

	for (sz i = 0; i < s_array_get_size(&window_ptr->input_events); ++i) {
		se_input_event* current_event = s_array_get(&window_ptr->input_events, s_array_handle(&window_ptr->input_events, (u32)i));
		if (current_event->id != input_event_id) {
			continue;
		}
		current_event->box = *box;
		current_event->depth = depth;
		current_event->on_interact_callback = on_interact_callback;
		current_event->on_stop_interact_callback = on_stop_interact_callback;
		current_event->callback_data = callback_data;
		return;
	}
}

void se_window_register_resize_event(const se_window_handle window, se_resize_event_callback callback, void* data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !callback) {
		return;
	}
	if (s_array_get_size(&window_ptr->resize_handles) >= SE_MAX_RESIZE_HANDLE) {
		return;
	}
	s_handle handle = s_array_increment(&window_ptr->resize_handles);
	se_resize_handle* new_event = s_array_get(&window_ptr->resize_handles, handle);
	new_event->callback = callback;
	new_event->data = data;
}

void se_window_set_text_callback(const se_window_handle window, se_window_text_callback callback, void* data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->text_callback = callback;
	window_ptr->text_callback_data = data;
}

void se_window_emit_text(const se_window_handle window, const c8* utf8_text) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !utf8_text || !window_ptr->text_callback) {
		return;
	}
	window_ptr->text_callback(window, utf8_text, window_ptr->text_callback_data);
}

void se_window_destroy(const se_window_handle window) {
	if (window == S_HANDLE_NULL) {
		return;
	}

	se_window_registry_refresh_owner_context();
	se_context* current_context = se_current_context();
	sz registry_index = 0;
	se_window_registry_entry* registry_entry = se_window_registry_find(current_context, window, &registry_index);
	if (!registry_entry || !registry_entry->ctx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return;
	}

	se_context* owner_context = registry_entry->ctx;
	const se_window_handle owner_window = registry_entry->window;
	se_window* window_ptr = s_array_get(&owner_context->windows, owner_window);
	if (!window_ptr) {
		s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)registry_index));
		se_window_backend_shutdown_if_unused();
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return;
	}

	if (s_array_get_size(&owner_context->windows) == 1 && se_window_context_has_live_resources(owner_context)) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return;
	}

	se_context* previous_context = se_push_tls_context(owner_context);
	(void)se_android_backend_make_current(true);
	if (window_ptr->quad.vao != 0) {
		se_quad_destroy(&window_ptr->quad);
	}
	if (window_ptr->shader != S_HANDLE_NULL &&
		s_array_get(&owner_context->shaders, window_ptr->shader) != NULL) {
		se_shader_destroy(window_ptr->shader);
	}
	window_ptr->shader = S_HANDLE_NULL;
	s_array_clear(&window_ptr->input_events);
	s_array_clear(&window_ptr->resize_handles);
	window_ptr->handle = NULL;

	s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)registry_index));
	s_array_remove(&owner_context->windows, owner_window);
	se_pop_tls_context(previous_context);
	se_window_backend_shutdown_if_unused();
	se_set_last_error(SE_RESULT_OK);
}

void se_window_destroy_all(void) {
	while (s_array_get_size(&windows_registry) > 0) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)(s_array_get_size(&windows_registry) - 1)));
		if (!entry || entry->window == S_HANDLE_NULL) {
			s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)(s_array_get_size(&windows_registry) - 1)));
			continue;
		}
		const sz windows_before = s_array_get_size(&windows_registry);
		se_window_destroy(entry->window);
		if (s_array_get_size(&windows_registry) == windows_before) {
			break;
		}
	}
	se_window_backend_shutdown_if_unused();
}

b8 se_window_backend_render_thread_attach(const se_window_handle window) {
	(void)window;
	return false;
}

void se_window_backend_render_thread_detach(void) {
}

void se_window_backend_render_thread_present(const se_window_handle window) {
	(void)window;
}

void se_window_backend_render_thread_set_vsync(const se_window_handle window, const b8 enabled) {
	(void)window;
	(void)enabled;
}

void* se_window_backend_get_gl_proc_address(const c8* name) {
	if (!name || name[0] == '\0') {
		return NULL;
	}
	return (void*)eglGetProcAddress(name);
}

b8 se_window_backend_has_current_context(void) {
	return eglGetCurrentContext() != EGL_NO_CONTEXT;
}

static AAsset* se_android_open_asset(const c8* path, const i32 mode) {
	if (!g_android.app || !g_android.app->activity || !g_android.app->activity->assetManager || !path || path[0] == '\0') {
		return NULL;
	}
	if (path[0] == '/') {
		return NULL;
	}

	while (path[0] == '.' && path[1] == '/') {
		path += 2;
	}
	return AAssetManager_open(g_android.app->activity->assetManager, path, mode);
}

static b8 se_android_path_has_prefix(const c8* path, const c8* prefix) {
	const sz prefix_len = prefix ? strlen(prefix) : 0u;
	return path && prefix && prefix_len > 0u && strncmp(path, prefix, prefix_len) == 0;
}

static const c8* se_android_skip_separators(const c8* path) {
	while (path && s_path_is_sep(path[0])) {
		path++;
	}
	return path;
}

static b8 se_android_path_is_absolute(const c8* path) {
	if (!path || path[0] == '\0') {
		return false;
	}
	if (s_path_is_sep(path[0])) {
		return true;
	}
	return path[0] != '\0' && path[1] == ':' && s_path_is_sep(path[2]);
}

static const c8* se_android_resource_relative_path(const c8* path) {
	const c8* relative = path;
	if (!path || path[0] == '\0') {
		return NULL;
	}
	if (se_android_path_has_prefix(relative, RESOURCES_DIR)) {
		relative += strlen(RESOURCES_DIR);
	}
	relative = se_android_skip_separators(relative);
	if (se_android_path_has_prefix(relative, SE_RESOURCE_INTERNAL("")) ||
		se_android_path_has_prefix(relative, SE_RESOURCE_PUBLIC("")) ||
		se_android_path_has_prefix(relative, SE_RESOURCE_EXAMPLE(""))) {
		return relative;
	}
	return NULL;
}

b8 se_window_backend_read_asset_text(const c8* path, c8** out_data, sz* out_size) {
	if (!out_data) {
		return false;
	}
	*out_data = NULL;
	if (out_size) {
		*out_size = 0;
	}

	AAsset* asset = se_android_open_asset(path, AASSET_MODE_BUFFER);
	if (!asset) {
		return false;
	}

	const off_t asset_size = AAsset_getLength(asset);
	c8* data = malloc((sz)asset_size + 1);
	if (!data) {
		AAsset_close(asset);
		return false;
	}
	const i32 read_size = AAsset_read(asset, data, asset_size);
	AAsset_close(asset);
	if (read_size < 0) {
		free(data);
		return false;
	}

	data[read_size] = '\0';
	*out_data = data;
	if (out_size) {
		*out_size = (sz)read_size;
	}
	return true;
}

b8 se_window_backend_read_asset_binary(const c8* path, u8** out_data, sz* out_size) {
	if (!out_data) {
		return false;
	}
	*out_data = NULL;
	if (out_size) {
		*out_size = 0;
	}

	AAsset* asset = se_android_open_asset(path, AASSET_MODE_STREAMING);
	if (!asset) {
		return false;
	}

	const off_t asset_size = AAsset_getLength(asset);
	u8* data = malloc((sz)asset_size);
	if (!data) {
		AAsset_close(asset);
		return false;
	}
	const i32 read_size = AAsset_read(asset, data, asset_size);
	AAsset_close(asset);
	if (read_size < 0) {
		free(data);
		return false;
	}

	*out_data = data;
	if (out_size) {
		*out_size = (sz)read_size;
	}
	return true;
}

b8 se_window_backend_get_asset_mtime(const c8* path, time_t* out_mtime) {
	(void)path;
	if (out_mtime) {
		*out_mtime = 0;
	}
	return false;
}

b8 se_window_backend_resolve_writable_path(const c8* path, c8* out_path, sz out_path_size) {
	const c8* base_path = NULL;
	const c8* relative_resource = NULL;
	const c8* write_path = path;
	c8 resolved_relative[SE_MAX_PATH_LENGTH] = {0};
	if (!path || path[0] == '\0' || !out_path || out_path_size == 0u || !g_android.app || !g_android.app->activity) {
		return false;
	}

	base_path = g_android.app->activity->internalDataPath;
	if (!base_path || base_path[0] == '\0') {
		base_path = g_android.app->activity->externalDataPath;
	}
	if (!base_path || base_path[0] == '\0') {
		return false;
	}

	relative_resource = se_android_resource_relative_path(path);
	if (relative_resource != NULL) {
		if (!s_path_join(resolved_relative, sizeof(resolved_relative), RESOURCES_DIR, relative_resource)) {
			return false;
		}
		write_path = resolved_relative;
	} else if (se_android_path_is_absolute(path)) {
		write_path = s_path_filename(path);
		if (!write_path || write_path[0] == '\0') {
			return false;
		}
	} else {
		write_path = se_android_skip_separators(path);
		if (!write_path || write_path[0] == '\0') {
			return false;
		}
	}

	return s_path_join(out_path, out_path_size, base_path, write_path);
}

#endif // SE_WINDOW_BACKEND_ANDROID
