// Syphax-Engine - Ougi Washi

#include "se_scene.h"
#include <stdlib.h>

// Scene handle is not responsible for allocating memory
// It is only used for referencing the scenes and use rendering handle to render the objects in the scenes or such.

#define SE_OBJECT_2D_VERTEX_SHADER_PATH "shaders/object_2d_vertex.glsl"
#define SE_OBJECT_2D_VERTEX_INSTANCED_SHADER_PATH "shaders/object_2d_vertex_instanced.glsl"

se_scene_handle* se_scene_handle_create(se_render_handle* render_handle, const se_scene_handle_params* params) {
    se_scene_handle* scene_handle = (se_scene_handle*)malloc(sizeof(se_scene_handle));
    memset(scene_handle, 0, sizeof(se_scene_handle));
    
    s_array_init(&scene_handle->objects_2d, params->objects_2d_count);
    s_array_init(&scene_handle->objects_3d, params->objects_3d_count);
    s_array_init(&scene_handle->scenes_2d, params->scenes_2d_count);
    s_array_init(&scene_handle->scenes_3d, params->scenes_3d_count);
    
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
    s_foreach(&scene_handle->objects_2d, i) {
        se_object_2d* current_object = s_array_get(&scene_handle->objects_2d, i);
        se_object_2d_destroy(scene_handle, current_object);
    }
    s_array_clear(&scene_handle->objects_2d);
   
    // TODO: destroy 3D objects
    //s_foreach(&scene_handle->objects_3d, i) {
    //    se_object_3d* current_object = s_array_get(&scene_handle->objects_3d, i);
    //    se_object_3d_destroy(scene_handle, current_object);
    //}
    s_array_clear(&scene_handle->objects_3d);
    
    s_foreach(&scene_handle->scenes_2d, i) {
        se_scene_2d* current_scene = s_array_get(&scene_handle->scenes_2d, i);
        se_scene_2d_destroy(scene_handle, current_scene);
    }
    s_array_clear(&scene_handle->scenes_2d);
    
    s_foreach(&scene_handle->scenes_3d, i) {
        se_scene_3d* current_scene = s_array_get(&scene_handle->scenes_3d, i);
        se_scene_3d_destroy(scene_handle, current_scene);
    }
    free(scene_handle);
}

se_object_2d* se_object_2d_create(se_scene_handle* scene_handle, const c8* fragment_shader_path, const se_vec2* position, const se_vec2* scale, const sz max_instances_count) {
    se_object_2d* new_object = s_array_increment(&scene_handle->objects_2d);
    se_quad_2d_create(&new_object->quad);
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
            new_object->shader = se_shader_load(scene_handle->render_handle, 
                    max_instances_count > 0 ? SE_OBJECT_2D_VERTEX_INSTANCED_SHADER_PATH : SE_OBJECT_2D_VERTEX_SHADER_PATH,
                    fragment_shader_path);
        }
        s_assertf(new_object->shader, "se_object_2d_create :: failed to load shader: %s", fragment_shader_path);
    }
    else {
        new_object->shader = NULL;
    }
   
    if (max_instances_count > 0) {
        s_array_init(&new_object->instances.ids, max_instances_count);
        s_array_init(&new_object->instances.transforms, max_instances_count);
        s_array_init(&new_object->instances.buffers, max_instances_count);
        // max_instances_count * 2 because we need to store the transform and buffer matrices
        s_array_init(&new_object->quad.instance_buffers, max_instances_count * 2);
        se_quad_2d_add_instance_buffer(&new_object->quad, new_object->instances.transforms.data, max_instances_count);
        se_quad_2d_add_instance_buffer(&new_object->quad, new_object->instances.buffers.data, max_instances_count);
    }
    return new_object;
}

void se_scene_2d_resize_callback(void* window, void* scene) {
    s_assertf(window, "se_scene_2d_resize_callback :: window is null");
    s_assertf(scene, "se_scene_2d_resize_callback :: scene is null");

    se_window* window_ptr = (se_window*)window;
    se_scene_2d* scene_ptr = (se_scene_2d*)scene;
    s_assertf(window_ptr, "se_scene_2d_resize_callback :: window_ptr is null");
    s_assertf(scene_ptr, "se_scene_2d_resize_callback :: scene_ptr is null");
    
    se_framebuffer_ptr framebuffer = scene_ptr->output;
    s_assertf(framebuffer, "se_scene_2d_resize_callback :: framebuffer is null");
    se_vec2 old_size = {0};
    se_framebuffer_get_size(framebuffer, &old_size);
    se_vec2 new_size = {framebuffer->ratio.x * window_ptr->width, framebuffer->ratio.y * window_ptr->height};
   
    // TODO: update object positions and scales
    //s_foreach(&scene_ptr->objects, i) {
    //    se_object_2d_ptr* current_object_ptr = s_array_get(&scene_ptr->objects, i);
    //    if (current_object_ptr == NULL || *current_object_ptr == NULL) {
    //        continue;
    //    }
    //    se_object_2d* current_object = *current_object_ptr;
    //    se_vec2* current_position = &current_object->position;
    //    se_vec2* current_scale = &current_object->scale;
    //    se_object_2d_set_position(current_object, &se_vec2(current_position->x * new_size.x / old_size.x, current_position->y * new_size.y / old_size.y));
    //    se_object_2d_set_scale(current_object, &se_vec2(current_scale->x * new_size.x / old_size.x, current_scale->y * new_size.y / old_size.y));
    //}

    se_framebuffer_set_size(framebuffer, &new_size);
}

void se_scene_2d_set_auto_resize(se_scene_2d* scene, se_window* window, const se_vec2* ratio) {
    s_assertf(scene, "se_scene_2d_set_auto_resize :: scene is null");
    s_assertf(window, "se_scene_2d_set_auto_resize :: window is null");
    s_assertf(ratio, "se_scene_2d_set_auto_resize :: ratio is null");
  
    // ratio can only be 1 for now as we are not updating the object positions and scales to match the new ratio
    s_assertf(ratio->x == 1.0 && ratio->y == 1.0, "se_scene_2d_set_auto_resize :: ratio can only be 1 for now, please wait for the next update");
    scene->output->auto_resize = true;
    scene->output->ratio = *ratio;
    se_window_register_resize_event(window, se_scene_2d_resize_callback, scene);
}

// The reason why we are passing the scene is because the size will depend on the buffer size
void se_object_2d_get_box_2d(se_object_2d* object, se_box_2d* out_box) {
    s_assertf(object, "se_object_2d_get_box_2d :: object is null");
    s_assertf(out_box, "se_object_2d_get_box_2d :: out_box is null");
    
    out_box->min = se_vec2(object->position.x - object->scale.x, object->position.y - object->scale.y);
    out_box->max = se_vec2(object->position.x + object->scale.x, object->position.y + object->scale.y);
}

void se_object_2d_destroy(se_scene_handle* scene_handle, se_object_2d* object) {
    s_assertf(scene_handle, "se_object_2d_destroy :: scene_handle is null");
    s_assertf(object, "se_object_2d_destroy :: object is null");
    s_array_remove(&scene_handle->objects_2d, object);
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

se_instance_id se_object_2d_add_instance(se_object_2d* object, const se_mat4* transform, const se_mat4* buffer) {
    s_assertf(object, "se_object_2d_set_instance_add :: object is null");
    s_assertf(transform, "se_object_2d_set_instance_add :: transform is null");
    s_assertf(buffer, "se_object_2d_set_instance_add :: buffer is null");
    s_assertf(s_array_get_capacity(&object->instances.ids) > 0, "se_object_2d_set_instance_add :: object instances capacity is 0");
    se_instance_id* new_instance_id = s_array_increment(&object->instances.ids);
    se_mat4* new_transform = s_array_increment(&object->instances.transforms);
    se_mat4* new_buffer = s_array_increment(&object->instances.buffers);
    *new_transform = *transform;
    *new_buffer = *buffer;
    se_object_2d_set_instances_dirty(object, true);
    
    return *new_instance_id;
}

i32 se_object_2d_get_instance_index(se_object_2d* object, const se_instance_id instance_id) {
    s_assertf(object, "se_object_2d_get_instance_index :: object is null");
    s_assertf(s_array_get_capacity(&object->instances.ids) > 0, "se_object_2d_get_instance_index :: object instances capacity is 0");
    s_foreach(&object->instances.ids, i) {
        se_instance_id* found_instance = s_array_get(&object->instances.ids, i);
        if (*found_instance == instance_id) {
            return i;
        }
    }
    return -1;
}

void se_object_2d_set_instance_transform(se_object_2d* object, const se_instance_id instance_id, const se_mat4* transform) {
    s_assertf(object, "se_object_2d_set_instance_transform :: object is null");
    s_assertf(transform, "se_object_2d_set_instance_transform :: transform is null");
    const i32 index = se_object_2d_get_instance_index(object, instance_id);
    if (index >= 0) {
        se_mat4* current_transform = s_array_get(&object->instances.transforms, index);
        *current_transform = *transform;
        se_print_mat4(current_transform);
        se_object_2d_set_instances_dirty(object, true);
    }
    else {
        printf("se_object_2d_set_instance_transform :: instance id %d not found\n", instance_id);
    }
}

void se_object_2d_set_instance_buffer(se_object_2d* object, const se_instance_id instance_id, const se_mat4* buffer) {
    s_assertf(object, "se_object_2d_set_instance_buffer :: object is null");
    s_assertf(buffer, "se_object_2d_set_instance_buffer :: buffer is null");
    const i32 index = se_object_2d_get_instance_index(object, instance_id);
    if (index >= 0) {
        se_mat4* current_buffer = s_array_get(&object->instances.buffers, index);
        *current_buffer = *buffer;
        se_object_2d_set_instances_dirty(object, true);
    }
    else {
        printf("se_object_2d_set_instance_buffer :: instance id %d not found\n", instance_id);
    }
}

void se_object_2d_set_instances_transforms(se_object_2d* object, const se_transforms* transforms) {
    s_assertf(object, "se_object_2d_set_instances_transforms :: object is null");
    s_assertf(transforms, "se_object_2d_set_instances_transforms :: transforms is null");
    s_assertf(s_array_get_capacity(&object->instances.transforms) == s_array_get_capacity(transforms), "se_object_2d_set_instances_transforms :: transforms capacity is not equal to object capacity");
    object->instances.transforms.size = s_array_get_size(transforms);
    *object->instances.transforms.data = *(transforms->data);
    se_object_2d_set_instances_dirty(object, true);
}

void se_object_2d_set_instances_buffers(se_object_2d* object, const se_buffers* buffers) {
    s_assertf(object, "se_object_2d_set_instances_buffers :: object is null");
    s_assertf(buffers, "se_object_2d_set_instances_buffers :: buffers is null");
    s_assertf(s_array_get_capacity(&object->instances.buffers) == s_array_get_capacity(buffers), "se_object_2d_set_instances_buffers :: buffers capacity is not equal to object capacity");
    object->instances.buffers.size = s_array_get_size(buffers);
    *object->instances.buffers.data = *(buffers->data);
    se_object_2d_set_instances_dirty(object, true);
}

void se_object_2d_set_instances_dirty(se_object_2d* object, const b8 dirty) {
    s_assertf(object, "se_object_2d_set_instances_dirty :: object is null");
    object->quad.instance_buffers_dirty = dirty;
}

b8 se_object_2d_are_instances_dirty(se_object_2d* object) {
    s_assertf(object, "se_object_2d_are_instances_dirty :: object is null");
    return object->quad.instance_buffers_dirty;
}

sz se_object_2d_get_instance_count(se_object_2d* object) {
    s_assertf(object, "se_object_2d_get_instance_count :: object is null");
    return s_array_get_size(&object->instances.ids);
}

se_scene_2d* se_scene_2d_create(se_scene_handle* scene_handle, const se_vec2* size, const u16 object_count) {
    printf("Creating scene 2D\n");
    s_assertf(scene_handle, "se_scene_2d_create :: scene_handle is null");
    s_assertf(size, "se_scene_2d_create :: size is null");
    s_assertf(object_count > 0, "se_scene_2d_create :: object_count is 0");
    se_scene_2d* new_scene = s_array_increment(&scene_handle->scenes_2d);
    if (scene_handle->render_handle) {
        new_scene->output = se_framebuffer_create(scene_handle->render_handle, size);
        s_array_init(&new_scene->objects, object_count);
    }
    else {
        new_scene->output = NULL;
    }
    return new_scene;
}

void se_scene_2d_destroy(se_scene_handle* scene_handle, se_scene_2d* scene) {
    s_assertf(scene_handle, "se_scene_2d_destroy :: scene_handle is null");
    s_assertf(scene, "se_scene_2d_destroy :: scene is null");
    // TODO: unsure if we should request cleanup of all render buffers and shaders
    s_array_remove(&scene_handle->scenes_2d, scene);
}

void se_scene_2d_render(se_scene_2d* scene, se_render_handle* render_handle) {
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
        const sz instance_count = se_object_2d_get_instance_count(current_object);
        se_quad_render(&current_object->quad, instance_count);
    }
    se_disable_blending();
    se_framebuffer_unbind(scene->output);
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
    s_array_remove(&scene->objects, &object);
}

se_scene_3d* se_scene_3d_create(se_scene_handle* scene_handle, const se_vec2* size, const u16 object_count) {
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
    s_array_remove(&scene_handle->scenes_3d, scene);
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
    s_array_remove(&scene->models, &model);
}

void se_scene_3d_set_camera(se_scene_3d* scene, se_camera* camera) {
    scene->camera = camera;
}

void se_scene_3d_add_post_process_buffer(se_scene_3d* scene, se_render_buffer* buffer) {
    s_array_add(&scene->post_process, buffer);
}

void se_scene_3d_remove_post_process_buffer(se_scene_3d* scene, se_render_buffer* buffer) {
    s_array_remove(&scene->post_process, &buffer);
}

