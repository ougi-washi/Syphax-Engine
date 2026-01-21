// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_gl.h"
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#define SE_MAX_WINDOWS 8
#define SE_MAX_KEY_COMBOS 8
#define SE_MAX_INPUT_EVENTS 1024
#define SE_MAX_RESIZE_HANDLE 1024

static se_windows windows_container = { 0 };

static void key_callback(GLFWwindow* glfw_handle, i32 key, i32 scancode, i32 action, i32 mods) {
    se_window* window = (se_window*)glfwGetWindowUserPointer(glfw_handle);
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            window->keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            window->keys[key] = false;
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
    if (button >= 0 && button < 8) {
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
    s_foreach(&window->resize_handles, i) {
        se_resize_handle* current_event_ptr = s_array_get(&window->resize_handles, i);
        if (current_event_ptr) {
            se_resize_handle current_event = *current_event_ptr;
            current_event.callback(window, current_event.data);
        }
    }
}

// TODO: move to opengl.c or such later on
void create_fullscreen_quad(GLuint* vao, GLuint* vbo, GLuint* ebo) {
    f32 quad_vertices[] = { 
        // positions     // texCoords 
        -1.0f,  1.0f,    0.0f, 0.0f,
        -1.0f, -1.0f,    0.0f, 1.0f,
         1.0f, -1.0f,    1.0f, 1.0f,
         1.0f,  1.0f,    1.0f, 0.0f 
    };
    
    GLuint indices[] = {
        0, 1, 2,  // First triangle
        2, 3, 0   // Second triangle
    };
    
    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);
    glGenBuffers(1, ebo);
    
    glBindVertexArray(*vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute (location = 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)(2 * sizeof(f32)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void gl_error_callback(i32 error, const c8* description) {
    printf("GLFW Error %d: %s\n", error, description);
}

se_window* se_window_create(se_render_handle* render_handle, const char* title, const u32 width, const u32 height) {
    s_assertf(render_handle, "se_window_create :: render_handle is null");
    s_assertf(title, "se_window_create :: title is null");

    glfwSetErrorCallback(gl_error_callback);
    
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return NULL;
    }

    if (s_array_get_capacity(&windows_container) == 0) {
        s_array_init(&windows_container, SE_MAX_WINDOWS);
    }
    se_window* new_window = s_array_increment(&windows_container);
    s_assertf(new_window, "Failed to create window");
    new_window->render_handle = render_handle;

    if (new_window == NULL) {
        printf("Failed to create window\n");
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
    s_assertf(new_window->handle, "Failed to create GLFW window");
    
    
    new_window->width = width;
    new_window->height = height;
    
    glfwMakeContextCurrent(new_window->handle);
    glfwSetWindowUserPointer(new_window->handle, new_window);
    glfwSwapInterval(0); 
    
    // Set callbacks
    glfwSetKeyCallback(new_window->handle, key_callback);
    glfwSetCursorPosCallback(new_window->handle, mouse_callback);
    glfwSetMouseButtonCallback(new_window->handle, mouse_button_callback);
    glfwSetFramebufferSizeCallback(new_window->handle, framebuffer_size_callback);
    
    se_init_opengl();
    
    glEnable(GL_DEPTH_TEST);
    
    //create_fullscreen_quad(&new_window->quad_vao, &new_window->quad_vbo, &new_window->quad_ebo);
    se_quad_2d_create(&new_window->quad, 0);
    new_window->shader = se_shader_load(render_handle, "shaders/render_quad_vert.glsl", "shaders/render_quad_frag.glsl");

    new_window->time.current = glfwGetTime();
    new_window->time.last_frame = new_window->time.current;
    new_window->time.delta = 0;
    new_window->frame_count = 0;
    new_window->target_fps = 60;
    printf("se_window_create :: created window %p\n", new_window);
    return new_window;
}

extern void se_window_update(se_window* window) {
    se_window_check_exit_keys(window);
    window->time.last_frame = window->time.current;
    window->time.current = glfwGetTime();
    window->time.delta = window->time.current - window->time.last_frame;
    window->frame_count++;
}

void se_window_render_quad(se_window* window) {
    //glBindVertexArray(window->quad_vao);
    //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    //glBindVertexArray(0);
    se_shader_use(window->render_handle, window->shader, true, false);
    se_quad_render(&window->quad, 0);
}

void se_window_render_screen(se_window* window) {
    f64 time_left = 1. / window->target_fps - window->time.delta;
    if (time_left > 0) {
        usleep(time_left * 1000000);
    }
    glfwSwapBuffers(window->handle);
}

void se_window_poll_events(){
    glfwPollEvents();
    s_foreach(&windows_container, i) {
        se_window* window = s_array_get(&windows_container, i);
        se_vec2 mouse_position = {0};
        se_window_get_mouse_position_normalized(window, &mouse_position);
        se_input_event* out_event = NULL;
        i32 out_depth = INT_MIN;
        b8 interacted = false;
        s_foreach(&window->input_events, j) {
            se_input_event* current_event = s_array_get(&window->input_events, j);
            if (    current_event->active &&
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

b8 se_window_is_key_down(se_window* window, i32 key) {
    s_assertf(window, "se_window_is_key_down :: window is null");
    return window->keys[key];
}

b8 se_window_is_mouse_down(se_window* window, i32 button) {
    s_assertf(window, "se_window_is_mouse_down :: window is null");
    return window->mouse_buttons[button];
}

void se_window_get_mouse_position_normalized(se_window* window, se_vec2* out_mouse_position) {
    s_assertf(window, "se_window_get_mouse_position_normalized :: window is null");
    s_assertf(out_mouse_position, "se_window_get_mouse_position_normalized :: out_mouse_position is null");
    *out_mouse_position = se_vec2((window->mouse_x / window->width) - .5, window->mouse_y / window->height - .5);
    out_mouse_position->x *= 2.;
    out_mouse_position->y *= 2.;
}

b8 se_window_should_close(se_window* window) {
    s_assertf(window, "se_window_should_close :: window is null");
    return glfwWindowShouldClose(window->handle);
}

void se_window_set_should_close(se_window* window, const b8 should_close) {
    s_assertf(window, "se_window_set_should_close :: window is null");
    glfwSetWindowShouldClose(window->handle, should_close);
}

void se_window_set_exit_keys(se_window* window, se_key_combo* keys) {
    s_assertf(window, "se_window_set_exit_keys :: window is null");
    s_assertf(keys, "se_window_set_exit_keys :: keys is null");
    window->exit_keys = keys;
}

void se_window_check_exit_keys(se_window* window) {
    s_assertf(window, "se_window_check_exit_keys :: window is null");
    if (!window->exit_keys) {
        return;
    }
    se_key_combo* keys = window->exit_keys;
    if (keys->size == 0) {
        return;
    }
    s_foreach(keys, i) {
        i32* current_key = s_array_get(keys, i);
        if (!se_window_is_key_down(window, *current_key)) {
            return;
        }
    }
    glfwSetWindowShouldClose(window->handle, GLFW_TRUE);
}

f64 se_window_get_delta_time(se_window* window) {
    return window->time.delta;
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

    //glDeleteVertexArrays(1, &window->quad_vao);
    //glDeleteBuffers(1, &window->quad_vbo);
    se_quad_destroy(&window->quad);

    glfwDestroyWindow(window->handle);
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

