// Syphax-Engine - Ougi Washi

#if defined(SE_WINDOW_BACKEND_GLFW)

#include "se_window.h"
#include "se_debug.h"
#include "render/se_gl.h"
#include <unistd.h>
#include <limits.h>
#include <string.h>

#define SE_MAX_WINDOWS 8
#define SE_MAX_KEY_COMBOS 8
#define SE_MAX_INPUT_EVENTS 1024
#define SE_MAX_RESIZE_HANDLE 1024

typedef struct {
	se_context* ctx;
	se_window_handle window;
} se_window_registry_entry;

typedef s_array(se_window_registry_entry, se_windows_registry);
static se_windows_registry windows_registry = {0};
static GLFWwindow* current_conext_window = NULL;

typedef struct {
	se_context* ctx;
	se_window_handle window;
} se_window_user_ptr;

static se_window* se_window_from_handle(se_context* context, const se_window_handle window) {
	return s_array_get(&context->windows, window);
}

static se_window* se_window_from_glfw(GLFWwindow* glfw_handle) {
	se_window_user_ptr* user_ptr = (se_window_user_ptr*)glfwGetWindowUserPointer(glfw_handle);
	if (!user_ptr) {
		return NULL;
	}
	return s_array_get(&user_ptr->ctx->windows, user_ptr->window);
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

static se_key se_window_map_glfw_key(const i32 key) {
	if (key >= SE_KEY_SPACE && key <= SE_KEY_MENU) {
		return (se_key)key;
	}
	return SE_KEY_UNKNOWN;
}

static sz se_window_utf8_encode(const u32 codepoint, c8 out_utf8[5]) {
	if (!out_utf8) {
		return 0;
	}
	if (codepoint <= 0x7FU) {
		out_utf8[0] = (c8)codepoint;
		out_utf8[1] = '\0';
		return 1;
	}
	if (codepoint <= 0x7FFU) {
		out_utf8[0] = (c8)(0xC0U | ((codepoint >> 6) & 0x1FU));
		out_utf8[1] = (c8)(0x80U | (codepoint & 0x3FU));
		out_utf8[2] = '\0';
		return 2;
	}
	if (codepoint <= 0xFFFFU) {
		out_utf8[0] = (c8)(0xE0U | ((codepoint >> 12) & 0x0FU));
		out_utf8[1] = (c8)(0x80U | ((codepoint >> 6) & 0x3FU));
		out_utf8[2] = (c8)(0x80U | (codepoint & 0x3FU));
		out_utf8[3] = '\0';
		return 3;
	}
	if (codepoint <= 0x10FFFFU) {
		out_utf8[0] = (c8)(0xF0U | ((codepoint >> 18) & 0x07U));
		out_utf8[1] = (c8)(0x80U | ((codepoint >> 12) & 0x3FU));
		out_utf8[2] = (c8)(0x80U | ((codepoint >> 6) & 0x3FU));
		out_utf8[3] = (c8)(0x80U | (codepoint & 0x3FU));
		out_utf8[4] = '\0';
		return 4;
	}
	return 0;
}

static void key_callback(GLFWwindow* glfw_handle, i32 key, i32 scancode, i32 action, i32 mods) {
	(void)scancode;
	(void)mods;
	se_window* window = se_window_from_glfw(glfw_handle);
	se_key mapped = se_window_map_glfw_key(key);
	if (mapped != SE_KEY_UNKNOWN) {
		if (action == GLFW_PRESS) {
			window->keys[mapped] = true;
		} else if (action == GLFW_RELEASE) {
			window->keys[mapped] = false;
		}
		window->diagnostics.key_events++;
	}
}

static void mouse_callback(GLFWwindow* glfw_handle, double xpos, double ypos) {
	se_window* window = se_window_from_glfw(glfw_handle);
	window->mouse_dx = xpos - window->mouse_x;
	window->mouse_dy = ypos - window->mouse_y;
	window->mouse_x = xpos;
	window->mouse_y = ypos;
	window->diagnostics.mouse_move_events++;
}

static void mouse_button_callback(GLFWwindow* glfw_handle, i32 button, i32 action, i32 mods) {
	(void)mods;
	se_window* window = se_window_from_glfw(glfw_handle);
	if (button >= 0 && button < SE_MOUSE_BUTTON_COUNT) {
		if (action == GLFW_PRESS) {
			window->mouse_buttons[button] = true;
		} else if (action == GLFW_RELEASE) {
			window->mouse_buttons[button] = false;
		}
		window->diagnostics.mouse_button_events++;
	}
}

static void scroll_callback(GLFWwindow* glfw_handle, double xoffset, double yoffset) {
	se_window* window = se_window_from_glfw(glfw_handle);
	window->scroll_dx += xoffset;
	window->scroll_dy += yoffset;
	window->diagnostics.scroll_events++;
}

static void char_callback(GLFWwindow* glfw_handle, unsigned int codepoint) {
	se_window_user_ptr* user_ptr = (se_window_user_ptr*)glfwGetWindowUserPointer(glfw_handle);
	if (!user_ptr || !user_ptr->ctx || user_ptr->window == S_HANDLE_NULL) {
		return;
	}
	se_window* window = s_array_get(&user_ptr->ctx->windows, user_ptr->window);
	if (!window || !window->text_callback) {
		return;
	}
	c8 utf8[5] = {0};
	if (se_window_utf8_encode((u32)codepoint, utf8) == 0) {
		return;
	}
	window->text_callback(user_ptr->window, utf8, window->text_callback_data);
}

static void framebuffer_size_callback(GLFWwindow* glfw_handle, i32 width, i32 height) {
	se_window_user_ptr* user_ptr = (se_window_user_ptr*)glfwGetWindowUserPointer(glfw_handle);
	if (!user_ptr) {
		return;
	}
	se_window* window = s_array_get(&user_ptr->ctx->windows, user_ptr->window);
	window->width = width;
	window->height = height;
	se_debug_log(SE_DEBUG_LEVEL_DEBUG, SE_DEBUG_CATEGORY_WINDOW, "Framebuffer resized: %d x %d", width, height);
	glViewport(0, 0, width, height);
	printf("frambuffer_size_change_fallback\n");
	for (sz i = 0; i < s_array_get_size(&window->resize_handles); ++i) {
		se_resize_handle* current_event_ptr = s_array_get(&window->resize_handles, s_array_handle(&window->resize_handles, (u32)i));
		if (current_event_ptr && current_event_ptr->callback) {
			current_event_ptr->callback(user_ptr->window, current_event_ptr->data);
		}
	}
}

void gl_error_callback(i32 error, const c8* description) {
	printf("GLFW Error %d: %s\n", error, description);
}

se_window_handle se_window_create(const char* title, const u32 width, const u32 height) {
	se_context* context = se_current_context();
	if (!context || !title) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	glfwSetErrorCallback(gl_error_callback);
	
	if (!glfwInit()) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return S_HANDLE_NULL;
	}

	if (s_array_get_capacity(&context->windows) == 0) {
		s_array_init(&context->windows);
	}
	se_window_handle window_handle = s_array_increment(&context->windows);
	se_window* new_window = s_array_get(&context->windows, window_handle);

	memset(new_window, 0, sizeof(se_window));
	s_array_init(&new_window->input_events);
	s_array_init(&new_window->resize_handles);

	// Set OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	 
	new_window->handle = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!new_window->handle) {
		s_array_clear(&new_window->input_events);
		s_array_clear(&new_window->resize_handles);
		s_array_remove(&context->windows, window_handle);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return S_HANDLE_NULL;
	}
	
	
	new_window->width = width;
	new_window->height = height;
	new_window->cursor_mode = SE_WINDOW_CURSOR_NORMAL;
	new_window->raw_mouse_motion_supported = glfwRawMouseMotionSupported();
	new_window->raw_mouse_motion_enabled = false;
	new_window->vsync_enabled = false;
	new_window->should_close = false;
	
	se_window_set_current_context(window_handle);
	se_window_user_ptr* user_ptr = malloc(sizeof(*user_ptr));
	if (!user_ptr) {
		glfwDestroyWindow((GLFWwindow *)new_window->handle);
		new_window->handle = NULL;
		s_array_clear(&new_window->input_events);
		s_array_clear(&new_window->resize_handles);
		s_array_remove(&context->windows, window_handle);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	user_ptr->ctx = context;
	user_ptr->window = window_handle;
	glfwSetWindowUserPointer((GLFWwindow*)new_window->handle, user_ptr);
	glfwSwapInterval(0);
	
	// Set callbacks
	glfwSetKeyCallback((GLFWwindow*)new_window->handle, key_callback);
	glfwSetCursorPosCallback((GLFWwindow*)new_window->handle, mouse_callback);
	glfwSetMouseButtonCallback((GLFWwindow*)new_window->handle, mouse_button_callback);
	glfwSetScrollCallback((GLFWwindow*)new_window->handle, scroll_callback);
	glfwSetCharCallback((GLFWwindow*)new_window->handle, char_callback);
	glfwSetFramebufferSizeCallback((GLFWwindow*)new_window->handle, framebuffer_size_callback);
	
	se_render_init();
	
	glEnable(GL_DEPTH_TEST);
	
	//create_fullscreen_quad(&new_window->quad_vao, &new_window->quad_vbo, &new_window->quad_ebo);
	se_result render_result = se_window_init_render(new_window, context);
	if (render_result != SE_RESULT_OK) {
		if (new_window->quad.vao != 0) {
			se_quad_destroy(&new_window->quad);
		}
		glfwDestroyWindow((GLFWwindow *)new_window->handle);
		new_window->handle = NULL;
		s_array_clear(&new_window->input_events);
		s_array_clear(&new_window->resize_handles);
		s_array_remove(&context->windows, window_handle);
		se_set_last_error(render_result);
		return S_HANDLE_NULL;
	}

	new_window->time.current = glfwGetTime();
	new_window->time.last_frame = new_window->time.current;
	new_window->time.delta = 0;
	new_window->frame_count = 0;
	new_window->target_fps = 30;

	if (s_array_get_capacity(&windows_registry) == 0) {
		s_array_init(&windows_registry);
	}
	se_window_registry_entry entry = {0};
	entry.ctx = context;
	entry.window = window_handle;
	s_handle entry_handle = s_array_increment(&windows_registry);
	se_window_registry_entry* entry_slot = s_array_get(&windows_registry, entry_handle);
	*entry_slot = entry;

	se_set_last_error(SE_RESULT_OK);
	return window_handle;
}

void se_window_attach_render(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_attach_render :: window is null");
	se_window_init_render(window_ptr, context);
}

extern void se_window_update(const se_window_handle window) {
	se_debug_frame_begin();
	se_debug_trace_begin("window_update");
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	se_window_check_exit_keys(window);
	memcpy(window_ptr->keys_prev, window_ptr->keys, sizeof(window_ptr->keys));
	memcpy(window_ptr->mouse_buttons_prev, window_ptr->mouse_buttons, sizeof(window_ptr->mouse_buttons));
	window_ptr->mouse_dx = 0.0;
	window_ptr->mouse_dy = 0.0;
	window_ptr->scroll_dx = 0.0;
	window_ptr->scroll_dy = 0.0;
	window_ptr->time.last_frame = window_ptr->time.current;
	window_ptr->time.current = glfwGetTime();
	window_ptr->time.delta = window_ptr->time.current - window_ptr->time.last_frame;
	window_ptr->time.frame_start = window_ptr->time.current;
	window_ptr->frame_count++;
	se_debug_trace_end("window_update");
}

void se_window_tick(const se_window_handle window) {
	se_window_update(window);
	se_debug_trace_begin("input_tick");
	se_window_poll_events();
	se_debug_trace_end("input_tick");
}

void se_window_set_current_context(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	current_conext_window = (GLFWwindow*)window_ptr->handle;
	glfwMakeContextCurrent((GLFWwindow*)window_ptr->handle);
}

void se_window_render_quad(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr->shader != S_HANDLE_NULL, "se_window_render_quad :: shader is null");
	se_shader_use(window_ptr->shader, true, false);
	se_quad_render(&window_ptr->quad, 0);
}

void se_window_render_screen(const se_window_handle window) {
	se_debug_trace_begin("window_present");
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	const f64 present_begin = glfwGetTime();
	window_ptr->diagnostics.last_sleep_duration = 0.0;
	if (!window_ptr->vsync_enabled && window_ptr->target_fps > 0) {
		const f64 target_frame_time = 1.0 / window_ptr->target_fps;
		const f64 now = glfwGetTime();
		const f64 elapsed = now - window_ptr->time.frame_start;
		f64 time_left = target_frame_time - elapsed;
		if (time_left > 0.0) {
			/*
			 * Coarse sleep first, then spin for the final sub-millisecond window.
			 * This avoids repeatedly sleeping in 1ms chunks, which caused
			 * significant oversleep and unstable high-FPS pacing.
			 */
			const f64 coarse_guard = 0.001;
			if (time_left > coarse_guard) {
				const f64 coarse_sleep = time_left - coarse_guard;
				usleep((useconds_t)(coarse_sleep * 1000000.0));
				window_ptr->diagnostics.last_sleep_duration += coarse_sleep;
			}
			const f64 wait_end = window_ptr->time.frame_start + target_frame_time;
			while (glfwGetTime() < wait_end) {
				/* busy wait for precise frame boundary */
			}
			const f64 after_wait = glfwGetTime();
			const f64 slept = after_wait - now;
			if (slept > window_ptr->diagnostics.last_sleep_duration) {
				window_ptr->diagnostics.last_sleep_duration = slept;
			}
		}
	}
	se_debug_render_overlay(window, NULL);
	glfwSwapBuffers((GLFWwindow*)window_ptr->handle);
	window_ptr->diagnostics.frames_presented++;
	window_ptr->diagnostics.last_present_duration = glfwGetTime() - present_begin;
	se_debug_trace_end("window_present");
	se_debug_frame_end();
}

void se_window_set_vsync(const se_window_handle window, const b8 enabled) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_vsync :: window is null");
	se_window_set_current_context(window);
	glfwSwapInterval(enabled ? 1 : 0);
	window_ptr->vsync_enabled = enabled;
}

b8 se_window_is_vsync_enabled(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_vsync_enabled :: window is null");
	return window_ptr->vsync_enabled;
}

void se_window_present(const se_window_handle window) {
	se_window_render_screen(window);
}

void se_window_present_frame(const se_window_handle window, const s_vec4* clear_color) {
	if (clear_color) {
		se_render_set_background_color(*clear_color);
	}
	se_render_clear();
	se_window_render_screen(window);
}

void se_window_poll_events(){
	glfwPollEvents();
	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (!entry || entry->window == S_HANDLE_NULL || !entry->ctx) {
			continue;
		}
		se_window* window = s_array_get(&entry->ctx->windows, entry->window);
		if (!window || window->handle == NULL) {
			continue;
		}
		s_vec2 mouse_position = {0};
		se_window_get_mouse_position_normalized(entry->window, &mouse_position);
		se_input_event* out_event = NULL;
		i32 out_depth = INT_MIN;
		b8 interacted = false;
		for (sz j = 0; j < s_array_get_size(&window->input_events); ++j) {
			se_input_event* current_event = s_array_get(&window->input_events, s_array_handle(&window->input_events, (u32)j));
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
				out_event->on_interact_callback(entry->window, out_event->callback_data);
			}
			out_event->interacted = true;
		} else {
			for (sz j = 0; j < s_array_get_size(&window->input_events); ++j) {
				se_input_event* current_event = s_array_get(&window->input_events, s_array_handle(&window->input_events, (u32)j));
				if (current_event->interacted) {
					if (current_event->on_stop_interact_callback) {
						current_event->on_stop_interact_callback(entry->window, current_event->callback_data);
					}
					current_event->interacted = false;
				}
			}
		}
	}
}

b8 se_window_is_key_down(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_key_down :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window_ptr->keys[key];
}

b8 se_window_is_key_pressed(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_key_pressed :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window_ptr->keys[key] && !window_ptr->keys_prev[key];
}

b8 se_window_is_key_released(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_key_released :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return !window_ptr->keys[key] && window_ptr->keys_prev[key];
}

b8 se_window_is_mouse_down(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_mouse_down :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window_ptr->mouse_buttons[button];
}

b8 se_window_is_mouse_pressed(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_mouse_pressed :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window_ptr->mouse_buttons[button] && !window_ptr->mouse_buttons_prev[button];
}

b8 se_window_is_mouse_released(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_mouse_released :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return !window_ptr->mouse_buttons[button] && window_ptr->mouse_buttons_prev[button];
}

void se_window_clear_input_state(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_clear_input_state :: window is null");
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
	s_assertf(window_ptr, "se_window_inject_key_state :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return;
	}
	window_ptr->keys[key] = down;
}

void se_window_inject_mouse_button_state(const se_window_handle window, const se_mouse_button button, const b8 down) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_inject_mouse_button_state :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return;
	}
	window_ptr->mouse_buttons[button] = down;
}

void se_window_inject_mouse_position(const se_window_handle window, const f32 x, const f32 y) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_inject_mouse_position :: window is null");
	const f64 prev_x = window_ptr->mouse_x;
	const f64 prev_y = window_ptr->mouse_y;
	window_ptr->mouse_x = x;
	window_ptr->mouse_y = y;
	window_ptr->mouse_dx = window_ptr->mouse_x - prev_x;
	window_ptr->mouse_dy = window_ptr->mouse_y - prev_y;
}

void se_window_inject_scroll_delta(const se_window_handle window, const f32 x, const f32 y) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_inject_scroll_delta :: window is null");
	window_ptr->scroll_dx = x;
	window_ptr->scroll_dy = y;
}

f32 se_window_get_mouse_position_x(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_mouse_position_x :: window is null");
	return (f32)window_ptr->mouse_x;
}

f32 se_window_get_mouse_position_y(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_mouse_position_y :: window is null");
	return (f32)window_ptr->mouse_y;
}

void se_window_get_size(const se_window_handle window, u32* out_width, u32* out_height) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_size :: window is null");
	if (!out_width && !out_height) {
		return;
	}

	i32 width = (i32)window_ptr->width;
	i32 height = (i32)window_ptr->height;
	if (window_ptr->handle) {
		glfwGetWindowSize((GLFWwindow*)window_ptr->handle, &width, &height);
	}

	if (out_width) {
		*out_width = (u32)s_max(width, 0);
	}
	if (out_height) {
		*out_height = (u32)s_max(height, 0);
	}
}

void se_window_get_framebuffer_size(const se_window_handle window, u32* out_width, u32* out_height) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_framebuffer_size :: window is null");
	if (!out_width && !out_height) {
		return;
	}

	i32 width = (i32)window_ptr->width;
	i32 height = (i32)window_ptr->height;
	if (window_ptr->handle) {
		glfwGetFramebufferSize((GLFWwindow*)window_ptr->handle, &width, &height);
	}

	if (out_width) {
		*out_width = (u32)s_max(width, 0);
	}
	if (out_height) {
		*out_height = (u32)s_max(height, 0);
	}
}

void se_window_get_content_scale(const se_window_handle window, f32* out_xscale, f32* out_yscale) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_content_scale :: window is null");
	if (!out_xscale && !out_yscale) {
		return;
	}

	f32 xscale = 1.0f;
	f32 yscale = 1.0f;
	if (window_ptr->handle) {
		glfwGetWindowContentScale((GLFWwindow*)window_ptr->handle, &xscale, &yscale);
	}

	if (out_xscale) {
		*out_xscale = xscale;
	}
	if (out_yscale) {
		*out_yscale = yscale;
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
	u32 window_w = 0;
	u32 window_h = 0;
	u32 fb_w = 0;
	u32 fb_h = 0;
	se_window_get_size(window, &window_w, &window_h);
	se_window_get_framebuffer_size(window, &fb_w, &fb_h);
	if (window_w <= 1 || window_h <= 1 || fb_w <= 1 || fb_h <= 1) {
		return false;
	}
	out_framebuffer->x = x * ((f32)fb_w / (f32)window_w);
	out_framebuffer->y = y * ((f32)fb_h / (f32)window_h);
	return true;
}

b8 se_window_framebuffer_to_window(const se_window_handle window, const f32 x, const f32 y, s_vec2* out_window) {
	if (!out_window) {
		return false;
	}
	u32 window_w = 0;
	u32 window_h = 0;
	u32 fb_w = 0;
	u32 fb_h = 0;
	se_window_get_size(window, &window_w, &window_h);
	se_window_get_framebuffer_size(window, &fb_w, &fb_h);
	if (window_w <= 1 || window_h <= 1 || fb_w <= 1 || fb_h <= 1) {
		return false;
	}
	out_window->x = x * ((f32)window_w / (f32)fb_w);
	out_window->y = y * ((f32)window_h / (f32)fb_h);
	return true;
}

void se_window_get_diagnostics(const se_window_handle window, se_window_diagnostics* out_diagnostics) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_diagnostics :: window is null");
	if (!out_diagnostics) {
		return;
	}
	*out_diagnostics = window_ptr->diagnostics;
}

void se_window_reset_diagnostics(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_reset_diagnostics :: window is null");
	memset(&window_ptr->diagnostics, 0, sizeof(window_ptr->diagnostics));
}

void se_window_get_mouse_position_normalized(const se_window_handle window, s_vec2* out_mouse_position) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_mouse_position_normalized :: window is null");
	s_assertf(out_mouse_position, "se_window_get_mouse_position_normalized :: out_mouse_position is null");
	*out_mouse_position = s_vec2((window_ptr->mouse_x / window_ptr->width) - .5, window_ptr->mouse_y / window_ptr->height - .5);
	out_mouse_position->x *= 2.;
	out_mouse_position->y *= 2.;
}

void se_window_get_mouse_delta(const se_window_handle window, s_vec2* out_mouse_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_mouse_delta :: window is null");
	s_assertf(out_mouse_delta, "se_window_get_mouse_delta :: out_mouse_delta is null");
	*out_mouse_delta = s_vec2(window_ptr->mouse_dx, window_ptr->mouse_dy);
}

void se_window_get_mouse_delta_normalized(const se_window_handle window, s_vec2* out_mouse_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_mouse_delta_normalized :: window is null");
	s_assertf(out_mouse_delta, "se_window_get_mouse_delta_normalized :: out_mouse_delta is null");
	*out_mouse_delta = s_vec2((window_ptr->mouse_dx / window_ptr->width), (window_ptr->mouse_dy / window_ptr->height));
}

void se_window_get_scroll_delta(const se_window_handle window, s_vec2* out_scroll_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_scroll_delta :: window is null");
	s_assertf(out_scroll_delta, "se_window_get_scroll_delta :: out_scroll_delta is null");
	*out_scroll_delta = s_vec2(window_ptr->scroll_dx, window_ptr->scroll_dy);
}

void se_window_set_cursor_mode(const se_window_handle window, const se_window_cursor_mode mode) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_cursor_mode :: window is null");
	i32 glfw_mode = GLFW_CURSOR_NORMAL;
	switch (mode) {
		case SE_WINDOW_CURSOR_NORMAL: glfw_mode = GLFW_CURSOR_NORMAL; break;
		case SE_WINDOW_CURSOR_HIDDEN: glfw_mode = GLFW_CURSOR_HIDDEN; break;
		case SE_WINDOW_CURSOR_DISABLED: glfw_mode = GLFW_CURSOR_DISABLED; break;
		default: glfw_mode = GLFW_CURSOR_NORMAL; break;
	}
	glfwSetInputMode((GLFWwindow*)window_ptr->handle, GLFW_CURSOR, glfw_mode);
	window_ptr->cursor_mode = mode;
}

se_window_cursor_mode se_window_get_cursor_mode(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_cursor_mode :: window is null");
	return window_ptr->cursor_mode;
}

b8 se_window_is_raw_mouse_motion_supported(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_raw_mouse_motion_supported :: window is null");
	return window_ptr->raw_mouse_motion_supported;
}

void se_window_set_raw_mouse_motion(const se_window_handle window, const b8 enabled) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_raw_mouse_motion :: window is null");
	if (!window_ptr->raw_mouse_motion_supported) {
		window_ptr->raw_mouse_motion_enabled = false;
		return;
	}
	const i32 glfw_enabled = enabled ? GLFW_TRUE : GLFW_FALSE;
	glfwSetInputMode((GLFWwindow*)window_ptr->handle, GLFW_RAW_MOUSE_MOTION, glfw_enabled);
	window_ptr->raw_mouse_motion_enabled = enabled;
}

b8 se_window_is_raw_mouse_motion_enabled(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_raw_mouse_motion_enabled :: window is null");
	return window_ptr->raw_mouse_motion_enabled;
}

b8 se_window_should_close(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_should_close :: window is null");
	return glfwWindowShouldClose((GLFWwindow*)window_ptr->handle);
}

void se_window_set_should_close(const se_window_handle window, const b8 should_close) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_should_close :: window is null");
	glfwSetWindowShouldClose((GLFWwindow*)window_ptr->handle, should_close);
}

void se_window_set_exit_keys(const se_window_handle window, se_key_combo* keys) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_exit_keys :: window is null");
	s_assertf(keys, "se_window_set_exit_keys :: keys is null");
	window_ptr->exit_keys = keys;
	window_ptr->use_exit_key = false;
}

void se_window_set_exit_key(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_exit_key :: window is null");
	window_ptr->exit_key = key;
	window_ptr->use_exit_key = true;
}

void se_window_check_exit_keys(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_check_exit_keys :: window is null");
	if (window_ptr->use_exit_key) {
		if (window_ptr->exit_key != SE_KEY_UNKNOWN && se_window_is_key_down(window, window_ptr->exit_key)) {
			glfwSetWindowShouldClose((GLFWwindow*)window_ptr->handle, GLFW_TRUE);
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
	glfwSetWindowShouldClose((GLFWwindow*)window_ptr->handle, GLFW_TRUE);
}

f64 se_window_get_delta_time(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr->time.delta;
}

f64 se_window_get_fps(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return 1. / window_ptr->time.delta;
}

f64 se_window_get_time(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr->time.current;
}

void se_window_set_target_fps(const se_window_handle window, const u16 fps) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_target_fps :: window is null");
	if (fps == 0) {
		se_debug_log(SE_DEBUG_LEVEL_WARN, SE_DEBUG_CATEGORY_WINDOW, "Ignored target_fps=0");
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	window_ptr->target_fps = fps;
	se_set_last_error(SE_RESULT_OK);
}

// TODO: move somewhere else and optimize it if it used a lot
i32 se_hash(const c8* str1, const i32 len1, const c8* str2, const i32 len2) {
	u32 hash = 5381;
	for (i32 i = 0; i < len1; i++) {
		hash = ((hash << 5) + hash) + str1[i];
	}
	for (i32 i = 0; i < len2; i++) {
		hash = ((hash << 5) + hash) + str2[i];
	}
	return hash;
}

i32 se_window_register_input_event(const se_window_handle window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_register_input_event :: window is null");
	s_assertf(box, "se_window_register_input_event :: box is null");

	if (s_array_get_size(&window_ptr->input_events) >= SE_MAX_INPUT_EVENTS) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return -1;
	}
	s_handle event_handle = s_array_increment(&window_ptr->input_events);
	se_input_event* new_event = s_array_get(&window_ptr->input_events, event_handle);
	s_assertf(new_event, "se_window_register_input_event :: Array is full");
	c8 event_ptr[16];
	c8 box_ptr[16];
	sprintf(event_ptr, "%p", new_event);
	sprintf(box_ptr, "%p", box);
	new_event->id = se_hash(event_ptr, strlen(event_ptr), box_ptr, strlen(box_ptr));
	new_event->box = *box;
	new_event->depth = depth;
	new_event->active = true;
	new_event->on_interact_callback = on_interact_callback;
	new_event->on_stop_interact_callback = on_stop_interact_callback;
	new_event->callback_data = callback_data;
	return new_event->id;
}

void se_window_update_input_event(const i32 input_event_id, const se_window_handle window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_update_input_event :: window is null");
	s_assertf(box, "se_window_update_input_event :: box is null");

	se_input_event* found_event = NULL;
	for (sz i = 0; i < s_array_get_size(&window_ptr->input_events); ++i) {
		se_input_event* current_event = s_array_get(&window_ptr->input_events, s_array_handle(&window_ptr->input_events, (u32)i));
		if (current_event->id == input_event_id) {
			found_event = current_event;
			break;
		}
	}
	if (found_event) {
		found_event->box = *box;
		found_event->depth = depth;
		found_event->on_interact_callback = on_interact_callback;
		found_event->on_stop_interact_callback = on_stop_interact_callback;
		found_event->callback_data = callback_data;
	} else {
		printf("se_window_update_input_event :: failed to find event with id: %d\n", input_event_id);
	}
}

void se_window_register_resize_event(const se_window_handle window, se_resize_event_callback callback, void* data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_register_resize_event :: window is null");
	s_assertf(callback, "se_window_register_resize_event :: callback is null");
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
	s_assertf(window_ptr, "se_window_set_text_callback :: window is null");
	window_ptr->text_callback = callback;
	window_ptr->text_callback_data = data;
}

void se_window_emit_text(const se_window_handle window, const c8* utf8_text) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_emit_text :: window is null");
	if (!utf8_text || !window_ptr->text_callback) {
		return;
	}
	window_ptr->text_callback(window, utf8_text, window_ptr->text_callback_data);
}

void se_window_destroy(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (window_ptr == NULL) {
		return;
	}

	if (window_ptr->quad.vao != 0) {
		se_quad_destroy(&window_ptr->quad);
	}
	s_array_clear(&window_ptr->input_events);
	s_array_clear(&window_ptr->resize_handles);

	if (window_ptr->handle) {
		se_window_user_ptr* user_ptr = (se_window_user_ptr*)glfwGetWindowUserPointer((GLFWwindow*)window_ptr->handle);
		if (user_ptr) {
			free(user_ptr);
		}
		glfwDestroyWindow((GLFWwindow*)window_ptr->handle);
	}
	window_ptr->handle = NULL;

	for (sz i = 0; i < s_array_get_size(&windows_registry); i++) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (entry && entry->window == window) {
			s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)i));
			break;
		}
	}

	s_array_remove(&context->windows, window);

	if (s_array_get_size(&windows_registry) == 0) {
		s_array_clear(&windows_registry);
		glfwTerminate();
	}
}

void se_window_destroy_all(void){
	while (s_array_get_size(&windows_registry) > 0) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)(s_array_get_size(&windows_registry) - 1)));
		if (!entry || entry->window == S_HANDLE_NULL) {
			s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)(s_array_get_size(&windows_registry) - 1)));
			continue;
		}
		se_window_destroy(entry->window);
	}
}

#endif // SE_WINDOW_BACKEND_GLFW
