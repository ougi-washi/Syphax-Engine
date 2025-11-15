// Syphax-Engine - Ougi Washi

#include "se_scene.h"
#include <stdlib.h>

// Scene handle is not responsible for allocating memory
// It is only used for referencing the scenes and use rendering handle to render the objects in the scenes or such.

#define SE_OBJECT_2D_VERTEX_SHADER_PATH "shaders/object_2d_vertex.glsl"

se_scene_handle* se_scene_handle_create(se_render_handle* render_handle) {
    se_scene_handle* scene_handle = (se_scene_handle*)malloc(sizeof(se_scene_handle));
    memset(scene_handle, 0, sizeof(se_scene_handle));
    
    // if render handle is null, this is a scene handle that is not used for rendering (eg. server side implementation)
    if (render_handle) { 
        scene_handle->render_handle = render_handle;
    }
    else {
        scene_handle->render_handle = NULL;
    }

    return scene_handle;
}

void se_scene_handle_cleanup(se_scene_handle* scene_handle) {
    free(scene_handle);
}

se_object_2d* se_object_2d_create(se_scene_handle* scene_handle, const c8* fragment_shader_path, const se_vec2* position, const se_vec2* scale) {
    se_object_2d* new_object = s_array_increment(&scene_handle->objects_2d);
    new_object->position = *position;
    new_object->scale = *scale;
    if (scene_handle->render_handle) {
        s_foreach(&scene_handle->render_handle->shaders, i) {
            se_shader* curr_shader = s_array_get(&scene_handle->render_handle->shaders, i);
            if (curr_shader && strcmp(curr_shader->fragment_path, fragment_shader_path) == 0) {
                new_object->shader = curr_shader;
                break;
            }
        }
        if (!new_object->shader) {
            new_object->shader = se_shader_load(scene_handle->render_handle, SE_OBJECT_2D_VERTEX_SHADER_PATH, fragment_shader_path);
        }
        s_assertf(new_object->shader, "se_object_2d_create :: failed to load shader: %s", fragment_shader_path);
    }
    else {
        new_object->shader = NULL;
    }
    
    return new_object;
}

void se_object_2d_destroy(se_scene_handle* scene_handle, se_object_2d* object) {
    s_array_remove(&scene_handle->objects_2d, se_object_2d, object);
}

void se_object_2d_set_position(se_object_2d* object, const se_vec2* position) {
    object->position = *position;
}

void se_object_2d_set_scale(se_object_2d* object, const se_vec2* scale) {
    object->scale = *scale;
}

void se_object_2d_set_shader(se_object_2d* object, se_shader* shader) {
    object->shader = shader;
}

void se_object_2d_update_uniforms(se_object_2d* object) {
    if (object->shader) {
        se_shader_set_vec2(object->shader, "u_position", &object->position);
        se_shader_set_vec2(object->shader, "u_scale", &object->scale);
    }
}

se_scene_2d* se_scene_2d_create(se_scene_handle* scene_handle, const se_vec2* size) {
    printf("Creating scene 2D\n");
    se_scene_2d* new_scene = s_array_increment(&scene_handle->scenes_2d);
    if (scene_handle->render_handle) {
        new_scene->output = se_framebuffer_create(scene_handle->render_handle, size);
    }
    else {
        new_scene->output = NULL;
    }
    return new_scene;
}

void se_scene_2d_destroy(se_scene_handle* scene_handle, se_scene_2d* scene) {
    // TODO: unsure if we should request cleanup of all render buffers and shaders
    s_array_remove(&scene_handle->scenes_2d, se_scene_2d, scene);
}

void se_scene_2d_render(se_scene_2d* scene, se_render_handle* render_handle, se_window* window) {
    if (render_handle == NULL) {
        return;
    }

    se_framebuffer_bind(scene->output);
    se_render_clear();
    se_enable_blending();
    
    s_foreach(&scene->objects, i) {
        se_object_2d_ptr* object_ptr = s_array_get(&scene->objects, i);
        if (object_ptr == NULL) {
            printf("Warning: se_scene_2d_render :: object_ptr is null\n");
            continue;
        }
        se_object_2d* current_object = *object_ptr;
        se_object_2d_update_uniforms(current_object);
        se_shader_use(render_handle, current_object->shader, true, true);
        se_window_render_quad(window);
    }
    se_disable_blending();
    se_framebuffer_unbind(scene->output);

    //se_shader_set_texture(scene->output->shader, "u_texture", scene->output->texture);
    //se_shader_use(render_handle, scene->output->shader, true);
    //se_render_buffer_bind(scene->output);
    //se_render_clear();
    //se_window_render_quad(window);
    //se_render_buffer_unbind(scene->output);

    //s_foreach(se_render_buffers_ptr, scene->render_buffers, i) {
    //    se_render_buffer_ptr* buffer_ptr = se_render_buffers_ptr_get(&scene->render_buffers, i);
    //    if (buffer_ptr == NULL) {
    //        continue;
    //    }

    //    se_render_buffer* buffer = *buffer_ptr;

    //    // render buffer
    //    se_render_buffer_bind(buffer);
    //    se_shader_use(render_handle, buffer->shader, true);
    //    se_render_clear();
    //    se_window_render_quad(window);
    //    se_render_buffer_unbind(buffer);

    //    // render output
    //    se_shader_set_texture(scene->output->shader, "u_texture", buffer->texture);
    //    se_shader_use(render_handle, scene->output->shader, true);
    //    se_render_buffer_bind(scene->output);
    //    se_render_clear();
    //    se_window_render_quad(window);
    //    se_render_buffer_unbind(scene->output);
    //}
}

void se_scene_2d_render_to_screen(se_scene_2d* scene, se_render_handle* render_handle, se_window* window) {
    if (render_handle == NULL) {
        return;
    }

    se_unbind_framebuffer();
    se_enable_blending();
    se_framebuffer_use_quad_shader(scene->output, render_handle);
    se_window_render_quad(window);
    se_disable_blending();
}

void se_scene_2d_add_object(se_scene_2d* scene, se_object_2d* object) {
    s_assertf(scene, "se_scene_2d_add_object :: scene is null");
    s_assertf(object, "se_scene_2d_add_object :: object is null");
    s_array_add(&scene->objects, object);
}

void se_scene_2d_remove_object(se_scene_2d* scene, se_object_2d* object) {
    s_assertf(scene, "se_scene_2d_remove_object :: scene is null");
    s_assertf(object, "se_scene_2d_remove_object :: object is null");
    s_array_remove(&scene->objects, se_object_2d_ptr, &object);
}

se_scene_3d* se_scene_3d_create(se_scene_handle* scene_handle, const se_vec2* size) {
    se_scene_3d* new_scene = s_array_increment(&scene_handle->scenes_3d);
    if (scene_handle->render_handle) {
        new_scene->camera = se_camera_create(scene_handle->render_handle);
    }
    else {
        new_scene->camera = NULL;
    }
    return new_scene;
}

void se_scene_3d_destroy(se_scene_handle* scene_handle, se_scene_3d* scene) {
    s_array_remove(&scene_handle->scenes_3d, se_scene_3d, scene);
}

void se_scene_3d_render(se_scene_3d* scene, se_render_handle* render_handle) {
    if (render_handle == NULL) {
        return;
    }

    s_foreach(&scene->models, i) {
        se_model_ptr* model_ptr = s_array_get(&scene->models, i);
        if (model_ptr == NULL) {
            continue;
        }
        
        se_model_render(render_handle, *model_ptr, scene->camera);
    }
    
    s_foreach(&scene->post_process, i) {
        se_render_buffer_ptr* buffer_ptr = s_array_get(&scene->post_process, i);
        if (buffer_ptr == NULL) {
            continue;
        }
        
        se_render_buffer_bind(*buffer_ptr);
        se_render_clear();
        se_render_buffer_unbind(*buffer_ptr);
    }
}

void se_scene_3d_add_model(se_scene_3d* scene, se_model* model) {
    s_array_add(&scene->models, model);
}

void se_scene_3d_remove_model(se_scene_3d* scene, se_model* model) {
    s_array_remove(&scene->models, se_model_ptr, &model);
}

void se_scene_3d_set_camera(se_scene_3d* scene, se_camera* camera) {
    scene->camera = camera;
}

void se_scene_3d_add_post_process_buffer(se_scene_3d* scene, se_render_buffer* buffer) {
    s_array_add(&scene->post_process, buffer);
}

void se_scene_3d_remove_post_process_buffer(se_scene_3d* scene, se_render_buffer* buffer) {
    s_array_remove(&scene->post_process, se_render_buffer_ptr, &buffer);
}

