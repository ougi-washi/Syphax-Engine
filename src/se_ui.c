// Syphax-Engine - Ougi Washi

#include "se_ui.h"

#include <string.h>

typedef struct {
	se_text_handle *text_handle;
	c8 text[SE_TEXT_CHAR_COUNT];
	se_font_handle font;
	s_vec2 position;
} se_ui_text_render_data;

static void se_ui_text_render(void *data);
static se_text_handle *se_ui_text_handle_get(se_context *ctx);

static se_text_handle *se_ui_text_handle_get(se_context *ctx) {
	s_assertf(ctx, "se_ui_text_handle_get :: ctx is null");
	if (!ctx->ui_text_handle) {
		ctx->ui_text_handle = se_text_handle_create(0);
	}
	return ctx->ui_text_handle;
}

void se_ui_element_destroy(const se_ui_element_handle ui) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_destroy :: ui is null");
	while (s_array_get_size(&ui_ptr->children) > 0) {
		se_ui_element_handle child_entry = s_array_handle(&ui_ptr->children, 0);
		se_ui_element_handle *child_ptr = s_array_get(&ui_ptr->children, child_entry);
		if (!child_ptr || *child_ptr == S_HANDLE_NULL) {
			s_array_remove(&ui_ptr->children, child_entry);
			continue;
		}
		if (s_array_get(&ctx->ui_elements, *child_ptr) == NULL) {
			s_array_remove(&ui_ptr->children, child_entry);
			continue;
		}
		se_ui_element_destroy(*child_ptr);
	}
	if (ui_ptr->parent != S_HANDLE_NULL) {
		se_ui_element_detach_child(ui_ptr->parent, ui);
	}
	s_array_clear(&ui_ptr->children);
	if (ui_ptr->scene_2d != S_HANDLE_NULL) {
		se_scene_2d_destroy(ui_ptr->scene_2d);
	}
	ui_ptr->scene_2d = S_HANDLE_NULL;
	ui_ptr->text = S_HANDLE_NULL;
	ui_ptr->parent = S_HANDLE_NULL;
	s_array_remove(&ctx->ui_elements, ui);
}

se_ui_element_handle se_ui_element_create(const se_window_handle window, const se_ui_element_params *params) {
	if (window == S_HANDLE_NULL || !params) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_context *ctx = se_current_context();
	se_ui_element_handle new_ui_handle = s_array_increment(&ctx->ui_elements);
	se_ui_element *new_ui = s_array_get(&ctx->ui_elements, new_ui_handle);
	memset(new_ui, 0, sizeof(*new_ui));
	new_ui->parent = S_HANDLE_NULL;
	new_ui->visible = params->visible;
	new_ui->layout = params->layout;
	new_ui->position = params->position;
	new_ui->size = params->size;
	new_ui->padding = params->padding;

	se_scene_2d_handle scene = se_scene_2d_create(&s_vec2(1920, 1080), 1);
	if (scene == S_HANDLE_NULL) {
		s_array_remove(&ctx->ui_elements, new_ui_handle);
		return S_HANDLE_NULL;
	}
	new_ui->scene_2d = scene;
	se_scene_2d_set_auto_resize(new_ui->scene_2d, window, &s_vec2(1., 1.)); // TODO: maybe expose option to enable/disable
	s_array_init(&new_ui->children);

	se_set_last_error(SE_RESULT_OK);
	return new_ui_handle;
}

se_ui_element_handle se_ui_element_text_create(const se_window_handle window, const se_ui_element_params *params, const c8 *text, const c8 *font_path, const f32 font_size) {
	if (window == S_HANDLE_NULL || !params || !text || !font_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	se_ui_element_handle new_ui = se_ui_element_create(window, params);
	if (new_ui == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_ui_element_set_text(new_ui, text, font_path, font_size);
	se_set_last_error(SE_RESULT_OK);
	return new_ui;
}

void se_ui_element_render(const se_ui_element_handle ui) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_render :: ui is null");

	if (!ui_ptr->visible) {
	    return;
	}

	for (sz i = 0; i < s_array_get_size(&ui_ptr->children); i++) {
		se_ui_element_handle child_entry = s_array_handle(&ui_ptr->children, (u32)i);
		se_ui_element_handle *current_ui_ptr = s_array_get(&ui_ptr->children, child_entry);
		if (!current_ui_ptr || *current_ui_ptr == S_HANDLE_NULL) {
			printf("se_ui_element_render :: children index %zu is null\n", i);
			continue;
		}
		if (s_array_get(&ctx->ui_elements, *current_ui_ptr) == NULL) {
			printf("se_ui_element_render :: children index %zu is invalid\n", i);
			continue;
		}
		se_ui_element_render(*current_ui_ptr);
	}

	se_scene_2d_bind(ui_ptr->scene_2d);
	se_scene_2d_render_raw(ui_ptr->scene_2d);
	if (ui_ptr->text != S_HANDLE_NULL) {
		se_ui_text *text = s_array_get(&ctx->ui_texts, ui_ptr->text);
		if (text) {
			se_text_handle *text_handle = se_ui_text_handle_get(ctx);
			se_font_handle font = se_font_load(text_handle, text->font_path, text->font_size); // TODO: store font instead of path
			if (font != S_HANDLE_NULL) {
				// TODO: add/store parameters, add allignment
				se_text_render(text_handle, font, text->characters, &s_vec2(ui_ptr->position.x + ui_ptr->padding.x, ui_ptr->position.y - ui_ptr->padding.y), &s_vec2(1., 1.), .03f);
			}
		}
	}
	se_scene_2d_unbind(ui_ptr->scene_2d);
}

void se_ui_element_render_to_screen(const se_ui_element_handle ui, const se_window_handle window) {
	s_assertf(window != S_HANDLE_NULL, "se_ui_element_render_to_screen :: window is null");
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_render_to_screen :: ui is null");
	if (!ui_ptr->visible) {
		return;
	}

	for (sz i = 0; i < s_array_get_size(&ui_ptr->children); i++) {
		se_ui_element_handle child_entry = s_array_handle(&ui_ptr->children, (u32)i);
		se_ui_element_handle *current_ui_ptr = s_array_get(&ui_ptr->children, child_entry);
		if (!current_ui_ptr || *current_ui_ptr == S_HANDLE_NULL) {
			printf("se_ui_element_render_to_screen :: children index %zu is null\n", i);
			continue;
		}
		if (s_array_get(&ctx->ui_elements, *current_ui_ptr) == NULL) {
			printf("se_ui_element_render_to_screen :: children index %zu is invalid\n", i);
			continue;
		}
		se_ui_element_render_to_screen(*current_ui_ptr, window);
	}
	se_scene_2d_render_to_screen(ui_ptr->scene_2d, window);
}

void se_ui_element_set_position(const se_ui_element_handle ui, const s_vec2 *position) {
	s_assertf(position, "se_ui_element_set_position :: position is null");
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_position :: ui is null");
	ui_ptr->position = *position;
	se_ui_element_update_objects(ui);
}

void se_ui_element_set_size(const se_ui_element_handle ui, const s_vec2 *size) {
	s_assertf(size, "se_ui_element_set_size :: size is null");
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_size :: ui is null");
	ui_ptr->size = *size;
	se_ui_element_update_objects(ui);
}

void se_ui_element_set_layout(const se_ui_element_handle ui, const se_ui_layout layout) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_set_layout :: ui is null");
	ui_ptr->layout = layout;
	se_ui_element_update_objects(ui);
}

void se_ui_element_set_visible(const se_ui_element_handle ui, const b8 visible) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_visible :: ui is null");
	ui_ptr->visible = visible;
}

void se_ui_element_set_text(const se_ui_element_handle ui, const c8 *text, const c8 *font_path, const f32 font_size) {
	s_assertf(text, "se_ui_element_set_text :: text is null");
	s_assertf(font_path, "se_ui_element_set_text :: font_path is null");
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_text :: ui is null");

	if (strlen(text) > SE_TEXT_CHAR_COUNT) {
		printf("se_ui_element_set_text :: text is too long, max length is %d\n", SE_TEXT_CHAR_COUNT);
		return;
	}

	if (ui_ptr->text == S_HANDLE_NULL) {
		ui_ptr->text = s_array_increment(&ctx->ui_texts);
	}
	se_ui_text *ui_text = s_array_get(&ctx->ui_texts, ui_ptr->text);
	s_assertf(ui_text, "se_ui_element_set_text :: ui_text is null");
	strcpy(ui_text->characters, text);
	strcpy(ui_text->font_path, font_path);
	ui_text->font_size = font_size;
}

void se_ui_element_update_objects(const se_ui_element_handle ui) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_update_objects :: ui is null");
	se_scene_2d *scene_2d = s_array_get(&ctx->scenes_2d, ui_ptr->scene_2d);
	s_assertf(scene_2d, "se_ui_element_update_objects :: scene_2d is null");

	sz object_count = s_array_get_size(&scene_2d->objects);
	if (object_count == 0) return;

	s_vec2 scale = {1, 1};
	s_vec2 position = {0, 0};

	const f32 element_top = ui_ptr->position.y + ui_ptr->size.y;     // Top edge in screen coords
	const f32 element_bottom = ui_ptr->position.y - ui_ptr->size.y;  // Bottom edge in screen coords
	const f32 element_left = ui_ptr->position.x - ui_ptr->size.x;    // Left edge
	const f32 element_right = ui_ptr->position.x + ui_ptr->size.x;   // Right edge

	const f32 element_width = element_right - element_left;   // Width in NDC
	const f32 element_height = element_top - element_bottom;  // Height in NDC

	if (ui_ptr->layout == SE_UI_LAYOUT_HORIZONTAL) {
	    scale.x = element_width / object_count;
	    scale.y = element_height;
	    position.x = element_left + scale.x * 0.5f;  // Center of first object
	    position.y = ui_ptr->position.y;  // Center vertically in element
	} else if (ui_ptr->layout == SE_UI_LAYOUT_VERTICAL) {
	    scale.y = element_height / object_count;
	    scale.x = element_width;
	    position.y = element_top - scale.y * 0.5f;  // Center of first object (from top)
	    position.x = ui_ptr->position.x;  // Center horizontally in element
	}

	s_vec2 object_scale = {
	    (scale.x * 0.5f) - ui_ptr->padding.x,
	    (scale.y * 0.5f) - ui_ptr->padding.y
	};

	for (sz i = 0; i < s_array_get_size(&scene_2d->objects); i++) {
		se_object_2d_handle entry = s_array_handle(&scene_2d->objects, (u32)i);
		se_object_2d_handle *current_object_2d_ptr = s_array_get(&scene_2d->objects, entry);
		if (!current_object_2d_ptr || *current_object_2d_ptr == S_HANDLE_NULL) {
			printf("se_ui_element_add_object :: one of the previous objects is null\n");
			continue;
		}
		se_object_2d *current_object_2d = s_array_get(&ctx->objects_2d, *current_object_2d_ptr);
		if (!current_object_2d) {
			printf("se_ui_element_add_object :: one of the previous objects is null\n");
			continue;
		}
		se_object_2d_set_position(*current_object_2d_ptr, &position);
		se_object_2d_set_scale(*current_object_2d_ptr, &object_scale);
		if (current_object_2d->is_custom && current_object_2d->custom.render == se_ui_text_render) {
			se_ui_text_render_data *text_render_data = (se_ui_text_render_data *)current_object_2d->custom.data;
			text_render_data->position = position;
		}
		if (ui_ptr->layout == SE_UI_LAYOUT_HORIZONTAL) {
			position.x += scale.x;
		} else if (ui_ptr->layout == SE_UI_LAYOUT_VERTICAL) {
			position.y -= scale.y;  // Go down (decrease y)
		}
	}
};

void se_ui_element_update_children(const se_ui_element_handle ui) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_update_children :: ui is null");
	u32 child_count = (u32)s_array_get_size(&ui_ptr->children);
	s_vec2 scale = {1, 1};
	s_vec2 position = {0, 0};

	f32 available_width = 2.0f * ui_ptr->size.x;   // Total width in normalized coords (-1 to 1 = 2 units)
	f32 available_height = 2.0f * ui_ptr->size.y;  // Total height in normalized coords

	if (ui_ptr->layout == SE_UI_LAYOUT_HORIZONTAL) {
	    scale.x = available_width / child_count;
	    scale.y = available_height;
	    position.x = -ui_ptr->size.x + scale.x * 0.5f;
	    position.y = 0;
	} else if (ui_ptr->layout == SE_UI_LAYOUT_VERTICAL) {
	    scale.y = available_height / child_count;
	    scale.x = available_width;
	    position.y = ui_ptr->size.y - scale.y * 0.5f;
	    position.x = 0;
	}

	for (u32 i = 0; i < child_count; i++) {
	    se_ui_element_handle child_entry = s_array_handle(&ui_ptr->children, i);
	    se_ui_element_handle *current_ui_ptr = s_array_get(&ui_ptr->children, child_entry);
	    if (!current_ui_ptr || *current_ui_ptr == S_HANDLE_NULL) {
	    	printf("se_ui_element_update_children :: ui %p, children index %d is null\n", ui_ptr, i);
	    	continue;
	    }
	    se_ui_element *current_ui = s_array_get(&ctx->ui_elements, *current_ui_ptr);
	    if (!current_ui) {
	    	printf("se_ui_element_update_children :: ui %p children index %d is null\n", ui_ptr, i);
	    	continue;
	    }
	    se_ui_element_set_position(*current_ui_ptr, &s_vec2(position.x + ui_ptr->position.x, position.y + ui_ptr->position.y));
	    se_ui_element_set_size(*current_ui_ptr, &s_vec2(scale.x * 0.5f, scale.y * 0.5f));
	    if (ui_ptr->layout == SE_UI_LAYOUT_HORIZONTAL) {
	    	position.x += scale.x;
	    } else if (ui_ptr->layout == SE_UI_LAYOUT_VERTICAL) {
	    	position.y -= scale.y;  // Go down (decrease y)
	    }
	}
}

se_object_2d_handle se_ui_element_add_object(const se_ui_element_handle ui, const c8 *fragment_shader_path) {
	if (!fragment_shader_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_add_object :: ui is null");

	s_mat3 transform = s_mat3_identity;
	se_object_2d_handle object_2d = se_object_2d_create(fragment_shader_path, &transform, 0);
	if (object_2d == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_scene_2d_add_object(ui_ptr->scene_2d, object_2d);
	se_ui_element_update_objects(ui);
	se_set_last_error(SE_RESULT_OK);
	return object_2d;
}

static void se_ui_text_render(void *data) {
	s_assertf(data, "se_ui_text_render :: data is null");
	se_ui_text_render_data *text_render_data = (se_ui_text_render_data *)data;
	s_assertf(text_render_data->text_handle, "se_ui_text_render :: text_render_data->text_handle is null");
	s_assertf(text_render_data->font != S_HANDLE_NULL, "se_ui_text_render :: text_render_data->font is null");
	s_assertf(text_render_data->text, "se_ui_text_render :: text_render_data->text is null");
	se_text_render(text_render_data->text_handle, text_render_data->font, text_render_data->text, &text_render_data->position, &s_vec2(1., 1.), .03f);
}

se_object_2d_handle se_ui_element_add_text(const se_ui_element_handle ui, const c8 *text, const c8 *font_path, const f32 font_size) {
	if (!text || !font_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_add_text :: ui is null");

	se_text_handle *text_handle = se_ui_text_handle_get(ctx);
	s_assertf(text_handle, "se_ui_element_add_text :: text_handle is null");


	if (strlen(text) >= SE_TEXT_CHAR_COUNT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	se_ui_text_render_data text_render_data = {
		.text_handle = text_handle,
		.font = S_HANDLE_NULL,
		.position = {0, 0}
	};
	se_font_handle font = se_font_load(text_handle, font_path, font_size);
	if (font == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	text_render_data.font = font;
	strcpy(text_render_data.text, text);

	se_object_custom object_custom = {0};
	object_custom.render = &se_ui_text_render;
	se_object_custom_set_data(&object_custom, &text_render_data, sizeof(text_render_data));

	se_object_2d_handle object_2d = se_object_2d_create_custom(&object_custom, &s_mat3_identity);
	if (object_2d == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_scene_2d_add_object(ui_ptr->scene_2d, object_2d);
	se_ui_element_update_objects(ui);
	
	se_set_last_error(SE_RESULT_OK);
	return object_2d;
}

void se_ui_element_remove_object(const se_ui_element_handle ui, const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_remove_object :: ui is null");
	se_scene_2d_remove_object(ui_ptr->scene_2d, object);
}

se_ui_element_handle se_ui_element_add_child(const se_window_handle window, const se_ui_element_handle parent_ui, const se_ui_element_params *params) {
	if (window == S_HANDLE_NULL || !params) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_context *ctx = se_current_context();
	se_ui_element *parent_ptr = s_array_get(&ctx->ui_elements, parent_ui);
	s_assertf(parent_ptr, "se_ui_element_add_child :: parent_ui is null");
	// TODO: maybe we need to force the child to have the sane window and render
	// handle as the parent

	se_ui_element_handle new_ui = se_ui_element_create(window, params);
	if (new_ui == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}

	// `se_ui_element_create()` grows `ctx->ui_elements` and may realloc the
	// underlying storage, so reacquire pointers after creation.
	parent_ptr = s_array_get(&ctx->ui_elements, parent_ui);
	se_ui_element *new_ui_ptr = s_array_get(&ctx->ui_elements, new_ui);
	if (!parent_ptr || !new_ui_ptr) {
		if (new_ui_ptr) {
			se_ui_element_destroy(new_ui);
		}
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	new_ui_ptr->parent = parent_ui;
	se_ui_element_handle child_value = new_ui;
	s_array_add(&parent_ptr->children, child_value);
	se_ui_element_update_children(parent_ui);
	se_set_last_error(SE_RESULT_OK);
	return new_ui;
}

void se_ui_element_detach_child(const se_ui_element_handle parent_ui, const se_ui_element_handle child) {
	if (parent_ui == S_HANDLE_NULL || child == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	se_context *ctx = se_current_context();
	se_ui_element *parent_ptr = s_array_get(&ctx->ui_elements, parent_ui);
	se_ui_element *child_ptr = s_array_get(&ctx->ui_elements, child);
	if (!parent_ptr || !child_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}

	b8 removed = false;
	for (sz i = 0; i < s_array_get_size(&parent_ptr->children); i++) {
		se_ui_element_handle entry = s_array_handle(&parent_ptr->children, (u32)i);
		se_ui_element_handle *current_child_ptr = s_array_get(&parent_ptr->children, entry);
		if (!current_child_ptr) {
			continue;
		}
		if (*current_child_ptr == child) {
			s_array_remove(&parent_ptr->children, entry);
			removed = true;
			break;
		}
	}

	if (!removed) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return;
	}

	if (child_ptr->parent == parent_ui) {
		child_ptr->parent = S_HANDLE_NULL;
	}
	se_ui_element_update_children(parent_ui);
	se_set_last_error(SE_RESULT_OK);
}
