// Syphax-Engine - Ougi Washi

#if defined(SE_WINDOW_BACKEND_GLFW)

#include "se_window.h"
#include "render/se_gl.h"
#include <unistd.h>
#include <limits.h>
#include <string.h>

#define SE_MAX_WINDOWS 8
#define SE_MAX_KEY_COMBOS 8
#define SE_MAX_INPUT_EVENTS 1024
#define SE_MAX_RESIZE_HANDLE 1024

static se_windows windows_container = { 0 };
static GLFWwindow* current_conext_window = NULL;

static se_result se_window_init_render(se_window* window, se_render_handle* render_handle) {
	s_assertf(window, "se_window_init_render :: window is null");
	if (!render_handle) {
		return SE_RESULT_INVALID_ARGUMENT;
	}
	if (window->render_handle == render_handle && window->shader) {
		return SE_RESULT_OK;
	}
	window->render_handle = render_handle;
	if (window->quad.vao == 0) {
		se_quad_2d_create(&window->quad, 0);
	}
	if (!window->shader) {
		se_shader *shader = se_shader_load(render_handle, SE_RESOURCE_INTERNAL("shaders/render_quad_vert.glsl"), SE_RESOURCE_INTERNAL("shaders/render_quad_frag.glsl"));
		if (!shader) {
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

static void key_callback(GLFWwindow* glfw_handle, i32 key, i32 scancode, i32 action, i32 mods) {
	se_window* window = (se_window*)glfwGetWindowUserPointer(glfw_handle);
	se_key mapped = se_window_map_glfw_key(key);
	if (mapped != SE_KEY_UNKNOWN) {
		if (action == GLFW_PRESS) {
			window->keys[mapped] = true;
		} else if (action == GLFW_RELEASE) {
			window->keys[mapped] = false;
		}
	}
}

static void mouse_callback(GLFWwindow* glfw_handle, double xpos, double ypos) {
	se_window* window = (se_window*)glfwGetWindowUserPointer(glfw_handle);
	window->mouse_dx = xpos - window->mouse_x;
	window->mouse_dy = ypos - window->mouse_y;
	window->mouse_x = xpos;
	window->mouse_y = ypos;
}

static void mouse_button_callback(GLFWwindow* glfw_handle, i32 button, i32 action, i32 mods) {
	se_window* window = (se_window*)glfwGetWindowUserPointer(glfw_handle);
	if (button >= 0 && button < SE_MOUSE_BUTTON_COUNT) {
		if (action == GLFW_PRESS) {
			window->mouse_buttons[button] = true;
		} else if (action == GLFW_RELEASE) {
			window->mouse_buttons[button] = false;
		}
	}
}

static void framebuffer_size_callback(GLFWwindow* glfw_handle, i32 width, i32 height) {
	se_window* window = (se_window*)glfwGetWindowUserPointer(glfw_handle);
	window->width = width;
	window->height = height;
	glViewport(0, 0, width, height);
	printf("frambuffer_size_change_fallback\n");
	s_foreach(&window->resize_handles, i) {
		se_resize_handle* current_event_ptr = s_array_get(&window->resize_handles, i);
		if (current_event_ptr) {
			se_resize_handle current_event = *current_event_ptr;
			current_event.callback(window, current_event.data);
		}
	}
}

void gl_error_callback(i32 error, const c8* description) {
	printf("GLFW Error %d: %s\n", error, description);
}

se_window* se_window_create(se_render_handle* render_handle, const char* title, const u32 width, const u32 height) {
	if (!title) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	glfwSetErrorCallback(gl_error_callback);
	
	if (!glfwInit()) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}

	if (s_array_get_capacity(&windows_container) == 0) {
		s_array_init(&windows_container, SE_MAX_WINDOWS);
	}
	se_window* new_window = s_array_increment(&windows_container);
	memset(new_window, 0, sizeof(se_window));
	new_window->render_handle = NULL;

	if (new_window == NULL) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	s_array_init(&new_window->input_events, SE_MAX_INPUT_EVENTS);
	s_array_init(&new_window->resize_handles, SE_MAX_RESIZE_HANDLE);

	// Set OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	 
	new_window->handle = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!new_window->handle) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}
	
	
	new_window->width = width;
	new_window->height = height;
	
	se_window_set_current_context(new_window);
	glfwSetWindowUserPointer((GLFWwindow*)new_window->handle, new_window);
	glfwSwapInterval(0); 
	
	// Set callbacks
	glfwSetKeyCallback((GLFWwindow*)new_window->handle, key_callback);
	glfwSetCursorPosCallback((GLFWwindow*)new_window->handle, mouse_callback);
	glfwSetMouseButtonCallback((GLFWwindow*)new_window->handle, mouse_button_callback);
	glfwSetFramebufferSizeCallback((GLFWwindow*)new_window->handle, framebuffer_size_callback);
	
	se_render_init();
	
	glEnable(GL_DEPTH_TEST);
	
	//create_fullscreen_quad(&new_window->quad_vao, &new_window->quad_vbo, &new_window->quad_ebo);
	se_result render_result = se_window_init_render(new_window, render_handle);
	if (render_result != SE_RESULT_OK) {
		se_set_last_error(render_result);
		return NULL;
	}

	new_window->time.current = glfwGetTime();
	new_window->time.last_frame = new_window->time.current;
	new_window->time.delta = 0;
	new_window->frame_count = 0;
	new_window->target_fps = 30;
	se_set_last_error(SE_RESULT_OK);
	return new_window;
}

void se_window_attach_render(se_window* window, se_render_handle* render_handle) {
	s_assertf(window, "se_window_attach_render :: window is null");
	se_window_init_render(window, render_handle);
}

extern void se_window_update(se_window* window) {
	se_window_check_exit_keys(window);
	memcpy(window->keys_prev, window->keys, sizeof(window->keys));
	memcpy(window->mouse_buttons_prev, window->mouse_buttons, sizeof(window->mouse_buttons));
	window->mouse_dx = 0.0;
	window->mouse_dy = 0.0;
	window->time.last_frame = window->time.current;
	window->time.current = glfwGetTime();
	window->time.delta = window->time.current - window->time.last_frame;
	window->time.frame_start = window->time.current;
	window->frame_count++;
}

void se_window_tick(se_window* window) {
	se_window_update(window);
	se_window_poll_events();
}

void se_window_set_current_context(se_window* window) {
	current_conext_window = (GLFWwindow*)window->handle;
	glfwMakeContextCurrent((GLFWwindow*)window->handle);
}

void se_window_render_quad(se_window* window) {
	s_assertf(window->shader, "se_window_render_quad :: shader is null");
	se_shader_use(window->render_handle, window->shader, true, false);
	se_quad_render(&window->quad, 0);
}

void se_window_render_screen(se_window* window) {
	f64 target_frame_time = 1.0 / window->target_fps;
	f64 now = glfwGetTime();
	f64 elapsed = now - window->time.frame_start;
	f64 time_left = target_frame_time - elapsed;
	
	if (time_left > 0) {
		usleep((time_left) * 1000000);
	}
	
	f64 wait_end = window->time.frame_start + target_frame_time;
	while (glfwGetTime() < wait_end) {
        usleep(1000);
	}
	
	glfwSwapBuffers((GLFWwindow*)window->handle);
}

void se_window_present(se_window* window) {
	se_window_render_screen(window);
}

void se_window_present_frame(se_window* window, const s_vec4* clear_color) {
	if (clear_color) {
		se_render_set_background_color(*clear_color);
	}
	se_render_clear();
	se_window_render_screen(window);
}

void se_window_poll_events(){
	glfwPollEvents();
	s_foreach(&windows_container, i) {
		se_window* window = s_array_get(&windows_container, i);
		s_vec2 mouse_position = {0};
		se_window_get_mouse_position_normalized(window, &mouse_position);
		se_input_event* out_event = NULL;
		i32 out_depth = INT_MIN;
		b8 interacted = false;
		s_foreach(&window->input_events, j) {
			se_input_event* current_event = s_array_get(&window->input_events, j);
			if (	current_event->active &&
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
		}
		else {
			s_foreach(&window->input_events, j) {
				se_input_event* current_event = s_array_get(&window->input_events, j);
				if (current_event->interacted) {
					if (current_event->on_stop_interact_callback) {
						current_event->on_stop_interact_callback(window, current_event->callback_data);
					}
					current_event->interacted = false;
				}
			}
		}
	}
}

b8 se_window_is_key_down(se_window* window, se_key key) {
	s_assertf(window, "se_window_is_key_down :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window->keys[key];
}

b8 se_window_is_key_pressed(se_window* window, se_key key) {
	s_assertf(window, "se_window_is_key_pressed :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window->keys[key] && !window->keys_prev[key];
}

b8 se_window_is_key_released(se_window* window, se_key key) {
	s_assertf(window, "se_window_is_key_released :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return !window->keys[key] && window->keys_prev[key];
}

b8 se_window_is_mouse_down(se_window* window, se_mouse_button button) {
	s_assertf(window, "se_window_is_mouse_down :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window->mouse_buttons[button];
}

b8 se_window_is_mouse_pressed(se_window* window, se_mouse_button button) {
	s_assertf(window, "se_window_is_mouse_pressed :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window->mouse_buttons[button] && !window->mouse_buttons_prev[button];
}

b8 se_window_is_mouse_released(se_window* window, se_mouse_button button) {
	s_assertf(window, "se_window_is_mouse_released :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return !window->mouse_buttons[button] && window->mouse_buttons_prev[button];
}

f32 se_window_get_mouse_position_x(se_window* window) {
	s_assertf(window, "se_window_get_mouse_position_x :: window is null");
	return window->mouse_x;
}

f32 se_window_get_mouse_position_y(se_window* window) {
	s_assertf(window, "se_window_get_mouse_position_y :: window is null");
	return window->mouse_y;
}

void se_window_get_mouse_position_normalized(se_window* window, s_vec2* out_mouse_position) {
	s_assertf(window, "se_window_get_mouse_position_normalized :: window is null");
	s_assertf(out_mouse_position, "se_window_get_mouse_position_normalized :: out_mouse_position is null");
	*out_mouse_position = s_vec2((window->mouse_x / window->width) - .5, window->mouse_y / window->height - .5);
	out_mouse_position->x *= 2.;
	out_mouse_position->y *= 2.;
}

void se_window_get_mouse_delta(se_window* window, s_vec2* out_mouse_delta) {
	s_assertf(window, "se_window_get_mouse_delta :: window is null");
	s_assertf(out_mouse_delta, "se_window_get_mouse_delta :: out_mouse_delta is null");
	*out_mouse_delta = s_vec2(window->mouse_dx, window->mouse_dy);
}

void se_window_get_mouse_delta_normalized(se_window* window, s_vec2* out_mouse_delta) {
	s_assertf(window, "se_window_get_mouse_delta_normalized :: window is null");
	s_assertf(out_mouse_delta, "se_window_get_mouse_delta_normalized :: out_mouse_delta is null");
	*out_mouse_delta = s_vec2((window->mouse_dx / window->width), (window->mouse_dy / window->height));
}

b8 se_window_should_close(se_window* window) {
	s_assertf(window, "se_window_should_close :: window is null");
	return glfwWindowShouldClose((GLFWwindow*)window->handle);
}

void se_window_set_should_close(se_window* window, const b8 should_close) {
	s_assertf(window, "se_window_set_should_close :: window is null");
	glfwSetWindowShouldClose((GLFWwindow*)window->handle, should_close);
}

void se_window_set_exit_keys(se_window* window, se_key_combo* keys) {
	s_assertf(window, "se_window_set_exit_keys :: window is null");
	s_assertf(keys, "se_window_set_exit_keys :: keys is null");
	window->exit_keys = keys;
	window->use_exit_key = false;
}

void se_window_set_exit_key(se_window* window, se_key key) {
	s_assertf(window, "se_window_set_exit_key :: window is null");
	window->exit_key = key;
	window->use_exit_key = true;
}

void se_window_check_exit_keys(se_window* window) {
	s_assertf(window, "se_window_check_exit_keys :: window is null");
	if (window->use_exit_key) {
		if (window->exit_key != SE_KEY_UNKNOWN && se_window_is_key_down(window, window->exit_key)) {
			glfwSetWindowShouldClose((GLFWwindow*)window->handle, GLFW_TRUE);
			return;
		}
	}
	if (!window->exit_keys) {
		return;
	}
	se_key_combo* keys = window->exit_keys;
	if (keys->size == 0) {
		return;
	}
	s_foreach(keys, i) {
		se_key* current_key = s_array_get(keys, i);
		if (!se_window_is_key_down(window, *current_key)) {
			return;
		}
	}
	glfwSetWindowShouldClose((GLFWwindow*)window->handle, GLFW_TRUE);
}

f64 se_window_get_delta_time(se_window* window) {
	return window->time.delta;
}

f64 se_window_get_fps(se_window* window) {
	return 1. / window->time.delta;
}

f64 se_window_get_time(se_window* window) {
	return window->time.current;
}

void se_window_set_target_fps(se_window* window, const u16 fps) {
	window->target_fps = fps;
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

i32 se_window_register_input_event(se_window* window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	s_assertf(window, "se_window_register_input_event :: window is null");
	s_assertf(box, "se_window_register_input_event :: box is null");

	se_input_event* new_event = s_array_increment(&window->input_events);
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

void se_window_update_input_event(const i32 input_event_id, se_window* window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	s_assertf(window, "se_window_update_input_event :: window is null");
	s_assertf(box, "se_window_update_input_event :: box is null");
	
	se_input_event* found_event = NULL;
	s_foreach(&window->input_events, i) {
		se_input_event* current_event = s_array_get(&window->input_events, i);
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
	}
	else {
		printf("se_window_update_input_event :: failed to find event with id: %d\n", input_event_id);
	}
}

void se_window_register_resize_event(se_window* window, se_resize_event_callback callback, void* data) {
	s_assertf(window, "se_window_register_resize_event :: window is null");
	s_assertf(callback, "se_window_register_resize_event :: callback is null");
	
	se_resize_handle* new_event = s_array_increment(&window->resize_handles);
	s_assertf(new_event, "se_window_register_resize_event :: Array is full");
	new_event->callback = callback;
	new_event->data = data;
}

void se_window_destroy(se_window* window) {
	s_assertf(window, "se_window_destroy :: window is null");
	s_assertf(window->handle, "se_window_destroy :: window->handle is null");

	se_quad_destroy(&window->quad);
	s_array_clear(&window->input_events);
	s_array_clear(&window->resize_handles);

	glfwDestroyWindow((GLFWwindow*)window->handle);
	window->handle = NULL;

	s_array_remove(&windows_container, window);
	if (s_array_get_size(&windows_container) == 0) {
		// Maybe clear up the window manager if needed
		s_array_clear(&windows_container);
		glfwTerminate();
	}
}

void se_window_destroy_all(){
	// TODO: implement single clear instead of destroying one by one 
	s_foreach_reverse(&windows_container, i) {
		se_window* window = s_array_get(&windows_container, i);
		se_window_destroy(window);
	}
}

#endif // SE_WINDOW_BACKEND_GLFW
