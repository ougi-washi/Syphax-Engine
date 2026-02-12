// Syphax-Engine - Ougi Washi

#include "se_ui.h"

#include <string.h>

typedef struct {
	se_text_handle *text_handle;
	c8 text[SE_TEXT_CHAR_COUNT];
	se_font *font;
	s_vec2 position;
} se_ui_text_render_data;

static void se_ui_text_render(se_context *ctx, void *data);

se_ui_handle *se_ui_handle_create(
	se_window *window,
	se_context *ctx,
	u16 objects_per_element_count,
	u16 children_per_element_count,
	u16 texts_count,
	u16 fonts_count) {
	if (!window || !ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	const u16 effective_objects_per_element_count = (objects_per_element_count == 0) ? 1 : objects_per_element_count;
	const u16 effective_children_per_element_count = (children_per_element_count == 0) ? 1 : children_per_element_count;
	const u16 effective_texts_count = (texts_count == 0) ? 2 : texts_count;
	const u16 effective_fonts_count = (fonts_count == 0) ? 2 : fonts_count;

	se_ui_handle *ui_handle = malloc(sizeof(se_ui_handle));
	if (!ui_handle) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	memset(ui_handle, 0, sizeof(se_ui_handle));
	ui_handle->window = window;
	ui_handle->ctx = ctx;
	if (s_array_get_capacity(&ctx->ui_elements) == 0) {
		s_array_init(&ctx->ui_elements, 2);
	}
	if (s_array_get_capacity(&ctx->ui_texts) == 0) {
		s_array_init(&ctx->ui_texts, effective_texts_count);
	}
	ui_handle->objects_per_element_count = effective_objects_per_element_count;
	ui_handle->children_per_element_count = effective_children_per_element_count;

	if (effective_fonts_count > 0) {
		se_text_handle *text_handle = se_text_handle_create(ctx, effective_fonts_count);
		if (!text_handle) {
			free(ui_handle);
			return NULL;
		}
		ui_handle->text_handle = text_handle;
	}
	se_set_last_error(SE_RESULT_OK);
	return ui_handle;
}

void se_ui_handle_destroy(se_ui_handle *ui_handle) {
	s_assertf(ui_handle, "se_ui_handle_destroy :: ui_handle is null");
	// Iterate in reverse to safely remove elements during iteration
	sz i = s_array_get_size(&ui_handle->ctx->ui_elements);
	while (i-- > 0) {
		se_ui_element *current_ui = s_array_get(&ui_handle->ctx->ui_elements, i);
		if (!current_ui) {
			continue;
		}
		// Only destroy elements owned by this handle
		if (current_ui->ui_handle == ui_handle) {
			se_ui_handle_destroy_element(ui_handle, current_ui);
		}
	}
	if (ui_handle->text_handle) { // Text handle is optional
		se_text_handle_destroy(ui_handle->text_handle);
	}
	free(ui_handle);
	ui_handle = NULL;
}

void se_ui_handle_destroy_element(se_ui_handle *ui_handle, se_ui_element *ui) {
	s_assertf(ui_handle, "se_ui_handle_destroy_element :: ui_handle is null");
	s_assertf(ui, "se_ui_handle_destroy_element :: ui is null");
	while (s_array_get_size(&ui->children) > 0) {
		se_ui_element_ptr *child_ptr = s_array_get(&ui->children, 0);
		if (!child_ptr || !*child_ptr) {
			s_array_remove_at(&ui->children, 0);
			continue;
		}
		se_ui_handle_destroy_element(ui_handle, *child_ptr);
	}
	if (ui->parent) {
		se_ui_element_detach_child(ui->parent, ui);
	}
	s_array_clear(&ui->children);
	if (ui->scene_2d) {
		se_scene_2d_destroy(ui_handle->ctx, ui->scene_2d);
	}
	ui->scene_2d = NULL;
	ui->text = NULL;
	ui->parent = NULL;
	for (sz i = 0; i < s_array_get_size(&ui_handle->ctx->ui_elements); i++) {
		se_ui_element *slot = s_array_get(&ui_handle->ctx->ui_elements, i);
		if (slot == ui) {
			s_array_remove_at(&ui_handle->ctx->ui_elements, i);
			break;
		}
	}
}

se_ui_element *se_ui_element_create(se_ui_handle *ui_handle, const se_ui_element_params *params) {
	if (!ui_handle || !params) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	if (s_array_get_capacity(&ui_handle->ctx->ui_elements) == s_array_get_size(&ui_handle->ctx->ui_elements)) {
		const sz current_capacity = s_array_get_capacity(&ui_handle->ctx->ui_elements);
		const sz next_capacity = (current_capacity == 0) ? 2 : (current_capacity + 2);
		s_array_resize(&ui_handle->ctx->ui_elements, next_capacity);
	}
	se_ui_element *new_ui = s_array_increment(&ui_handle->ctx->ui_elements);
	memset(new_ui, 0, sizeof(*new_ui));
	new_ui->ui_handle = ui_handle;
	new_ui->parent = NULL;
	new_ui->visible = params->visible;
	new_ui->layout = params->layout;
	new_ui->position = params->position;
	new_ui->size = params->size;
	new_ui->padding = params->padding;

	se_scene_2d *scene = se_scene_2d_create(ui_handle->ctx, &s_vec2(1920, 1080), ui_handle->objects_per_element_count);
	if (!scene) {
		s_array_remove_at(&ui_handle->ctx->ui_elements, s_array_get_size(&ui_handle->ctx->ui_elements) - 1);
		return NULL;
	}
	new_ui->scene_2d = scene;
	se_scene_2d_set_auto_resize(new_ui->scene_2d, ui_handle->window, &s_vec2(1., 1.)); // TODO: maybe expose option to enable/disable
	s_array_init(&new_ui->children, ui_handle->children_per_element_count);

	se_set_last_error(SE_RESULT_OK);
	return new_ui;
}

se_ui_element *se_ui_element_text_create(se_ui_handle *ui_handle, const se_ui_element_params *params, const c8 *text, const c8 *font_path, const f32 font_size) {
	if (!ui_handle || !params || !text || !font_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_ui_element *new_ui = se_ui_element_create(ui_handle, params);
	if (!new_ui) {
		return NULL;
	}
	se_ui_element_set_text(new_ui, text, font_path, font_size);
	se_set_last_error(SE_RESULT_OK);
	return new_ui;
}

void se_ui_element_render(se_ui_element *ui) {
	s_assertf(ui, "se_ui_element_render :: ui is null");
	s_assertf(ui->ui_handle, "se_ui_element_render :: ui_handle is null");
	s_assertf(ui->scene_2d, "se_ui_element_render :: scene_2d is null");

	se_ui_handle *ui_handle = ui->ui_handle;
	s_assertf(ui_handle, "se_ui_element_render :: ui_handle is null");

	if (!ui->visible) {
	    return;
	}

	s_foreach(&ui->children, i) {
	    se_ui_element_ptr *current_ui_ptr = s_array_get(&ui->children, i);
	    if (!current_ui_ptr) {
	    	printf("se_ui_element_render :: children index %zu is null\n", i);
	    	continue;
	    }
	    se_ui_element *current_ui = *current_ui_ptr;
	    if (!current_ui) {
	    	printf("se_ui_element_render :: children index %zu is null\n", i);
	    	continue;
	    }
	    se_ui_element_render(current_ui);
	}

	se_scene_2d_bind(ui->scene_2d);
	se_scene_2d_render_raw(ui->scene_2d, ui->ui_handle->ctx);
	if (ui->text) {
		se_font *font = se_font_load(ui_handle->text_handle, ui->text->font_path, ui->text->font_size); // TODO: store font instead of path
		if (font) {
			// TODO: add/store parameters, add allignment
			se_text_render(ui_handle->text_handle, font, ui->text->characters, &s_vec2(ui->position.x + ui->padding.x, ui->position.y - ui->padding.y), &s_vec2(1., 1.), .03f); 
		}
	}
	se_scene_2d_unbind(ui->scene_2d);
}

void se_ui_element_render_to_screen(se_ui_element *ui) {
	s_assertf(ui, "se_ui_element_render_to_screen :: ui is null");
	s_assertf(ui->scene_2d, "se_ui_element_render_to_screen :: scene_2d is null");
	se_ui_handle *ui_handle = ui->ui_handle;
	s_assertf(ui_handle, "se_ui_element_render_to_screen :: ui_handle is null");
	s_assertf(ui_handle->ctx, "se_ui_element_render_to_screen :: ctx is null");
	s_assertf(ui_handle->window, "se_ui_element_render_to_screen :: window is null");
	if (!ui->visible) {
		return;
	}

	s_foreach(&ui->children, i) {
	    se_ui_element_ptr *current_ui_ptr = s_array_get(&ui->children, i);
	    if (!current_ui_ptr) {
	    	printf("se_ui_element_render_to_screen :: children index %zu is null\n", i);
	    	continue;
	    }
	    se_ui_element *current_ui = *current_ui_ptr;
	    if (!current_ui) {
	    	printf("se_ui_element_render_to_screen :: children index %zu is null\n", i);
	    	continue;
	    }
	    se_ui_element_render_to_screen(current_ui);
	    }
    se_scene_2d_render_to_screen(ui->scene_2d, ui_handle->ctx, ui_handle->window);
}

void se_ui_element_set_position(se_ui_element *ui, const s_vec2 *position) {
	s_assertf(ui, "se_ui_element_set_position :: ui is null");
	s_assertf(position, "se_ui_element_set_position :: position is null");
	ui->position = *position;
	se_ui_element_update_objects(ui);
}

void se_ui_element_set_size(se_ui_element *ui, const s_vec2 *size) {
	s_assertf(ui, "se_ui_element_set_size :: ui is null");
	s_assertf(size, "se_ui_element_set_size :: size is null");
	ui->size = *size;
	se_ui_element_update_objects(ui);
}

void se_ui_element_set_layout(se_ui_element *ui, const se_ui_layout layout) {
	s_assertf(ui, "se_ui_set_layout :: ui is null");
	ui->layout = layout;
	se_ui_element_update_objects(ui);
}

void se_ui_element_set_visible(se_ui_element *ui, const b8 visible) {
	s_assertf(ui, "se_ui_element_set_visible :: ui is null");
	ui->visible = visible;
}

void se_ui_element_set_text(se_ui_element *ui, const c8 *text, const c8 *font_path, const f32 font_size) {
	s_assertf(ui, "se_ui_element_set_text :: ui is null");
	s_assertf(text, "se_ui_element_set_text :: text is null");
	s_assertf(font_path, "se_ui_element_set_text :: font_path is null");
	se_ui_handle *ui_handle = ui->ui_handle;
	s_assertf(ui_handle, "se_ui_element_set_text :: ui_handle is null");

	if (strlen(text) > SE_TEXT_CHAR_COUNT) {
		printf("se_ui_element_set_text :: text is too long, max length is %d\n", SE_TEXT_CHAR_COUNT);
		return;
	}

	if (!ui->text) {
		if (s_array_get_capacity(&ui_handle->ctx->ui_texts) <= s_array_get_size(&ui_handle->ctx->ui_texts)) {
			const sz current_capacity = s_array_get_capacity(&ui_handle->ctx->ui_texts);
			const sz next_capacity = (current_capacity == 0) ? 2 : (current_capacity + 2);
			s_array_resize(&ui_handle->ctx->ui_texts, next_capacity);
		}
		ui->text = s_array_increment(&ui_handle->ctx->ui_texts);
	}

	strcpy(ui->text->characters, text);
	strcpy(ui->text->font_path, font_path);
	ui->text->font_size = font_size;
}

void se_ui_element_update_objects(se_ui_element *ui) {
	s_assertf(ui, "se_ui_element_update_objects :: ui is null");
	se_scene_2d *scene_2d = ui->scene_2d;
	s_assertf(scene_2d, "se_ui_element_update_objects :: scene_2d is null");

	sz object_count = s_array_get_size(&scene_2d->objects);
	if (object_count == 0) return;

	s_vec2 scale = {1, 1};
	s_vec2 position = {0, 0};

	const f32 element_top = ui->position.y + ui->size.y;     // Top edge in screen coords
	const f32 element_bottom = ui->position.y - ui->size.y;  // Bottom edge in screen coords
	const f32 element_left = ui->position.x - ui->size.x;    // Left edge
	const f32 element_right = ui->position.x + ui->size.x;   // Right edge

	const f32 element_width = element_right - element_left;   // Width in NDC
	const f32 element_height = element_top - element_bottom;  // Height in NDC

	if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
	    scale.x = element_width / object_count;
	    scale.y = element_height;
	    position.x = element_left + scale.x * 0.5f;  // Center of first object
	    position.y = ui->position.y;  // Center vertically in element
	} else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
	    scale.y = element_height / object_count;
	    scale.x = element_width;
	    position.y = element_top - scale.y * 0.5f;  // Center of first object (from top)
	    position.x = ui->position.x;  // Center horizontally in element
	}

	s_vec2 object_scale = {
	    (scale.x * 0.5f) - ui->padding.x,
	    (scale.y * 0.5f) - ui->padding.y
	};

	s_foreach(&scene_2d->objects, i) {
	    se_object_2d_ptr *current_object_2d_ptr = s_array_get(&scene_2d->objects, i);
	    if (!current_object_2d_ptr) {
	    	printf("se_ui_element_add_object :: one of the previous objects is null\n");
	    	continue;
	    }
	    se_object_2d *current_object_2d = *current_object_2d_ptr;
	    if (!current_object_2d) {
	    	printf("se_ui_element_add_object :: one of the previous objects is null\n");
	    	continue;
	    }
	    se_object_2d_set_position(current_object_2d, &position);
	    se_object_2d_set_scale(current_object_2d, &object_scale);
	    if (current_object_2d->is_custom && current_object_2d->custom.render == se_ui_text_render) {
	    	se_ui_text_render_data *text_render_data = (se_ui_text_render_data *)current_object_2d->custom.data;
	    	text_render_data->position = position;
	    }
	    if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
	    	position.x += scale.x;
	    } else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
	    	position.y -= scale.y;  // Go down (decrease y)
	    }
	}
};

void se_ui_element_update_children(se_ui_element *ui) {
	s_assertf(ui, "se_ui_element_update_children :: ui is null");
	u32 child_count = s_array_get_size(&ui->children);
	s_vec2 scale = {1, 1};
	s_vec2 position = {0, 0};

	f32 available_width = 2.0f * ui->size.x;   // Total width in normalized coords (-1 to 1 = 2 units)
	f32 available_height = 2.0f * ui->size.y;  // Total height in normalized coords

	if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
	    scale.x = available_width / child_count;
	    scale.y = available_height;
	    position.x = -ui->size.x + scale.x * 0.5f;
	    position.y = 0;
	} else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
	    scale.y = available_height / child_count;
	    scale.x = available_width;
	    position.y = ui->size.y - scale.y * 0.5f;
	    position.x = 0;
	}

	for (u32 i = 0; i < child_count; i++) {
	    se_ui_element_ptr *current_ui_ptr = s_array_get(&ui->children, i);
	    if (!current_ui_ptr) {
	    	printf("se_ui_element_update_children :: ui %p, children index %d is null\n", ui, i);
	    	continue;
	    }
	    se_ui_element *current_ui = *current_ui_ptr;
	    if (!current_ui) {
	    	printf("se_ui_element_update_children :: ui %p children index %d is null\n", ui, i);
	    	continue;
	    }
    se_ui_element_set_position(current_ui, &s_vec2(position.x + ui->position.x, position.y + ui->position.y));
    se_ui_element_set_size(current_ui, &s_vec2(scale.x * 0.5f, scale.y * 0.5f));
	    if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
	    	position.x += scale.x;
	    } else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
	    	position.y -= scale.y;  // Go down (decrease y)
	    }
	}
}

se_object_2d_ptr se_ui_element_add_object(se_ui_element *ui, const c8 *fragment_shader_path) {
	if (!ui || !fragment_shader_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_scene_2d *scene_2d = ui->scene_2d;
	s_assertf(scene_2d, "se_ui_element_add_object :: scene_2d is null");

	se_ui_handle *ui_handle = ui->ui_handle;
	s_assertf(ui_handle, "se_ui_element_add_object :: ui_handle is null");

	s_mat3 transform = s_mat3_identity;
	se_object_2d_ptr object_2d = se_object_2d_create(ui_handle->ctx, fragment_shader_path, &transform, 0);
	if (!object_2d) {
		return NULL;
	}
	se_scene_2d_add_object(ui->scene_2d, object_2d);
	se_ui_element_update_objects(ui);
	se_set_last_error(SE_RESULT_OK);
	return object_2d;
}

static void se_ui_text_render(se_context *ctx, void *data) {
	s_assertf(ctx, "se_ui_text_render :: ctx is null");
	s_assertf(data, "se_ui_text_render :: data is null");

	se_ui_text_render_data *text_render_data = (se_ui_text_render_data *)data;
	s_assertf(text_render_data->text_handle, "se_ui_text_render :: text_render_data->text_handle is null");
	s_assertf(text_render_data->font, "se_ui_text_render :: text_render_data->font is null");
	s_assertf(text_render_data->text, "se_ui_text_render :: text_render_data->text is null");

	se_text_render(text_render_data->text_handle, text_render_data->font, text_render_data->text, &text_render_data->position, &s_vec2(1., 1.), .03f);
}

se_object_2d_ptr se_ui_element_add_text(se_ui_element *ui, const c8 *text, const c8 *font_path, const f32 font_size) {
	if (!ui || !text || !font_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_ui_handle *ui_handle = ui->ui_handle;
	s_assertf(ui_handle, "se_ui_element_add_text :: ui_handle is null");

	se_text_handle *text_handle = ui_handle->text_handle;
	s_assertf(text_handle, "se_ui_element_add_text :: text_handle is null");


	se_context *ctx = ui_handle->ctx;
	s_assertf(ctx, "se_ui_element_add_text :: ctx is null");
	if (strlen(text) >= SE_TEXT_CHAR_COUNT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_ui_text_render_data text_render_data = {
		.text_handle = text_handle,
		.font = NULL,
		.position = {0, 0}
	};
	se_font *font = se_font_load(ui_handle->text_handle, font_path, font_size);
	if (!font) {
		return NULL;
	}
	text_render_data.font = font;
	strcpy(text_render_data.text, text);

	se_object_custom object_custom = {0};
	object_custom.render = &se_ui_text_render;
	se_object_custom_set_data(&object_custom, &text_render_data, sizeof(text_render_data));

	se_object_2d_ptr object_2d = se_object_2d_create_custom(ctx, &object_custom, &s_mat3_identity);
	if (!object_2d) {
		return NULL;
	}
	se_scene_2d_add_object(ui->scene_2d, object_2d);
	se_ui_element_update_objects(ui);
	
	se_set_last_error(SE_RESULT_OK);
	return object_2d;
}

void se_ui_element_remove_object(se_ui_element *ui, se_object_2d *object) {
	s_assertf(ui, "se_ui_element_remove_object :: ui is null");
	s_assertf(object, "se_ui_element_remove_object :: object is null");
	se_scene_2d_remove_object(ui->scene_2d, object);
}

se_ui_element *se_ui_element_add_child(se_ui_element *parent_ui, const se_ui_element_params *params) {
	if (!parent_ui || !params) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	// TODO: maybe we need to force the child to have the sane window and render
	// handle as the parent

	se_ui_element *new_ui = se_ui_element_create(parent_ui->ui_handle, params);
	if (!new_ui) {
		return NULL;
	}
	new_ui->parent = parent_ui;
	if (s_array_get_size(&parent_ui->children) == s_array_get_capacity(&parent_ui->children)) {
		const sz current_capacity = s_array_get_capacity(&parent_ui->children);
		const sz next_capacity = (current_capacity == 0) ? 2 : (current_capacity + 2);
		s_array_resize(&parent_ui->children, next_capacity);
	}
	s_array_add(&parent_ui->children, new_ui);
	se_ui_element_update_children(parent_ui);
	se_set_last_error(SE_RESULT_OK);
	return new_ui;
}

void se_ui_element_detach_child(se_ui_element *parent_ui, se_ui_element *child) {
	s_assertf(parent_ui, "se_ui_element_detach_child :: parent_ui is null");
	s_assertf(child, "se_ui_element_detach_child :: child is null");
	s_foreach(&parent_ui->children, i) {
		se_ui_element_ptr *current_child_ptr = s_array_get(&parent_ui->children, i);
		if (!current_child_ptr) {
			continue;
		}
		if (*current_child_ptr == child) {
			s_array_remove_at(&parent_ui->children, i);
			break;
		}
	}
	child->parent = NULL;
}
