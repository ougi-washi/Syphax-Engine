// Syphax-Engine - Ougi Washi

#include "se_scene.h"
#include "se_gl.h"
#include <stdlib.h>
#include <string.h>

// Scene handle is not responsible for allocating memory
// It is only used for referencing the scenes and use rendering handle to render
// the objects in the scenes or such.

#define SE_OBJECT_2D_VERTEX_SHADER_PATH "shaders/object_2d_vertex.glsl"

se_scene_handle *se_scene_handle_create(se_render_handle *render_handle, const se_scene_handle_params *params) {
	se_scene_handle_params resolved = SE_SCENE_HANDLE_PARAMS_DEFAULTS;
	if (params) {
		resolved = *params;
		if (resolved.objects_2d_count == 0) {
			resolved.objects_2d_count = SE_SCENE_HANDLE_PARAMS_DEFAULTS.objects_2d_count;
		}
		if (resolved.objects_3d_count == 0) {
			resolved.objects_3d_count = SE_SCENE_HANDLE_PARAMS_DEFAULTS.objects_3d_count;
		}
		if (resolved.scenes_2d_count == 0) {
			resolved.scenes_2d_count = SE_SCENE_HANDLE_PARAMS_DEFAULTS.scenes_2d_count;
		}
		if (resolved.scenes_3d_count == 0) {
			resolved.scenes_3d_count = SE_SCENE_HANDLE_PARAMS_DEFAULTS.scenes_3d_count;
		}
	}
	se_scene_handle *scene_handle = (se_scene_handle *)malloc(sizeof(se_scene_handle));
	memset(scene_handle, 0, sizeof(se_scene_handle));

	s_array_init(&scene_handle->objects_2d, resolved.objects_2d_count);
	s_array_init(&scene_handle->objects_3d, resolved.objects_3d_count);
	s_array_init(&scene_handle->scenes_2d, resolved.scenes_2d_count);
	s_array_init(&scene_handle->scenes_3d, resolved.scenes_3d_count);

	// if render handle is null, this is a scene handle that is not used for
	// rendering (eg. server side implementation)
	if (render_handle) {
		scene_handle->render_handle = render_handle;
	} else {
		scene_handle->render_handle = NULL;
	}

	return scene_handle;
}

void se_scene_handle_cleanup(se_scene_handle *scene_handle) {
	s_assertf(scene_handle, "se_scene_handle_cleanup :: scene_handle is null");
	printf("se_scene_handle_cleanup :: scene_handle: %p\n", scene_handle);
	s_foreach(&scene_handle->objects_2d, i) {
		se_object_2d *current_object = s_array_get(&scene_handle->objects_2d, i);
		se_object_2d_destroy(scene_handle, current_object);
	}
	s_array_clear(&scene_handle->objects_2d);

	s_foreach(&scene_handle->objects_3d, i) {
		se_object_3d *current_object = s_array_get(&scene_handle->objects_3d, i);
		se_object_3d_destroy(scene_handle, current_object);
	}
	s_array_clear(&scene_handle->objects_3d);

	s_foreach(&scene_handle->scenes_2d, i) {
		se_scene_2d *current_scene = s_array_get(&scene_handle->scenes_2d, i);
		se_scene_2d_destroy(scene_handle, current_scene);
	}
	s_array_clear(&scene_handle->scenes_2d);

	s_foreach(&scene_handle->scenes_3d, i) {
		se_scene_3d *current_scene = s_array_get(&scene_handle->scenes_3d, i);
		se_scene_3d_destroy(scene_handle, current_scene);
	}
	free(scene_handle);
}

se_object_2d *se_object_2d_create(se_scene_handle *scene_handle, const c8 *fragment_shader_path, const s_mat3 *transform, const sz max_instances_count) {
	s_assertf(scene_handle, "se_object_2d_create :: scene_handle is null");
	s_assertf(fragment_shader_path, "se_object_2d_create :: fragment_shader_path is null");
	s_assertf(transform, "se_object_2d_create :: transform is null");
	if (s_array_get_capacity(&scene_handle->objects_2d) == s_array_get_size(&scene_handle->objects_2d)) {
		printf("se_object_2d_create :: scene_handle->objects_2d is full\n");
		return NULL;
	}
	se_object_2d *new_object = s_array_increment(&scene_handle->objects_2d);
	const sz instance_capacity = (max_instances_count > 0) ? max_instances_count : 1;
	se_quad_2d_create(&new_object->quad, (u32)(instance_capacity * 2));
	new_object->transform = *transform;
	new_object->is_custom = false;
	new_object->is_visible = true;
    if (scene_handle->render_handle) {
		s_foreach(&scene_handle->render_handle->shaders, i) {
		    se_shader *curr_shader = s_array_get(&scene_handle->render_handle->shaders, i);
		    if (curr_shader && strcmp(curr_shader->fragment_path, fragment_shader_path) == 0) {
		    	new_object->shader = curr_shader;
		    	break;
		    }
		}
		if (!new_object->shader) {
		    new_object->shader = se_shader_load(
		                    scene_handle->render_handle,
		                    SE_OBJECT_2D_VERTEX_SHADER_PATH,
			            fragment_shader_path);
		}
		s_assertf(new_object->shader, "se_object_2d_create :: failed to load shader: %s", fragment_shader_path);
    } 
    else {
		new_object->shader = NULL;
	}

	s_array_init(&new_object->instances.ids, instance_capacity);
	s_array_init(&new_object->instances.transforms, instance_capacity);
	s_array_init(&new_object->instances.buffers, instance_capacity);
	s_array_init(&new_object->render_transforms, instance_capacity);
	se_quad_2d_add_instance_buffer_mat3(&new_object->quad,
							 new_object->render_transforms.data,
							 instance_capacity);
	se_quad_2d_add_instance_buffer(&new_object->quad,
							 new_object->instances.buffers.data,
							 instance_capacity);
	if (max_instances_count == 0) {
		se_instance_id *new_instance_id = s_array_increment(&new_object->instances.ids);
		s_mat3 *new_transform = s_array_increment(&new_object->instances.transforms);
		s_mat4 *new_buffer = s_array_increment(&new_object->instances.buffers);
		s_mat3 *new_render_transform = s_array_increment(&new_object->render_transforms);
		*new_instance_id = 0;
		*new_transform = s_mat3_identity;
		*new_buffer = s_mat4_identity;
		*new_render_transform = *transform;
		se_object_2d_set_instances_dirty(new_object, true);
	}
	return new_object;
}

se_object_2d *se_object_2d_create_custom(se_scene_handle *scene_handle, se_object_custom *custom, const s_mat3 *transform) {
	s_assertf(scene_handle, "se_object_2d_create_custom :: scene_handle is null");
	s_assertf(custom, "se_object_2d_create_custom :: custom is null");
	s_assertf(transform, "se_object_2d_create_custom :: transform is null");
	s_assertf(custom->data_size <= SE_OBJECT_CUSTOM_DATA_SIZE, "se_object_2d_create_custom :: data_size exceeds SE_OBJECT_CUSTOM_DATA_SIZE");
	se_object_2d *new_object = s_array_increment(&scene_handle->objects_2d);
	new_object->transform = *transform;
	new_object->is_custom = true;
	new_object->is_visible = true;
	memcpy(&new_object->custom, custom, sizeof(se_object_custom));
	return new_object;
}

void se_object_custom_set_data(se_object_custom *custom, const void *data, const sz size) {
	s_assertf(custom, "se_object_custom_set_data :: custom is null");
	s_assertf(data, "se_object_custom_set_data :: data is null");
	s_assertf(size <= SE_OBJECT_CUSTOM_DATA_SIZE, "se_object_custom_set_data :: size exceeds SE_OBJECT_CUSTOM_DATA_SIZE");
	memcpy(custom->data, data, size);
	custom->data_size = size;
}

void se_scene_2d_resize_callback(void *window, void *scene) {
	s_assertf(window, "se_scene_2d_resize_callback :: window is null");
	s_assertf(scene, "se_scene_2d_resize_callback :: scene is null");

	se_window *window_ptr = (se_window *)window;
	se_scene_2d *scene_ptr = (se_scene_2d *)scene;
	s_assertf(window_ptr, "se_scene_2d_resize_callback :: window_ptr is null");
	s_assertf(scene_ptr, "se_scene_2d_resize_callback :: scene_ptr is null");

	se_render_handle *render_handle_ptr = window_ptr->render_handle;
	s_assertf(render_handle_ptr, "se_scene_2d_resize_callback :: render_handle_ptr is null");

	se_framebuffer_ptr framebuffer = scene_ptr->output;
	s_assertf(framebuffer, "se_scene_2d_resize_callback :: framebuffer is null");
	s_vec2 old_size = {0};
	se_framebuffer_get_size(framebuffer, &old_size);
	s_vec2 new_size = {framebuffer->ratio.x * window_ptr->width,
						framebuffer->ratio.y * window_ptr->height};

	// TODO: update object positions and scales
	// s_foreach(&scene_ptr->objects, i) {
	//	se_object_2d_ptr* current_object_ptr = s_array_get(&scene_ptr->objects,
	//	i); if (current_object_ptr == NULL || *current_object_ptr == NULL) {
	//		continue;
	//	}
	//	se_object_2d* current_object = *current_object_ptr;
	//	s_vec2* current_position = &current_object->position;
	//	s_vec2* current_scale = &current_object->scale;
	//	se_object_2d_set_position(current_object, &s_vec2(current_position->x *
	//	new_size.x / old_size.x, current_position->y * new_size.y /
	//	old_size.y)); se_object_2d_set_scale(current_object,
	//	&s_vec2(current_scale->x * new_size.x / old_size.x, current_scale->y *
	//	new_size.y / old_size.y));
	//}

	se_framebuffer_set_size(framebuffer, &new_size);
	se_scene_2d_render_to_buffer(scene, render_handle_ptr);
}

void se_scene_2d_set_auto_resize(se_scene_2d *scene, se_window *window, const s_vec2 *ratio) {
	s_assertf(scene, "se_scene_2d_set_auto_resize :: scene is null");
	s_assertf(window, "se_scene_2d_set_auto_resize :: window is null");
	s_assertf(ratio, "se_scene_2d_set_auto_resize :: ratio is null");

	// ratio can only be 1 for now as we are not updating the object positions and
	// scales to match the new ratio
	s_assertf(ratio->x == 1.0 && ratio->y == 1.0,
				"se_scene_2d_set_auto_resize :: ratio can only be 1 for now, "
				"please wait for the next update");
	scene->output->auto_resize = true;
	scene->output->ratio = *ratio;
	se_window_register_resize_event(window, se_scene_2d_resize_callback, scene);
}

void se_scene_3d_resize_callback(void *window, void *scene) {
	s_assertf(window, "se_scene_3d_resize_callback :: window is null");
	s_assertf(scene, "se_scene_3d_resize_callback :: scene is null");

	se_window *window_ptr = (se_window *)window;
	se_scene_3d *scene_ptr = (se_scene_3d *)scene;
	s_assertf(window_ptr, "se_scene_3d_resize_callback :: window_ptr is null");
	s_assertf(scene_ptr, "se_scene_3d_resize_callback :: scene_ptr is null");

	se_render_handle *render_handle_ptr = window_ptr->render_handle;
	s_assertf(render_handle_ptr, "se_scene_3d_resize_callback :: render_handle_ptr is null");

	se_framebuffer_ptr framebuffer = scene_ptr->output;
	s_assertf(framebuffer, "se_scene_3d_resize_callback :: framebuffer is null");

	s_vec2 new_size = {framebuffer->ratio.x * window_ptr->width,
						framebuffer->ratio.y * window_ptr->height};

	se_framebuffer_set_size(framebuffer, &new_size);
	if (scene_ptr->camera) {
		se_camera_set_aspect(scene_ptr->camera, new_size.x, new_size.y);
	}
	se_scene_3d_render_to_buffer(scene_ptr, render_handle_ptr);
}

void se_scene_3d_set_auto_resize(se_scene_3d *scene, se_window *window, const s_vec2 *ratio) {
	s_assertf(scene, "se_scene_3d_set_auto_resize :: scene is null");
	s_assertf(window, "se_scene_3d_set_auto_resize :: window is null");
	s_assertf(ratio, "se_scene_3d_set_auto_resize :: ratio is null");

	s_assertf(ratio->x == 1.0 && ratio->y == 1.0,
			"se_scene_3d_set_auto_resize :: ratio can only be 1 for now, "
			"please wait for the next update");
	scene->output->auto_resize = true;
	scene->output->ratio = *ratio;
	se_window_register_resize_event(window, se_scene_3d_resize_callback, scene);
}

void se_object_2d_destroy(se_scene_handle *scene_handle, se_object_2d *object) {
	s_assertf(scene_handle, "se_object_2d_destroy :: scene_handle is null");
	s_assertf(object, "se_object_2d_destroy :: object is null");
	printf("se_object_2d_destroy :: scene_handle: %p, object: %p\n", scene_handle, object);
	if (!object->is_custom) {
		se_quad_destroy(&object->quad);
	}
	s_array_clear(&object->render_transforms);
	s_array_clear(&object->instances.ids);
	s_array_clear(&object->instances.transforms);
	s_array_clear(&object->instances.buffers);
	s_array_remove(&scene_handle->objects_2d, object);
}

void se_object_2d_set_transform(se_object_2d *object, const s_mat3 *transform) {
	s_assertf(object, "se_object_2d_set_transform :: object is null");
	s_assertf(transform, "se_object_2d_set_transform :: transform is null");
	object->transform = *transform;
	s_foreach(&object->instances.transforms, i) {
		s_mat3 *current_transform = s_array_get(&object->instances.transforms, i);
		s_mat3 *current_render_transform = s_array_get(&object->render_transforms, i);
		*current_render_transform = s_mat3_mul(&object->transform, current_transform);
	}
	se_object_2d_set_instances_dirty(object, true);
}

s_mat3 se_object_2d_get_transform(se_object_2d *object) {
	s_assertf(object, "se_object_2d_get_transform :: object is null");
	return object->transform;
}

void se_object_2d_set_position(se_object_2d *object, const s_vec2 *position) {
	s_assertf(object, "se_object_2d_set_position :: object is null");
	s_assertf(position, "se_object_2d_set_position :: position is null");
    s_mat3_set_translation(&object->transform, position);
	s_foreach(&object->instances.transforms, i) {
		s_mat3 *current_transform = s_array_get(&object->instances.transforms, i);
		s_mat3 *current_render_transform = s_array_get(&object->render_transforms, i);
		*current_render_transform = s_mat3_mul(&object->transform, current_transform);
	}
	se_object_2d_set_instances_dirty(object, true);
}

s_vec2 se_object_2d_get_position(se_object_2d *object) {
	s_assertf(object, "se_object_2d_get_position :: object is null");
    s_vec2 out_pos = s_mat3_get_translation(&object->transform);
    return out_pos;
}

void se_object_2d_set_scale(se_object_2d *object, const s_vec2 *scale) {
	s_assertf(object, "se_object_2d_set_scale :: object is null");
	s_assertf(scale, "se_object_2d_set_scale :: scale is null");
    s_mat3_set_scale(&object->transform, scale);
	s_foreach(&object->instances.transforms, i) {
		s_mat3 *current_transform = s_array_get(&object->instances.transforms, i);
		s_mat3 *current_render_transform = s_array_get(&object->render_transforms, i);
		*current_render_transform = s_mat3_mul(&object->transform, current_transform);
	}
	se_object_2d_set_instances_dirty(object, true);
}

s_vec2 se_object_2d_get_scale(se_object_2d *object) {
	s_assertf(object, "se_object_2d_get_scale :: object is null");
    s_vec2 out_scale = s_mat3_get_scale(&object->transform);
    return out_scale;
}

void se_object_2d_get_box_2d(se_object_2d *object, se_box_2d *out_box) {
	s_assertf(object, "se_object_2d_get_box_2d :: object is null");
	s_assertf(out_box, "se_object_2d_get_box_2d :: out_box is null");
    se_box_2d_make(out_box, &object->transform);
}

void se_object_2d_set_shader(se_object_2d *object, se_shader *shader) {
	object->shader = shader;
}

void se_object_2d_update_uniforms(se_object_2d *object) {
	(void)object;
}

se_instance_id se_object_2d_add_instance(se_object_2d *object, const s_mat3 *transform, const s_mat4 *buffer) {
	s_assertf(object, "se_object_2d_set_instance_add :: object is null");
	s_assertf(transform, "se_object_2d_set_instance_add :: transform is null");
	s_assertf(buffer, "se_object_2d_set_instance_add :: buffer is null");
	s_assertf(s_array_get_capacity(&object->instances.ids) > 0, "se_object_2d_set_instance_add :: object instances capacity is 0");

	const sz prev_instances_count = s_array_get_size(&object->instances.ids);
	se_instance_id new_id = 0;
	if (prev_instances_count > 0) {
		new_id = *s_array_get(&object->instances.ids, prev_instances_count - 1) + 1;
	}

	se_instance_id *new_instance_id = s_array_increment(&object->instances.ids);
	s_mat3 *new_transform = s_array_increment(&object->instances.transforms);
	s_mat4 *new_buffer = s_array_increment(&object->instances.buffers);
	s_mat3 *new_render_transform = s_array_increment(&object->render_transforms);

	*new_instance_id = new_id;
	*new_transform = *transform;
	*new_buffer = *buffer;
	*new_render_transform = s_mat3_mul(&object->transform, transform);
	se_object_2d_set_instances_dirty(object, true);

	return *new_instance_id;
}

i32 se_object_2d_get_instance_index(se_object_2d *object, const se_instance_id instance_id) {
	s_assertf(object, "se_object_2d_get_instance_index :: object is null");
	s_assertf(s_array_get_capacity(&object->instances.ids) > 0, "se_object_2d_get_instance_index :: object instances capacity is 0");
	s_foreach(&object->instances.ids, i) {
		se_instance_id *found_instance = s_array_get(&object->instances.ids, i);
		if (*found_instance == instance_id) {
		return i;
		}
	}
	return -1;
}

void se_object_2d_set_instance_transform(se_object_2d *object, const se_instance_id instance_id, const s_mat3 *transform) {
	s_assertf(object, "se_object_2d_set_instance_transform :: object is null");
	s_assertf(transform, "se_object_2d_set_instance_transform :: transform is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat3 *current_transform =
			s_array_get(&object->instances.transforms, index);
		*current_transform = *transform;
		s_mat3 *current_render_transform = s_array_get(&object->render_transforms, index);
		*current_render_transform = s_mat3_mul(&object->transform, current_transform);
		se_object_2d_set_instances_dirty(object, true);
	} else {
		printf("se_object_2d_set_instance_transform :: instance id %d not found\n",
			 instance_id);
	}
}

void se_object_2d_set_instance_buffer(se_object_2d *object, const se_instance_id instance_id, const s_mat4 *buffer) {
	s_assertf(object, "se_object_2d_set_instance_buffer :: object is null");
	s_assertf(buffer, "se_object_2d_set_instance_buffer :: buffer is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat4 *current_buffer = s_array_get(&object->instances.buffers, index);
		*current_buffer = *buffer;
		se_object_2d_set_instances_dirty(object, true);
	} else {
		printf("se_object_2d_set_instance_buffer :: instance id %d not found\n",
			 instance_id);
	}
}

void se_object_2d_set_instances_transforms(se_object_2d *object, const se_transforms_2d *transforms) {
	s_assertf(object, "se_object_2d_set_instances_transforms :: object is null");
	s_assertf(transforms, "se_object_2d_set_instances_transforms :: transforms is null");
	s_assertf(s_array_get_capacity(&object->instances.transforms) == s_array_get_capacity(transforms), "se_object_2d_set_instances_transforms :: transforms capacity is not equal to object capacity");
	object->instances.transforms.size = s_array_get_size(transforms);
	*object->instances.transforms.data = *(transforms->data);
	object->render_transforms.size = object->instances.transforms.size;
	s_foreach(&object->instances.transforms, i) {
		s_mat3 *current_transform = s_array_get(&object->instances.transforms, i);
		s_mat3 *current_render_transform = s_array_get(&object->render_transforms, i);
		*current_render_transform = s_mat3_mul(&object->transform, current_transform);
	}
	se_object_2d_set_instances_dirty(object, true);
}

void se_object_2d_set_instances_buffers(se_object_2d *object, const se_buffers *buffers) {
	s_assertf(object, "se_object_2d_set_instances_buffers :: object is null");
	s_assertf(buffers, "se_object_2d_set_instances_buffers :: buffers is null");
	s_assertf(s_array_get_capacity(&object->instances.buffers) == s_array_get_capacity(buffers), "se_object_2d_set_instances_buffers :: buffers capacity is not equal to object capacity");
	object->instances.buffers.size = s_array_get_size(buffers);
	*object->instances.buffers.data = *(buffers->data);
	se_object_2d_set_instances_dirty(object, true);
}

void se_object_2d_set_instances_dirty(se_object_2d *object, const b8 dirty) {
	s_assertf(object, "se_object_2d_set_instances_dirty :: object is null");
	object->quad.instance_buffers_dirty = dirty;
}

b8 se_object_2d_are_instances_dirty(se_object_2d *object) {
	s_assertf(object, "se_object_2d_are_instances_dirty :: object is null");
	return object->quad.instance_buffers_dirty;
}

sz se_object_2d_get_instance_count(se_object_2d *object) {
	s_assertf(object, "se_object_2d_get_instance_count :: object is null");
	return s_array_get_size(&object->instances.ids);
}

se_object_3d *se_object_3d_create(se_scene_handle *scene_handle, se_model *model, const s_mat4 *transform, const sz max_instances_count) {
	s_assertf(scene_handle, "se_object_3d_create :: scene_handle is null");
	s_assertf(model, "se_object_3d_create :: model is null");
	s_assertf(transform, "se_object_3d_create :: transform is null");
	if (s_array_get_capacity(&scene_handle->objects_3d) == s_array_get_size(&scene_handle->objects_3d)) {
		printf("se_object_3d_create :: scene_handle->objects_3d is full\n");
		return NULL;
	}

	se_object_3d *new_object = s_array_increment(&scene_handle->objects_3d);
	memset(new_object, 0, sizeof(se_object_3d));
	new_object->model = model;
	new_object->transform = *transform;
	new_object->is_visible = true;
	const sz instance_capacity = (max_instances_count > 0) ? max_instances_count : 1;
	s_array_init(&new_object->instances.ids, instance_capacity);
	s_array_init(&new_object->instances.transforms, instance_capacity);
	s_array_init(&new_object->instances.buffers, instance_capacity);
	s_array_init(&new_object->render_transforms, instance_capacity);

	const sz mesh_count = s_array_get_size(&model->meshes);
	if (mesh_count > 0) {
		s_array_init(&new_object->mesh_instances, mesh_count);
		s_foreach(&model->meshes, i) {
			se_mesh *mesh = s_array_get(&model->meshes, i);
			se_mesh_instance *mesh_instance = s_array_increment(&new_object->mesh_instances);
			se_mesh_instance_create(mesh_instance, mesh, (u32)instance_capacity);
			se_mesh_instance_add_buffer(mesh_instance, new_object->render_transforms.data, instance_capacity);
		}
	}

	if (max_instances_count == 0) {
		se_instance_id *new_instance_id = s_array_increment(&new_object->instances.ids);
		s_mat4 *new_transform = s_array_increment(&new_object->instances.transforms);
		s_mat4 *new_buffer = s_array_increment(&new_object->instances.buffers);
		s_mat4 *new_render_transform = s_array_increment(&new_object->render_transforms);
		*new_instance_id = 0;
		*new_transform = s_mat4_identity;
		*new_buffer = s_mat4_identity;
		*new_render_transform = s_mat4_identity;
		se_object_3d_set_instances_dirty(new_object, true);
	}

	return new_object;
}

void se_object_3d_destroy(se_scene_handle *scene_handle, se_object_3d *object) {
	s_assertf(scene_handle, "se_object_3d_destroy :: scene_handle is null");
	s_assertf(object, "se_object_3d_destroy :: object is null");
	s_foreach(&object->mesh_instances, i) {
		se_mesh_instance *mesh_instance = s_array_get(&object->mesh_instances, i);
		se_mesh_instance_destroy(mesh_instance);
	}
	s_array_clear(&object->mesh_instances);
	s_array_clear(&object->render_transforms);
	s_array_clear(&object->instances.ids);
	s_array_clear(&object->instances.transforms);
	s_array_clear(&object->instances.buffers);
	s_array_remove(&scene_handle->objects_3d, object);
}

void se_object_3d_set_transform(se_object_3d *object, const s_mat4 *transform) {
	s_assertf(object, "se_object_3d_set_transform :: object is null");
	s_assertf(transform, "se_object_3d_set_transform :: transform is null");
	object->transform = *transform;
}

s_mat4 se_object_3d_get_transform(se_object_3d *object) {
	s_assertf(object, "se_object_3d_get_transform :: object is null");
	return object->transform;
}

se_instance_id se_object_3d_add_instance(se_object_3d *object, const s_mat4 *transform, const s_mat4 *buffer) {
	s_assertf(object, "se_object_3d_add_instance :: object is null");
	s_assertf(transform, "se_object_3d_add_instance :: transform is null");
	s_assertf(buffer, "se_object_3d_add_instance :: buffer is null");
	s_assertf(s_array_get_capacity(&object->instances.ids) > 0, "se_object_3d_add_instance :: object instances capacity is 0");

	const sz prev_instances_count = s_array_get_size(&object->instances.ids);
	se_instance_id new_id = 0;
	if (prev_instances_count > 0) {
		new_id = *s_array_get(&object->instances.ids, prev_instances_count - 1) + 1;
	}

	se_instance_id *new_instance_id = s_array_increment(&object->instances.ids);
	s_mat4 *new_transform = s_array_increment(&object->instances.transforms);
	s_mat4 *new_buffer = s_array_increment(&object->instances.buffers);
	s_mat4 *new_render_transform = s_array_increment(&object->render_transforms);

	*new_instance_id = new_id;
	*new_transform = *transform;
	*new_buffer = *buffer;
	*new_render_transform = s_mat4_identity;
	se_object_3d_set_instances_dirty(object, true);

	return *new_instance_id;
}

i32 se_object_3d_get_instance_index(se_object_3d *object, const se_instance_id instance_id) {
	s_assertf(object, "se_object_3d_get_instance_index :: object is null");
	s_assertf(s_array_get_capacity(&object->instances.ids) > 0, "se_object_3d_get_instance_index :: object instances capacity is 0");
	s_foreach(&object->instances.ids, i) {
		se_instance_id *found_instance = s_array_get(&object->instances.ids, i);
		if (*found_instance == instance_id) {
			return i;
		}
	}
	return -1;
}

void se_object_3d_set_instance_transform(se_object_3d *object, const se_instance_id instance_id, const s_mat4 *transform) {
	s_assertf(object, "se_object_3d_set_instance_transform :: object is null");
	s_assertf(transform, "se_object_3d_set_instance_transform :: transform is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat4 *current_transform = s_array_get(&object->instances.transforms, index);
		*current_transform = *transform;
		se_object_3d_set_instances_dirty(object, true);
	} else {
		printf("se_object_3d_set_instance_transform :: instance id %d not found\n", instance_id);
	}
}

void se_object_3d_set_instance_buffer(se_object_3d *object, const se_instance_id instance_id, const s_mat4 *buffer) {
	s_assertf(object, "se_object_3d_set_instance_buffer :: object is null");
	s_assertf(buffer, "se_object_3d_set_instance_buffer :: buffer is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat4 *current_buffer = s_array_get(&object->instances.buffers, index);
		*current_buffer = *buffer;
		se_object_3d_set_instances_dirty(object, true);
	} else {
		printf("se_object_3d_set_instance_buffer :: instance id %d not found\n", instance_id);
	}
}

void se_object_3d_set_instances_dirty(se_object_3d *object, const b8 dirty) {
	s_assertf(object, "se_object_3d_set_instances_dirty :: object is null");
	s_foreach(&object->mesh_instances, i) {
		se_mesh_instance *mesh_instance = s_array_get(&object->mesh_instances, i);
		mesh_instance->instance_buffers_dirty = dirty;
	}
}

b8 se_object_3d_are_instances_dirty(se_object_3d *object) {
	s_assertf(object, "se_object_3d_are_instances_dirty :: object is null");
	if (s_array_get_size(&object->mesh_instances) == 0) {
		return false;
	}
	se_mesh_instance *mesh_instance = s_array_get(&object->mesh_instances, 0);
	return mesh_instance->instance_buffers_dirty;
}

sz se_object_3d_get_instance_count(se_object_3d *object) {
	s_assertf(object, "se_object_3d_get_instance_count :: object is null");
	return s_array_get_size(&object->instances.ids);
}

se_scene_2d *se_scene_2d_create(se_scene_handle *scene_handle,
									const s_vec2 *size, const u16 object_count) {
	s_assertf(scene_handle->render_handle,
				"se_scene_2d_create :: scene_handle->render_handle is null");
	s_assertf(scene_handle, "se_scene_2d_create :: scene_handle is null");
	s_assertf(size, "se_scene_2d_create :: size is null");
	s_assertf(object_count > 0, "se_scene_2d_create :: object_count is 0");
	se_scene_2d *new_scene = s_array_increment(&scene_handle->scenes_2d);
	new_scene->output = se_framebuffer_create(scene_handle->render_handle, size);
	s_array_init(&new_scene->objects, object_count);
	printf("se_scene_2d_create :: created scene 2D %p\n", new_scene);
	return new_scene;
}

void se_scene_2d_destroy(se_scene_handle *scene_handle, se_scene_2d *scene) {
	s_assertf(scene_handle, "se_scene_2d_destroy :: scene_handle is null");
	s_assertf(scene, "se_scene_2d_destroy :: scene is null");
	if (scene->output) {
		se_framebuffer_cleanup(scene->output);
		scene->output = NULL;
	}
	s_array_clear(&scene->objects);
	s_array_remove(&scene_handle->scenes_2d, scene);
}

void se_scene_2d_bind(se_scene_2d *scene) {
	s_assertf(scene, "se_scene_2d_bind :: scene is null");
	se_framebuffer_bind(scene->output);
	se_enable_blending();
}

void se_scene_2d_unbind(se_scene_2d *scene) {
	s_assertf(scene, "se_scene_2d_unbind :: scene is null");
	se_framebuffer_unbind(scene->output);
	se_disable_blending();
}

void se_scene_2d_render_raw(se_scene_2d *scene, se_render_handle *render_handle) {
	s_assertf(scene, "se_scene_2d_render_raw :: scene is null");
	s_assertf(render_handle, "se_scene_2d_render_raw :: render_handle is null");

	se_render_clear();
		s_foreach(&scene->objects, i) {
			se_object_2d_ptr *current_object_2d_ptr = s_array_get(&scene->objects, i);
			if (current_object_2d_ptr == NULL) {
			    printf("se_scene_2d_render_to_buffer :: current_object_2d_ptr is null\n");
			    continue;
			}
			se_object_2d *current_object_2d = *current_object_2d_ptr;
			if (current_object_2d) {
			    if (current_object_2d->is_custom) {
			    	current_object_2d->custom.render(render_handle, current_object_2d->custom.data);
			    } else {
			    	se_shader_use(render_handle, current_object_2d->shader, true, true);
			    	const sz instance_count = se_object_2d_get_instance_count(current_object_2d);
			    	se_quad_render(&current_object_2d->quad, instance_count);
			    }
			}
		}
}

void se_scene_2d_render_to_buffer(se_scene_2d *scene, se_render_handle *render_handle) {
	s_assertf(scene, "se_scene_2d_render_to_buffer :: scene is null");
	s_assertf(render_handle, "se_scene_2d_render_to_buffer :: render_handle is null");

	se_scene_2d_bind(scene);
	se_scene_2d_render_raw(scene, render_handle);
	se_scene_2d_unbind(scene);
}

void se_scene_2d_render_to_screen(se_scene_2d *scene, se_render_handle *render_handle, se_window *window) {
	if (render_handle == NULL) {
		return;
	}

	se_unbind_framebuffer();
	se_enable_blending();
	se_shader_set_texture(window->shader, "u_texture", scene->output->texture);
	se_window_render_quad(window);
	se_disable_blending();
}

void se_scene_2d_draw(se_scene_2d *scene, se_render_handle *render_handle, se_window *window) {
	se_scene_2d_render_to_buffer(scene, render_handle);
	se_render_clear();
	se_scene_2d_render_to_screen(scene, render_handle, window);
	se_window_render_screen(window);
}

void se_scene_2d_add_object(se_scene_2d *scene, se_object_2d *object) {
	s_assertf(scene, "se_scene_2d_add_object :: scene is null");
	s_assertf(object, "se_scene_2d_add_object :: object is null");
	printf("se_scene_2d_add_object :: scene: %p, object: %p\n", scene, object);
	s_array_add(&scene->objects, object);
}

void se_scene_2d_remove_object(se_scene_2d *scene, se_object_2d *object) {
	s_assertf(scene, "se_scene_2d_remove_object :: scene is null");
	s_assertf(object, "se_scene_2d_remove_object :: object is null");
	printf("se_scene_2d_remove_object :: scene: %p, object: %p\n", scene, object);
	s_array_remove(&scene->objects, &object);
}

se_scene_3d *se_scene_3d_create(se_scene_handle *scene_handle, const s_vec2 *size, const u16 object_count) {
	s_assertf(scene_handle, "se_scene_3d_create :: scene_handle is null");
	s_assertf(scene_handle->render_handle,
			"se_scene_3d_create :: scene_handle->render_handle is null");
	s_assertf(size, "se_scene_3d_create :: size is null");
	s_assertf(object_count > 0, "se_scene_3d_create :: object_count is 0");
	se_scene_3d *new_scene = s_array_increment(&scene_handle->scenes_3d);
	new_scene->output = se_framebuffer_create(scene_handle->render_handle, size);
	s_array_init(&new_scene->objects, object_count);
	s_array_init(&new_scene->post_process, object_count);
	new_scene->output_shader = NULL;
	new_scene->camera = se_camera_create(scene_handle->render_handle);
	new_scene->enable_culling = true;
	if (new_scene->camera) {
		se_camera_set_aspect(new_scene->camera, size->x, size->y);
	}
	printf("se_scene_3d_create :: created scene 3D %p\n", new_scene);
	return new_scene;
}

void se_scene_3d_destroy(se_scene_handle *scene_handle, se_scene_3d *scene) {
	printf("se_scene_3d_destroy :: scene: %p\n", scene);
	if (scene->camera && scene_handle->render_handle) {
		se_camera_destroy(scene_handle->render_handle, scene->camera);
		scene->camera = NULL;
	}
	if (scene->output) {
		se_framebuffer_cleanup(scene->output);
		scene->output = NULL;
	}
	s_array_clear(&scene->post_process);
	s_array_clear(&scene->objects);
	s_array_remove(&scene_handle->scenes_3d, scene);
}

void se_scene_3d_render_to_buffer(se_scene_3d *scene, se_render_handle *render_handle) {
	if (render_handle == NULL) {
		return;
	}
	s_assertf(scene, "se_scene_3d_render_to_buffer :: scene is null");
	s_assertf(scene->output, "se_scene_3d_render_to_buffer :: scene output is null");

	se_framebuffer_bind(scene->output);
	se_render_clear();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	if (scene->enable_culling) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
	} else {
		glDisable(GL_CULL_FACE);
	}

	s_mat4 view = s_mat4_identity;
	s_mat4 proj = s_mat4_identity;
	if (scene->camera) {
		view = se_camera_get_view_matrix(scene->camera);
		proj = se_camera_get_projection_matrix(scene->camera);
	}
	s_mat4 vp = s_mat4_mul(&proj, &view);

	s_foreach(&scene->objects, i) {
		se_object_3d_ptr *object_ptr = s_array_get(&scene->objects, i);
		if (object_ptr == NULL || *object_ptr == NULL) {
			continue;
		}
		se_object_3d *object = *object_ptr;
		if (!object->is_visible || object->model == NULL) {
			continue;
		}

		const sz instance_count = se_object_3d_get_instance_count(object);
		const sz mesh_count = s_array_get_size(&object->mesh_instances);
		sz mesh_index = 0;
		s_foreach(&object->model->meshes, m) {
			se_mesh *mesh = s_array_get(&object->model->meshes, m);
			if (mesh_index >= mesh_count) {
				break;
			}
			se_mesh_instance *mesh_instance = s_array_get(&object->mesh_instances, mesh_index);
			mesh_index++;
			if (mesh_instance == NULL) {
				continue;
			}

			s_foreach(&object->instances.transforms, j) {
				s_mat4 *instance_transform = s_array_get(&object->instances.transforms, j);
				s_mat4 *out_buffer = s_array_get(&object->render_transforms, j);
				s_mat4 model = s_mat4_mul(&object->transform, instance_transform);
				model = s_mat4_mul(&model, &mesh->matrix);
				*out_buffer = s_mat4_mul(&vp, &model);
			}
			mesh_instance->instance_buffers_dirty = true;
			se_mesh_instance_update(mesh_instance);

			se_shader *shader = mesh->shader;
			if (!shader) {
				continue;
			}
			if (mesh->texture_id != 0) {
				se_shader_set_texture(shader, "u_texture", mesh->texture_id);
			}

			se_shader_use(render_handle, shader, true, true);
			glBindVertexArray(mesh_instance->vao);
			glDrawElementsInstanced(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0, (GLsizei)instance_count);
		}
	}
	se_framebuffer_unbind(scene->output);
}

void se_scene_3d_render_to_screen(se_scene_3d *scene, se_render_handle *render_handle, se_window *window) {
	if (render_handle == NULL) {
		return;
	}

	se_unbind_framebuffer();
	se_render_clear();
	se_enable_blending();
	se_shader_set_texture(window->shader, "u_texture", scene->output->texture);
	se_window_render_quad(window);
	se_disable_blending();
}

void se_scene_3d_draw(se_scene_3d *scene, se_render_handle *render_handle, se_window *window) {
	se_scene_3d_render_to_buffer(scene, render_handle);
	se_scene_3d_render_to_screen(scene, render_handle, window);
	se_window_render_screen(window);
}

void se_scene_3d_add_object(se_scene_3d *scene, se_object_3d *object) {
	s_assertf(scene, "se_scene_3d_add_object :: scene is null");
	s_assertf(object, "se_scene_3d_add_object :: object is null");
	printf("se_scene_3d_add_object :: scene: %p, object: %p\n", scene, object);
	s_array_add(&scene->objects, object);
}

void se_scene_3d_remove_object(se_scene_3d *scene, se_object_3d *object) {
	s_assertf(scene, "se_scene_3d_remove_object :: scene is null");
	s_assertf(object, "se_scene_3d_remove_object :: object is null");
	printf("se_scene_3d_remove_object :: scene: %p, object: %p\n", scene, object);
	s_array_remove(&scene->objects, &object);
}

void se_scene_3d_set_camera(se_scene_3d *scene, se_camera *camera) {
	scene->camera = camera;
}

void se_scene_3d_set_culling(se_scene_3d *scene, const b8 enabled) {
	s_assertf(scene, "se_scene_3d_set_culling :: scene is null");
	scene->enable_culling = enabled;
}

void se_scene_3d_add_post_process_buffer(se_scene_3d *scene, se_render_buffer *buffer) {
	s_array_add(&scene->post_process, buffer);
}

void se_scene_3d_remove_post_process_buffer(se_scene_3d *scene, se_render_buffer *buffer) {
	s_array_remove(&scene->post_process, &buffer);
}
