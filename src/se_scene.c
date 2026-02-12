// Syphax-Engine - Ougi Washi

#include "se_scene.h"
#include "render/se_gl.h"
#include <stdlib.h>
#include <string.h>

// Scene/object memory is owned by the se_context arrays.

#define SE_OBJECT_2D_VERTEX_SHADER_PATH SE_RESOURCE_INTERNAL("shaders/object_2d_vertex.glsl")

static se_object_2d* se_object_2d_from_handle(se_context *ctx, const se_object_2d_handle object) {
	return s_array_get(&ctx->objects_2d, object);
}

static se_object_3d* se_object_3d_from_handle(se_context *ctx, const se_object_3d_handle object) {
	return s_array_get(&ctx->objects_3d, object);
}

static se_scene_2d* se_scene_2d_from_handle(se_context *ctx, const se_scene_2d_handle scene) {
	return s_array_get(&ctx->scenes_2d, scene);
}

static se_scene_3d* se_scene_3d_from_handle(se_context *ctx, const se_scene_3d_handle scene) {
	return s_array_get(&ctx->scenes_3d, scene);
}

static se_model* se_model_from_handle(se_context *ctx, const se_model_handle model) {
	return s_array_get(&ctx->models, model);
}

se_object_2d_handle se_object_2d_create(const c8 *fragment_shader_path, const s_mat3 *transform, const sz max_instances_count) {
	se_context *ctx = se_current_context();
	if (!ctx || !fragment_shader_path || !transform) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->objects_2d) == 0) {
		s_array_init(&ctx->objects_2d);
	}
	if (s_array_get_size(&ctx->objects_2d) >= SE_MAX_2D_OBJECTS) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_object_2d_handle object_handle = s_array_increment(&ctx->objects_2d);
	se_object_2d *new_object = s_array_get(&ctx->objects_2d, object_handle);
	memset(new_object, 0, sizeof(*new_object));
	const sz instance_capacity = (max_instances_count > 0) ? max_instances_count : 1;
	se_quad_2d_create(&new_object->quad, (u32)(instance_capacity * 2));
	new_object->transform = *transform;
	new_object->is_custom = false;
	new_object->is_visible = true;
	new_object->shader = S_HANDLE_NULL;
	for (sz i = 0; i < s_array_get_size(&ctx->shaders); ++i) {
		se_shader_handle shader_handle = s_array_handle(&ctx->shaders, (u32)i);
		se_shader *curr_shader = s_array_get(&ctx->shaders, shader_handle);
		if (curr_shader && strcmp(curr_shader->fragment_path, fragment_shader_path) == 0) {
			new_object->shader = shader_handle;
			break;
		}
	}
	if (new_object->shader == S_HANDLE_NULL) {
		se_shader_handle shader = se_shader_load(
			SE_OBJECT_2D_VERTEX_SHADER_PATH,
			fragment_shader_path);
		if (shader == S_HANDLE_NULL) {
			se_quad_destroy(&new_object->quad);
			memset(new_object, 0, sizeof(*new_object));
			s_array_remove(&ctx->objects_2d, object_handle);
			return S_HANDLE_NULL;
		}
		new_object->shader = shader;
	}

	s_array_init(&new_object->instances.ids);
	s_array_init(&new_object->instances.transforms);
	s_array_init(&new_object->instances.buffers);
	s_array_init(&new_object->render_transforms);
	s_array_reserve(&new_object->instances.ids, instance_capacity);
	s_array_reserve(&new_object->instances.transforms, instance_capacity);
	s_array_reserve(&new_object->instances.buffers, instance_capacity);
	s_array_reserve(&new_object->render_transforms, instance_capacity);
	se_quad_2d_add_instance_buffer_mat3(&new_object->quad,
							 s_array_get_data(&new_object->render_transforms),
							 instance_capacity);
	se_quad_2d_add_instance_buffer(&new_object->quad,
							 s_array_get_data(&new_object->instances.buffers),
							 instance_capacity);
	if (max_instances_count == 0) {
		s_handle id_handle = s_array_increment(&new_object->instances.ids);
		s_handle transform_handle = s_array_increment(&new_object->instances.transforms);
		s_handle buffer_handle = s_array_increment(&new_object->instances.buffers);
		s_handle render_handle = s_array_increment(&new_object->render_transforms);
		se_instance_id *new_instance_id = s_array_get(&new_object->instances.ids, id_handle);
		s_mat3 *new_transform = s_array_get(&new_object->instances.transforms, transform_handle);
		s_mat4 *new_buffer = s_array_get(&new_object->instances.buffers, buffer_handle);
		s_mat3 *new_render_transform = s_array_get(&new_object->render_transforms, render_handle);
		*new_instance_id = 0;
		*new_transform = s_mat3_identity;
		*new_buffer = s_mat4_identity;
		*new_render_transform = *transform;
		se_object_2d_set_instances_dirty(object_handle, true);
	}
	se_set_last_error(SE_RESULT_OK);
	return object_handle;
}

se_object_2d_handle se_object_2d_create_custom(se_object_custom *custom, const s_mat3 *transform) {
	se_context *ctx = se_current_context();
	if (!ctx || !custom || !transform) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	s_assertf(custom->data_size <= SE_OBJECT_CUSTOM_DATA_SIZE, "se_object_2d_create_custom :: data_size exceeds SE_OBJECT_CUSTOM_DATA_SIZE");
	if (s_array_get_capacity(&ctx->objects_2d) == 0) {
		s_array_init(&ctx->objects_2d);
	}
	if (s_array_get_size(&ctx->objects_2d) >= SE_MAX_2D_OBJECTS) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_object_2d_handle object_handle = s_array_increment(&ctx->objects_2d);
	se_object_2d *new_object = s_array_get(&ctx->objects_2d, object_handle);
	memset(new_object, 0, sizeof(*new_object));
	new_object->transform = *transform;
	new_object->is_custom = true;
	new_object->is_visible = true;
	memcpy(&new_object->custom, custom, sizeof(se_object_custom));
	se_set_last_error(SE_RESULT_OK);
	return object_handle;
}

void se_object_custom_set_data(se_object_custom *custom, const void *data, const sz size) {
	s_assertf(custom, "se_object_custom_set_data :: custom is null");
	s_assertf(data, "se_object_custom_set_data :: data is null");
	s_assertf(size <= SE_OBJECT_CUSTOM_DATA_SIZE, "se_object_custom_set_data :: size exceeds SE_OBJECT_CUSTOM_DATA_SIZE");
	memcpy(custom->data, data, size);
	custom->data_size = size;
}

void se_object_2d_destroy(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_object_2d_destroy :: ctx is null");
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_destroy :: object is null");
	if (!object_ptr->is_custom) {
		se_quad_destroy(&object_ptr->quad);
		object_ptr->quad.vao = 0;
		object_ptr->quad.vbo = 0;
		object_ptr->quad.ebo = 0;
		object_ptr->shader = S_HANDLE_NULL;
	}
	s_array_clear(&object_ptr->render_transforms);
	s_array_clear(&object_ptr->instances.ids);
	s_array_clear(&object_ptr->instances.transforms);
	s_array_clear(&object_ptr->instances.buffers);
	object_ptr->is_visible = false;
	if (object_ptr->is_custom) {
		object_ptr->custom.render = NULL;
		object_ptr->custom.data_size = 0;
	}

	s_array_remove(&ctx->objects_2d, object);
}

void se_object_2d_set_transform(const se_object_2d_handle object, const s_mat3 *transform) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_transform :: object is null");
	s_assertf(transform, "se_object_2d_set_transform :: transform is null");
	object_ptr->transform = *transform;
	for (sz i = 0; i < s_array_get_size(&object_ptr->instances.transforms); ++i) {
		s_mat3 *current_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)i));
		s_mat3 *current_render_transform = s_array_get(&object_ptr->render_transforms, s_array_handle(&object_ptr->render_transforms, (u32)i));
		*current_render_transform = s_mat3_mul(&object_ptr->transform, current_transform);
	}
	se_object_2d_set_instances_dirty(object, true);
}

s_mat3 se_object_2d_get_transform(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_transform :: object is null");
	return object_ptr->transform;
}

void se_object_2d_set_position(const se_object_2d_handle object, const s_vec2 *position) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_position :: object is null");
	s_assertf(position, "se_object_2d_set_position :: position is null");
	s_mat3_set_translation(&object_ptr->transform, position);
	for (sz i = 0; i < s_array_get_size(&object_ptr->instances.transforms); ++i) {
		s_mat3 *current_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)i));
		s_mat3 *current_render_transform = s_array_get(&object_ptr->render_transforms, s_array_handle(&object_ptr->render_transforms, (u32)i));
		*current_render_transform = s_mat3_mul(&object_ptr->transform, current_transform);
	}
	se_object_2d_set_instances_dirty(object, true);
}

s_vec2 se_object_2d_get_position(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_position :: object is null");
	s_vec2 out_pos = s_mat3_get_translation(&object_ptr->transform);
	return out_pos;
}

void se_object_2d_set_scale(const se_object_2d_handle object, const s_vec2 *scale) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_scale :: object is null");
	s_assertf(scale, "se_object_2d_set_scale :: scale is null");
	s_mat3_set_scale(&object_ptr->transform, scale);
	for (sz i = 0; i < s_array_get_size(&object_ptr->instances.transforms); ++i) {
		s_mat3 *current_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)i));
		s_mat3 *current_render_transform = s_array_get(&object_ptr->render_transforms, s_array_handle(&object_ptr->render_transforms, (u32)i));
		*current_render_transform = s_mat3_mul(&object_ptr->transform, current_transform);
	}
	se_object_2d_set_instances_dirty(object, true);
}

s_vec2 se_object_2d_get_scale(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_scale :: object is null");
	s_vec2 out_scale = s_mat3_get_scale(&object_ptr->transform);
	return out_scale;
}

void se_object_2d_get_box_2d(const se_object_2d_handle object, se_box_2d *out_box) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_box_2d :: object is null");
	s_assertf(out_box, "se_object_2d_get_box_2d :: out_box is null");
	se_box_2d_make(out_box, &object_ptr->transform);
}

void se_object_2d_set_shader(const se_object_2d_handle object, const se_shader_handle shader) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	object_ptr->shader = shader;
}

se_shader_handle se_object_2d_get_shader(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_shader :: object is null");
	if (object_ptr->is_custom) {
		return S_HANDLE_NULL;
	}
	return object_ptr->shader;
}

void se_object_2d_update_uniforms(const se_object_2d_handle object) {
	(void)object;
}

se_instance_id se_object_2d_add_instance(const se_object_2d_handle object, const s_mat3 *transform, const s_mat4 *buffer) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_instance_add :: object is null");
	s_assertf(transform, "se_object_2d_set_instance_add :: transform is null");
	s_assertf(buffer, "se_object_2d_set_instance_add :: buffer is null");
	if (s_array_get_size(&object_ptr->instances.ids) >= s_array_get_capacity(&object_ptr->instances.ids)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return -1;
	}

	const sz prev_instances_count = s_array_get_size(&object_ptr->instances.ids);
	se_instance_id new_id = 0;
	if (prev_instances_count > 0) {
		se_instance_id *last_id = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)(prev_instances_count - 1)));
		new_id = *last_id + 1;
	}

	s_handle id_handle = s_array_increment(&object_ptr->instances.ids);
	s_handle transform_handle = s_array_increment(&object_ptr->instances.transforms);
	s_handle buffer_handle = s_array_increment(&object_ptr->instances.buffers);
	s_handle render_handle = s_array_increment(&object_ptr->render_transforms);
	se_instance_id *new_instance_id = s_array_get(&object_ptr->instances.ids, id_handle);
	s_mat3 *new_transform = s_array_get(&object_ptr->instances.transforms, transform_handle);
	s_mat4 *new_buffer = s_array_get(&object_ptr->instances.buffers, buffer_handle);
	s_mat3 *new_render_transform = s_array_get(&object_ptr->render_transforms, render_handle);

	*new_instance_id = new_id;
	*new_transform = *transform;
	*new_buffer = *buffer;
	*new_render_transform = s_mat3_mul(&object_ptr->transform, transform);
	se_object_2d_set_instances_dirty(object, true);

	return *new_instance_id;
}

i32 se_object_2d_get_instance_index(const se_object_2d_handle object, const se_instance_id instance_id) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_instance_index :: object is null");
	for (sz i = 0; i < s_array_get_size(&object_ptr->instances.ids); ++i) {
		se_instance_id *found_instance = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)i));
		if (*found_instance == instance_id) {
			return (i32)i;
		}
	}
	return -1;
}

void se_object_2d_set_instance_transform(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat3 *transform) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_instance_transform :: object is null");
	s_assertf(transform, "se_object_2d_set_instance_transform :: transform is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat3 *current_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)index));
		*current_transform = *transform;
		s_mat3 *current_render_transform = s_array_get(&object_ptr->render_transforms, s_array_handle(&object_ptr->render_transforms, (u32)index));
		*current_render_transform = s_mat3_mul(&object_ptr->transform, current_transform);
		se_object_2d_set_instances_dirty(object, true);
	} else {
		printf("se_object_2d_set_instance_transform :: instance id %d not found\n", instance_id);
	}
}

void se_object_2d_set_instance_buffer(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat4 *buffer) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_instance_buffer :: object is null");
	s_assertf(buffer, "se_object_2d_set_instance_buffer :: buffer is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat4 *current_buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)index));
		*current_buffer = *buffer;
		se_object_2d_set_instances_dirty(object, true);
	} else {
		printf("se_object_2d_set_instance_buffer :: instance id %d not found\n", instance_id);
	}
}

void se_object_2d_set_instances_transforms(const se_object_2d_handle object, const se_transforms_2d *transforms) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_instances_transforms :: object is null");
	s_assertf(transforms, "se_object_2d_set_instances_transforms :: transforms is null");

	const sz count = s_array_get_size(transforms);
	s_array_clear(&object_ptr->instances.transforms);
	s_array_clear(&object_ptr->render_transforms);
	s_array_init(&object_ptr->instances.transforms);
	s_array_init(&object_ptr->render_transforms);
	s_array_reserve(&object_ptr->instances.transforms, count);
	s_array_reserve(&object_ptr->render_transforms, count);
	for (sz i = 0; i < count; ++i) {
		const se_transforms_2d *src_transforms = transforms;
		const s_mat3 *src_transform = s_array_get((se_transforms_2d *)src_transforms, s_array_handle((se_transforms_2d *)src_transforms, (u32)i));
		s_handle dst_handle = s_array_increment(&object_ptr->instances.transforms);
		s_handle render_handle = s_array_increment(&object_ptr->render_transforms);
		s_mat3 *dst_transform = s_array_get(&object_ptr->instances.transforms, dst_handle);
		s_mat3 *dst_render = s_array_get(&object_ptr->render_transforms, render_handle);
		*dst_transform = *src_transform;
		*dst_render = s_mat3_mul(&object_ptr->transform, dst_transform);
	}
	se_object_2d_set_instances_dirty(object, true);
}

void se_object_2d_set_instances_buffers(const se_object_2d_handle object, const se_buffers *buffers) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_instances_buffers :: object is null");
	s_assertf(buffers, "se_object_2d_set_instances_buffers :: buffers is null");

	const sz count = s_array_get_size(buffers);
	s_array_clear(&object_ptr->instances.buffers);
	s_array_init(&object_ptr->instances.buffers);
	s_array_reserve(&object_ptr->instances.buffers, count);
	for (sz i = 0; i < count; ++i) {
		const se_buffers *src_buffers = buffers;
		const s_mat4 *src_buffer = s_array_get((se_buffers *)src_buffers, s_array_handle((se_buffers *)src_buffers, (u32)i));
		s_handle dst_handle = s_array_increment(&object_ptr->instances.buffers);
		s_mat4 *dst_buffer = s_array_get(&object_ptr->instances.buffers, dst_handle);
		*dst_buffer = *src_buffer;
	}
	se_object_2d_set_instances_dirty(object, true);
}

void se_object_2d_set_instances_dirty(const se_object_2d_handle object, const b8 dirty) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_instances_dirty :: object is null");
	object_ptr->quad.instance_buffers_dirty = dirty;
}

b8 se_object_2d_are_instances_dirty(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_are_instances_dirty :: object is null");
	return object_ptr->quad.instance_buffers_dirty;
}

sz se_object_2d_get_instance_count(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_instance_count :: object is null");
	return s_array_get_size(&object_ptr->instances.ids);
}

se_scene_2d_handle se_scene_2d_create(const s_vec2 *size, const u16 object_count) {
	se_context *ctx = se_current_context();
	if (!ctx || !size || object_count == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->scenes_2d) == 0) {
		s_array_init(&ctx->scenes_2d);
	}
	if (s_array_get_size(&ctx->scenes_2d) >= SE_MAX_SCENES) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_scene_2d_handle scene_handle = s_array_increment(&ctx->scenes_2d);
	se_scene_2d *new_scene = s_array_get(&ctx->scenes_2d, scene_handle);
	memset(new_scene, 0, sizeof(*new_scene));
	se_framebuffer_handle framebuffer = se_framebuffer_create(size);
	if (framebuffer == S_HANDLE_NULL) {
		s_array_remove(&ctx->scenes_2d, scene_handle);
		return S_HANDLE_NULL;
	}
	new_scene->output = framebuffer;
	s_array_init(&new_scene->objects);
	s_array_reserve(&new_scene->objects, object_count);
	se_set_last_error(SE_RESULT_OK);
	return scene_handle;
}

static void se_scene_2d_resize_callback(const se_window_handle window, void *scene_data) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_scene_2d_resize_callback :: ctx is null");
	s_assertf(scene_data, "se_scene_2d_resize_callback :: scene_data is null");

	se_scene_2d_handle scene = (se_scene_2d_handle)(uintptr_t)scene_data;
	se_window *window_ptr = s_array_get(&ctx->windows, window);
	se_scene_2d *scene_ptr = s_array_get(&ctx->scenes_2d, scene);
	s_assertf(window_ptr, "se_scene_2d_resize_callback :: window_ptr is null");
	s_assertf(scene_ptr, "se_scene_2d_resize_callback :: scene_ptr is null");

	se_framebuffer_handle framebuffer = scene_ptr->output;
	s_assertf(framebuffer != S_HANDLE_NULL, "se_scene_2d_resize_callback :: framebuffer is null");
	s_vec2 old_size = {0};
	se_framebuffer_get_size(framebuffer, &old_size);
	s_vec2 new_size = {0};
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, framebuffer);
	new_size = s_vec2(framebuffer_ptr->ratio.x * window_ptr->width,
						framebuffer_ptr->ratio.y * window_ptr->height);

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
	se_scene_2d_render_to_buffer(scene);
}

void se_scene_2d_set_auto_resize(const se_scene_2d_handle scene, const se_window_handle window, const s_vec2 *ratio) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_set_auto_resize :: scene is null");
	s_assertf(ratio, "se_scene_2d_set_auto_resize :: ratio is null");

	s_assertf(ratio->x == 1.0 && ratio->y == 1.0,
				"se_scene_2d_set_auto_resize :: ratio can only be 1 for now, "
				"please wait for the next update");
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, scene_ptr->output);
	framebuffer_ptr->auto_resize = true;
	framebuffer_ptr->ratio = *ratio;
	se_window_register_resize_event(window, se_scene_2d_resize_callback, (void *)(uintptr_t)scene);
}

void se_scene_2d_destroy(const se_scene_2d_handle scene) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_scene_2d_destroy :: ctx is null");
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_destroy :: scene is null");
	se_framebuffer_destroy(scene_ptr->output);
	scene_ptr->output = S_HANDLE_NULL;
	s_array_clear(&scene_ptr->objects);
	s_array_remove(&ctx->scenes_2d, scene);
}

void se_scene_2d_bind(const se_scene_2d_handle scene) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_bind :: scene is null");
	se_framebuffer_bind(scene_ptr->output);
	se_render_set_blending(true);
}

void se_scene_2d_unbind(const se_scene_2d_handle scene) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_unbind :: scene is null");
	se_framebuffer_unbind(scene_ptr->output);
	se_render_set_blending(false);
}

void se_scene_2d_render_raw(const se_scene_2d_handle scene) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_render_raw :: scene is null");
	s_assertf(ctx, "se_scene_2d_render_raw :: ctx is null");

	se_render_clear();
	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_2d_handle object_handle = *s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (object_handle == S_HANDLE_NULL) {
			continue;
		}
		se_object_2d *current_object_2d = se_object_2d_from_handle(ctx, object_handle);
		if (!current_object_2d || !current_object_2d->is_visible) {
			continue;
		}
		if (current_object_2d->is_custom) {
			if (current_object_2d->custom.render) {
				current_object_2d->custom.render(current_object_2d->custom.data);
			}
		} else {
			se_shader_use(current_object_2d->shader, true, true);
			const sz instance_count = se_object_2d_get_instance_count(object_handle);
			se_quad_render(&current_object_2d->quad, instance_count);
		}
	}
}

void se_scene_2d_render_to_buffer(const se_scene_2d_handle scene) {
	se_scene_2d_bind(scene);
	se_scene_2d_render_raw(scene);
	se_scene_2d_unbind(scene);
}

void se_scene_2d_render_to_screen(const se_scene_2d_handle scene, const se_window_handle window) {
	se_context *ctx = se_current_context();
	if (ctx == NULL) {
		return;
	}

	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	se_window *window_ptr = s_array_get(&ctx->windows, window);
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, scene_ptr->output);
	se_render_unbind_framebuffer();
	se_render_set_blending(true);
	se_shader_set_texture(window_ptr->shader, "u_texture", framebuffer_ptr->texture);
	se_window_render_quad(window);
	se_render_set_blending(false);
}

void se_scene_2d_draw(const se_scene_2d_handle scene, const se_window_handle window) {
	se_scene_2d_render_to_buffer(scene);
	se_render_clear();
	se_scene_2d_render_to_screen(scene, window);
	se_window_render_screen(window);
}

void se_scene_2d_add_object(const se_scene_2d_handle scene, const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_add_object :: scene is null");
	s_array_add(&scene_ptr->objects, object);
}

void se_scene_2d_remove_object(const se_scene_2d_handle scene, const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_remove_object :: scene is null");
	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_2d_handle current = *s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (current == object) {
			s_array_remove(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
			break;
		}
	}
}

se_scene_3d_handle se_scene_3d_create(const s_vec2 *size, const u16 object_count) {
	se_context *ctx = se_current_context();
	if (!ctx || !size || object_count == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->scenes_3d) == 0) {
		s_array_init(&ctx->scenes_3d);
	}
	if (s_array_get_size(&ctx->scenes_3d) >= SE_MAX_SCENES) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_scene_3d_handle scene_handle = s_array_increment(&ctx->scenes_3d);
	se_scene_3d *new_scene = s_array_get(&ctx->scenes_3d, scene_handle);
	memset(new_scene, 0, sizeof(*new_scene));
	se_framebuffer_handle framebuffer = se_framebuffer_create(size);
	if (framebuffer == S_HANDLE_NULL) {
		s_array_remove(&ctx->scenes_3d, scene_handle);
		return S_HANDLE_NULL;
	}
	new_scene->output = framebuffer;
	s_array_init(&new_scene->objects);
	s_array_init(&new_scene->post_process);
	s_array_reserve(&new_scene->objects, object_count);
	s_array_reserve(&new_scene->post_process, object_count);
	new_scene->output_shader = S_HANDLE_NULL;
	se_camera_handle camera = se_camera_create();
	if (camera == S_HANDLE_NULL) {
		se_framebuffer_destroy(new_scene->output);
		s_array_remove(&ctx->scenes_3d, scene_handle);
		return S_HANDLE_NULL;
	}
	new_scene->camera = camera;
	new_scene->enable_culling = true;
	if (new_scene->camera != S_HANDLE_NULL) {
		se_camera_set_aspect(new_scene->camera, size->x, size->y);
	}
	se_set_last_error(SE_RESULT_OK);
	return scene_handle;
}

se_scene_3d_handle se_scene_3d_create_for_window(const se_window_handle window, const u16 object_count) {
	se_context *ctx = se_current_context();
	if (!ctx || window == S_HANDLE_NULL || object_count == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_window *window_ptr = s_array_get(&ctx->windows, window);
	const s_vec2 size = s_vec2((f32)window_ptr->width, (f32)window_ptr->height);
	se_scene_3d_handle scene = se_scene_3d_create(&size, object_count);
	if (scene == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}

	const s_vec2 ratio = s_vec2(1.0f, 1.0f);
	se_scene_3d_set_auto_resize(scene, window, &ratio);
	se_set_last_error(SE_RESULT_OK);
	return scene;
}

static void se_scene_3d_resize_callback(const se_window_handle window, void *scene_data) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_scene_3d_resize_callback :: ctx is null");
	s_assertf(scene_data, "se_scene_3d_resize_callback :: scene is null");

	se_scene_3d_handle scene = (se_scene_3d_handle)(uintptr_t)scene_data;
	se_window *window_ptr = s_array_get(&ctx->windows, window);
	se_scene_3d *scene_ptr = s_array_get(&ctx->scenes_3d, scene);
	s_assertf(window_ptr, "se_scene_3d_resize_callback :: window_ptr is null");
	s_assertf(scene_ptr, "se_scene_3d_resize_callback :: scene_ptr is null");

	se_framebuffer_handle framebuffer = scene_ptr->output;
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, framebuffer);
	s_vec2 new_size = {framebuffer_ptr->ratio.x * window_ptr->width,
						framebuffer_ptr->ratio.y * window_ptr->height};

	se_framebuffer_set_size(framebuffer, &new_size);
	if (scene_ptr->camera != S_HANDLE_NULL) {
		se_camera_set_aspect(scene_ptr->camera, new_size.x, new_size.y);
	}
	se_scene_3d_render_to_buffer(scene);
}

void se_scene_3d_set_auto_resize(const se_scene_3d_handle scene, const se_window_handle window, const s_vec2 *ratio) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_set_auto_resize :: scene is null");
	s_assertf(ratio, "se_scene_3d_set_auto_resize :: ratio is null");

	s_assertf(ratio->x == 1.0 && ratio->y == 1.0,
			"se_scene_3d_set_auto_resize :: ratio can only be 1 for now, "
			"please wait for the next update");
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, scene_ptr->output);
	framebuffer_ptr->auto_resize = true;
	framebuffer_ptr->ratio = *ratio;
	se_window_register_resize_event(window, se_scene_3d_resize_callback, (void *)(uintptr_t)scene);
}

void se_scene_3d_destroy(const se_scene_3d_handle scene) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	printf("se_scene_3d_destroy :: scene: %p\n", scene_ptr);
	if (scene_ptr->camera != S_HANDLE_NULL && ctx) {
		se_camera_destroy(scene_ptr->camera);
		scene_ptr->camera = S_HANDLE_NULL;
	}
	if (scene_ptr->output != S_HANDLE_NULL) {
		se_framebuffer_destroy(scene_ptr->output);
		scene_ptr->output = S_HANDLE_NULL;
	}
	s_array_clear(&scene_ptr->post_process);
	s_array_clear(&scene_ptr->objects);
	s_array_remove(&ctx->scenes_3d, scene);
}

void se_scene_3d_render_to_buffer(const se_scene_3d_handle scene) {
	se_context *ctx = se_current_context();
	if (ctx == NULL) {
		return;
	}
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_render_to_buffer :: scene is null");
	s_assertf(scene_ptr->output != S_HANDLE_NULL, "se_scene_3d_render_to_buffer :: scene output is null");

	se_framebuffer_bind(scene_ptr->output);
	se_render_clear();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	if (scene_ptr->enable_culling) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
	} else {
		glDisable(GL_CULL_FACE);
	}

	s_mat4 view = s_mat4_identity;
	s_mat4 proj = s_mat4_identity;
	if (scene_ptr->camera != S_HANDLE_NULL) {
		view = se_camera_get_view_matrix(scene_ptr->camera);
		proj = se_camera_get_projection_matrix(scene_ptr->camera);
	}
	s_mat4 vp = s_mat4_mul(&proj, &view);

	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_3d_handle object_handle = *s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (object_handle == S_HANDLE_NULL) {
			continue;
		}
		se_object_3d *object = se_object_3d_from_handle(ctx, object_handle);
		if (!object || !object->is_visible || object->model == S_HANDLE_NULL) {
			continue;
		}
		se_model *model = se_model_from_handle(ctx, object->model);

		const sz instance_count = se_object_3d_get_instance_count(object_handle);
		const sz mesh_count = s_array_get_size(&object->mesh_instances);
		sz mesh_index = 0;
		for (sz m = 0; m < s_array_get_size(&model->meshes); ++m) {
			se_mesh *mesh = s_array_get(&model->meshes, s_array_handle(&model->meshes, (u32)m));
			if (mesh_index >= mesh_count) {
				break;
			}
			se_mesh_instance *mesh_instance = s_array_get(&object->mesh_instances, s_array_handle(&object->mesh_instances, (u32)mesh_index));
			mesh_index++;
			if (mesh == NULL || !se_mesh_has_gpu_data(mesh) || mesh->gpu.index_count == 0) {
				continue;
			}
			if (mesh_instance == NULL || mesh_instance->vao == 0) {
				continue;
			}

			for (sz j = 0; j < s_array_get_size(&object->instances.transforms); ++j) {
				s_mat4 *instance_transform = s_array_get(&object->instances.transforms, s_array_handle(&object->instances.transforms, (u32)j));
				s_mat4 *out_buffer = s_array_get(&object->render_transforms, s_array_handle(&object->render_transforms, (u32)j));
				s_mat4 model_matrix = s_mat4_mul(&object->transform, instance_transform);
				model_matrix = s_mat4_mul(&model_matrix, &mesh->matrix);
				*out_buffer = s_mat4_mul(&vp, &model_matrix);
			}
			mesh_instance->instance_buffers_dirty = true;
			se_mesh_instance_update(mesh_instance);

			se_shader_handle shader = mesh->shader;
			if (shader == S_HANDLE_NULL) {
				continue;
			}

			se_shader_use(shader, true, true);
			glBindVertexArray(mesh_instance->vao);
			glDrawElementsInstanced(GL_TRIANGLES, mesh->gpu.index_count, GL_UNSIGNED_INT, 0, (GLsizei)instance_count);
		}
	}
	se_framebuffer_unbind(scene_ptr->output);
}

void se_scene_3d_render_to_screen(const se_scene_3d_handle scene, const se_window_handle window) {
	se_context *ctx = se_current_context();
	if (ctx == NULL) {
		return;
	}
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	se_window *window_ptr = s_array_get(&ctx->windows, window);
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, scene_ptr->output);

	se_render_unbind_framebuffer();
	se_render_clear();
	se_render_set_blending(true);
	se_shader_set_texture(window_ptr->shader, "u_texture", framebuffer_ptr->texture);
	se_window_render_quad(window);
	se_render_set_blending(false);
}

void se_scene_3d_draw(const se_scene_3d_handle scene, const se_window_handle window) {
	se_scene_3d_render_to_buffer(scene);
	se_scene_3d_render_to_screen(scene, window);
	se_window_render_screen(window);
}

void se_scene_3d_add_object(const se_scene_3d_handle scene, const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_add_object :: scene is null");
	s_array_add(&scene_ptr->objects, object);
}

se_object_3d_handle se_scene_3d_add_model(const se_scene_3d_handle scene, const se_model_handle model, const s_mat4 *transform) {
	se_context *ctx = se_current_context();
	if (!ctx || model == S_HANDLE_NULL || !transform) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_object_3d_handle object = se_object_3d_create(model, transform, 0);
	if (object == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_scene_3d_add_object(scene, object);
	se_set_last_error(SE_RESULT_OK);
	return object;
}

void se_scene_3d_remove_object(const se_scene_3d_handle scene, const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_remove_object :: scene is null");
	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_3d_handle current = *s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (current == object) {
			s_array_remove(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
			break;
		}
	}
}

void se_scene_3d_set_camera(const se_scene_3d_handle scene, const se_camera_handle camera) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_set_camera :: scene is null");
	scene_ptr->camera = camera;
}

se_camera_handle se_scene_3d_get_camera(const se_scene_3d_handle scene) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_set_last_error(SE_RESULT_OK);
	return scene_ptr->camera;
}

void se_scene_3d_set_culling(const se_scene_3d_handle scene, const b8 enabled) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_set_culling :: scene is null");
	scene_ptr->enable_culling = enabled;
}

void se_scene_3d_add_post_process_buffer(const se_scene_3d_handle scene, const se_render_buffer_handle buffer) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_array_add(&scene_ptr->post_process, buffer);
}

void se_scene_3d_remove_post_process_buffer(const se_scene_3d_handle scene, const se_render_buffer_handle buffer) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	for (sz i = 0; i < s_array_get_size(&scene_ptr->post_process); ++i) {
		se_render_buffer_handle current = *s_array_get(&scene_ptr->post_process, s_array_handle(&scene_ptr->post_process, (u32)i));
		if (current == buffer) {
			s_array_remove(&scene_ptr->post_process, s_array_handle(&scene_ptr->post_process, (u32)i));
			break;
		}
	}
}

se_object_3d_handle se_object_3d_create(const se_model_handle model, const s_mat4 *transform, const sz max_instances_count) {
	se_context *ctx = se_current_context();
	if (!ctx || model == S_HANDLE_NULL || !transform) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->objects_3d) == 0) {
		s_array_init(&ctx->objects_3d);
	}
	if (s_array_get_size(&ctx->objects_3d) >= SE_MAX_3D_OBJECTS) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_object_3d_handle object_handle = s_array_increment(&ctx->objects_3d);
	se_object_3d *new_object = s_array_get(&ctx->objects_3d, object_handle);
	memset(new_object, 0, sizeof(*new_object));
	new_object->model = model;
	new_object->transform = *transform;
	new_object->is_visible = true;
	const sz instance_capacity = (max_instances_count > 0) ? max_instances_count : 1;
	s_array_init(&new_object->instances.ids);
	s_array_init(&new_object->instances.transforms);
	s_array_init(&new_object->instances.buffers);
	s_array_init(&new_object->render_transforms);
	s_array_reserve(&new_object->instances.ids, instance_capacity);
	s_array_reserve(&new_object->instances.transforms, instance_capacity);
	s_array_reserve(&new_object->instances.buffers, instance_capacity);
	s_array_reserve(&new_object->render_transforms, instance_capacity);

	se_model *model_ptr = se_model_from_handle(ctx, model);
	const sz mesh_count = s_array_get_size(&model_ptr->meshes);
	if (mesh_count > 0) {
		s_array_init(&new_object->mesh_instances);
		s_array_reserve(&new_object->mesh_instances, mesh_count);
		for (sz i = 0; i < mesh_count; ++i) {
			se_mesh *mesh = s_array_get(&model_ptr->meshes, s_array_handle(&model_ptr->meshes, (u32)i));
			s_handle mesh_inst_handle = s_array_increment(&new_object->mesh_instances);
			se_mesh_instance *mesh_instance = s_array_get(&new_object->mesh_instances, mesh_inst_handle);
			memset(mesh_instance, 0, sizeof(*mesh_instance));
			if (!se_mesh_has_gpu_data(mesh)) {
				continue;
			}
			se_mesh_instance_create(mesh_instance, mesh, (u32)instance_capacity);
			se_mesh_instance_add_buffer(mesh_instance, s_array_get_data(&new_object->render_transforms), instance_capacity);
		}
	}

	if (max_instances_count == 0) {
		s_handle id_handle = s_array_increment(&new_object->instances.ids);
		s_handle transform_handle = s_array_increment(&new_object->instances.transforms);
		s_handle buffer_handle = s_array_increment(&new_object->instances.buffers);
		s_handle render_handle = s_array_increment(&new_object->render_transforms);
		se_instance_id *new_instance_id = s_array_get(&new_object->instances.ids, id_handle);
		s_mat4 *new_transform = s_array_get(&new_object->instances.transforms, transform_handle);
		s_mat4 *new_buffer = s_array_get(&new_object->instances.buffers, buffer_handle);
		s_mat4 *new_render_transform = s_array_get(&new_object->render_transforms, render_handle);
		*new_instance_id = 0;
		*new_transform = s_mat4_identity;
		*new_buffer = s_mat4_identity;
		*new_render_transform = s_mat4_identity;
		se_object_3d_set_instances_dirty(object_handle, true);
	}

	se_set_last_error(SE_RESULT_OK);
	return object_handle;
}

void se_object_3d_destroy(const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_object_3d_destroy :: ctx is null");
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_destroy :: object is null");
	for (sz i = 0; i < s_array_get_size(&object_ptr->mesh_instances); ++i) {
		se_mesh_instance *mesh_instance = s_array_get(&object_ptr->mesh_instances, s_array_handle(&object_ptr->mesh_instances, (u32)i));
		se_mesh_instance_destroy(mesh_instance);
	}
	s_array_clear(&object_ptr->mesh_instances);
	s_array_clear(&object_ptr->render_transforms);
	s_array_clear(&object_ptr->instances.ids);
	s_array_clear(&object_ptr->instances.transforms);
	s_array_clear(&object_ptr->instances.buffers);
	object_ptr->model = S_HANDLE_NULL;
	object_ptr->is_visible = false;
	s_array_remove(&ctx->objects_3d, object);
}

void se_object_3d_set_transform(const se_object_3d_handle object, const s_mat4 *transform) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_transform :: object is null");
	s_assertf(transform, "se_object_3d_set_transform :: transform is null");
	object_ptr->transform = *transform;
}

s_mat4 se_object_3d_get_transform(const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_get_transform :: object is null");
	return object_ptr->transform;
}

se_instance_id se_object_3d_add_instance(const se_object_3d_handle object, const s_mat4 *transform, const s_mat4 *buffer) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_add_instance :: object is null");
	s_assertf(transform, "se_object_3d_add_instance :: transform is null");
	s_assertf(buffer, "se_object_3d_add_instance :: buffer is null");
	if (s_array_get_size(&object_ptr->instances.ids) >= s_array_get_capacity(&object_ptr->instances.ids)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return -1;
	}

	const sz prev_instances_count = s_array_get_size(&object_ptr->instances.ids);
	se_instance_id new_id = 0;
	if (prev_instances_count > 0) {
		se_instance_id *last_id = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)(prev_instances_count - 1)));
		new_id = *last_id + 1;
	}

	s_handle id_handle = s_array_increment(&object_ptr->instances.ids);
	s_handle transform_handle = s_array_increment(&object_ptr->instances.transforms);
	s_handle buffer_handle = s_array_increment(&object_ptr->instances.buffers);
	s_handle render_handle = s_array_increment(&object_ptr->render_transforms);
	se_instance_id *new_instance_id = s_array_get(&object_ptr->instances.ids, id_handle);
	s_mat4 *new_transform = s_array_get(&object_ptr->instances.transforms, transform_handle);
	s_mat4 *new_buffer = s_array_get(&object_ptr->instances.buffers, buffer_handle);
	s_mat4 *new_render_transform = s_array_get(&object_ptr->render_transforms, render_handle);

	*new_instance_id = new_id;
	*new_transform = *transform;
	*new_buffer = *buffer;
	*new_render_transform = s_mat4_identity;
	se_object_3d_set_instances_dirty(object, true);

	return *new_instance_id;
}

i32 se_object_3d_get_instance_index(const se_object_3d_handle object, const se_instance_id instance_id) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_get_instance_index :: object is null");
	for (sz i = 0; i < s_array_get_size(&object_ptr->instances.ids); ++i) {
		se_instance_id *found_instance = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)i));
		if (*found_instance == instance_id) {
			return (i32)i;
		}
	}
	return -1;
}

void se_object_3d_set_instance_transform(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4 *transform) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_instance_transform :: object is null");
	s_assertf(transform, "se_object_3d_set_instance_transform :: transform is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat4 *current_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)index));
		*current_transform = *transform;
		se_object_3d_set_instances_dirty(object, true);
	} else {
		printf("se_object_3d_set_instance_transform :: instance id %d not found\n", instance_id);
	}
}

void se_object_3d_set_instance_buffer(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4 *buffer) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_instance_buffer :: object is null");
	s_assertf(buffer, "se_object_3d_set_instance_buffer :: buffer is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat4 *current_buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)index));
		*current_buffer = *buffer;
		se_object_3d_set_instances_dirty(object, true);
	} else {
		printf("se_object_3d_set_instance_buffer :: instance id %d not found\n", instance_id);
	}
}

void se_object_3d_set_instances_dirty(const se_object_3d_handle object, const b8 dirty) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_instances_dirty :: object is null");
	for (sz i = 0; i < s_array_get_size(&object_ptr->mesh_instances); ++i) {
		se_mesh_instance *mesh_instance = s_array_get(&object_ptr->mesh_instances, s_array_handle(&object_ptr->mesh_instances, (u32)i));
		mesh_instance->instance_buffers_dirty = dirty;
	}
}

b8 se_object_3d_are_instances_dirty(const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_are_instances_dirty :: object is null");
	if (s_array_get_size(&object_ptr->mesh_instances) == 0) {
		return false;
	}
	se_mesh_instance *mesh_instance = s_array_get(&object_ptr->mesh_instances, s_array_handle(&object_ptr->mesh_instances, 0));
	return mesh_instance->instance_buffers_dirty;
}

sz se_object_3d_get_instance_count(const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_get_instance_count :: object is null");
	return s_array_get_size(&object_ptr->instances.ids);
}
