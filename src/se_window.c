// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_gl.h"
#include <unistd.h>
#include <stdlib.h>

//static se_windows* windows_container = NULL;

#define SE_MAX_WINDOWS 8
#define SE_MAX_KEY_COMBOS 8

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

se_window* se_window_create(const char* title, const u32 width, const u32 height) {
    glfwSetErrorCallback(gl_error_callback);
    
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return NULL;
    }

    //se_windows* windows_container = se_windows_handle_get();
    if (s_array_get_capacity(&windows_container) == 0) {
        s_array_init(&windows_container, SE_MAX_WINDOWS);
    }
    se_window* new_window = s_array_increment(&windows_container);
    if (new_window == NULL) {
        printf("Failed to create window\n");
        return NULL;
    }
    // Set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
   
    new_window->handle = glfwCreateWindow(width, height, title, NULL, NULL);
    s_assertf(new_window->handle, "Failed to create GLFW window");
    
    // TODO: figure out why this is causing errors (GLFW Error 65538: Cannot set swap interval without a current OpenGL or OpenGL ES context)
    // glfwSwapInterval(1); 
    
    new_window->width = width;
    new_window->height = height;
    
    glfwMakeContextCurrent(new_window->handle);
    glfwSetWindowUserPointer(new_window->handle, new_window);
    
    // Set callbacks
    glfwSetKeyCallback(new_window->handle, key_callback);
    glfwSetCursorPosCallback(new_window->handle, mouse_callback);
    glfwSetMouseButtonCallback(new_window->handle, mouse_button_callback);
    glfwSetFramebufferSizeCallback(new_window->handle, framebuffer_size_callback);
    
    se_init_opengl();
    
    glEnable(GL_DEPTH_TEST);
    
    create_fullscreen_quad(&new_window->quad_vao, &new_window->quad_vbo, &new_window->quad_ebo);
    
    new_window->time.current = glfwGetTime();
    new_window->time.last_frame = new_window->time.current;
    new_window->time.delta = 0;
    new_window->frame_count = 0;
    new_window->target_fps = 60;
    printf("Created window %p\n", new_window);
    return new_window;
}

extern void se_window_update(se_window* window) {
    window->time.last_frame = window->time.current;
    window->time.current = glfwGetTime();
    window->time.delta = window->time.current - window->time.last_frame;
    window->frame_count++;
}

void se_window_render_quad(se_window* window) {
    glBindVertexArray(window->quad_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
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
}

b8 se_window_is_key_down(se_window* window, i32 key) {
    return window->keys[key];
}

b8 se_window_should_close(se_window* window) {
    return glfwWindowShouldClose(window->handle);
}

void se_window_check_exit_keys(se_window* window, key_combo* keys) {
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


void se_window_destroy(se_window* window) {
    s_assertf(window, "se_window_destroy :: window is null");
    s_assertf(window->handle, "se_window_destroy :: window->handle is null");

    glDeleteVertexArrays(1, &window->quad_vao);
    glDeleteBuffers(1, &window->quad_vbo);

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

