// Syphax-Engine - Ougi Washi

#include "se_ui.h"
#include "se_debug.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#if defined(SE_RENDER_BACKEND_OPENGL) || defined(SE_RENDER_BACKEND_GLES)
#include "render/se_gl.h"
#endif

typedef struct {
	se_text_handle *text_handle;
	c8 text[SE_TEXT_CHAR_COUNT];
	se_font_handle font;
	s_vec2 position;
} se_ui_text_render_data;

static void se_ui_text_render(void *data);
static se_text_handle *se_ui_text_handle_get(se_context *ctx);
static void se_ui_theme_init_once(void);
static void se_ui_element_apply_layout_rules(se_ui_element* ui_ptr, const se_ui_element* parent_ptr);
static b8 se_ui_point_inside_element(const se_ui_element* ui_ptr, const s_vec2* point_ndc);
static void se_ui_element_collect_hit(const se_ui_element_handle root, const s_vec2* point_ndc, se_ui_element_handle* out_ui, i32* out_z);
static void se_ui_element_sort_children(const se_ui_element_handle ui);
static void se_ui_element_mark_dirty(void);
static void se_ui_debug_collect(const se_ui_element_handle ui, u32* out_count, u32* out_hovered);

static se_ui_theme g_ui_theme = {0};
static b8 g_ui_theme_initialized = false;
static b8 g_ui_debug_overlay_enabled = false;
static i32 g_ui_batch_depth = 0;
static b8 g_ui_batch_dirty = false;

static se_text_handle *se_ui_text_handle_get(se_context *ctx) {
	s_assertf(ctx, "se_ui_text_handle_get :: ctx is null");
	if (!ctx->ui_text_handle) {
		ctx->ui_text_handle = se_text_handle_create(0);
	}
	return ctx->ui_text_handle;
}

static void se_ui_theme_init_once(void) {
	if (g_ui_theme_initialized) {
		return;
	}
	g_ui_theme.panel_color = s_vec4(0.10f, 0.12f, 0.16f, 0.90f);
	g_ui_theme.text_color = s_vec4(0.92f, 0.93f, 0.95f, 1.00f);
	g_ui_theme.accent_color = s_vec4(0.15f, 0.63f, 0.45f, 1.00f);
	g_ui_theme.corner_radius = 0.0f;
	g_ui_theme.border_width = 0.0f;
	g_ui_theme_initialized = true;
}

static void se_ui_element_mark_dirty(void) {
	if (g_ui_batch_depth > 0) {
		g_ui_batch_dirty = true;
	}
}

static void se_ui_element_apply_layout_rules(se_ui_element* ui_ptr, const se_ui_element* parent_ptr) {
	if (!ui_ptr) {
		return;
	}
	const se_ui_layout_rules* rules = &ui_ptr->layout_rules;
	const f32 parent_left = parent_ptr ? (parent_ptr->position.x - parent_ptr->size.x) : -1.0f;
	const f32 parent_right = parent_ptr ? (parent_ptr->position.x + parent_ptr->size.x) : 1.0f;
	const f32 parent_bottom = parent_ptr ? (parent_ptr->position.y - parent_ptr->size.y) : -1.0f;
	const f32 parent_top = parent_ptr ? (parent_ptr->position.y + parent_ptr->size.y) : 1.0f;

	const f32 anchor_left = parent_left + (parent_right - parent_left) * rules->anchor_min.x + rules->margin.left;
	const f32 anchor_right = parent_left + (parent_right - parent_left) * rules->anchor_max.x - rules->margin.right;
	const f32 anchor_bottom = parent_bottom + (parent_top - parent_bottom) * rules->anchor_min.y + rules->margin.bottom;
	const f32 anchor_top = parent_bottom + (parent_top - parent_bottom) * rules->anchor_max.y - rules->margin.top;
	const f32 available_width = s_max(anchor_right - anchor_left, 0.0f);
	const f32 available_height = s_max(anchor_top - anchor_bottom, 0.0f);

	const f32 max_half_x = rules->max_size.x > 0.0f ? rules->max_size.x : 10000.0f;
	const f32 max_half_y = rules->max_size.y > 0.0f ? rules->max_size.y : 10000.0f;
	f32 half_width = rules->stretch_x ? available_width * 0.5f : ui_ptr->size.x;
	f32 half_height = rules->stretch_y ? available_height * 0.5f : ui_ptr->size.y;
	half_width = s_max(rules->min_size.x, s_min(max_half_x, half_width));
	half_height = s_max(rules->min_size.y, s_min(max_half_y, half_height));

	f32 center_x = (anchor_left + anchor_right) * 0.5f;
	f32 center_y = (anchor_bottom + anchor_top) * 0.5f;
	if (!rules->stretch_x) {
		if (rules->align_x == SE_UI_ALIGN_START) {
			center_x = anchor_left + half_width;
		} else if (rules->align_x == SE_UI_ALIGN_END) {
			center_x = anchor_right - half_width;
		}
	}
	if (!rules->stretch_y) {
		if (rules->align_y == SE_UI_ALIGN_START) {
			center_y = anchor_bottom + half_height;
		} else if (rules->align_y == SE_UI_ALIGN_END) {
			center_y = anchor_top - half_height;
		}
	}
	ui_ptr->position = s_vec2(center_x, center_y);
	ui_ptr->size = s_vec2(half_width, half_height);
}

static b8 se_ui_point_inside_element(const se_ui_element* ui_ptr, const s_vec2* point_ndc) {
	if (!ui_ptr || !point_ndc || !ui_ptr->visible) {
		return false;
	}
	const f32 left = ui_ptr->position.x - ui_ptr->size.x;
	const f32 right = ui_ptr->position.x + ui_ptr->size.x;
	const f32 bottom = ui_ptr->position.y - ui_ptr->size.y;
	const f32 top = ui_ptr->position.y + ui_ptr->size.y;
	return point_ndc->x >= left && point_ndc->x <= right && point_ndc->y >= bottom && point_ndc->y <= top;
}

static void se_ui_element_collect_hit(const se_ui_element_handle root, const s_vec2* point_ndc, se_ui_element_handle* out_ui, i32* out_z) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, root);
	if (!ui_ptr || !ui_ptr->visible) {
		return;
	}

	for (sz i = 0; i < s_array_get_size(&ui_ptr->children); i++) {
		se_ui_element_handle *child_ptr = s_array_get(&ui_ptr->children, s_array_handle(&ui_ptr->children, (u32)i));
		if (!child_ptr || *child_ptr == S_HANDLE_NULL) {
			continue;
		}
		se_ui_element_collect_hit(*child_ptr, point_ndc, out_ui, out_z);
	}

	if (!ui_ptr->interactable || !se_ui_point_inside_element(ui_ptr, point_ndc)) {
		return;
	}
	if (ui_ptr->z_order >= *out_z) {
		*out_ui = root;
		*out_z = ui_ptr->z_order;
	}
}

static void se_ui_element_sort_children(const se_ui_element_handle ui) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr) {
		return;
	}
	const sz count = s_array_get_size(&ui_ptr->children);
	for (sz i = 0; i < count; ++i) {
		for (sz j = i + 1; j < count; ++j) {
			se_ui_element_handle *a = s_array_get(&ui_ptr->children, s_array_handle(&ui_ptr->children, (u32)i));
			se_ui_element_handle *b = s_array_get(&ui_ptr->children, s_array_handle(&ui_ptr->children, (u32)j));
			if (!a || !b || *a == S_HANDLE_NULL || *b == S_HANDLE_NULL) {
				continue;
			}
			se_ui_element *a_ui = s_array_get(&ctx->ui_elements, *a);
			se_ui_element *b_ui = s_array_get(&ctx->ui_elements, *b);
			if (!a_ui || !b_ui) {
				continue;
			}
			if (a_ui->z_order > b_ui->z_order) {
				se_ui_element_handle temp = *a;
				*a = *b;
				*b = temp;
			}
		}
	}
}

static void se_ui_debug_collect(const se_ui_element_handle ui, u32* out_count, u32* out_hovered) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr) {
		return;
	}
	if (out_count) {
		(*out_count)++;
	}
	if (out_hovered && ui_ptr->hovered) {
		(*out_hovered)++;
	}
	for (sz i = 0; i < s_array_get_size(&ui_ptr->children); ++i) {
		se_ui_element_handle *child = s_array_get(&ui_ptr->children, s_array_handle(&ui_ptr->children, (u32)i));
		if (!child || *child == S_HANDLE_NULL) {
			continue;
		}
		se_ui_debug_collect(*child, out_count, out_hovered);
	}
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
	if (ui_ptr->text != S_HANDLE_NULL) {
		se_ui_text *ui_text = s_array_get(&ctx->ui_texts, ui_ptr->text);
		if (ui_text) {
			s_array_clear(&ui_text->characters);
			s_array_clear(&ui_text->font_path);
		}
		s_array_remove(&ctx->ui_texts, ui_ptr->text);
	}
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
	se_ui_theme_init_once();
	se_ui_element_handle new_ui_handle = s_array_increment(&ctx->ui_elements);
	se_ui_element *new_ui = s_array_get(&ctx->ui_elements, new_ui_handle);
	memset(new_ui, 0, sizeof(*new_ui));
	new_ui->parent = S_HANDLE_NULL;
	new_ui->visible = params->visible;
	new_ui->layout = params->layout;
	new_ui->position = params->position;
	new_ui->size = params->size;
	new_ui->padding = params->padding;
	new_ui->layout_rules.anchor_min = s_vec2(0.0f, 0.0f);
	new_ui->layout_rules.anchor_max = s_vec2(1.0f, 1.0f);
	new_ui->layout_rules.margin = (se_ui_margin){0, 0, 0, 0};
	new_ui->layout_rules.min_size = s_vec2(0.0f, 0.0f);
	new_ui->layout_rules.max_size = s_vec2(0.0f, 0.0f);
	new_ui->layout_rules.align_x = SE_UI_ALIGN_CENTER;
	new_ui->layout_rules.align_y = SE_UI_ALIGN_CENTER;
	new_ui->layout_rules.stretch_x = false;
	new_ui->layout_rules.stretch_y = false;
	new_ui->theme = g_ui_theme;
	new_ui->z_order = 0;
	new_ui->widget_type = SE_UI_WIDGET_NONE;
	new_ui->slider_min = 0.0f;
	new_ui->slider_max = 1.0f;
	new_ui->slider_value = 0.0f;
	new_ui->image_texture = S_HANDLE_NULL;
	new_ui->clip_enabled = false;
	new_ui->interactable = false;
	new_ui->consume_input = true;
	new_ui->checkbox_value = false;
	new_ui->use_custom_theme = false;
	new_ui->hovered = false;
	new_ui->pressed = false;

	se_scene_2d_handle scene = se_scene_2d_create(&s_vec2(1920, 1080), 1);
	if (scene == S_HANDLE_NULL) {
		s_array_remove(&ctx->ui_elements, new_ui_handle);
		return S_HANDLE_NULL;
	}
	new_ui->scene_2d = scene;
	se_scene_2d_set_auto_resize(new_ui->scene_2d, window, &s_vec2(1., 1.)); // TODO: maybe expose option to enable/disable
	s_array_init(&new_ui->children);
	se_ui_element_mark_dirty();

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
	se_ui_element_sort_children(ui);

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
			const c8* font_path = "";
			if (text->font_path.b.size > 0 && text->font_path.b.data != NULL) {
				font_path = (const c8*)text->font_path.b.data;
			}
			const c8* characters = "";
			if (text->characters.b.size > 0 && text->characters.b.data != NULL) {
				characters = (const c8*)text->characters.b.data;
			}
			se_text_handle *text_handle = se_ui_text_handle_get(ctx);
			se_font_handle font = S_HANDLE_NULL;
			if (font_path[0] != '\0') {
				font = se_font_load(text_handle, font_path, text->font_size); // TODO: store font instead of path
			}
			if (font != S_HANDLE_NULL && characters[0] != '\0') {
				// TODO: add/store parameters, add allignment
				se_text_render(text_handle, font, characters, &s_vec2(ui_ptr->position.x + ui_ptr->padding.x, ui_ptr->position.y - ui_ptr->padding.y), &s_vec2(1., 1.), .03f);
			}
		}
	}
	se_scene_2d_unbind(ui_ptr->scene_2d);
}

void se_ui_element_render_to_screen(const se_ui_element_handle ui, const se_window_handle window) {
	se_debug_trace_begin("ui_render");
	s_assertf(window != S_HANDLE_NULL, "se_ui_element_render_to_screen :: window is null");
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_render_to_screen :: ui is null");
	if (!ui_ptr->visible) {
		se_debug_trace_end("ui_render");
		return;
	}
	se_ui_element_sort_children(ui);

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
	b8 scissor_active = false;
#if defined(SE_RENDER_BACKEND_OPENGL) || defined(SE_RENDER_BACKEND_GLES)
	if (ui_ptr->clip_enabled) {
		u32 width = 0;
		u32 height = 0;
		se_window_get_size(window, &width, &height);
		if (width > 1 && height > 1) {
			const f32 left = ui_ptr->position.x - ui_ptr->size.x;
			const f32 right = ui_ptr->position.x + ui_ptr->size.x;
			const f32 top = ui_ptr->position.y + ui_ptr->size.y;
			const f32 bottom = ui_ptr->position.y - ui_ptr->size.y;
			s_vec2 top_left = {0};
			s_vec2 bottom_right = {0};
			if (se_window_normalized_to_pixel(window, left, top, &top_left) &&
				se_window_normalized_to_pixel(window, right, bottom, &bottom_right)) {
				const i32 scissor_x = (i32)s_max(0.0f, floorf(top_left.x));
				const i32 scissor_h = (i32)s_max(0.0f, ceilf(bottom_right.y - top_left.y));
				const i32 scissor_w = (i32)s_max(0.0f, ceilf(bottom_right.x - top_left.x));
				const i32 scissor_y = (i32)s_max(0.0f, (f32)height - ceilf(bottom_right.y));
				glEnable(GL_SCISSOR_TEST);
				glScissor(scissor_x, scissor_y, scissor_w, scissor_h);
				scissor_active = true;
			}
		}
	}
#endif
	se_scene_2d_render_to_screen(ui_ptr->scene_2d, window);
#if defined(SE_RENDER_BACKEND_OPENGL) || defined(SE_RENDER_BACKEND_GLES)
	if (scissor_active) {
		glDisable(GL_SCISSOR_TEST);
	}
#endif
	if (ui_ptr->parent == S_HANDLE_NULL && g_ui_debug_overlay_enabled) {
		se_ui_render_debug_overlay(ui, window);
	}
	se_debug_trace_end("ui_render");
}

void se_ui_element_set_position(const se_ui_element_handle ui, const s_vec2 *position) {
	s_assertf(position, "se_ui_element_set_position :: position is null");
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_position :: ui is null");
	ui_ptr->position = *position;
	if (g_ui_batch_depth > 0) {
		se_ui_element_mark_dirty();
		return;
	}
	se_ui_element_update_objects(ui);
	se_ui_element_update_children(ui);
}

void se_ui_element_set_size(const se_ui_element_handle ui, const s_vec2 *size) {
	s_assertf(size, "se_ui_element_set_size :: size is null");
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_size :: ui is null");
	ui_ptr->size = *size;
	if (g_ui_batch_depth > 0) {
		se_ui_element_mark_dirty();
		return;
	}
	se_ui_element_update_objects(ui);
	se_ui_element_update_children(ui);
}

void se_ui_element_set_layout(const se_ui_element_handle ui, const se_ui_layout layout) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_set_layout :: ui is null");
	ui_ptr->layout = layout;
	if (g_ui_batch_depth > 0) {
		se_ui_element_mark_dirty();
		return;
	}
	se_ui_element_update_objects(ui);
	se_ui_element_update_children(ui);
}

void se_ui_element_set_visible(const se_ui_element_handle ui, const b8 visible) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_visible :: ui is null");
	ui_ptr->visible = visible;
	se_ui_element_mark_dirty();
}

void se_ui_element_rebuild_tree(const se_ui_element_handle ui) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr) {
		return;
	}
	se_ui_element_update_objects(ui);
	se_ui_element_update_children(ui);
	for (sz i = 0; i < s_array_get_size(&ui_ptr->children); ++i) {
		se_ui_element_handle *child = s_array_get(&ui_ptr->children, s_array_handle(&ui_ptr->children, (u32)i));
		if (!child || *child == S_HANDLE_NULL) {
			continue;
		}
		se_ui_element_rebuild_tree(*child);
	}
}

void se_ui_element_set_layout_rules(const se_ui_element_handle ui, const se_ui_layout_rules* rules) {
	if (!rules) {
		return;
	}
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_layout_rules :: ui is null");
	ui_ptr->layout_rules = *rules;
	if (g_ui_batch_depth > 0) {
		se_ui_element_mark_dirty();
		return;
	}
	if (ui_ptr->parent != S_HANDLE_NULL) {
		se_ui_element_rebuild_tree(ui_ptr->parent);
	} else {
		se_ui_element_rebuild_tree(ui);
	}
}

void se_ui_element_set_theme(const se_ui_element_handle ui, const se_ui_theme* theme) {
	if (!theme) {
		return;
	}
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_theme :: ui is null");
	ui_ptr->theme = *theme;
	ui_ptr->use_custom_theme = true;
}

void se_ui_element_set_z_order(const se_ui_element_handle ui, const i32 z_order) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_z_order :: ui is null");
	ui_ptr->z_order = z_order;
	if (ui_ptr->parent != S_HANDLE_NULL) {
		se_ui_element_sort_children(ui_ptr->parent);
	}
}

i32 se_ui_element_get_z_order(const se_ui_element_handle ui) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr) {
		return 0;
	}
	return ui_ptr->z_order;
}

void se_ui_element_set_clipping(const se_ui_element_handle ui, const b8 enabled) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_clipping :: ui is null");
	ui_ptr->clip_enabled = enabled;
}

void se_ui_element_set_interaction(const se_ui_element_handle ui, const b8 interactable, const b8 consume_input, se_ui_interaction_callback on_click, void* user_data) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_interaction :: ui is null");
	ui_ptr->interactable = interactable;
	ui_ptr->consume_input = consume_input;
	ui_ptr->on_click = on_click;
	ui_ptr->on_click_user_data = user_data;
}

b8 se_ui_element_hit_test(const se_ui_element_handle root, const s_vec2* point_ndc, se_ui_element_handle* out_ui) {
	if (!point_ndc || !out_ui) {
		return false;
	}
	*out_ui = S_HANDLE_NULL;
	i32 out_z = -2147483647;
	se_ui_element_collect_hit(root, point_ndc, out_ui, &out_z);
	return *out_ui != S_HANDLE_NULL;
}

b8 se_ui_element_dispatch_pointer(const se_ui_element_handle root, const s_vec2* point_ndc, const b8 pressed, const b8 released) {
	if (!point_ndc) {
		return false;
	}
	se_ui_element_handle hit = S_HANDLE_NULL;
	if (!se_ui_element_hit_test(root, point_ndc, &hit)) {
		return false;
	}
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, hit);
	if (!ui_ptr || !ui_ptr->interactable) {
		return false;
	}
	ui_ptr->hovered = true;
	if (pressed) {
		ui_ptr->pressed = true;
	}
	if (released && ui_ptr->pressed) {
		ui_ptr->pressed = false;
		if (ui_ptr->widget_type == SE_UI_WIDGET_CHECKBOX) {
			ui_ptr->checkbox_value = !ui_ptr->checkbox_value;
		}
		if (ui_ptr->widget_type == SE_UI_WIDGET_SLIDER) {
			const f32 left = ui_ptr->position.x - ui_ptr->size.x;
			const f32 right = ui_ptr->position.x + ui_ptr->size.x;
			const f32 width = s_max(right - left, 0.0001f);
			const f32 t = s_max(0.0f, s_min(1.0f, (point_ndc->x - left) / width));
			ui_ptr->slider_value = ui_ptr->slider_min + (ui_ptr->slider_max - ui_ptr->slider_min) * t;
		}
		if (ui_ptr->on_click) {
			se_debug_log(SE_DEBUG_LEVEL_DEBUG, SE_DEBUG_CATEGORY_UI, "UI click widget=%d", ui_ptr->widget_type);
			ui_ptr->on_click(hit, ui_ptr->on_click_user_data);
		}
	}
	return ui_ptr->consume_input;
}

void se_ui_element_set_text(const se_ui_element_handle ui, const c8 *text, const c8 *font_path, const f32 font_size) {
	s_assertf(text, "se_ui_element_set_text :: text is null");
	s_assertf(font_path, "se_ui_element_set_text :: font_path is null");
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	s_assertf(ui_ptr, "se_ui_element_set_text :: ui is null");

	const sz text_size = strlen(text) + 1;
	const sz font_path_size = strlen(font_path) + 1;
	if (text_size > SE_TEXT_CHAR_COUNT) {
		printf("se_ui_element_set_text :: text is too long, max length is %d\n", SE_TEXT_CHAR_COUNT);
		return;
	}

	if (ui_ptr->text == S_HANDLE_NULL) {
		ui_ptr->text = s_array_increment(&ctx->ui_texts);
	}
	se_ui_text *ui_text = s_array_get(&ctx->ui_texts, ui_ptr->text);
	s_assertf(ui_text, "se_ui_element_set_text :: ui_text is null");

	s_array_clear(&ui_text->characters);
	s_array_init(&ui_text->characters);
	s_array_reserve(&ui_text->characters, text_size);
	for (sz i = 0; i < text_size; ++i) {
		s_handle character_handle = s_array_increment(&ui_text->characters);
		c8 *dst = s_array_get(&ui_text->characters, character_handle);
		*dst = text[i];
	}

	s_array_clear(&ui_text->font_path);
	s_array_init(&ui_text->font_path);
	s_array_reserve(&ui_text->font_path, font_path_size);
	for (sz i = 0; i < font_path_size; ++i) {
		s_handle path_char_handle = s_array_increment(&ui_text->font_path);
		c8 *dst = s_array_get(&ui_text->font_path, path_char_handle);
		*dst = font_path[i];
	}

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
	se_ui_element_sort_children(ui);
	u32 child_count = (u32)s_array_get_size(&ui_ptr->children);

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
	    se_ui_element_apply_layout_rules(current_ui, ui_ptr);
	    se_ui_element_update_objects(*current_ui_ptr);
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

void se_ui_set_theme(const se_ui_theme* theme) {
	if (!theme) {
		return;
	}
	g_ui_theme = *theme;
	g_ui_theme_initialized = true;
}

void se_ui_get_theme(se_ui_theme* out_theme) {
	if (!out_theme) {
		return;
	}
	se_ui_theme_init_once();
	*out_theme = g_ui_theme;
}

void se_ui_begin_batch(void) {
	g_ui_batch_depth++;
}

void se_ui_end_batch(void) {
	if (g_ui_batch_depth <= 0) {
		g_ui_batch_depth = 0;
		return;
	}
	g_ui_batch_depth--;
	if (g_ui_batch_depth > 0 || !g_ui_batch_dirty) {
		return;
	}
	g_ui_batch_dirty = false;
	se_context *ctx = se_current_context();
	if (!ctx) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&ctx->ui_elements); ++i) {
		se_ui_element_handle ui = s_array_handle(&ctx->ui_elements, (u32)i);
		se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
		if (!ui_ptr || ui_ptr->parent != S_HANDLE_NULL) {
			continue;
		}
		se_ui_element_rebuild_tree(ui);
	}
}

se_ui_element_handle se_ui_panel_create(const se_window_handle window, const se_ui_element_params* params) {
	se_ui_element_handle ui = se_ui_element_create(window, params);
	if (ui != S_HANDLE_NULL) {
		se_context *ctx = se_current_context();
		se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
		if (ui_ptr) {
			ui_ptr->widget_type = SE_UI_WIDGET_PANEL;
		}
	}
	return ui;
}

se_ui_element_handle se_ui_label_create(const se_window_handle window, const se_ui_element_params* params, const c8* text, const c8* font_path, const f32 font_size) {
	se_ui_element_handle ui = se_ui_element_text_create(window, params, text, font_path, font_size);
	if (ui != S_HANDLE_NULL) {
		se_context *ctx = se_current_context();
		se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
		if (ui_ptr) {
			ui_ptr->widget_type = SE_UI_WIDGET_LABEL;
		}
	}
	return ui;
}

se_ui_element_handle se_ui_button_create(const se_window_handle window, const se_ui_element_params* params, const c8* text, const c8* font_path, const f32 font_size, se_ui_interaction_callback on_click, void* user_data) {
	se_ui_element_handle ui = se_ui_element_text_create(window, params, text, font_path, font_size);
	if (ui == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr) {
		return S_HANDLE_NULL;
	}
	ui_ptr->widget_type = SE_UI_WIDGET_BUTTON;
	se_ui_element_set_interaction(ui, true, true, on_click, user_data);
	return ui;
}

se_ui_element_handle se_ui_slider_create(const se_window_handle window, const se_ui_element_params* params, const f32 min_value, const f32 max_value, const f32 value) {
	se_ui_element_handle ui = se_ui_element_create(window, params);
	if (ui == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr) {
		return S_HANDLE_NULL;
	}
	ui_ptr->widget_type = SE_UI_WIDGET_SLIDER;
	ui_ptr->slider_min = min_value;
	ui_ptr->slider_max = s_max(max_value, min_value);
	ui_ptr->slider_value = s_max(ui_ptr->slider_min, s_min(ui_ptr->slider_max, value));
	ui_ptr->interactable = true;
	ui_ptr->consume_input = true;
	return ui;
}

b8 se_ui_slider_set_value(const se_ui_element_handle ui, const f32 value) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr || ui_ptr->widget_type != SE_UI_WIDGET_SLIDER) {
		return false;
	}
	ui_ptr->slider_value = s_max(ui_ptr->slider_min, s_min(ui_ptr->slider_max, value));
	return true;
}

f32 se_ui_slider_get_value(const se_ui_element_handle ui) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr || ui_ptr->widget_type != SE_UI_WIDGET_SLIDER) {
		return 0.0f;
	}
	return ui_ptr->slider_value;
}

se_ui_element_handle se_ui_checkbox_create(const se_window_handle window, const se_ui_element_params* params, const b8 checked) {
	se_ui_element_handle ui = se_ui_element_create(window, params);
	if (ui == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr) {
		return S_HANDLE_NULL;
	}
	ui_ptr->widget_type = SE_UI_WIDGET_CHECKBOX;
	ui_ptr->checkbox_value = checked;
	ui_ptr->interactable = true;
	ui_ptr->consume_input = true;
	return ui;
}

b8 se_ui_checkbox_set_checked(const se_ui_element_handle ui, const b8 checked) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr || ui_ptr->widget_type != SE_UI_WIDGET_CHECKBOX) {
		return false;
	}
	ui_ptr->checkbox_value = checked;
	return true;
}

b8 se_ui_checkbox_is_checked(const se_ui_element_handle ui) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr || ui_ptr->widget_type != SE_UI_WIDGET_CHECKBOX) {
		return false;
	}
	return ui_ptr->checkbox_value;
}

se_ui_element_handle se_ui_image_create(const se_window_handle window, const se_ui_element_params* params, const se_texture_handle texture) {
	se_ui_element_handle ui = se_ui_element_create(window, params);
	if (ui == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, ui);
	if (!ui_ptr) {
		return S_HANDLE_NULL;
	}
	ui_ptr->widget_type = SE_UI_WIDGET_IMAGE;
	ui_ptr->image_texture = texture;
	return ui;
}

b8 se_ui_world_to_window(const se_camera_handle camera, const se_window_handle window, const s_vec3* world, s_vec2* out_window_pixel) {
	if (!world || !out_window_pixel) {
		return false;
	}
	u32 width = 0;
	u32 height = 0;
	se_window_get_size(window, &width, &height);
	return se_camera_world_to_screen(camera, world, (f32)width, (f32)height, out_window_pixel);
}

b8 se_ui_world_to_ndc(const se_camera_handle camera, const s_vec3* world, s_vec2* out_ndc) {
	if (!world || !out_ndc) {
		return false;
	}
	s_vec3 ndc = {0};
	if (!se_camera_world_to_ndc(camera, world, &ndc)) {
		return false;
	}
	out_ndc->x = ndc.x;
	out_ndc->y = ndc.y;
	return true;
}

b8 se_ui_minimap_world_to_ui(const se_box_2d* world_rect, const se_box_2d* ui_rect, const s_vec2* world_point, s_vec2* out_ui_point) {
	if (!world_rect || !ui_rect || !world_point || !out_ui_point) {
		return false;
	}
	const f32 world_w = world_rect->max.x - world_rect->min.x;
	const f32 world_h = world_rect->max.y - world_rect->min.y;
	if (fabsf(world_w) <= 0.00001f || fabsf(world_h) <= 0.00001f) {
		return false;
	}
	const f32 t_x = (world_point->x - world_rect->min.x) / world_w;
	const f32 t_y = (world_point->y - world_rect->min.y) / world_h;
	out_ui_point->x = ui_rect->min.x + t_x * (ui_rect->max.x - ui_rect->min.x);
	out_ui_point->y = ui_rect->min.y + t_y * (ui_rect->max.y - ui_rect->min.y);
	return true;
}

b8 se_ui_minimap_ui_to_world(const se_box_2d* world_rect, const se_box_2d* ui_rect, const s_vec2* ui_point, s_vec2* out_world_point) {
	if (!world_rect || !ui_rect || !ui_point || !out_world_point) {
		return false;
	}
	const f32 ui_w = ui_rect->max.x - ui_rect->min.x;
	const f32 ui_h = ui_rect->max.y - ui_rect->min.y;
	if (fabsf(ui_w) <= 0.00001f || fabsf(ui_h) <= 0.00001f) {
		return false;
	}
	const f32 t_x = (ui_point->x - ui_rect->min.x) / ui_w;
	const f32 t_y = (ui_point->y - ui_rect->min.y) / ui_h;
	out_world_point->x = world_rect->min.x + t_x * (world_rect->max.x - world_rect->min.x);
	out_world_point->y = world_rect->min.y + t_y * (world_rect->max.y - world_rect->min.y);
	return true;
}

void se_ui_set_debug_overlay(const b8 enabled) {
	g_ui_debug_overlay_enabled = enabled;
}

b8 se_ui_is_debug_overlay_enabled(void) {
	return g_ui_debug_overlay_enabled;
}

void se_ui_render_debug_overlay(const se_ui_element_handle root, const se_window_handle window) {
	se_context *ctx = se_current_context();
	se_ui_element *ui_ptr = s_array_get(&ctx->ui_elements, root);
	if (!ui_ptr || !ui_ptr->visible) {
		return;
	}
	u32 element_count = 0;
	u32 hovered_count = 0;
	se_ui_debug_collect(root, &element_count, &hovered_count);
	se_text_handle *text_handle = se_ui_text_handle_get(ctx);
	if (!text_handle) {
		return;
	}
	se_font_handle font = se_font_load(text_handle, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 18.0f);
	if (font == S_HANDLE_NULL) {
		return;
	}
	c8 debug_text[256] = {0};
	snprintf(
		debug_text,
		sizeof(debug_text),
		"UI dbg: elements=%u hovered=%u root=[%.2f %.2f %.2f %.2f]",
		element_count,
		hovered_count,
		ui_ptr->position.x - ui_ptr->size.x,
		ui_ptr->position.y - ui_ptr->size.y,
		ui_ptr->position.x + ui_ptr->size.x,
		ui_ptr->position.y + ui_ptr->size.y);
	(void)window;
	se_text_render(text_handle, font, debug_text, &s_vec2(-0.95f, 0.92f), &s_vec2(1.0f, 1.0f), 0.02f);
}
