// Syphax-Engine - Ougi Washi

#include "se_ui.h"
#include "se_debug.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef s_array(c8, se_ui_chars);
typedef s_array(se_ui_widget_handle, se_ui_widget_handles);

typedef struct {
	se_text_handle* text_handle;
	se_font_handle font;
	c8 text[SE_TEXT_CHAR_COUNT];
	s_vec2 position;
	s_vec2 scale;
	f32 new_line_offset;
	b8 visible;
} se_ui_text_render_data;

typedef enum {
	SE_UI_DEFER_ATTACH = 0,
	SE_UI_DEFER_DETACH,
	SE_UI_DEFER_REMOVE
} se_ui_deferred_op_type;

typedef struct {
	se_ui_deferred_op_type type;
	se_ui_widget_handle parent;
	se_ui_widget_handle child;
} se_ui_deferred_op;
typedef s_array(se_ui_deferred_op, se_ui_deferred_ops);

typedef struct se_ui_text_cache {
	c8 font_path[SE_MAX_PATH_LENGTH];
	f32 font_size;
	se_font_handle font;
} se_ui_text_cache;

typedef struct se_ui_widget {
	se_ui_handle ui;
	se_ui_widget_type type;
	se_ui_widget_handle parent;
	se_ui_widget_handles children;
	se_ui_widget_desc desc;
	se_ui_callbacks callbacks;
	u64 creation_index;

	b8 hovered : 1;
	b8 pressed : 1;
	b8 focused : 1;
	b8 computed_visible : 1;
	b8 has_effective_clip : 1;

	s_vec2 measured_half_size;
	se_box_2d bounds;
	se_box_2d content_bounds;
	se_box_2d effective_clip_bounds;

	se_ui_chars text;
	se_ui_chars placeholder;
	se_ui_chars font_path;
	se_ui_chars id;
	f32 font_size;
	u32 max_length;
	u32 caret;
	u32 selection_start;
	u32 selection_end;
	b8 submit_on_enter : 1;

	f32 value;
	f32 wheel_step;
	f32 content_extent;
	f32 viewport_extent;
	f32 drag_start_value;
	f32 drag_start_pointer_y;
	se_box_2d scroll_track_bounds;
	se_box_2d scroll_thumb_bounds;
	b8 thumb_dragging : 1;
	b8 single_select_enabled : 1;
	b8 has_scroll_item_styles : 1;
	se_ui_widget_handle selected_item;
	se_ui_style scroll_item_style_normal;
	se_ui_style scroll_item_style_selected;

	se_object_2d_handle background_object;
	se_object_2d_handle text_object;
	se_object_2d_handle caret_object;
	se_object_2d_handle selection_object;
	se_object_2d_handle scrollbar_track_object;
	se_object_2d_handle scrollbar_thumb_object;
} se_ui_widget;

typedef struct se_ui_root {
	se_window_handle window;
	se_scene_2d_handle scene;
	se_ui_widget_handles draw_order;
	se_ui_deferred_ops deferred_ops;
	se_ui_widget_handle hovered_widget;
	se_ui_widget_handle pressed_widget;
	se_ui_widget_handle focused_widget;
	se_ui_widget_handle capture_widget;
	b8 dispatching : 1;
	b8 destroying : 1;
	b8 dirty_layout : 1;
	b8 dirty_visual : 1;
	b8 dirty_text : 1;
	b8 dirty_structure : 1;
	b8 text_bridge_owner : 1;
	se_window_text_callback previous_text_callback;
	void* previous_text_callback_data;
	f64 caret_blink_accumulator;
	b8 caret_visible : 1;
	u64 next_creation_index;
} se_ui_root;

#define SE_UI_COLOR_SHADER_PATH SE_RESOURCE_INTERNAL("shaders/ui_color_frag.glsl")
#define SE_UI_DEFAULT_FONT_PATH SE_UI_FONT_DEFAULT
#define SE_UI_SCROLLBAR_WIDTH 0.032f
#define SE_UI_TEXT_SCALE 1.0f
#define SE_UI_LINE_OFFSET 0.03f
#define SE_UI_TEXT_CHAR_WIDTH_FACTOR 0.0009f
#define SE_UI_CARET_BLINK_SECONDS 0.5f

static b8 g_ui_debug_overlay_enabled = false;

static void se_ui_window_text_bridge(se_window_handle window, const c8* utf8_text, void* data);
static se_ui_root* se_ui_root_from_handle(se_context* ctx, const se_ui_handle ui);
static se_ui_widget* se_ui_widget_from_handle(se_context* ctx, const se_ui_widget_handle widget);
static b8 se_ui_parent_context(const se_ui_widget_handle parent, se_ui_handle* out_ui, se_ui_root** out_root);
static void se_ui_mark_layout_dirty_internal(se_ui_root* root);
static void se_ui_mark_visual_dirty_internal(se_ui_root* root);
static void se_ui_mark_text_dirty_internal(se_ui_root* root);
static void se_ui_mark_structure_dirty_internal(se_ui_root* root);
static void se_ui_widget_destroy_recursive(se_ui_root* root, const se_ui_widget_handle widget);
static b8 se_ui_widget_attach_immediate(se_ui_root* root, const se_ui_widget_handle parent, const se_ui_widget_handle child);
static b8 se_ui_widget_detach_immediate(se_ui_root* root, const se_ui_widget_handle child);
static void se_ui_apply_deferred_ops(se_ui_root* root);
static void se_ui_rebuild(se_ui_root* root);
static void se_ui_widget_update_visual_recursive(se_ui_root* root, const se_ui_widget_handle widget, const se_box_2d* parent_clip, const b8 parent_visible);
static void se_ui_sort_widget_handles(se_context* ctx, se_ui_widget_handles* handles);
static void se_ui_layout_recursive(se_ui_root* root, const se_ui_widget_handle widget, const se_box_2d* parent_content, const se_box_2d* parent_clip, const b8 parent_visible);
static void se_ui_layout_children(se_ui_root* root, const se_ui_widget_handle parent);
static s_vec2 se_ui_measure_recursive(se_ui_root* root, const se_ui_widget_handle widget);
static void se_ui_rebuild_scene_draw_list(se_ui_root* root);
static void se_ui_update_text_proxy(const se_ui_widget* widget, const c8* display_text, const s_vec2* position, const b8 visible);
static b8 se_ui_handle_text_input_for_widget(se_ui_root* root, const se_ui_widget_handle widget, const c8* utf8_text, const b8 fire_callback);
static b8 se_ui_textbox_key_edit(se_ui_root* root, const se_ui_widget_handle textbox);
static se_ui_handle se_ui_root_handle(se_context* ctx, se_ui_root* root);
static se_ui_widget_handle se_ui_widget_find_scrollbox_ancestor(se_context* ctx, const se_ui_widget_handle widget_handle);
static b8 se_ui_scrollbox_set_selected_internal(se_ui_root* root, const se_ui_widget_handle scrollbox_handle, const se_ui_widget_handle item_handle, const b8 fire_callbacks);
static void se_ui_scrollbox_refresh_item_styles(se_ui_root* root, se_ui_widget* scrollbox, const se_ui_widget_handle scrollbox_handle);
static void se_ui_scrollbox_item_removed(se_ui_root* root, const se_ui_widget_handle scrollbox_handle, const se_ui_widget_handle item_handle);

static se_ui_root* se_ui_root_from_handle(se_context* ctx, const se_ui_handle ui) {
	if (!ctx || ui == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&ctx->ui_roots, ui);
}

static se_ui_widget* se_ui_widget_from_handle(se_context* ctx, const se_ui_widget_handle widget) {
	if (!ctx || widget == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&ctx->ui_elements, widget);
}

static b8 se_ui_parent_context(const se_ui_widget_handle parent, se_ui_handle* out_ui, se_ui_root** out_root) {
	se_context* ctx = se_current_context();
	if (!ctx || parent == S_HANDLE_NULL || !out_ui || !out_root) {
		return false;
	}
	se_ui_widget* parent_widget = se_ui_widget_from_handle(ctx, parent);
	if (!parent_widget) {
		return false;
	}
	se_ui_root* root = se_ui_root_from_handle(ctx, parent_widget->ui);
	if (!root) {
		return false;
	}
	*out_ui = parent_widget->ui;
	*out_root = root;
	return true;
}

static f32 se_ui_clamp01(const f32 value) {
	if (value < 0.0f) {
		return 0.0f;
	}
	if (value > 1.0f) {
		return 1.0f;
	}
	return value;
}

static f32 se_ui_clampf(const f32 value, const f32 min_value, const f32 max_value) {
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}

static se_box_2d se_ui_make_box(const f32 left, const f32 bottom, const f32 right, const f32 top) {
	se_box_2d box = {0};
	box.min = s_vec2(s_min(left, right), s_min(bottom, top));
	box.max = s_vec2(s_max(left, right), s_max(bottom, top));
	return box;
}

static f32 se_ui_box_width(const se_box_2d* box) {
	return box ? (box->max.x - box->min.x) : 0.0f;
}

static f32 se_ui_box_height(const se_box_2d* box) {
	return box ? (box->max.y - box->min.y) : 0.0f;
}

static b8 se_ui_box_contains(const se_box_2d* box, const s_vec2* p) {
	if (!box || !p) {
		return false;
	}
	return p->x >= box->min.x && p->x <= box->max.x && p->y >= box->min.y && p->y <= box->max.y;
}

static b8 se_ui_box_intersects(const se_box_2d* a, const se_box_2d* b) {
	if (!a || !b) {
		return false;
	}
	return a->min.x <= b->max.x && a->max.x >= b->min.x && a->min.y <= b->max.y && a->max.y >= b->min.y;
}

static f32 se_ui_widget_text_start_x(const se_ui_widget* widget, const u32 text_len) {
	if (!widget) {
		return 0.0f;
	}
	const f32 estimated_w = (f32)text_len * widget->font_size * SE_UI_TEXT_CHAR_WIDTH_FACTOR;
	const f32 left = widget->content_bounds.min.x + widget->desc.layout.padding.left;
	const f32 right = widget->content_bounds.max.x - widget->desc.layout.padding.right;
	f32 x = left;
	if (widget->desc.layout.align_horizontal == SE_UI_ALIGN_CENTER) {
		x = (left + right) * 0.5f - estimated_w * 0.5f;
	} else if (widget->desc.layout.align_horizontal == SE_UI_ALIGN_END) {
		x = right - estimated_w;
	}
	return x;
}

static se_box_2d se_ui_box_intersection(const se_box_2d* a, const se_box_2d* b) {
	if (!a) {
		return b ? *b : se_ui_make_box(0, 0, 0, 0);
	}
	if (!b) {
		return *a;
	}
	if (!se_ui_box_intersects(a, b)) {
		return se_ui_make_box(0.0f, 0.0f, 0.0f, 0.0f);
	}
	return se_ui_make_box(
		s_max(a->min.x, b->min.x),
		s_max(a->min.y, b->min.y),
		s_min(a->max.x, b->max.x),
		s_min(a->max.y, b->max.y));
}

static se_box_2d se_ui_box_translate(const se_box_2d* box, const f32 dx, const f32 dy) {
	if (!box) {
		return se_ui_make_box(0.0f, 0.0f, 0.0f, 0.0f);
	}
	return se_ui_make_box(box->min.x + dx, box->min.y + dy, box->max.x + dx, box->max.y + dy);
}

static void se_ui_chars_init(se_ui_chars* chars) {
	if (!chars) {
		return;
	}
	s_array_init(chars);
	c8 zero = '\0';
	s_array_add(chars, zero);
}

static const c8* se_ui_chars_cstr(const se_ui_chars* chars) {
	if (!chars || chars->b.size == 0 || chars->b.data == NULL) {
		return "";
	}
	return (const c8*)chars->b.data;
}

static u32 se_ui_chars_length(const se_ui_chars* chars) {
	const c8* text = se_ui_chars_cstr(chars);
	return (u32)strlen(text);
}

static void se_ui_chars_set(se_ui_chars* chars, const c8* text) {
	if (!chars) {
		return;
	}
	if (!text) {
		text = "";
	}
	s_array_clear(chars);
	s_array_init(chars);
	const sz len = strlen(text) + 1;
	s_array_reserve(chars, len);
	for (sz i = 0; i < len; ++i) {
		c8 c = text[i];
		s_array_add(chars, c);
	}
}

static b8 se_ui_chars_replace_range(se_ui_chars* chars, u32 start, u32 end, const c8* insert_text, const u32 max_length, u32* out_new_caret) {
	if (!chars) {
		return false;
	}
	const c8* src = se_ui_chars_cstr(chars);
	const u32 len = (u32)strlen(src);
	start = (u32)s_min(start, len);
	end = (u32)s_min(end, len);
	if (end < start) {
		u32 temp = start;
		start = end;
		end = temp;
	}
	if (!insert_text) {
		insert_text = "";
	}
	const u32 insert_len = (u32)strlen(insert_text);
	const u32 base_len = len - (end - start);
	u32 final_insert_len = insert_len;
	if (base_len + final_insert_len > max_length) {
		if (base_len >= max_length) {
			final_insert_len = 0;
		} else {
			final_insert_len = max_length - base_len;
		}
	}
	const u32 new_len = base_len + final_insert_len;
	if (new_len >= (u32)SE_TEXT_CHAR_COUNT) {
		return false;
	}
	c8 temp[SE_TEXT_CHAR_COUNT] = {0};
	if (start > 0) {
		memcpy(temp, src, start);
	}
	if (final_insert_len > 0) {
		memcpy(temp + start, insert_text, final_insert_len);
	}
	if (end < len) {
		memcpy(temp + start + final_insert_len, src + end, len - end);
	}
	temp[new_len] = '\0';
	se_ui_chars_set(chars, temp);
	if (out_new_caret) {
		*out_new_caret = start + final_insert_len;
	}
	return true;
}

static se_text_handle* se_ui_text_handle_get(se_context* ctx) {
	s_assertf(ctx, "se_ui_text_handle_get :: ctx is null");
	if (!ctx->ui_text_handle) {
		ctx->ui_text_handle = se_text_handle_create(0);
	}
	return ctx->ui_text_handle;
}

static se_font_handle se_ui_font_cache_get(se_context* ctx, const c8* font_path, const f32 font_size) {
	if (!ctx || !font_path || font_path[0] == '\0') {
		return S_HANDLE_NULL;
	}
	for (sz i = 0; i < s_array_get_size(&ctx->ui_texts); ++i) {
		se_ui_text_cache* cache = s_array_get(&ctx->ui_texts, s_array_handle(&ctx->ui_texts, (u32)i));
		if (!cache) {
			continue;
		}
		if (fabsf(cache->font_size - font_size) > 0.0001f) {
			continue;
		}
		if (strcmp(cache->font_path, font_path) == 0) {
			return cache->font;
		}
	}
	se_font_handle font = se_font_load(font_path, font_size);
	if (font == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_ui_text_cache new_cache = {0};
	strncpy(new_cache.font_path, font_path, sizeof(new_cache.font_path) - 1);
	new_cache.font_path[sizeof(new_cache.font_path) - 1] = '\0';
	new_cache.font_size = font_size;
	new_cache.font = font;
	s_array_add(&ctx->ui_texts, new_cache);
	return font;
}

static void se_ui_text_render(void* data) {
	if (!data) {
		return;
	}
	se_ui_text_render_data* text_data = (se_ui_text_render_data*)data;
	if (!text_data->visible) {
		return;
	}
	if (!text_data->text_handle || text_data->font == S_HANDLE_NULL || text_data->text[0] == '\0') {
		return;
	}
	se_text_render(text_data->text_handle, text_data->font, text_data->text, &text_data->position, &text_data->scale, text_data->new_line_offset);
}

static se_object_2d_handle se_ui_create_color_object(se_ui_root* root) {
	if (!root) {
		return S_HANDLE_NULL;
	}
	se_object_2d_handle object = se_object_2d_create(SE_UI_COLOR_SHADER_PATH, &s_mat3_identity, 0);
	if (object == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_scene_2d_add_object(root->scene, object);
	return object;
}

static se_object_2d_handle se_ui_create_text_object(se_ui_root* root, se_context* ctx) {
	if (!root || !ctx) {
		return S_HANDLE_NULL;
	}
	se_object_custom custom = {0};
	custom.render = se_ui_text_render;
	se_ui_text_render_data data = {0};
	data.text_handle = se_ui_text_handle_get(ctx);
	data.font = S_HANDLE_NULL;
	data.visible = false;
	data.scale = s_vec2(SE_UI_TEXT_SCALE, SE_UI_TEXT_SCALE);
	data.new_line_offset = SE_UI_LINE_OFFSET;
	se_object_custom_set_data(&custom, &data, sizeof(data));
	se_object_2d_handle object = se_object_2d_create_custom(&custom, &s_mat3_identity);
	if (object == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_scene_2d_add_object(root->scene, object);
	return object;
}

static void se_ui_object_set_visible(se_context* ctx, const se_object_2d_handle object, const b8 visible) {
	if (!ctx || object == S_HANDLE_NULL) {
		return;
	}
	se_object_2d* object_ptr = s_array_get(&ctx->objects_2d, object);
	if (!object_ptr) {
		return;
	}
	object_ptr->is_visible = visible;
}

static void se_ui_object_set_rect_style(
	se_context* ctx,
	const se_object_2d_handle object,
	const se_box_2d* bounds,
	const s_vec4* fill_color,
	const s_vec4* border_color,
	const f32 border_width,
	const s_vec4* effect,
	const b8 visible) {
	if (!ctx || object == S_HANDLE_NULL || !bounds) {
		return;
	}
	const s_vec4 fill = fill_color ? *fill_color : s_vec4(0.0f, 0.0f, 0.0f, 0.0f);
	const s_vec4 border = border_color ? *border_color : s_vec4(0.0f, 0.0f, 0.0f, 0.0f);
	const s_vec4 fx = effect ? *effect : s_vec4(0.0f, 1.0f, 1.0f, 0.0f);
	const s_vec2 position = s_vec2((bounds->min.x + bounds->max.x) * 0.5f, (bounds->min.y + bounds->max.y) * 0.5f);
	const s_vec2 scale = s_vec2(
		s_max(0.00001f, (bounds->max.x - bounds->min.x) * 0.5f),
		s_max(0.00001f, (bounds->max.y - bounds->min.y) * 0.5f));
	se_object_2d_set_position(object, &position);
	se_object_2d_set_scale(object, &scale);
	s_mat4 buffer = s_mat4_identity;
	buffer.m[0][0] = fill.x;
	buffer.m[0][1] = fill.y;
	buffer.m[0][2] = fill.z;
	buffer.m[0][3] = fill.w;
	buffer.m[1][0] = border.x;
	buffer.m[1][1] = border.y;
	buffer.m[1][2] = border.z;
	buffer.m[1][3] = border.w;
	buffer.m[2][0] = scale.x;
	buffer.m[2][1] = scale.y;
	buffer.m[2][2] = s_max(0.0f, border_width);
	buffer.m[2][3] = 0.0f;
	buffer.m[3][0] = fx.x;
	buffer.m[3][1] = fx.y;
	buffer.m[3][2] = fx.z;
	buffer.m[3][3] = fx.w;
	se_object_2d_set_instance_buffer(object, 0, &buffer);
	se_ui_object_set_visible(ctx, object, visible);
}

static void se_ui_object_set_rect_color(
	se_context* ctx,
	const se_object_2d_handle object,
	const se_box_2d* bounds,
	const s_vec4* color,
	const b8 visible) {
	if (!color) {
		return;
	}
	se_ui_object_set_rect_style(ctx, object, bounds, color, &s_vec4(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, NULL, visible);
}

static void se_ui_destroy_object(se_context* ctx, se_ui_root* root, se_object_2d_handle* object) {
	if (!ctx || !root || !object || *object == S_HANDLE_NULL) {
		return;
	}
	if (root->destroying) {
		*object = S_HANDLE_NULL;
		return;
	}
	if (s_array_get(&ctx->objects_2d, *object)) {
		se_scene_2d_remove_object(root->scene, *object);
		se_object_2d_destroy(*object);
	}
	*object = S_HANDLE_NULL;
}

static void se_ui_update_text_proxy(const se_ui_widget* widget, const c8* display_text, const s_vec2* position, const b8 visible) {
	if (!widget || widget->text_object == S_HANDLE_NULL || !position) {
		return;
	}
	se_context* ctx = se_current_context();
	if (!ctx) {
		return;
	}
	se_object_2d* object_ptr = s_array_get(&ctx->objects_2d, widget->text_object);
	if (!object_ptr || !object_ptr->is_custom) {
		return;
	}
	se_ui_text_render_data* data = (se_ui_text_render_data*)object_ptr->custom.data;
	if (!data) {
		return;
	}
	const c8* text = display_text ? display_text : "";
	strncpy(data->text, text, sizeof(data->text) - 1);
	data->text[sizeof(data->text) - 1] = '\0';
	data->position = *position;
	data->visible = visible;
	data->font = se_ui_font_cache_get(ctx, se_ui_chars_cstr(&widget->font_path), widget->font_size > 0.0f ? widget->font_size : 24.0f);
}

static void se_ui_textbox_build_display_text_with_caret(const c8* text, const u32 caret, c8* out_text, const sz out_size) {
	if (!out_text || out_size == 0) {
		return;
	}
	const c8* src = text ? text : "";
	const sz len = strlen(src);
	const sz caret_pos = (sz)s_min((u32)len, caret);
	sz write = 0;
	for (sz i = 0; i < caret_pos && write + 1 < out_size; ++i) {
		out_text[write++] = src[i];
	}
	if (write + 1 < out_size) {
		out_text[write++] = '|';
		if (write + 1 < out_size) {
			out_text[write++] = SE_TEXT_CURSOR_SENTINEL;
		}
	}
	for (sz i = caret_pos; i < len && write + 1 < out_size; ++i) {
		out_text[write++] = src[i];
	}
	out_text[write] = '\0';
}

static se_ui_style_state se_ui_widget_state_style(const se_ui_widget* widget) {
	if (!widget) {
		return SE_UI_STYLE_STATE_DEFAULT;
	}
	if (!widget->desc.enabled) {
		return widget->desc.style.disabled;
	}
	if (widget->pressed) {
		return widget->desc.style.pressed;
	}
	if (widget->focused && (widget->type == SE_UI_WIDGET_TEXTBOX || widget->type == SE_UI_WIDGET_SCROLLBOX)) {
		return widget->desc.style.focused;
	}
	if (widget->hovered) {
		return widget->desc.style.hovered;
	}
	return widget->desc.style.normal;
}

static b8 se_ui_handle_list_remove(se_ui_widget_handles* handles, const se_ui_widget_handle value) {
	if (!handles || value == S_HANDLE_NULL) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size(handles); ++i) {
		se_ui_widget_handle* current = s_array_get(handles, s_array_handle(handles, (u32)i));
		if (!current) {
			continue;
		}
		if (*current == value) {
			s_array_remove(handles, s_array_handle(handles, (u32)i));
			return true;
		}
	}
	return false;
}

static void se_ui_sort_widget_handles(se_context* ctx, se_ui_widget_handles* handles) {
	if (!ctx || !handles) {
		return;
	}
	const sz count = s_array_get_size(handles);
	for (sz i = 0; i < count; ++i) {
		for (sz j = i + 1; j < count; ++j) {
			se_ui_widget_handle* a = s_array_get(handles, s_array_handle(handles, (u32)i));
			se_ui_widget_handle* b = s_array_get(handles, s_array_handle(handles, (u32)j));
			if (!a || !b) {
				continue;
			}
			se_ui_widget* wa = se_ui_widget_from_handle(ctx, *a);
			se_ui_widget* wb = se_ui_widget_from_handle(ctx, *b);
			if (!wa || !wb) {
				continue;
			}
			if (wa->desc.z_order > wb->desc.z_order ||
				(wa->desc.z_order == wb->desc.z_order && wa->creation_index > wb->creation_index)) {
				se_ui_widget_handle temp = *a;
				*a = *b;
				*b = temp;
			}
		}
	}
}

static se_ui_handle se_ui_find_root_owner_for_window(se_context* ctx, const se_window_handle window, const se_ui_handle ignore_ui) {
	if (!ctx || window == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	for (sz i = 0; i < s_array_get_size(&ctx->ui_roots); ++i) {
		const se_ui_handle ui = s_array_handle(&ctx->ui_roots, (u32)i);
		if (ignore_ui != S_HANDLE_NULL && ui == ignore_ui) {
			continue;
		}
		se_ui_root* root = s_array_get(&ctx->ui_roots, ui);
		if (!root || root->window != window) {
			continue;
		}
		return ui;
	}
	return S_HANDLE_NULL;
}

static void se_ui_text_bridge_install(se_context* ctx, const se_ui_handle ui) {
	if (!ctx) {
		return;
	}
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!root) {
		return;
	}
	if (se_ui_find_root_owner_for_window(ctx, root->window, ui) != S_HANDLE_NULL) {
		return;
	}
	se_window* window_ptr = s_array_get(&ctx->windows, root->window);
	if (!window_ptr) {
		return;
	}
	root->text_bridge_owner = true;
	root->previous_text_callback = window_ptr->text_callback;
	root->previous_text_callback_data = window_ptr->text_callback_data;
	se_window_set_text_callback(root->window, se_ui_window_text_bridge, NULL);
}

static void se_ui_text_bridge_uninstall(se_context* ctx, const se_ui_handle ui) {
	if (!ctx) {
		return;
	}
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!root || !root->text_bridge_owner) {
		return;
	}
	se_ui_handle transfer = S_HANDLE_NULL;
	for (sz i = 0; i < s_array_get_size(&ctx->ui_roots); ++i) {
		se_ui_handle current = s_array_handle(&ctx->ui_roots, (u32)i);
		if (current == ui) {
			continue;
		}
		se_ui_root* other = s_array_get(&ctx->ui_roots, current);
		if (!other || other->window != root->window) {
			continue;
		}
		transfer = current;
		break;
	}
	if (transfer != S_HANDLE_NULL) {
		se_ui_root* other = se_ui_root_from_handle(ctx, transfer);
		if (other) {
			other->text_bridge_owner = true;
			other->previous_text_callback = root->previous_text_callback;
			other->previous_text_callback_data = root->previous_text_callback_data;
		}
	} else {
		se_window_set_text_callback(root->window, root->previous_text_callback, root->previous_text_callback_data);
	}
	root->text_bridge_owner = false;
	root->previous_text_callback = NULL;
	root->previous_text_callback_data = NULL;
}

static void se_ui_mark_layout_dirty_internal(se_ui_root* root) {
	if (root) {
		root->dirty_layout = true;
	}
}

static void se_ui_mark_visual_dirty_internal(se_ui_root* root) {
	if (root) {
		root->dirty_visual = true;
	}
}

static void se_ui_mark_text_dirty_internal(se_ui_root* root) {
	if (root) {
		root->dirty_text = true;
	}
}

static void se_ui_mark_structure_dirty_internal(se_ui_root* root) {
	if (root) {
		root->dirty_structure = true;
	}
}

static b8 se_ui_edge_equal(const se_ui_edge* a, const se_ui_edge* b) {
	if (!a || !b) {
		return false;
	}
	return fabsf(a->left - b->left) <= 0.00001f &&
		fabsf(a->right - b->right) <= 0.00001f &&
		fabsf(a->top - b->top) <= 0.00001f &&
		fabsf(a->bottom - b->bottom) <= 0.00001f;
}

static b8 se_ui_vec2_equal(const s_vec2* a, const s_vec2* b) {
	if (!a || !b) {
		return false;
	}
	return fabsf(a->x - b->x) <= 0.00001f && fabsf(a->y - b->y) <= 0.00001f;
}

static b8 se_ui_style_state_equal(const se_ui_style_state* a, const se_ui_style_state* b) {
	if (!a || !b) {
		return false;
	}
	return fabsf(a->background_color.x - b->background_color.x) <= 0.00001f &&
		fabsf(a->background_color.y - b->background_color.y) <= 0.00001f &&
		fabsf(a->background_color.z - b->background_color.z) <= 0.00001f &&
		fabsf(a->background_color.w - b->background_color.w) <= 0.00001f &&
		fabsf(a->border_color.x - b->border_color.x) <= 0.00001f &&
		fabsf(a->border_color.y - b->border_color.y) <= 0.00001f &&
		fabsf(a->border_color.z - b->border_color.z) <= 0.00001f &&
		fabsf(a->border_color.w - b->border_color.w) <= 0.00001f &&
		fabsf(a->text_color.x - b->text_color.x) <= 0.00001f &&
		fabsf(a->text_color.y - b->text_color.y) <= 0.00001f &&
		fabsf(a->text_color.z - b->text_color.z) <= 0.00001f &&
		fabsf(a->text_color.w - b->text_color.w) <= 0.00001f &&
		fabsf(a->border_width - b->border_width) <= 0.00001f;
}

static b8 se_ui_style_equal(const se_ui_style* a, const se_ui_style* b) {
	if (!a || !b) {
		return false;
	}
	return se_ui_style_state_equal(&a->normal, &b->normal) &&
		se_ui_style_state_equal(&a->hovered, &b->hovered) &&
		se_ui_style_state_equal(&a->pressed, &b->pressed) &&
		se_ui_style_state_equal(&a->disabled, &b->disabled) &&
		se_ui_style_state_equal(&a->focused, &b->focused);
}

static b8 se_ui_layout_equal(const se_ui_layout* a, const se_ui_layout* b) {
	if (!a || !b) {
		return false;
	}
	return a->direction == b->direction &&
		a->align_horizontal == b->align_horizontal &&
		a->align_vertical == b->align_vertical &&
		fabsf(a->spacing - b->spacing) <= 0.00001f &&
		se_ui_vec2_equal(&a->anchor_min, &b->anchor_min) &&
		se_ui_vec2_equal(&a->anchor_max, &b->anchor_max) &&
		se_ui_edge_equal(&a->margin, &b->margin) &&
		se_ui_edge_equal(&a->padding, &b->padding) &&
		se_ui_vec2_equal(&a->min_size, &b->min_size) &&
		se_ui_vec2_equal(&a->max_size, &b->max_size) &&
		a->use_anchors == b->use_anchors &&
		a->clip_children == b->clip_children;
}

static void* se_ui_callback_data_or_default(void* specific, void* fallback) {
	return specific ? specific : fallback;
}

static b8 se_ui_callbacks_equal(const se_ui_callbacks* a, const se_ui_callbacks* b) {
	if (!a || !b) {
		return false;
	}
	return a->on_hover_start == b->on_hover_start &&
		a->on_hover_start_data == b->on_hover_start_data &&
		a->on_hover_end == b->on_hover_end &&
		a->on_hover_end_data == b->on_hover_end_data &&
		a->on_focus == b->on_focus &&
		a->on_focus_data == b->on_focus_data &&
		a->on_blur == b->on_blur &&
		a->on_blur_data == b->on_blur_data &&
		a->on_press == b->on_press &&
		a->on_press_data == b->on_press_data &&
		a->on_release == b->on_release &&
		a->on_release_data == b->on_release_data &&
		a->on_click == b->on_click &&
		a->on_click_data == b->on_click_data &&
		a->on_change == b->on_change &&
		a->on_change_data == b->on_change_data &&
		a->on_submit == b->on_submit &&
		a->on_submit_data == b->on_submit_data &&
		a->on_scroll == b->on_scroll &&
		a->on_scroll_data == b->on_scroll_data &&
		a->user_data == b->user_data;
}

static b8 se_ui_callbacks_supported_for_type(const se_ui_widget_type type, const se_ui_callbacks* callbacks) {
	if (!callbacks) {
		return false;
	}
	if (type == SE_UI_WIDGET_TEXTBOX) {
		return callbacks->on_scroll == NULL;
	}
	if (type == SE_UI_WIDGET_SCROLLBOX) {
		return callbacks->on_submit == NULL;
	}
	if (type == SE_UI_WIDGET_BUTTON) {
		return callbacks->on_focus == NULL &&
			callbacks->on_blur == NULL &&
			callbacks->on_change == NULL &&
			callbacks->on_submit == NULL &&
			callbacks->on_scroll == NULL;
	}
	if (type == SE_UI_WIDGET_PANEL || type == SE_UI_WIDGET_TEXT) {
		return callbacks->on_focus == NULL &&
			callbacks->on_blur == NULL &&
			callbacks->on_change == NULL &&
			callbacks->on_submit == NULL &&
			callbacks->on_scroll == NULL;
	}
	return false;
}

static void se_ui_callbacks_filter_for_type(const se_ui_widget_type type, se_ui_callbacks* callbacks) {
	if (!callbacks) {
		return;
	}
	if (type == SE_UI_WIDGET_TEXTBOX) {
		callbacks->on_scroll = NULL;
		callbacks->on_scroll_data = NULL;
		return;
	}
	if (type == SE_UI_WIDGET_SCROLLBOX) {
		callbacks->on_submit = NULL;
		callbacks->on_submit_data = NULL;
		return;
	}
	if (type == SE_UI_WIDGET_BUTTON || type == SE_UI_WIDGET_PANEL || type == SE_UI_WIDGET_TEXT) {
		callbacks->on_focus = NULL;
		callbacks->on_focus_data = NULL;
		callbacks->on_blur = NULL;
		callbacks->on_blur_data = NULL;
		callbacks->on_change = NULL;
		callbacks->on_change_data = NULL;
		callbacks->on_submit = NULL;
		callbacks->on_submit_data = NULL;
		callbacks->on_scroll = NULL;
		callbacks->on_scroll_data = NULL;
	}
}

typedef enum {
	SE_UI_CALLBACK_CLICK = 0,
	SE_UI_CALLBACK_HOVER,
	SE_UI_CALLBACK_PRESS,
	SE_UI_CALLBACK_CHANGE,
	SE_UI_CALLBACK_SUBMIT,
	SE_UI_CALLBACK_SCROLL
} se_ui_callback_kind;

static b8 se_ui_widget_callback_supported(const se_ui_widget* widget, const i32 callback_kind) {
	if (!widget) {
		return false;
	}
	switch (callback_kind) {
		case SE_UI_CALLBACK_CLICK:
		case SE_UI_CALLBACK_HOVER:
		case SE_UI_CALLBACK_PRESS:
			return true;
		case SE_UI_CALLBACK_CHANGE:
			return widget->type == SE_UI_WIDGET_TEXTBOX || widget->type == SE_UI_WIDGET_SCROLLBOX;
		case SE_UI_CALLBACK_SUBMIT:
			return widget->type == SE_UI_WIDGET_TEXTBOX;
		case SE_UI_CALLBACK_SCROLL:
			return widget->type == SE_UI_WIDGET_SCROLLBOX;
		default:
			return false;
	}
}

void se_ui_get_default_style(se_ui_style* out_style) {
	if (!out_style) {
		return;
	}
	*out_style = SE_UI_STYLE_DEFAULT;
}

void se_ui_get_default_layout(se_ui_layout* out_layout) {
	if (!out_layout) {
		return;
	}
	*out_layout = SE_UI_LAYOUT_DEFAULT;
}

void se_ui_get_default_widget_desc(se_ui_widget_desc* out_desc) {
	if (!out_desc) {
		return;
	}
	*out_desc = SE_UI_WIDGET_DESC_DEFAULTS;
}

static se_ui_widget_desc se_ui_widget_desc_or_default(const se_ui_widget_desc* desc) {
	if (!desc) {
		return SE_UI_WIDGET_DESC_DEFAULTS;
	}
	return *desc;
}

static void se_ui_widget_init_string_fields(se_ui_widget* widget) {
	if (!widget) {
		return;
	}
	se_ui_chars_init(&widget->text);
	se_ui_chars_init(&widget->placeholder);
	se_ui_chars_init(&widget->font_path);
	se_ui_chars_init(&widget->id);
}

static void se_ui_widget_clear_string_fields(se_ui_widget* widget) {
	if (!widget) {
		return;
	}
	s_array_clear(&widget->text);
	s_array_clear(&widget->placeholder);
	s_array_clear(&widget->font_path);
	s_array_clear(&widget->id);
}

static void se_ui_widget_destroy_proxies(se_context* ctx, se_ui_root* root, se_ui_widget* widget) {
	if (!ctx || !root || !widget) {
		return;
	}
	se_ui_destroy_object(ctx, root, &widget->background_object);
	se_ui_destroy_object(ctx, root, &widget->text_object);
	se_ui_destroy_object(ctx, root, &widget->caret_object);
	se_ui_destroy_object(ctx, root, &widget->selection_object);
	se_ui_destroy_object(ctx, root, &widget->scrollbar_track_object);
	se_ui_destroy_object(ctx, root, &widget->scrollbar_thumb_object);
}

static se_ui_widget_handle se_ui_widget_create_internal(se_ui_root* root, const se_ui_widget_type type, const se_ui_widget_desc* desc) {
	se_context* ctx = se_current_context();
	if (!ctx || !root) {
		return S_HANDLE_NULL;
	}
	const se_ui_widget_desc resolved_desc = se_ui_widget_desc_or_default(desc);
	if (resolved_desc.id && resolved_desc.id[0] != '\0') {
		const se_ui_handle ui_handle = se_ui_root_handle(ctx, root);
		for (sz i = 0; i < s_array_get_size(&ctx->ui_elements); ++i) {
			se_ui_widget* existing = s_array_get(&ctx->ui_elements, s_array_handle(&ctx->ui_elements, (u32)i));
			if (!existing || existing->ui != ui_handle) {
				continue;
			}
			if (strcmp(se_ui_chars_cstr(&existing->id), resolved_desc.id) == 0) {
				return S_HANDLE_NULL;
			}
		}
	}
	se_ui_widget_handle handle = s_array_increment(&ctx->ui_elements);
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, handle);
	if (!widget) {
		return S_HANDLE_NULL;
	}
	memset(widget, 0, sizeof(*widget));
	widget->ui = se_ui_root_handle(ctx, root);
	widget->type = type;
	widget->parent = S_HANDLE_NULL;
	widget->desc = resolved_desc;
	widget->creation_index = ++root->next_creation_index;
	widget->computed_visible = widget->desc.visible;
	widget->measured_half_size = widget->desc.size;
	widget->bounds = se_ui_make_box(widget->desc.position.x - widget->desc.size.x, widget->desc.position.y - widget->desc.size.y, widget->desc.position.x + widget->desc.size.x, widget->desc.position.y + widget->desc.size.y);
	widget->content_bounds = widget->bounds;
	widget->effective_clip_bounds = widget->bounds;
	widget->has_effective_clip = false;
	widget->font_size = 24.0f;
	widget->max_length = 256;
	widget->wheel_step = 0.08f;
	widget->submit_on_enter = true;
	s_array_init(&widget->children);
	se_ui_widget_init_string_fields(widget);
	se_ui_chars_set(&widget->id, widget->desc.id ? widget->desc.id : "");
	widget->desc.id = se_ui_chars_cstr(&widget->id);
	widget->single_select_enabled = true;
	widget->selected_item = S_HANDLE_NULL;
	widget->has_scroll_item_styles = false;
	widget->scroll_item_style_normal = SE_UI_STYLE_DEFAULT;
	widget->scroll_item_style_selected = SE_UI_STYLE_DEFAULT;
	widget->scroll_item_style_selected.normal.background_color = s_vec4(0.16f, 0.26f, 0.38f, 0.95f);
	widget->scroll_item_style_selected.hovered.background_color = s_vec4(0.20f, 0.32f, 0.46f, 0.98f);
	widget->scroll_item_style_selected.pressed.background_color = s_vec4(0.12f, 0.22f, 0.34f, 0.98f);
	widget->scroll_item_style_selected.normal.border_color = s_vec4(0.28f, 0.64f, 0.90f, 1.0f);
	widget->scroll_item_style_selected.hovered.border_color = s_vec4(0.34f, 0.70f, 0.95f, 1.0f);
	widget->scroll_item_style_selected.pressed.border_color = s_vec4(0.22f, 0.56f, 0.84f, 1.0f);

	widget->background_object = se_ui_create_color_object(root);
	if (type == SE_UI_WIDGET_BUTTON || type == SE_UI_WIDGET_TEXT || type == SE_UI_WIDGET_TEXTBOX) {
		widget->text_object = se_ui_create_text_object(root, ctx);
	}
	if (type == SE_UI_WIDGET_TEXTBOX) {
		widget->selection_object = se_ui_create_color_object(root);
		widget->caret_object = se_ui_create_color_object(root);
	}
	if (type == SE_UI_WIDGET_SCROLLBOX) {
		widget->scrollbar_track_object = se_ui_create_color_object(root);
		widget->scrollbar_thumb_object = se_ui_create_color_object(root);
		widget->has_scroll_item_styles = true;
	}

	se_ui_mark_structure_dirty_internal(root);
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return handle;
}

static b8 se_ui_widget_attach_immediate(se_ui_root* root, const se_ui_widget_handle parent, const se_ui_widget_handle child) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || parent == S_HANDLE_NULL || child == S_HANDLE_NULL || parent == child) {
		return false;
	}
	se_ui_widget* parent_ptr = se_ui_widget_from_handle(ctx, parent);
	se_ui_widget* child_ptr = se_ui_widget_from_handle(ctx, child);
	if (!parent_ptr || !child_ptr) {
		return false;
	}
	if (parent_ptr->ui != child_ptr->ui) {
		return false;
	}
	se_ui_widget_handle walk = parent_ptr->parent;
	while (walk != S_HANDLE_NULL) {
		if (walk == child) {
			return false;
		}
		se_ui_widget* walk_ptr = se_ui_widget_from_handle(ctx, walk);
		if (!walk_ptr) {
			break;
		}
		walk = walk_ptr->parent;
	}
	if (child_ptr->parent == parent) {
		return true;
	}
	const se_ui_widget_handle old_parent_handle = child_ptr->parent;
	if (child_ptr->parent != S_HANDLE_NULL) {
		se_ui_widget* old_parent = se_ui_widget_from_handle(ctx, child_ptr->parent);
		if (old_parent) {
			(void)se_ui_handle_list_remove(&old_parent->children, child);
		}
		if (old_parent && old_parent->type == SE_UI_WIDGET_SCROLLBOX) {
			se_ui_scrollbox_item_removed(root, old_parent_handle, child);
		}
	}
	child_ptr->parent = parent;
	s_array_add(&parent_ptr->children, child);
	if (parent_ptr->type == SE_UI_WIDGET_SCROLLBOX) {
		se_ui_scrollbox_refresh_item_styles(root, parent_ptr, parent);
	}
	se_ui_mark_structure_dirty_internal(root);
	se_ui_mark_layout_dirty_internal(root);
	return true;
}

static b8 se_ui_widget_detach_immediate(se_ui_root* root, const se_ui_widget_handle child) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || child == S_HANDLE_NULL) {
		return false;
	}
	se_ui_widget* child_ptr = se_ui_widget_from_handle(ctx, child);
	if (!child_ptr) {
		return false;
	}
	if (child_ptr->parent == S_HANDLE_NULL) {
		return true;
	}
	se_ui_widget* parent_ptr = se_ui_widget_from_handle(ctx, child_ptr->parent);
	if (parent_ptr) {
		(void)se_ui_handle_list_remove(&parent_ptr->children, child);
		if (parent_ptr->type == SE_UI_WIDGET_SCROLLBOX) {
			se_ui_scrollbox_item_removed(root, child_ptr->parent, child);
		}
	}
	child_ptr->parent = S_HANDLE_NULL;
	se_ui_mark_structure_dirty_internal(root);
	se_ui_mark_layout_dirty_internal(root);
	return true;
}

static void se_ui_widget_destroy_recursive(se_ui_root* root, const se_ui_widget_handle widget_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || widget_handle == S_HANDLE_NULL) {
		return;
	}

	// Child removals compact ctx->ui_elements, so widget pointers may move.
	// Always re-fetch via handle after each mutation to avoid stale pointers.
	for (;;) {
		se_ui_widget* widget = se_ui_widget_from_handle(ctx, widget_handle);
		if (!widget) {
			return;
		}
		const sz child_count = s_array_get_size(&widget->children);
		if (child_count == 0) {
			break;
		}
		const s_handle entry_handle = s_array_handle(&widget->children, (u32)(child_count - 1));
		se_ui_widget_handle* child_ptr = s_array_get(&widget->children, entry_handle);
		if (!child_ptr) {
			(void)s_array_remove(&widget->children, entry_handle);
			continue;
		}
		const se_ui_widget_handle child_handle = *child_ptr;
		(void)s_array_remove(&widget->children, entry_handle);
		if (child_handle == S_HANDLE_NULL || child_handle == widget_handle) {
			continue;
		}
		if (widget->type == SE_UI_WIDGET_SCROLLBOX) {
			se_ui_scrollbox_item_removed(root, widget_handle, child_handle);
		}
		se_ui_widget* child_widget = se_ui_widget_from_handle(ctx, child_handle);
		if (child_widget) {
			child_widget->parent = S_HANDLE_NULL;
		}
		se_ui_widget_destroy_recursive(root, child_handle);
	}

	se_ui_widget* widget = se_ui_widget_from_handle(ctx, widget_handle);
	if (!widget) {
		return;
	}
	if (widget->parent != S_HANDLE_NULL) {
		se_ui_widget* parent = se_ui_widget_from_handle(ctx, widget->parent);
		if (parent) {
			(void)se_ui_handle_list_remove(&parent->children, widget_handle);
			if (parent->type == SE_UI_WIDGET_SCROLLBOX) {
				se_ui_scrollbox_item_removed(root, widget->parent, widget_handle);
			}
		}
	}
	if (root->hovered_widget == widget_handle) {
		root->hovered_widget = S_HANDLE_NULL;
	}
	if (root->pressed_widget == widget_handle) {
		root->pressed_widget = S_HANDLE_NULL;
	}
	if (root->focused_widget == widget_handle) {
		root->focused_widget = S_HANDLE_NULL;
	}
	if (root->capture_widget == widget_handle) {
		root->capture_widget = S_HANDLE_NULL;
	}
	se_ui_widget_destroy_proxies(ctx, root, widget);
	s_array_clear(&widget->children);
	se_ui_widget_clear_string_fields(widget);
	(void)s_array_remove(&ctx->ui_elements, widget_handle);
	se_ui_mark_structure_dirty_internal(root);
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
}

static void se_ui_apply_deferred_ops(se_ui_root* root) {
	se_context* ctx = se_current_context();
	if (!ctx || !root) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&root->deferred_ops); ++i) {
		se_ui_deferred_op* op = s_array_get(&root->deferred_ops, s_array_handle(&root->deferred_ops, (u32)i));
		if (!op) {
			continue;
		}
		if (op->type == SE_UI_DEFER_ATTACH) {
			(void)se_ui_widget_attach_immediate(root, op->parent, op->child);
		} else if (op->type == SE_UI_DEFER_DETACH) {
			(void)se_ui_widget_detach_immediate(root, op->child);
		} else if (op->type == SE_UI_DEFER_REMOVE) {
			se_ui_widget_destroy_recursive(root, op->child);
		}
	}
	s_array_clear(&root->deferred_ops);
}

static s_vec2 se_ui_measure_recursive(se_ui_root* root, const se_ui_widget_handle widget_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || widget_handle == S_HANDLE_NULL) {
		return s_vec2(0.0f, 0.0f);
	}
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, widget_handle);
	if (!widget) {
		return s_vec2(0.0f, 0.0f);
	}

	f32 measured_width = s_max(0.0f, widget->desc.size.x * 2.0f);
	f32 measured_height = s_max(0.0f, widget->desc.size.y * 2.0f);
	f32 child_total_w = 0.0f;
	f32 child_total_h = 0.0f;
	f32 child_max_w = 0.0f;
	f32 child_max_h = 0.0f;
	u32 flow_count = 0;

	for (sz i = 0; i < s_array_get_size(&widget->children); ++i) {
		se_ui_widget_handle* child_handle = s_array_get(&widget->children, s_array_handle(&widget->children, (u32)i));
		if (!child_handle || *child_handle == S_HANDLE_NULL) {
			continue;
		}
		se_ui_widget* child = se_ui_widget_from_handle(ctx, *child_handle);
		if (!child) {
			continue;
		}
		const s_vec2 child_half = se_ui_measure_recursive(root, *child_handle);
		if (child->desc.layout.use_anchors) {
			continue;
		}
		const f32 child_w = child_half.x * 2.0f + child->desc.layout.margin.left + child->desc.layout.margin.right;
		const f32 child_h = child_half.y * 2.0f + child->desc.layout.margin.top + child->desc.layout.margin.bottom;
		child_total_w += child_w;
		child_total_h += child_h;
		child_max_w = s_max(child_max_w, child_w);
		child_max_h = s_max(child_max_h, child_h);
		flow_count++;
	}

	if (flow_count > 0) {
		if (widget->desc.layout.direction == SE_UI_LAYOUT_VERTICAL) {
			child_total_h += widget->desc.layout.spacing * (f32)(flow_count - 1);
			measured_width = measured_width > 0.0f ? measured_width : child_max_w;
			measured_height = measured_height > 0.0f ? measured_height : child_total_h;
		} else {
			child_total_w += widget->desc.layout.spacing * (f32)(flow_count - 1);
			measured_width = measured_width > 0.0f ? measured_width : child_total_w;
			measured_height = measured_height > 0.0f ? measured_height : child_max_h;
		}
	}

	measured_width += widget->desc.layout.padding.left + widget->desc.layout.padding.right;
	measured_height += widget->desc.layout.padding.top + widget->desc.layout.padding.bottom;

	if (widget->desc.layout.min_size.x > 0.0f) {
		measured_width = s_max(measured_width, widget->desc.layout.min_size.x * 2.0f);
	}
	if (widget->desc.layout.min_size.y > 0.0f) {
		measured_height = s_max(measured_height, widget->desc.layout.min_size.y * 2.0f);
	}
	if (widget->desc.layout.max_size.x > 0.0f) {
		measured_width = s_min(measured_width, widget->desc.layout.max_size.x * 2.0f);
	}
	if (widget->desc.layout.max_size.y > 0.0f) {
		measured_height = s_min(measured_height, widget->desc.layout.max_size.y * 2.0f);
	}

	widget->measured_half_size = s_vec2(measured_width * 0.5f, measured_height * 0.5f);
	return widget->measured_half_size;
}

static se_box_2d se_ui_compute_anchor_bounds(const se_ui_widget* widget, const se_box_2d* parent_content) {
	if (!widget || !parent_content) {
		return se_ui_make_box(0.0f, 0.0f, 0.0f, 0.0f);
	}
	const f32 parent_w = se_ui_box_width(parent_content);
	const f32 parent_h = se_ui_box_height(parent_content);
	const f32 anchor_left = parent_content->min.x + parent_w * se_ui_clamp01(widget->desc.layout.anchor_min.x) + widget->desc.layout.margin.left;
	const f32 anchor_right = parent_content->min.x + parent_w * se_ui_clamp01(widget->desc.layout.anchor_max.x) - widget->desc.layout.margin.right;
	const f32 anchor_bottom = parent_content->min.y + parent_h * se_ui_clamp01(widget->desc.layout.anchor_min.y) + widget->desc.layout.margin.bottom;
	const f32 anchor_top = parent_content->min.y + parent_h * se_ui_clamp01(widget->desc.layout.anchor_max.y) - widget->desc.layout.margin.top;
	const f32 available_w = s_max(0.0f, anchor_right - anchor_left);
	const f32 available_h = s_max(0.0f, anchor_top - anchor_bottom);

	f32 child_w = widget->measured_half_size.x * 2.0f;
	f32 child_h = widget->measured_half_size.y * 2.0f;

	if (widget->desc.layout.align_horizontal == SE_UI_ALIGN_STRETCH) {
		child_w = available_w;
	} else {
		child_w = s_min(child_w, available_w);
	}
	if (widget->desc.layout.align_vertical == SE_UI_ALIGN_STRETCH) {
		child_h = available_h;
	} else {
		child_h = s_min(child_h, available_h);
	}

	f32 x = (anchor_left + anchor_right) * 0.5f;
	if (widget->desc.layout.align_horizontal == SE_UI_ALIGN_START) {
		x = anchor_left + child_w * 0.5f;
	} else if (widget->desc.layout.align_horizontal == SE_UI_ALIGN_END) {
		x = anchor_right - child_w * 0.5f;
	}

	f32 y = (anchor_bottom + anchor_top) * 0.5f;
	if (widget->desc.layout.align_vertical == SE_UI_ALIGN_START) {
		y = anchor_top - child_h * 0.5f;
	} else if (widget->desc.layout.align_vertical == SE_UI_ALIGN_END) {
		y = anchor_bottom + child_h * 0.5f;
	}

	return se_ui_make_box(x - child_w * 0.5f, y - child_h * 0.5f, x + child_w * 0.5f, y + child_h * 0.5f);
}

static void se_ui_layout_children(se_ui_root* root, const se_ui_widget_handle parent_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || parent_handle == S_HANDLE_NULL) {
		return;
	}
	se_ui_widget* parent = se_ui_widget_from_handle(ctx, parent_handle);
	if (!parent) {
		return;
	}
	se_ui_sort_widget_handles(ctx, &parent->children);

	se_ui_widget_handles flow_children = {0};
	se_ui_widget_handles anchored_children = {0};
	s_array_init(&flow_children);
	s_array_init(&anchored_children);

	for (sz i = 0; i < s_array_get_size(&parent->children); ++i) {
		se_ui_widget_handle* child_handle = s_array_get(&parent->children, s_array_handle(&parent->children, (u32)i));
		if (!child_handle || *child_handle == S_HANDLE_NULL) {
			continue;
		}
		se_ui_widget* child = se_ui_widget_from_handle(ctx, *child_handle);
		if (!child) {
			continue;
		}
		if (child->desc.layout.use_anchors) {
			s_array_add(&anchored_children, *child_handle);
		} else {
			s_array_add(&flow_children, *child_handle);
		}
	}

	se_box_2d content = parent->content_bounds;
	if (parent->type == SE_UI_WIDGET_SCROLLBOX) {
		content.max.x -= SE_UI_SCROLLBAR_WIDTH;
		if (content.max.x < content.min.x) {
			content.max.x = content.min.x;
		}
	}

	const f32 content_w = se_ui_box_width(&content);
	const f32 content_h = se_ui_box_height(&content);
	const u32 flow_count = (u32)s_array_get_size(&flow_children);
	f32 total_main = 0.0f;
	for (sz i = 0; i < s_array_get_size(&flow_children); ++i) {
		se_ui_widget_handle* child_handle = s_array_get(&flow_children, s_array_handle(&flow_children, (u32)i));
		se_ui_widget* child = child_handle ? se_ui_widget_from_handle(ctx, *child_handle) : NULL;
		if (!child) {
			continue;
		}
		if (parent->desc.layout.direction == SE_UI_LAYOUT_VERTICAL) {
			total_main += child->measured_half_size.y * 2.0f + child->desc.layout.margin.top + child->desc.layout.margin.bottom;
		} else {
			total_main += child->measured_half_size.x * 2.0f + child->desc.layout.margin.left + child->desc.layout.margin.right;
		}
	}
	if (flow_count > 1) {
		total_main += parent->desc.layout.spacing * (f32)(flow_count - 1);
	}

	f32 extra_main = 0.0f;
	if (parent->desc.layout.direction == SE_UI_LAYOUT_VERTICAL) {
		extra_main = s_max(0.0f, content_h - total_main);
	} else {
		extra_main = s_max(0.0f, content_w - total_main);
	}
	f32 stretch_bonus = 0.0f;
	if (flow_count > 0) {
		if (parent->desc.layout.direction == SE_UI_LAYOUT_VERTICAL && parent->desc.layout.align_vertical == SE_UI_ALIGN_STRETCH) {
			stretch_bonus = extra_main / (f32)flow_count;
		} else if (parent->desc.layout.direction == SE_UI_LAYOUT_HORIZONTAL && parent->desc.layout.align_horizontal == SE_UI_ALIGN_STRETCH) {
			stretch_bonus = extra_main / (f32)flow_count;
		}
	}

	f32 cursor = 0.0f;
	if (parent->desc.layout.direction == SE_UI_LAYOUT_VERTICAL) {
		f32 offset = 0.0f;
		if (parent->desc.layout.align_vertical == SE_UI_ALIGN_CENTER) {
			offset = extra_main * 0.5f;
		} else if (parent->desc.layout.align_vertical == SE_UI_ALIGN_END) {
			offset = extra_main;
		}
		cursor = content.max.y - offset;
	} else {
		f32 offset = 0.0f;
		if (parent->desc.layout.align_horizontal == SE_UI_ALIGN_CENTER) {
			offset = extra_main * 0.5f;
		} else if (parent->desc.layout.align_horizontal == SE_UI_ALIGN_END) {
			offset = extra_main;
		}
		cursor = content.min.x + offset;
	}

	for (sz i = 0; i < s_array_get_size(&flow_children); ++i) {
		se_ui_widget_handle* child_handle = s_array_get(&flow_children, s_array_handle(&flow_children, (u32)i));
		se_ui_widget* child = child_handle ? se_ui_widget_from_handle(ctx, *child_handle) : NULL;
		if (!child) {
			continue;
		}

		f32 child_w = child->measured_half_size.x * 2.0f;
		f32 child_h = child->measured_half_size.y * 2.0f;
		if (parent->desc.layout.direction == SE_UI_LAYOUT_VERTICAL) {
			child_h += stretch_bonus;
			cursor -= child->desc.layout.margin.top;
			const f32 slot_top = cursor;
			const f32 slot_bottom = slot_top - child_h;
			const f32 available_w = s_max(0.0f, content_w - child->desc.layout.margin.left - child->desc.layout.margin.right);
			if (child->desc.layout.align_horizontal == SE_UI_ALIGN_STRETCH) {
				child_w = available_w;
			} else {
				child_w = s_min(child_w, available_w);
			}
			f32 x = (content.min.x + content.max.x) * 0.5f;
			if (child->desc.layout.align_horizontal == SE_UI_ALIGN_START) {
				x = content.min.x + child->desc.layout.margin.left + child_w * 0.5f;
			} else if (child->desc.layout.align_horizontal == SE_UI_ALIGN_END) {
				x = content.max.x - child->desc.layout.margin.right - child_w * 0.5f;
			}
			const f32 y = (slot_top + slot_bottom) * 0.5f;
			child->bounds = se_ui_make_box(x - child_w * 0.5f, y - child_h * 0.5f, x + child_w * 0.5f, y + child_h * 0.5f);
			cursor = slot_bottom - child->desc.layout.margin.bottom;
			if (i + 1 < s_array_get_size(&flow_children)) {
				cursor -= parent->desc.layout.spacing;
			}
		} else {
			child_w += stretch_bonus;
			cursor += child->desc.layout.margin.left;
			const f32 slot_left = cursor;
			const f32 slot_right = slot_left + child_w;
			const f32 available_h = s_max(0.0f, content_h - child->desc.layout.margin.top - child->desc.layout.margin.bottom);
			if (child->desc.layout.align_vertical == SE_UI_ALIGN_STRETCH) {
				child_h = available_h;
			} else {
				child_h = s_min(child_h, available_h);
			}
			f32 y = (content.min.y + content.max.y) * 0.5f;
			if (child->desc.layout.align_vertical == SE_UI_ALIGN_START) {
				y = content.max.y - child->desc.layout.margin.top - child_h * 0.5f;
			} else if (child->desc.layout.align_vertical == SE_UI_ALIGN_END) {
				y = content.min.y + child->desc.layout.margin.bottom + child_h * 0.5f;
			}
			const f32 x = (slot_left + slot_right) * 0.5f;
			child->bounds = se_ui_make_box(x - child_w * 0.5f, y - child_h * 0.5f, x + child_w * 0.5f, y + child_h * 0.5f);
			cursor = slot_right + child->desc.layout.margin.right;
			if (i + 1 < s_array_get_size(&flow_children)) {
				cursor += parent->desc.layout.spacing;
			}
		}
	}

	if (parent->type == SE_UI_WIDGET_SCROLLBOX) {
		const f32 viewport_main = (parent->desc.layout.direction == SE_UI_LAYOUT_VERTICAL) ? content_h : content_w;
		parent->viewport_extent = viewport_main;
		parent->content_extent = total_main;
		parent->value = se_ui_clamp01(parent->value);
		const f32 overflow = s_max(0.0f, total_main - viewport_main);
		const f32 offset = overflow * parent->value;
		for (sz i = 0; i < s_array_get_size(&flow_children); ++i) {
			se_ui_widget_handle* child_handle = s_array_get(&flow_children, s_array_handle(&flow_children, (u32)i));
			se_ui_widget* child = child_handle ? se_ui_widget_from_handle(ctx, *child_handle) : NULL;
			if (!child) {
				continue;
			}
			if (parent->desc.layout.direction == SE_UI_LAYOUT_VERTICAL) {
				child->bounds = se_ui_box_translate(&child->bounds, 0.0f, offset);
			} else {
				child->bounds = se_ui_box_translate(&child->bounds, -offset, 0.0f);
			}
		}

		if (overflow > 0.0001f) {
			const se_box_2d track = se_ui_make_box(content.max.x, content.min.y, parent->content_bounds.max.x, parent->content_bounds.max.y);
			parent->scroll_track_bounds = track;
			const f32 track_len = se_ui_box_height(&track);
			const f32 thumb_ratio = se_ui_clampf(parent->viewport_extent / s_max(parent->content_extent, 0.0001f), 0.12f, 1.0f);
			const f32 thumb_len = track_len * thumb_ratio;
			const f32 travel = s_max(0.0f, track_len - thumb_len);
			const f32 thumb_top = track.max.y - travel * parent->value;
			parent->scroll_thumb_bounds = se_ui_make_box(track.min.x, thumb_top - thumb_len, track.max.x, thumb_top);
		} else {
			parent->scroll_track_bounds = se_ui_make_box(0.0f, 0.0f, 0.0f, 0.0f);
			parent->scroll_thumb_bounds = se_ui_make_box(0.0f, 0.0f, 0.0f, 0.0f);
		}
	}

	for (sz i = 0; i < s_array_get_size(&anchored_children); ++i) {
		se_ui_widget_handle* child_handle = s_array_get(&anchored_children, s_array_handle(&anchored_children, (u32)i));
		se_ui_widget* child = child_handle ? se_ui_widget_from_handle(ctx, *child_handle) : NULL;
		if (!child) {
			continue;
		}
		child->bounds = se_ui_compute_anchor_bounds(child, &content);
	}

	s_array_clear(&flow_children);
	s_array_clear(&anchored_children);
}

static void se_ui_layout_recursive(se_ui_root* root, const se_ui_widget_handle widget_handle, const se_box_2d* parent_content, const se_box_2d* parent_clip, const b8 parent_visible) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || widget_handle == S_HANDLE_NULL || !parent_content) {
		return;
	}
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, widget_handle);
	if (!widget) {
		return;
	}

	if (widget->parent == S_HANDLE_NULL) {
		if (widget->desc.layout.use_anchors) {
			widget->bounds = se_ui_compute_anchor_bounds(widget, parent_content);
		} else {
			const s_vec2 half = widget->measured_half_size;
			widget->bounds = se_ui_make_box(
				widget->desc.position.x - half.x,
				widget->desc.position.y - half.y,
				widget->desc.position.x + half.x,
				widget->desc.position.y + half.y);
		}
	}

	widget->content_bounds = widget->bounds;
	widget->content_bounds.min.x += widget->desc.layout.padding.left;
	widget->content_bounds.max.x -= widget->desc.layout.padding.right;
	widget->content_bounds.min.y += widget->desc.layout.padding.bottom;
	widget->content_bounds.max.y -= widget->desc.layout.padding.top;
	if (widget->content_bounds.max.x < widget->content_bounds.min.x) {
		widget->content_bounds.max.x = widget->content_bounds.min.x;
	}
	if (widget->content_bounds.max.y < widget->content_bounds.min.y) {
		widget->content_bounds.max.y = widget->content_bounds.min.y;
	}

	widget->computed_visible = parent_visible && widget->desc.visible;
	if (parent_clip) {
		widget->effective_clip_bounds = se_ui_box_intersection(parent_clip, &widget->bounds);
		widget->has_effective_clip = true;
		if (!se_ui_box_intersects(&widget->effective_clip_bounds, &widget->bounds)) {
			widget->computed_visible = false;
		}
	} else {
		widget->effective_clip_bounds = widget->bounds;
		widget->has_effective_clip = false;
	}

	se_box_2d child_clip = widget->effective_clip_bounds;
	const se_box_2d* child_clip_ptr = parent_clip;
	if (widget->desc.layout.clip_children || widget->type == SE_UI_WIDGET_SCROLLBOX) {
		child_clip = se_ui_box_intersection(parent_clip, &widget->content_bounds);
		child_clip_ptr = &child_clip;
	}

	se_ui_layout_children(root, widget_handle);

	for (sz i = 0; i < s_array_get_size(&widget->children); ++i) {
		se_ui_widget_handle* child = s_array_get(&widget->children, s_array_handle(&widget->children, (u32)i));
		if (!child || *child == S_HANDLE_NULL) {
			continue;
		}
		se_ui_layout_recursive(root, *child, &widget->content_bounds, child_clip_ptr, widget->computed_visible);
	}
}

static void se_ui_rebuild_draw_order(se_ui_root* root) {
	se_context* ctx = se_current_context();
	if (!ctx || !root) {
		return;
	}
	const se_ui_handle ui_handle = se_ui_root_handle(ctx, root);
	s_array_clear(&root->draw_order);
	s_array_init(&root->draw_order);
	for (sz i = 0; i < s_array_get_size(&ctx->ui_elements); ++i) {
		const se_ui_widget_handle handle = s_array_handle(&ctx->ui_elements, (u32)i);
		se_ui_widget* widget = se_ui_widget_from_handle(ctx, handle);
		if (!widget || widget->ui != ui_handle) {
			continue;
		}
		s_array_add(&root->draw_order, handle);
	}
	se_ui_sort_widget_handles(ctx, &root->draw_order);
}

static se_ui_handle se_ui_root_handle_from_ptr(se_context* ctx, se_ui_root* root) {
	if (!ctx || !root) {
		return S_HANDLE_NULL;
	}
	for (sz i = 0; i < s_array_get_size(&ctx->ui_roots); ++i) {
		se_ui_root* candidate = s_array_get(&ctx->ui_roots, s_array_handle(&ctx->ui_roots, (u32)i));
		if (candidate == root) {
			return s_array_handle(&ctx->ui_roots, (u32)i);
		}
	}
	return S_HANDLE_NULL;
}

static se_ui_handle se_ui_root_handle(se_context* ctx, se_ui_root* root) {
	return se_ui_root_handle_from_ptr(ctx, root);
}

static void se_ui_rebuild_scene_draw_list(se_ui_root* root) {
	se_context* ctx = se_current_context();
	if (!ctx || !root) {
		return;
	}
	se_scene_2d* scene = s_array_get(&ctx->scenes_2d, root->scene);
	if (!scene) {
		return;
	}
	s_array_clear(&scene->objects);
	s_array_init(&scene->objects);
	s_array_reserve(&scene->objects, s_array_get_size(&root->draw_order) * 8);

	for (sz i = 0; i < s_array_get_size(&root->draw_order); ++i) {
		se_ui_widget_handle* handle = s_array_get(&root->draw_order, s_array_handle(&root->draw_order, (u32)i));
		se_ui_widget* widget = handle ? se_ui_widget_from_handle(ctx, *handle) : NULL;
		if (!widget) {
			continue;
		}
		if (widget->background_object != S_HANDLE_NULL) s_array_add(&scene->objects, widget->background_object);
		if (widget->selection_object != S_HANDLE_NULL) s_array_add(&scene->objects, widget->selection_object);
		if (widget->text_object != S_HANDLE_NULL) s_array_add(&scene->objects, widget->text_object);
		if (widget->caret_object != S_HANDLE_NULL) s_array_add(&scene->objects, widget->caret_object);
		if (widget->scrollbar_track_object != S_HANDLE_NULL) s_array_add(&scene->objects, widget->scrollbar_track_object);
		if (widget->scrollbar_thumb_object != S_HANDLE_NULL) s_array_add(&scene->objects, widget->scrollbar_thumb_object);
	}
}

static void se_ui_widget_update_visual_recursive(se_ui_root* root, const se_ui_widget_handle widget_handle, const se_box_2d* parent_clip, const b8 parent_visible) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || widget_handle == S_HANDLE_NULL) {
		return;
	}
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, widget_handle);
	if (!widget) {
		return;
	}
	const b8 visible = parent_visible && widget->computed_visible;
	const se_ui_style_state state = se_ui_widget_state_style(widget);

	se_box_2d draw_bounds = widget->bounds;
	if (parent_clip) {
		draw_bounds = se_ui_box_intersection(parent_clip, &widget->bounds);
	}
	b8 has_area = se_ui_box_width(&draw_bounds) > 0.00001f && se_ui_box_height(&draw_bounds) > 0.00001f;

	if (widget->background_object != S_HANDLE_NULL) {
		const b8 has_fill = state.background_color.w > 0.001f;
		const b8 has_border = state.border_width > 0.00001f && state.border_color.w > 0.001f;
		se_ui_object_set_rect_style(
			ctx,
			widget->background_object,
			&widget->bounds,
			&state.background_color,
			&state.border_color,
			state.border_width,
			NULL,
			visible && has_area && (has_fill || has_border));
	}

	if (widget->text_object != S_HANDLE_NULL) {
		const c8* text_ptr = se_ui_chars_cstr(&widget->text);
		if (widget->type == SE_UI_WIDGET_TEXTBOX && text_ptr[0] == '\0' && !widget->focused) {
			text_ptr = se_ui_chars_cstr(&widget->placeholder);
		}
		c8 textbox_display_with_caret[SE_TEXT_CHAR_COUNT] = {0};
		const c8* draw_text = text_ptr;
		const u32 text_len = (u32)strlen(text_ptr);
		if (widget->type == SE_UI_WIDGET_TEXTBOX && widget->focused) {
			se_ui_textbox_build_display_text_with_caret(text_ptr, widget->caret, textbox_display_with_caret, sizeof(textbox_display_with_caret));
			draw_text = textbox_display_with_caret;
		}
		const f32 x = se_ui_widget_text_start_x(widget, text_len);
		const f32 y_center = (widget->content_bounds.min.y + widget->content_bounds.max.y) * 0.5f;
		s_vec2 text_pos = s_vec2(x, y_center);
		se_ui_update_text_proxy(widget, draw_text, &text_pos, visible && draw_text[0] != '\0');
	}

	if (widget->type == SE_UI_WIDGET_TEXTBOX) {
		const b8 has_selection = widget->selection_end > widget->selection_start;
		const c8* text_ptr = se_ui_chars_cstr(&widget->text);
		const u32 text_len = (u32)strlen(text_ptr);
		const f32 char_w = widget->font_size * SE_UI_TEXT_CHAR_WIDTH_FACTOR;
		const f32 base_x = se_ui_widget_text_start_x(widget, text_len);
		const f32 y_mid = (widget->content_bounds.min.y + widget->content_bounds.max.y) * 0.5f;
		if (widget->selection_object != S_HANDLE_NULL && has_selection) {
			const u32 a = (u32)s_min(widget->selection_start, widget->selection_end);
			const u32 b = (u32)s_max(widget->selection_start, widget->selection_end);
			const se_box_2d selection_bounds = se_ui_make_box(
				base_x + (f32)a * char_w,
				y_mid - 0.018f,
				base_x + (f32)b * char_w,
				y_mid + 0.018f);
			const s_vec4 selection_color = s_vec4(0.22f, 0.46f, 0.82f, 0.40f);
			se_ui_object_set_rect_color(ctx, widget->selection_object, &selection_bounds, &selection_color, visible);
		} else if (widget->selection_object != S_HANDLE_NULL) {
			se_ui_object_set_visible(ctx, widget->selection_object, false);
		}
		if (widget->caret_object != S_HANDLE_NULL) {
			se_ui_object_set_visible(ctx, widget->caret_object, false);
		}
	}

	if (widget->type == SE_UI_WIDGET_SCROLLBOX) {
		const s_vec4 track_color = s_vec4(0.08f, 0.10f, 0.12f, 0.65f);
		const s_vec4 thumb_color = widget->thumb_dragging ? s_vec4(0.30f, 0.72f, 0.62f, 0.95f) : s_vec4(0.22f, 0.56f, 0.48f, 0.90f);
		const b8 has_scroll = widget->content_extent > widget->viewport_extent + 0.0001f;
		if (widget->scrollbar_track_object != S_HANDLE_NULL) {
			se_ui_object_set_rect_color(ctx, widget->scrollbar_track_object, &widget->scroll_track_bounds, &track_color, visible && has_scroll);
		}
		if (widget->scrollbar_thumb_object != S_HANDLE_NULL) {
			se_ui_object_set_rect_color(ctx, widget->scrollbar_thumb_object, &widget->scroll_thumb_bounds, &thumb_color, visible && has_scroll);
		}
	}

	const se_box_2d* child_clip = parent_clip;
	se_box_2d local_clip = {0};
	if (widget->desc.layout.clip_children || widget->type == SE_UI_WIDGET_SCROLLBOX) {
		local_clip = se_ui_box_intersection(parent_clip, &widget->content_bounds);
		child_clip = &local_clip;
	}
	for (sz i = 0; i < s_array_get_size(&widget->children); ++i) {
		se_ui_widget_handle* child = s_array_get(&widget->children, s_array_handle(&widget->children, (u32)i));
		if (!child || *child == S_HANDLE_NULL) {
			continue;
		}
		se_ui_widget_update_visual_recursive(root, *child, child_clip, visible);
	}
}

static void se_ui_rebuild(se_ui_root* root) {
	se_context* ctx = se_current_context();
	if (!ctx || !root) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&ctx->ui_elements); ++i) {
		se_ui_widget* widget = s_array_get(&ctx->ui_elements, s_array_handle(&ctx->ui_elements, (u32)i));
		if (!widget) {
			continue;
		}
		if (widget->ui != se_ui_root_handle(ctx, root)) {
			continue;
		}
		if (widget->parent != S_HANDLE_NULL) {
			continue;
		}
		(void)se_ui_measure_recursive(root, s_array_handle(&ctx->ui_elements, (u32)i));
	}

	const se_box_2d full_view = se_ui_make_box(-1.0f, -1.0f, 1.0f, 1.0f);
	for (sz i = 0; i < s_array_get_size(&ctx->ui_elements); ++i) {
		const se_ui_widget_handle handle = s_array_handle(&ctx->ui_elements, (u32)i);
		se_ui_widget* widget = se_ui_widget_from_handle(ctx, handle);
		if (!widget || widget->ui != se_ui_root_handle(ctx, root) || widget->parent != S_HANDLE_NULL) {
			continue;
		}
		se_ui_layout_recursive(root, handle, &full_view, NULL, true);
	}

	se_ui_rebuild_draw_order(root);
	for (sz i = 0; i < s_array_get_size(&ctx->ui_elements); ++i) {
		const se_ui_widget_handle handle = s_array_handle(&ctx->ui_elements, (u32)i);
		se_ui_widget* widget = se_ui_widget_from_handle(ctx, handle);
		if (!widget || widget->ui != se_ui_root_handle(ctx, root) || widget->parent != S_HANDLE_NULL) {
			continue;
		}
		se_ui_widget_update_visual_recursive(root, handle, NULL, true);
	}
	se_ui_rebuild_scene_draw_list(root);
}

static se_ui_widget_handle se_ui_hit_test(se_ui_root* root, const s_vec2* point_ndc) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || !point_ndc) {
		return S_HANDLE_NULL;
	}
	for (sz i = s_array_get_size(&root->draw_order); i > 0; --i) {
		se_ui_widget_handle* handle = s_array_get(&root->draw_order, s_array_handle(&root->draw_order, (u32)(i - 1)));
		se_ui_widget* widget = handle ? se_ui_widget_from_handle(ctx, *handle) : NULL;
		if (!widget || !widget->computed_visible || !widget->desc.visible || !widget->desc.enabled || !widget->desc.interactable) {
			continue;
		}
		if (!se_ui_box_contains(&widget->bounds, point_ndc)) {
			continue;
		}
		if (widget->has_effective_clip && !se_ui_box_contains(&widget->effective_clip_bounds, point_ndc)) {
			continue;
		}
		return *handle;
	}
	return S_HANDLE_NULL;
}

static void se_ui_fire_hover_start(se_ui_root* root, const se_ui_widget_handle widget_handle, const s_vec2* point_ndc) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, widget_handle);
	if (!widget) {
		return;
	}
	widget->hovered = true;
	if (widget->callbacks.on_hover_start) {
		se_ui_hover_event event = {
			.ui = widget->ui,
			.widget = widget_handle,
			.pointer_ndc = point_ndc ? *point_ndc : s_vec2(0.0f, 0.0f)
		};
		widget->callbacks.on_hover_start(&event, se_ui_callback_data_or_default(widget->callbacks.on_hover_start_data, widget->callbacks.user_data));
	}
	se_ui_mark_visual_dirty_internal(root);
}

static void se_ui_fire_hover_end(se_ui_root* root, const se_ui_widget_handle widget_handle, const s_vec2* point_ndc) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, widget_handle);
	if (!widget) {
		return;
	}
	widget->hovered = false;
	if (widget->callbacks.on_hover_end) {
		se_ui_hover_event event = {
			.ui = widget->ui,
			.widget = widget_handle,
			.pointer_ndc = point_ndc ? *point_ndc : s_vec2(0.0f, 0.0f)
		};
		widget->callbacks.on_hover_end(&event, se_ui_callback_data_or_default(widget->callbacks.on_hover_end_data, widget->callbacks.user_data));
	}
	se_ui_mark_visual_dirty_internal(root);
}

static void se_ui_set_focus_internal(se_ui_root* root, const se_ui_widget_handle widget_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || root->focused_widget == widget_handle) {
		return;
	}
	if (root->focused_widget != S_HANDLE_NULL) {
		se_ui_widget* old_focus = se_ui_widget_from_handle(ctx, root->focused_widget);
		if (old_focus) {
			old_focus->focused = false;
			if (old_focus->callbacks.on_blur) {
				se_ui_focus_event event = { .ui = old_focus->ui, .widget = root->focused_widget };
				old_focus->callbacks.on_blur(&event, se_ui_callback_data_or_default(old_focus->callbacks.on_blur_data, old_focus->callbacks.user_data));
			}
		}
	}
	root->focused_widget = widget_handle;
	if (widget_handle != S_HANDLE_NULL) {
		se_ui_widget* new_focus = se_ui_widget_from_handle(ctx, widget_handle);
		if (new_focus) {
			new_focus->focused = true;
			if (new_focus->callbacks.on_focus) {
				se_ui_focus_event event = { .ui = new_focus->ui, .widget = widget_handle };
				new_focus->callbacks.on_focus(&event, se_ui_callback_data_or_default(new_focus->callbacks.on_focus_data, new_focus->callbacks.user_data));
			}
		}
	}
	root->caret_visible = true;
	root->caret_blink_accumulator = 0.0;
	se_ui_mark_visual_dirty_internal(root);
}

static void se_ui_fire_press(se_ui_root* root, const se_ui_widget_handle widget_handle, const s_vec2* point_ndc, const b8 pressed_state, const se_mouse_button button) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, widget_handle);
	if (!widget) {
		return;
	}
	widget->pressed = pressed_state;
	se_ui_press_callback callback = pressed_state ? widget->callbacks.on_press : widget->callbacks.on_release;
	void* callback_data = pressed_state
		? se_ui_callback_data_or_default(widget->callbacks.on_press_data, widget->callbacks.user_data)
		: se_ui_callback_data_or_default(widget->callbacks.on_release_data, widget->callbacks.user_data);
	if (callback) {
		se_ui_press_event event = {
			.ui = widget->ui,
			.widget = widget_handle,
			.pointer_ndc = point_ndc ? *point_ndc : s_vec2(0.0f, 0.0f),
			.button = button
		};
		callback(&event, callback_data);
	}
	se_ui_mark_visual_dirty_internal(root);
}

static void se_ui_fire_click(se_ui_widget* widget, const se_ui_widget_handle widget_handle, const s_vec2* point_ndc, const se_mouse_button button) {
	if (!widget || !widget->callbacks.on_click) {
		return;
	}
	se_ui_click_event event = {
		.ui = widget->ui,
		.widget = widget_handle,
		.pointer_ndc = point_ndc ? *point_ndc : s_vec2(0.0f, 0.0f),
		.button = button
	};
	widget->callbacks.on_click(&event, se_ui_callback_data_or_default(widget->callbacks.on_click_data, widget->callbacks.user_data));
}

static void se_ui_fire_change(se_ui_widget* widget, const se_ui_widget_handle widget_handle) {
	if (!widget || !widget->callbacks.on_change) {
		return;
	}
	se_ui_change_event event = {
		.ui = widget->ui,
		.widget = widget_handle,
		.value = widget->value,
		.text = se_ui_chars_cstr(&widget->text)
	};
	widget->callbacks.on_change(&event, se_ui_callback_data_or_default(widget->callbacks.on_change_data, widget->callbacks.user_data));
}

static void se_ui_fire_submit(se_ui_widget* widget, const se_ui_widget_handle widget_handle) {
	if (!widget || !widget->callbacks.on_submit) {
		return;
	}
	se_ui_submit_event event = {
		.ui = widget->ui,
		.widget = widget_handle,
		.text = se_ui_chars_cstr(&widget->text)
	};
	widget->callbacks.on_submit(&event, se_ui_callback_data_or_default(widget->callbacks.on_submit_data, widget->callbacks.user_data));
}

static void se_ui_fire_scroll(se_ui_widget* widget, const se_ui_widget_handle widget_handle, const s_vec2* point_ndc) {
	if (!widget || !widget->callbacks.on_scroll) {
		return;
	}
	se_ui_scroll_event event = {
		.ui = widget->ui,
		.widget = widget_handle,
		.value = widget->value,
		.pointer_ndc = point_ndc ? *point_ndc : s_vec2(0.0f, 0.0f)
	};
	widget->callbacks.on_scroll(&event, se_ui_callback_data_or_default(widget->callbacks.on_scroll_data, widget->callbacks.user_data));
}

static se_ui_widget_handle se_ui_widget_find_scrollbox_ancestor(se_context* ctx, const se_ui_widget_handle widget_handle) {
	if (!ctx || widget_handle == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_ui_widget_handle current = widget_handle;
	while (current != S_HANDLE_NULL) {
		se_ui_widget* widget = se_ui_widget_from_handle(ctx, current);
		if (!widget) {
			return S_HANDLE_NULL;
		}
		if (widget->type == SE_UI_WIDGET_SCROLLBOX) {
			return current;
		}
		current = widget->parent;
	}
	return S_HANDLE_NULL;
}

static void se_ui_scrollbox_refresh_item_styles(se_ui_root* root, se_ui_widget* scrollbox, const se_ui_widget_handle scrollbox_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || !scrollbox || scrollbox->type != SE_UI_WIDGET_SCROLLBOX) {
		return;
	}
	if (!scrollbox->has_scroll_item_styles) {
		return;
	}
	b8 changed = false;
	for (sz i = 0; i < s_array_get_size(&scrollbox->children); ++i) {
		se_ui_widget_handle* child_handle = s_array_get(&scrollbox->children, s_array_handle(&scrollbox->children, (u32)i));
		se_ui_widget* child = child_handle ? se_ui_widget_from_handle(ctx, *child_handle) : NULL;
		if (!child) {
			continue;
		}
		const b8 selected = (*child_handle == scrollbox->selected_item);
		const se_ui_style* target = selected ? &scrollbox->scroll_item_style_selected : &scrollbox->scroll_item_style_normal;
		if (!se_ui_style_equal(&child->desc.style, target)) {
			child->desc.style = *target;
			changed = true;
		}
	}
	if (changed) {
		se_ui_mark_visual_dirty_internal(root);
	}
	if (scrollbox->selected_item != S_HANDLE_NULL && !se_ui_widget_exists(scrollbox->ui, scrollbox->selected_item)) {
		scrollbox->selected_item = S_HANDLE_NULL;
	}
	if (scrollbox->selected_item != S_HANDLE_NULL) {
		se_ui_widget* selected = se_ui_widget_from_handle(ctx, scrollbox->selected_item);
		if (!selected || selected->parent != scrollbox_handle) {
			scrollbox->selected_item = S_HANDLE_NULL;
		}
	}
}

static b8 se_ui_scrollbox_set_selected_internal(
	se_ui_root* root,
	const se_ui_widget_handle scrollbox_handle,
	const se_ui_widget_handle item_handle,
	const b8 fire_callbacks) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || scrollbox_handle == S_HANDLE_NULL) {
		return false;
	}
	se_ui_widget* scrollbox = se_ui_widget_from_handle(ctx, scrollbox_handle);
	if (!scrollbox || scrollbox->type != SE_UI_WIDGET_SCROLLBOX) {
		return false;
	}
	if (!scrollbox->single_select_enabled) {
		return false;
	}
	if (item_handle != S_HANDLE_NULL) {
		se_ui_widget* item = se_ui_widget_from_handle(ctx, item_handle);
		if (!item || item->ui != scrollbox->ui || item->parent != scrollbox_handle) {
			return false;
		}
	}
	if (scrollbox->selected_item == item_handle) {
		return true;
	}
	scrollbox->selected_item = item_handle;
	se_ui_scrollbox_refresh_item_styles(root, scrollbox, scrollbox_handle);
	se_ui_mark_visual_dirty_internal(root);
	if (fire_callbacks) {
		se_ui_fire_change(scrollbox, scrollbox_handle);
	}
	return true;
}

static void se_ui_scrollbox_item_removed(se_ui_root* root, const se_ui_widget_handle scrollbox_handle, const se_ui_widget_handle item_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || scrollbox_handle == S_HANDLE_NULL || item_handle == S_HANDLE_NULL) {
		return;
	}
	se_ui_widget* scrollbox = se_ui_widget_from_handle(ctx, scrollbox_handle);
	if (!scrollbox || scrollbox->type != SE_UI_WIDGET_SCROLLBOX) {
		return;
	}
	if (scrollbox->selected_item != item_handle) {
		se_ui_scrollbox_refresh_item_styles(root, scrollbox, scrollbox_handle);
		return;
	}
	se_ui_widget_handle fallback = S_HANDLE_NULL;
	for (sz i = 0; i < s_array_get_size(&scrollbox->children); ++i) {
		se_ui_widget_handle* child = s_array_get(&scrollbox->children, s_array_handle(&scrollbox->children, (u32)i));
		if (!child || *child == S_HANDLE_NULL || *child == item_handle) {
			continue;
		}
		fallback = *child;
		break;
	}
	(void)se_ui_scrollbox_set_selected_internal(root, scrollbox_handle, fallback, !root->destroying);
}

static b8 se_ui_textbox_has_selection(const se_ui_widget* widget) {
	if (!widget) {
		return false;
	}
	return widget->selection_end > widget->selection_start;
}

static void se_ui_textbox_clear_selection(se_ui_widget* widget) {
	if (!widget) {
		return;
	}
	widget->selection_start = widget->caret;
	widget->selection_end = widget->caret;
}

static void se_ui_textbox_move_caret(se_ui_widget* widget, const u32 new_caret, const b8 extend_selection) {
	if (!widget) {
		return;
	}
	const u32 len = se_ui_chars_length(&widget->text);
	const u32 clamped = (u32)s_min(new_caret, len);
	if (extend_selection) {
		if (!se_ui_textbox_has_selection(widget)) {
			widget->selection_start = widget->caret;
		}
		widget->selection_end = clamped;
	} else {
		widget->selection_start = clamped;
		widget->selection_end = clamped;
	}
	widget->caret = clamped;
}

static u32 se_ui_textbox_caret_from_pointer(const se_ui_widget* widget, const s_vec2* pointer_ndc) {
	if (!widget || !pointer_ndc) {
		return 0;
	}
	const u32 text_len = se_ui_chars_length(&widget->text);
	const f32 char_w = widget->font_size * SE_UI_TEXT_CHAR_WIDTH_FACTOR;
	if (char_w <= 0.00001f) {
		return 0;
	}
	const f32 base_x = se_ui_widget_text_start_x(widget, text_len);
	const f32 relative_x = (pointer_ndc->x - base_x) / char_w;
	i32 caret = (i32)floorf(relative_x + 0.5f);
	if (caret < 0) {
		caret = 0;
	}
	if ((u32)caret > text_len) {
		caret = (i32)text_len;
	}
	return (u32)caret;
}

static b8 se_ui_handle_text_input_for_widget(se_ui_root* root, const se_ui_widget_handle widget_handle, const c8* utf8_text, const b8 fire_callback) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || widget_handle == S_HANDLE_NULL || !utf8_text) {
		return false;
	}
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, widget_handle);
	if (!widget || widget->type != SE_UI_WIDGET_TEXTBOX || !widget->desc.enabled) {
		return false;
	}
	const u32 sel_a = (u32)s_min(widget->selection_start, widget->selection_end);
	const u32 sel_b = (u32)s_max(widget->selection_start, widget->selection_end);
	u32 new_caret = widget->caret;
	if (!se_ui_chars_replace_range(&widget->text, sel_a, sel_b, utf8_text, widget->max_length, &new_caret)) {
		return false;
	}
	widget->caret = new_caret;
	se_ui_textbox_clear_selection(widget);
	root->caret_visible = true;
	root->caret_blink_accumulator = 0.0;
	se_ui_mark_text_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	if (fire_callback) {
		se_ui_fire_change(widget, widget_handle);
	}
	return true;
}

static b8 se_ui_textbox_key_edit(se_ui_root* root, const se_ui_widget_handle textbox) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || textbox == S_HANDLE_NULL) {
		return false;
	}
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, textbox);
	if (!widget || widget->type != SE_UI_WIDGET_TEXTBOX || !widget->desc.enabled) {
		return false;
	}
	b8 changed = false;
	const b8 shift = se_window_is_key_down(root->window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(root->window, SE_KEY_RIGHT_SHIFT);

	if (se_window_is_key_pressed(root->window, SE_KEY_LEFT)) {
		if (widget->caret > 0) {
			se_ui_textbox_move_caret(widget, widget->caret - 1, shift);
			changed = true;
		}
	}
	if (se_window_is_key_pressed(root->window, SE_KEY_RIGHT)) {
		const u32 len = se_ui_chars_length(&widget->text);
		if (widget->caret < len) {
			se_ui_textbox_move_caret(widget, widget->caret + 1, shift);
			changed = true;
		}
	}
	if (se_window_is_key_pressed(root->window, SE_KEY_HOME)) {
		se_ui_textbox_move_caret(widget, 0, shift);
		changed = true;
	}
	if (se_window_is_key_pressed(root->window, SE_KEY_END)) {
		se_ui_textbox_move_caret(widget, se_ui_chars_length(&widget->text), shift);
		changed = true;
	}

	if (se_window_is_key_pressed(root->window, SE_KEY_BACKSPACE)) {
		const u32 len = se_ui_chars_length(&widget->text);
		const u32 sel_a = (u32)s_min(widget->selection_start, widget->selection_end);
		const u32 sel_b = (u32)s_max(widget->selection_start, widget->selection_end);
		u32 new_caret = widget->caret;
		if (sel_b > sel_a) {
			if (se_ui_chars_replace_range(&widget->text, sel_a, sel_b, "", widget->max_length, &new_caret)) {
				changed = true;
			}
		} else if (widget->caret > 0 && len > 0) {
			if (se_ui_chars_replace_range(&widget->text, widget->caret - 1, widget->caret, "", widget->max_length, &new_caret)) {
				changed = true;
			}
		}
		widget->caret = new_caret;
		se_ui_textbox_clear_selection(widget);
	}

	if (se_window_is_key_pressed(root->window, SE_KEY_DELETE)) {
		const u32 len = se_ui_chars_length(&widget->text);
		const u32 sel_a = (u32)s_min(widget->selection_start, widget->selection_end);
		const u32 sel_b = (u32)s_max(widget->selection_start, widget->selection_end);
		u32 new_caret = widget->caret;
		if (sel_b > sel_a) {
			if (se_ui_chars_replace_range(&widget->text, sel_a, sel_b, "", widget->max_length, &new_caret)) {
				changed = true;
			}
		} else if (widget->caret < len) {
			if (se_ui_chars_replace_range(&widget->text, widget->caret, widget->caret + 1, "", widget->max_length, &new_caret)) {
				changed = true;
			}
		}
		widget->caret = new_caret;
		se_ui_textbox_clear_selection(widget);
	}

	if (se_window_is_key_pressed(root->window, SE_KEY_ENTER) || se_window_is_key_pressed(root->window, SE_KEY_KP_ENTER)) {
		if (widget->submit_on_enter) {
			se_ui_fire_submit(widget, textbox);
		}
	}

	if (changed) {
		root->caret_visible = true;
		root->caret_blink_accumulator = 0.0;
		se_ui_mark_text_dirty_internal(root);
		se_ui_mark_visual_dirty_internal(root);
		se_ui_fire_change(widget, textbox);
	}
	return changed;
}

static void se_ui_update_scroll_from_wheel(se_ui_root* root, const se_ui_widget_handle scrollbox, const f32 scroll_delta, const s_vec2* point_ndc) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || scrollbox == S_HANDLE_NULL || fabsf(scroll_delta) <= 0.0001f) {
		return;
	}
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, scrollbox);
	if (!widget || widget->type != SE_UI_WIDGET_SCROLLBOX || !widget->desc.enabled) {
		return;
	}
	if (widget->content_extent <= widget->viewport_extent + 0.0001f) {
		return;
	}
	const f32 old_value = widget->value;
	widget->value = se_ui_clamp01(widget->value - scroll_delta * s_max(widget->wheel_step, 0.001f));
	if (fabsf(old_value - widget->value) > 0.0001f) {
		se_ui_mark_layout_dirty_internal(root);
		se_ui_mark_visual_dirty_internal(root);
		se_ui_fire_scroll(widget, scrollbox, point_ndc);
		se_ui_fire_change(widget, scrollbox);
	}
}

static b8 se_ui_process_scroll_drag(se_ui_root* root, const s_vec2* pointer_ndc, const b8 mouse_down) {
	se_context* ctx = se_current_context();
	if (!ctx || !root || root->capture_widget == S_HANDLE_NULL || !pointer_ndc) {
		return false;
	}
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, root->capture_widget);
	if (!widget || widget->type != SE_UI_WIDGET_SCROLLBOX) {
		return false;
	}
	if (!mouse_down) {
		widget->thumb_dragging = false;
		root->capture_widget = S_HANDLE_NULL;
		se_ui_mark_visual_dirty_internal(root);
		return false;
	}
	const f32 track_h = se_ui_box_height(&widget->scroll_track_bounds);
	const f32 thumb_h = se_ui_box_height(&widget->scroll_thumb_bounds);
	const f32 travel = s_max(0.0001f, track_h - thumb_h);
	const f32 delta = pointer_ndc->y - widget->drag_start_pointer_y;
	const f32 old_value = widget->value;
	widget->value = se_ui_clamp01(widget->drag_start_value - delta / travel);
	if (fabsf(old_value - widget->value) > 0.0001f) {
		se_ui_mark_layout_dirty_internal(root);
		se_ui_mark_visual_dirty_internal(root);
		se_ui_fire_scroll(widget, root->capture_widget, pointer_ndc);
		se_ui_fire_change(widget, root->capture_widget);
	}
	return true;
}

static void se_ui_window_text_bridge(se_window_handle window, const c8* utf8_text, void* data) {
	(void)data;
	if (!utf8_text || utf8_text[0] == '\0') {
		return;
	}
	se_context* ctx = se_current_context();
	if (!ctx) {
		return;
	}
	b8 consumed = false;
	for (sz i = 0; i < s_array_get_size(&ctx->ui_roots); ++i) {
		const se_ui_handle ui = s_array_handle(&ctx->ui_roots, (u32)i);
		se_ui_root* root = se_ui_root_from_handle(ctx, ui);
		if (!root || root->window != window || root->focused_widget == S_HANDLE_NULL) {
			continue;
		}
		if (se_ui_handle_text_input_for_widget(root, root->focused_widget, utf8_text, true)) {
			consumed = true;
		}
	}
	if (consumed) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&ctx->ui_roots); ++i) {
		se_ui_root* root = s_array_get(&ctx->ui_roots, s_array_handle(&ctx->ui_roots, (u32)i));
		if (!root || root->window != window || !root->text_bridge_owner || !root->previous_text_callback) {
			continue;
		}
		root->previous_text_callback(window, utf8_text, root->previous_text_callback_data);
		break;
	}
}

se_ui_handle se_ui_create(const se_window_handle window, const u16 widget_capacity) {
	se_context* ctx = se_current_context();
	if (!ctx || window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->ui_roots) == 0) {
		s_array_init(&ctx->ui_roots);
	}
	if (s_array_get_capacity(&ctx->ui_elements) == 0) {
		s_array_init(&ctx->ui_elements);
	}
	if (s_array_get_capacity(&ctx->ui_texts) == 0) {
		s_array_init(&ctx->ui_texts);
	}

	const se_ui_handle ui = s_array_increment(&ctx->ui_roots);
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!root) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	memset(root, 0, sizeof(*root));
	root->window = window;
	root->caret_visible = true;
	root->next_creation_index = 1;
	s_array_init(&root->draw_order);
	s_array_init(&root->deferred_ops);

	u32 window_w = 0;
	u32 window_h = 0;
	se_window_get_size(window, &window_w, &window_h);
	if (window_w == 0) window_w = 1280;
	if (window_h == 0) window_h = 720;
	const u16 object_capacity = (u16)s_max(64, (i32)(widget_capacity > 0 ? widget_capacity * 8 : 512));
	root->scene = se_scene_2d_create(&s_vec2((f32)window_w, (f32)window_h), object_capacity);
	if (root->scene == S_HANDLE_NULL) {
		s_array_clear(&root->draw_order);
		s_array_clear(&root->deferred_ops);
		s_array_remove(&ctx->ui_roots, ui);
		return S_HANDLE_NULL;
	}
	se_scene_2d_set_auto_resize(root->scene, window, &s_vec2(1.0f, 1.0f));
	root->dirty_layout = true;
	root->dirty_visual = true;
	root->dirty_text = true;
	root->dirty_structure = true;
	se_ui_text_bridge_install(ctx, ui);
	se_set_last_error(SE_RESULT_OK);
	return ui;
}

void se_ui_destroy(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!ctx || !root) {
		return;
	}
	root->destroying = true;
	se_ui_text_bridge_uninstall(ctx, ui);
	for (;;) {
		se_ui_widget_handle top = S_HANDLE_NULL;
		for (sz i = 0; i < s_array_get_size(&ctx->ui_elements); ++i) {
			const se_ui_widget_handle handle = s_array_handle(&ctx->ui_elements, (u32)i);
			se_ui_widget* widget = se_ui_widget_from_handle(ctx, handle);
			if (!widget || widget->ui != ui || widget->parent != S_HANDLE_NULL) {
				continue;
			}
			top = handle;
			break;
		}
		if (top == S_HANDLE_NULL) {
			break;
		}
		se_ui_widget_destroy_recursive(root, top);
	}
	if (root->scene != S_HANDLE_NULL) {
		se_scene_2d_destroy_full(root->scene, false);
		root->scene = S_HANDLE_NULL;
	}
	s_array_clear(&root->draw_order);
	s_array_clear(&root->deferred_ops);
	s_array_remove(&ctx->ui_roots, ui);
}

void se_ui_destroy_all(void) {
	se_context* ctx = se_current_context();
	if (!ctx) {
		return;
	}
	while (s_array_get_size(&ctx->ui_roots) > 0) {
		const se_ui_handle ui = s_array_handle(&ctx->ui_roots, (u32)0);
		se_ui_destroy(ui);
	}
}

void se_ui_tick(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!ctx || !root) {
		return;
	}
	if (root->dirty_layout || root->dirty_structure) {
		se_ui_rebuild(root);
	}

	s_vec2 pointer_ndc = {0};
	const f32 mouse_x = se_window_get_mouse_position_x(root->window);
	const f32 mouse_y = se_window_get_mouse_position_y(root->window);
	if (!se_window_pixel_to_normalized(root->window, mouse_x, mouse_y, &pointer_ndc)) {
		se_window_get_mouse_position_normalized(root->window, &pointer_ndc);
	}

	root->dispatching = true;
	const se_ui_widget_handle hit = se_ui_hit_test(root, &pointer_ndc);
	if (hit != root->hovered_widget) {
		if (root->hovered_widget != S_HANDLE_NULL) {
			se_ui_fire_hover_end(root, root->hovered_widget, &pointer_ndc);
		}
		root->hovered_widget = hit;
		if (hit != S_HANDLE_NULL) {
			se_ui_fire_hover_start(root, hit, &pointer_ndc);
		}
	}

	const b8 mouse_pressed = se_window_is_mouse_pressed(root->window, SE_MOUSE_LEFT);
	const b8 mouse_released = se_window_is_mouse_released(root->window, SE_MOUSE_LEFT);
	const b8 mouse_down = se_window_is_mouse_down(root->window, SE_MOUSE_LEFT);

	if (mouse_pressed) {
		root->pressed_widget = hit;
		if (hit != S_HANDLE_NULL) {
			se_ui_widget* hit_widget = se_ui_widget_from_handle(ctx, hit);
			if (hit_widget) {
				if (hit_widget->type != SE_UI_WIDGET_SCROLLBOX) {
					const se_ui_widget_handle scrollbox = se_ui_widget_find_scrollbox_ancestor(ctx, hit);
					if (scrollbox != S_HANDLE_NULL) {
						(void)se_ui_scrollbox_set_selected_internal(root, scrollbox, hit, true);
					}
				}
				se_ui_fire_press(root, hit, &pointer_ndc, true, SE_MOUSE_LEFT);
				if (hit_widget->type == SE_UI_WIDGET_TEXTBOX) {
					se_ui_set_focus_internal(root, hit);
					se_ui_textbox_move_caret(hit_widget, se_ui_textbox_caret_from_pointer(hit_widget, &pointer_ndc), false);
					root->caret_visible = true;
					root->caret_blink_accumulator = 0.0;
					se_ui_mark_visual_dirty_internal(root);
				} else if (root->focused_widget != S_HANDLE_NULL) {
					se_ui_set_focus_internal(root, S_HANDLE_NULL);
				}
				if (hit_widget->type == SE_UI_WIDGET_SCROLLBOX && se_ui_box_contains(&hit_widget->scroll_thumb_bounds, &pointer_ndc)) {
					hit_widget->thumb_dragging = true;
					hit_widget->drag_start_pointer_y = pointer_ndc.y;
					hit_widget->drag_start_value = hit_widget->value;
					root->capture_widget = hit;
					se_ui_mark_visual_dirty_internal(root);
				}
			}
		} else {
			se_ui_set_focus_internal(root, S_HANDLE_NULL);
		}
	}

	if (root->capture_widget != S_HANDLE_NULL) {
		(void)se_ui_process_scroll_drag(root, &pointer_ndc, mouse_down);
	}

	if (mouse_released) {
		if (root->pressed_widget != S_HANDLE_NULL) {
			se_ui_widget* pressed_widget = se_ui_widget_from_handle(ctx, root->pressed_widget);
			if (pressed_widget) {
				se_ui_fire_press(root, root->pressed_widget, &pointer_ndc, false, SE_MOUSE_LEFT);
				if (root->pressed_widget == hit) {
					se_ui_fire_click(pressed_widget, root->pressed_widget, &pointer_ndc, SE_MOUSE_LEFT);
					if (pressed_widget->type == SE_UI_WIDGET_SCROLLBOX) {
						se_ui_set_focus_internal(root, root->pressed_widget);
					} else {
						const se_ui_widget_handle scrollbox = se_ui_widget_find_scrollbox_ancestor(ctx, root->pressed_widget);
						if (scrollbox != S_HANDLE_NULL) {
							(void)se_ui_scrollbox_set_selected_internal(root, scrollbox, root->pressed_widget, true);
						}
					}
				}
			}
		}
		root->pressed_widget = S_HANDLE_NULL;
		if (root->capture_widget != S_HANDLE_NULL) {
			se_ui_widget* capture_widget = se_ui_widget_from_handle(ctx, root->capture_widget);
			if (capture_widget) {
				capture_widget->thumb_dragging = false;
			}
			root->capture_widget = S_HANDLE_NULL;
			se_ui_mark_visual_dirty_internal(root);
		}
	}

	s_vec2 scroll_delta = {0};
	se_window_get_scroll_delta(root->window, &scroll_delta);
	if (fabsf(scroll_delta.y) > 0.0001f) {
		se_ui_widget_handle scroll_target = root->capture_widget;
		if (scroll_target == S_HANDLE_NULL && hit != S_HANDLE_NULL) {
			se_ui_widget* hit_widget = se_ui_widget_from_handle(ctx, hit);
			if (hit_widget && hit_widget->type == SE_UI_WIDGET_SCROLLBOX) {
				scroll_target = hit;
			}
		}
		if (scroll_target != S_HANDLE_NULL) {
			se_ui_update_scroll_from_wheel(root, scroll_target, scroll_delta.y, &pointer_ndc);
		}
	}

	if (root->focused_widget != S_HANDLE_NULL) {
		(void)se_ui_textbox_key_edit(root, root->focused_widget);
	}

	if (root->focused_widget != S_HANDLE_NULL) {
		se_ui_widget* focused_widget = se_ui_widget_from_handle(ctx, root->focused_widget);
		if (focused_widget && focused_widget->type == SE_UI_WIDGET_TEXTBOX && focused_widget->desc.enabled) {
			root->caret_visible = true;
			se_ui_mark_visual_dirty_internal(root);
		} else if (!root->caret_visible) {
			root->caret_visible = true;
		}
		root->caret_blink_accumulator = 0.0;
	} else {
		root->caret_visible = false;
		root->caret_blink_accumulator = 0.0;
	}

	root->dispatching = false;
	se_ui_apply_deferred_ops(root);
}

void se_ui_draw(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!ctx || !root) {
		return;
	}
	se_debug_trace_begin("ui_render");
	if (root->dirty_layout || root->dirty_visual || root->dirty_text || root->dirty_structure) {
		se_ui_rebuild(root);
		se_scene_2d_render_to_buffer(root->scene);
		root->dirty_layout = false;
		root->dirty_visual = false;
		root->dirty_text = false;
		root->dirty_structure = false;
	}
	se_scene_2d_render_to_screen(root->scene, root->window);

	if (g_ui_debug_overlay_enabled) {
		se_text_handle* text_handle = se_ui_text_handle_get(ctx);
		const se_font_handle font = se_ui_font_cache_get(ctx, SE_UI_DEFAULT_FONT_PATH, 16.0f);
		if (text_handle && font != S_HANDLE_NULL) {
			c8 debug_line[256] = {0};
			snprintf(
				debug_line,
				sizeof(debug_line),
				"UI roots=%zu widgets=%zu focused=%llu",
				s_array_get_size(&ctx->ui_roots),
				s_array_get_size(&ctx->ui_elements),
				(unsigned long long)root->focused_widget);
			se_text_render(text_handle, font, debug_line, &s_vec2(-0.98f, 0.94f), &s_vec2(1.0f, 1.0f), 0.02f);
		}
	}
	se_debug_trace_end("ui_render");
}

void se_ui_mark_layout_dirty(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_mark_layout_dirty_internal(root);
}

void se_ui_mark_visual_dirty(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_mark_visual_dirty_internal(root);
}

void se_ui_mark_text_dirty(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_mark_text_dirty_internal(root);
}

void se_ui_mark_structure_dirty(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_mark_structure_dirty_internal(root);
}

b8 se_ui_is_dirty(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!root) {
		return false;
	}
	return root->dirty_layout || root->dirty_visual || root->dirty_text || root->dirty_structure;
}

se_scene_2d_handle se_ui_get_scene(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	return root ? root->scene : S_HANDLE_NULL;
}

se_window_handle se_ui_get_window(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	return root ? root->window : S_HANDLE_NULL;
}

se_ui_widget_handle se_ui_get_hovered_widget(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	return root ? root->hovered_widget : S_HANDLE_NULL;
}

se_ui_widget_handle se_ui_get_focused_widget(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	return root ? root->focused_widget : S_HANDLE_NULL;
}

b8 se_ui_focus_widget(const se_ui_handle ui, const se_ui_widget_handle widget) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui || widget_ptr->type != SE_UI_WIDGET_TEXTBOX) {
		return false;
	}
	se_ui_set_focus_internal(root, widget);
	return true;
}

void se_ui_clear_focus(const se_ui_handle ui) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!root) {
		return;
	}
	se_ui_set_focus_internal(root, S_HANDLE_NULL);
}

static se_ui_widget_handle se_ui_add_widget_common(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_widget_type type, const se_ui_widget_desc* desc) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!ctx || !root) {
		return S_HANDLE_NULL;
	}
	se_ui_widget_handle widget = se_ui_widget_create_internal(root, type, desc);
	if (widget == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	if (parent != S_HANDLE_NULL) {
		if (!se_ui_widget_attach_immediate(root, parent, widget)) {
			se_ui_widget_destroy_recursive(root, widget);
			return S_HANDLE_NULL;
		}
	}
	return widget;
}

static void se_ui_apply_widget_basics(se_ui_widget_desc* widget_desc, const c8* id, const s_vec2 position, const s_vec2 size) {
	if (!widget_desc) {
		return;
	}
	if (id && id[0] != '\0') {
		widget_desc->id = id;
	}
	if (fabsf(position.x) > 0.00001f || fabsf(position.y) > 0.00001f) {
		widget_desc->position = position;
	}
	if (size.x > 0.0f || size.y > 0.0f) {
		widget_desc->size = s_vec2(s_max(0.0f, size.x), s_max(0.0f, size.y));
	}
}

static b8 se_ui_callbacks_has_any_handler(const se_ui_callbacks* callbacks) {
	if (!callbacks) {
		return false;
	}
	return callbacks->on_hover_start != NULL ||
		callbacks->on_hover_end != NULL ||
		callbacks->on_focus != NULL ||
		callbacks->on_blur != NULL ||
		callbacks->on_press != NULL ||
		callbacks->on_release != NULL ||
		callbacks->on_click != NULL ||
		callbacks->on_change != NULL ||
		callbacks->on_submit != NULL ||
		callbacks->on_scroll != NULL;
}

static b8 se_ui_callbacks_has_pointer_handlers(const se_ui_callbacks* callbacks) {
	if (!callbacks) {
		return false;
	}
	return callbacks->on_hover_start != NULL ||
		callbacks->on_hover_end != NULL ||
		callbacks->on_press != NULL ||
		callbacks->on_release != NULL ||
		callbacks->on_click != NULL ||
		callbacks->on_scroll != NULL;
}

#define SE_UI_ARGS_TO_CALLBACKS(dst, args) \
	do { \
		(dst).on_hover_start = (args).on_hover_start_fn; \
		(dst).on_hover_start_data = (args).on_hover_start_data; \
		(dst).on_hover_end = (args).on_hover_end_fn; \
		(dst).on_hover_end_data = (args).on_hover_end_data; \
		(dst).on_focus = (args).on_focus_fn; \
		(dst).on_focus_data = (args).on_focus_data; \
		(dst).on_blur = (args).on_blur_fn; \
		(dst).on_blur_data = (args).on_blur_data; \
		(dst).on_press = (args).on_press_fn; \
		(dst).on_press_data = (args).on_press_data; \
		(dst).on_release = (args).on_release_fn; \
		(dst).on_release_data = (args).on_release_data; \
		(dst).on_click = (args).on_click_fn; \
		(dst).on_click_data = (args).on_click_data; \
		(dst).on_change = (args).on_change_fn; \
		(dst).on_change_data = (args).on_change_data; \
		(dst).on_submit = (args).on_submit_fn; \
		(dst).on_submit_data = (args).on_submit_data; \
		(dst).on_scroll = (args).on_scroll_fn; \
		(dst).on_scroll_data = (args).on_scroll_data; \
		(dst).user_data = (args).user_data; \
	} while (0)

se_ui_widget_handle se_ui_panel_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_panel_desc* desc) {
	se_ui_panel_desc defaults = SE_UI_PANEL_DESC_DEFAULTS;
	se_ui_widget_desc base = desc ? desc->widget : defaults.widget;
	base.interactable = false;
	return se_ui_add_widget_common(ui, parent, SE_UI_WIDGET_PANEL, &base);
}

se_ui_widget_handle se_ui_button_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_button_desc* desc) {
	se_ui_button_desc defaults = SE_UI_BUTTON_DESC_DEFAULTS;
	const se_ui_button_desc* cfg = desc ? desc : &defaults;
	se_ui_widget_desc base = cfg->widget;
	base.interactable = true;
	se_ui_widget_handle widget = se_ui_add_widget_common(ui, parent, SE_UI_WIDGET_BUTTON, &base);
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr) {
		return S_HANDLE_NULL;
	}
	widget_ptr->callbacks = cfg->callbacks;
	widget_ptr->font_size = cfg->font_size > 0.0f ? cfg->font_size : 24.0f;
	se_ui_chars_set(&widget_ptr->font_path, cfg->font_path ? cfg->font_path : SE_UI_DEFAULT_FONT_PATH);
	se_ui_chars_set(&widget_ptr->text, cfg->text ? cfg->text : "Button");
	se_ui_mark_text_dirty_internal(se_ui_root_from_handle(ctx, ui));
	return widget;
}

se_ui_widget_handle se_ui_text_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_text_desc* desc) {
	se_ui_text_desc defaults = SE_UI_TEXT_DESC_DEFAULTS;
	const se_ui_text_desc* cfg = desc ? desc : &defaults;
	se_ui_widget_desc base = cfg->widget;
	base.interactable = false;
	const se_ui_style default_style = SE_UI_STYLE_DEFAULT;
	if (se_ui_style_equal(&base.style, &default_style)) {
		base.style.normal.background_color.w = 0.0f;
		base.style.hovered.background_color.w = 0.0f;
		base.style.pressed.background_color.w = 0.0f;
		base.style.disabled.background_color.w = 0.0f;
		base.style.focused.background_color.w = 0.0f;
		base.style.normal.border_width = 0.0f;
		base.style.hovered.border_width = 0.0f;
		base.style.pressed.border_width = 0.0f;
		base.style.disabled.border_width = 0.0f;
		base.style.focused.border_width = 0.0f;
	}
	se_ui_widget_handle widget = se_ui_add_widget_common(ui, parent, SE_UI_WIDGET_TEXT, &base);
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr) {
		return S_HANDLE_NULL;
	}
	widget_ptr->font_size = cfg->font_size > 0.0f ? cfg->font_size : 24.0f;
	se_ui_chars_set(&widget_ptr->font_path, cfg->font_path ? cfg->font_path : SE_UI_DEFAULT_FONT_PATH);
	se_ui_chars_set(&widget_ptr->text, cfg->text ? cfg->text : "");
	se_ui_mark_text_dirty_internal(se_ui_root_from_handle(ctx, ui));
	return widget;
}

se_ui_widget_handle se_ui_textbox_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_textbox_desc* desc) {
	se_ui_textbox_desc defaults = SE_UI_TEXTBOX_DESC_DEFAULTS;
	const se_ui_textbox_desc* cfg = desc ? desc : &defaults;
	se_ui_widget_desc base = cfg->widget;
	base.interactable = true;
	se_ui_widget_handle widget = se_ui_add_widget_common(ui, parent, SE_UI_WIDGET_TEXTBOX, &base);
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr) {
		return S_HANDLE_NULL;
	}
	widget_ptr->callbacks = cfg->callbacks;
	widget_ptr->font_size = cfg->font_size > 0.0f ? cfg->font_size : 24.0f;
	widget_ptr->max_length = cfg->max_length > 0 ? cfg->max_length : 256;
	widget_ptr->submit_on_enter = cfg->submit_on_enter;
	se_ui_chars_set(&widget_ptr->font_path, cfg->font_path ? cfg->font_path : SE_UI_DEFAULT_FONT_PATH);
	se_ui_chars_set(&widget_ptr->text, cfg->text ? cfg->text : "");
	se_ui_chars_set(&widget_ptr->placeholder, cfg->placeholder ? cfg->placeholder : "");
	widget_ptr->caret = se_ui_chars_length(&widget_ptr->text);
	widget_ptr->selection_start = widget_ptr->caret;
	widget_ptr->selection_end = widget_ptr->caret;
	se_ui_mark_text_dirty_internal(se_ui_root_from_handle(ctx, ui));
	return widget;
}

se_ui_widget_handle se_ui_scrollbox_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_scrollbox_desc* desc) {
	se_ui_scrollbox_desc defaults = SE_UI_SCROLLBOX_DESC_DEFAULTS;
	const se_ui_scrollbox_desc* cfg = desc ? desc : &defaults;
	se_ui_widget_desc base = cfg->widget;
	base.interactable = true;
	se_ui_widget_handle widget = se_ui_add_widget_common(ui, parent, SE_UI_WIDGET_SCROLLBOX, &base);
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr) {
		return S_HANDLE_NULL;
	}
	widget_ptr->callbacks = cfg->callbacks;
	widget_ptr->wheel_step = cfg->wheel_step > 0.0f ? cfg->wheel_step : 0.08f;
	widget_ptr->value = se_ui_clamp01(cfg->value);
	se_ui_mark_layout_dirty_internal(se_ui_root_from_handle(ctx, ui));
	return widget;
}

se_ui_widget_handle se_ui_add_root_impl(se_ui_handle ui, se_ui_panel_args args) {
	se_ui_panel_desc desc = SE_UI_PANEL_DESC_DEFAULTS;
	desc.widget.size = s_vec2(1.0f, 1.0f);
	se_ui_apply_widget_basics(&desc.widget, args.id, args.position, args.size);

	se_ui_callbacks callbacks = {0};
	SE_UI_ARGS_TO_CALLBACKS(callbacks, args);
	se_ui_callbacks_filter_for_type(SE_UI_WIDGET_PANEL, &callbacks);
	if (se_ui_callbacks_has_pointer_handlers(&callbacks)) {
		desc.widget.interactable = true;
	}

	se_ui_widget_handle widget = se_ui_panel_add(ui, S_HANDLE_NULL, &desc);
	if (widget == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	if (se_ui_callbacks_has_pointer_handlers(&callbacks)) {
		(void)se_ui_widget_set_interactable(ui, widget, true);
	}
	if (se_ui_callbacks_has_any_handler(&callbacks)) {
		(void)se_ui_widget_set_callbacks(ui, widget, &callbacks);
	}
	return widget;
}

se_ui_widget_handle se_ui_add_panel_impl(se_ui_widget_handle parent, se_ui_panel_args args) {
	se_ui_handle ui = S_HANDLE_NULL;
	se_ui_root* root = NULL;
	if (!se_ui_parent_context(parent, &ui, &root) || !root) {
		return S_HANDLE_NULL;
	}

	se_ui_panel_desc desc = SE_UI_PANEL_DESC_DEFAULTS;
	se_ui_apply_widget_basics(&desc.widget, args.id, args.position, args.size);

	se_ui_callbacks callbacks = {0};
	SE_UI_ARGS_TO_CALLBACKS(callbacks, args);
	se_ui_callbacks_filter_for_type(SE_UI_WIDGET_PANEL, &callbacks);
	if (se_ui_callbacks_has_pointer_handlers(&callbacks)) {
		desc.widget.interactable = true;
	}

	se_ui_widget_handle widget = se_ui_panel_add(ui, parent, &desc);
	if (widget == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	if (se_ui_callbacks_has_pointer_handlers(&callbacks)) {
		(void)se_ui_widget_set_interactable(ui, widget, true);
	}
	if (se_ui_callbacks_has_any_handler(&callbacks)) {
		(void)se_ui_widget_set_callbacks(ui, widget, &callbacks);
	}
	return widget;
}

se_ui_widget_handle se_ui_add_button_impl(se_ui_widget_handle parent, se_ui_button_args args) {
	se_ui_handle ui = S_HANDLE_NULL;
	se_ui_root* root = NULL;
	if (!se_ui_parent_context(parent, &ui, &root) || !root) {
		return S_HANDLE_NULL;
	}

	se_ui_button_desc desc = SE_UI_BUTTON_DESC_DEFAULTS;
	se_ui_apply_widget_basics(&desc.widget, args.id, args.position, args.size);
	desc.text = args.text ? args.text : desc.text;
	if (args.font_path && args.font_path[0] != '\0') {
		desc.font_path = args.font_path;
	}
	if (args.font_size > 0.0f) {
		desc.font_size = args.font_size;
	}
	SE_UI_ARGS_TO_CALLBACKS(desc.callbacks, args);
	se_ui_callbacks_filter_for_type(SE_UI_WIDGET_BUTTON, &desc.callbacks);
	return se_ui_button_add(ui, parent, &desc);
}

se_ui_widget_handle se_ui_add_text_impl(se_ui_widget_handle parent, se_ui_text_args args) {
	se_ui_handle ui = S_HANDLE_NULL;
	se_ui_root* root = NULL;
	if (!se_ui_parent_context(parent, &ui, &root) || !root) {
		return S_HANDLE_NULL;
	}

	se_ui_text_desc desc = SE_UI_TEXT_DESC_DEFAULTS;
	se_ui_apply_widget_basics(&desc.widget, args.id, args.position, args.size);
	desc.text = args.text ? args.text : desc.text;
	if (args.font_path && args.font_path[0] != '\0') {
		desc.font_path = args.font_path;
	}
	if (args.font_size > 0.0f) {
		desc.font_size = args.font_size;
	}

	se_ui_callbacks callbacks = {0};
	SE_UI_ARGS_TO_CALLBACKS(callbacks, args);
	se_ui_callbacks_filter_for_type(SE_UI_WIDGET_TEXT, &callbacks);
	if (se_ui_callbacks_has_pointer_handlers(&callbacks)) {
		desc.widget.interactable = true;
	}

	se_ui_widget_handle widget = se_ui_text_add(ui, parent, &desc);
	if (widget == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	if (se_ui_callbacks_has_pointer_handlers(&callbacks)) {
		(void)se_ui_widget_set_interactable(ui, widget, true);
	}
	if (se_ui_callbacks_has_any_handler(&callbacks)) {
		(void)se_ui_widget_set_callbacks(ui, widget, &callbacks);
	}
	return widget;
}

se_ui_widget_handle se_ui_add_textbox_impl(se_ui_widget_handle parent, se_ui_textbox_args args) {
	se_ui_handle ui = S_HANDLE_NULL;
	se_ui_root* root = NULL;
	if (!se_ui_parent_context(parent, &ui, &root) || !root) {
		return S_HANDLE_NULL;
	}

	se_ui_textbox_desc desc = SE_UI_TEXTBOX_DESC_DEFAULTS;
	se_ui_apply_widget_basics(&desc.widget, args.id, args.position, args.size);
	desc.text = args.text ? args.text : desc.text;
	desc.placeholder = args.placeholder ? args.placeholder : desc.placeholder;
	if (args.font_path && args.font_path[0] != '\0') {
		desc.font_path = args.font_path;
	}
	if (args.font_size > 0.0f) {
		desc.font_size = args.font_size;
	}
	if (args.max_length > 0) {
		desc.max_length = args.max_length;
	}
	if (args.set_submit_on_enter) {
		desc.submit_on_enter = args.submit_on_enter;
	}
	SE_UI_ARGS_TO_CALLBACKS(desc.callbacks, args);
	se_ui_callbacks_filter_for_type(SE_UI_WIDGET_TEXTBOX, &desc.callbacks);
	return se_ui_textbox_add(ui, parent, &desc);
}

se_ui_widget_handle se_ui_add_scrollbox_impl(se_ui_widget_handle parent, se_ui_scrollbox_args args) {
	se_ui_handle ui = S_HANDLE_NULL;
	se_ui_root* root = NULL;
	if (!se_ui_parent_context(parent, &ui, &root) || !root) {
		return S_HANDLE_NULL;
	}

	se_ui_scrollbox_desc desc = SE_UI_SCROLLBOX_DESC_DEFAULTS;
	se_ui_apply_widget_basics(&desc.widget, args.id, args.position, args.size);
	desc.widget.layout.clip_children = true;
	desc.value = se_ui_clamp01(args.value);
	if (args.wheel_step > 0.0f) {
		desc.wheel_step = args.wheel_step;
	}
	SE_UI_ARGS_TO_CALLBACKS(desc.callbacks, args);
	se_ui_callbacks_filter_for_type(SE_UI_WIDGET_SCROLLBOX, &desc.callbacks);
	return se_ui_scrollbox_add(ui, parent, &desc);
}

se_ui_widget_handle se_ui_add_spacer_impl(se_ui_widget_handle parent, se_ui_panel_args args) {
	se_ui_handle ui = S_HANDLE_NULL;
	se_ui_root* root = NULL;
	if (!se_ui_parent_context(parent, &ui, &root) || !root) {
		return S_HANDLE_NULL;
	}

	se_ui_panel_desc desc = SE_UI_PANEL_DESC_DEFAULTS;
	se_ui_apply_widget_basics(&desc.widget, args.id, args.position, args.size);
	desc.widget.enabled = false;
	desc.widget.interactable = false;
	desc.widget.visible = false;
	desc.widget.style = SE_UI_STYLE_DEFAULT;
	desc.widget.style.normal.background_color.w = 0.0f;
	desc.widget.style.normal.border_width = 0.0f;
	desc.widget.style.hovered = desc.widget.style.normal;
	desc.widget.style.pressed = desc.widget.style.normal;
	desc.widget.style.disabled = desc.widget.style.normal;
	desc.widget.style.focused = desc.widget.style.normal;
	return se_ui_panel_add(ui, parent, &desc);
}

#undef SE_UI_ARGS_TO_CALLBACKS

se_ui_widget_handle se_ui_panel_create(se_ui_handle ui, se_ui_widget_handle parent, s_vec2 size) {
	se_ui_panel_desc desc = SE_UI_PANEL_DESC_DEFAULTS;
	desc.widget.size = s_vec2(s_max(0.0f, size.x), s_max(0.0f, size.y));
	return se_ui_panel_add(ui, parent, &desc);
}

se_ui_widget_handle se_ui_text_create(se_ui_handle ui, se_ui_widget_handle parent, const c8* text) {
	se_ui_text_desc desc = SE_UI_TEXT_DESC_DEFAULTS;
	desc.text = text ? text : "";
	return se_ui_text_add(ui, parent, &desc);
}

se_ui_widget_handle se_ui_button_create(se_ui_handle ui, se_ui_widget_handle parent, const c8* text) {
	se_ui_button_desc desc = SE_UI_BUTTON_DESC_DEFAULTS;
	desc.text = text ? text : "Button";
	return se_ui_button_add(ui, parent, &desc);
}

se_ui_widget_handle se_ui_textbox_create(se_ui_handle ui, se_ui_widget_handle parent, const c8* placeholder) {
	se_ui_textbox_desc desc = SE_UI_TEXTBOX_DESC_DEFAULTS;
	desc.placeholder = placeholder ? placeholder : "";
	return se_ui_textbox_add(ui, parent, &desc);
}

se_ui_widget_handle se_ui_scrollbox_create(se_ui_handle ui, se_ui_widget_handle parent, s_vec2 size) {
	se_ui_scrollbox_desc desc = SE_UI_SCROLLBOX_DESC_DEFAULTS;
	desc.widget.size = s_vec2(s_max(0.0f, size.x), s_max(0.0f, size.y));
	desc.widget.layout.clip_children = true;
	return se_ui_scrollbox_add(ui, parent, &desc);
}

se_ui_widget_handle se_ui_spacer_create(se_ui_handle ui, se_ui_widget_handle parent, s_vec2 size) {
	se_ui_panel_desc desc = SE_UI_PANEL_DESC_DEFAULTS;
	desc.widget.size = s_vec2(s_max(0.0f, size.x), s_max(0.0f, size.y));
	desc.widget.enabled = false;
	desc.widget.interactable = false;
	desc.widget.visible = false;
	desc.widget.style = SE_UI_STYLE_DEFAULT;
	desc.widget.style.normal.background_color.w = 0.0f;
	desc.widget.style.normal.border_width = 0.0f;
	desc.widget.style.hovered = desc.widget.style.normal;
	desc.widget.style.pressed = desc.widget.style.normal;
	desc.widget.style.disabled = desc.widget.style.normal;
	desc.widget.style.focused = desc.widget.style.normal;
	return se_ui_panel_add(ui, parent, &desc);
}

b8 se_ui_widget_set_callbacks(se_ui_handle ui, se_ui_widget_handle widget, const se_ui_callbacks* callbacks) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui || !callbacks) {
		return false;
	}
	if (!se_ui_callbacks_supported_for_type(widget_ptr->type, callbacks)) {
		return false;
	}
	if (se_ui_callbacks_equal(&widget_ptr->callbacks, callbacks)) {
		return true;
	}
	widget_ptr->callbacks = *callbacks;
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_set_user_data(se_ui_handle ui, se_ui_widget_handle widget, void* user_data) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (widget_ptr->callbacks.user_data == user_data) {
		return true;
	}
	widget_ptr->callbacks.user_data = user_data;
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_on_click(se_ui_handle ui, se_ui_widget_handle widget, se_ui_click_callback cb, void* user_data) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr || widget_ptr->ui != ui || !se_ui_widget_callback_supported(widget_ptr, SE_UI_CALLBACK_CLICK)) {
		return false;
	}
	widget_ptr->callbacks.on_click = cb;
	widget_ptr->callbacks.on_click_data = user_data;
	widget_ptr->callbacks.user_data = user_data;
	return true;
}

b8 se_ui_widget_on_hover(se_ui_handle ui, se_ui_widget_handle widget, se_ui_hover_callback on_start, se_ui_hover_callback on_end, void* user_data) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr || widget_ptr->ui != ui || !se_ui_widget_callback_supported(widget_ptr, SE_UI_CALLBACK_HOVER)) {
		return false;
	}
	widget_ptr->callbacks.on_hover_start = on_start;
	widget_ptr->callbacks.on_hover_end = on_end;
	widget_ptr->callbacks.on_hover_start_data = user_data;
	widget_ptr->callbacks.on_hover_end_data = user_data;
	widget_ptr->callbacks.user_data = user_data;
	return true;
}

b8 se_ui_widget_on_press(se_ui_handle ui, se_ui_widget_handle widget, se_ui_press_callback on_press, se_ui_press_callback on_release, void* user_data) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr || widget_ptr->ui != ui || !se_ui_widget_callback_supported(widget_ptr, SE_UI_CALLBACK_PRESS)) {
		return false;
	}
	widget_ptr->callbacks.on_press = on_press;
	widget_ptr->callbacks.on_release = on_release;
	widget_ptr->callbacks.on_press_data = user_data;
	widget_ptr->callbacks.on_release_data = user_data;
	widget_ptr->callbacks.user_data = user_data;
	return true;
}

b8 se_ui_widget_on_change(se_ui_handle ui, se_ui_widget_handle widget, se_ui_change_callback cb, void* user_data) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr || widget_ptr->ui != ui || !se_ui_widget_callback_supported(widget_ptr, SE_UI_CALLBACK_CHANGE)) {
		return false;
	}
	widget_ptr->callbacks.on_change = cb;
	widget_ptr->callbacks.on_change_data = user_data;
	widget_ptr->callbacks.user_data = user_data;
	return true;
}

b8 se_ui_widget_on_submit(se_ui_handle ui, se_ui_widget_handle widget, se_ui_submit_callback cb, void* user_data) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr || widget_ptr->ui != ui || !se_ui_widget_callback_supported(widget_ptr, SE_UI_CALLBACK_SUBMIT)) {
		return false;
	}
	widget_ptr->callbacks.on_submit = cb;
	widget_ptr->callbacks.on_submit_data = user_data;
	widget_ptr->callbacks.user_data = user_data;
	return true;
}

b8 se_ui_widget_on_scroll(se_ui_handle ui, se_ui_widget_handle widget, se_ui_scroll_callback cb, void* user_data) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr || widget_ptr->ui != ui || !se_ui_widget_callback_supported(widget_ptr, SE_UI_CALLBACK_SCROLL)) {
		return false;
	}
	widget_ptr->callbacks.on_scroll = cb;
	widget_ptr->callbacks.on_scroll_data = user_data;
	widget_ptr->callbacks.user_data = user_data;
	return true;
}

b8 se_ui_widget_set_stack_vertical(se_ui_handle ui, se_ui_widget_handle widget, f32 spacing) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	const f32 clamped_spacing = s_max(0.0f, spacing);
	const b8 changed = widget_ptr->desc.layout.direction != SE_UI_LAYOUT_VERTICAL || fabsf(widget_ptr->desc.layout.spacing - clamped_spacing) > 0.00001f;
	if (!changed) {
		return true;
	}
	widget_ptr->desc.layout.direction = SE_UI_LAYOUT_VERTICAL;
	widget_ptr->desc.layout.spacing = clamped_spacing;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_set_stack_horizontal(se_ui_handle ui, se_ui_widget_handle widget, f32 spacing) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	const f32 clamped_spacing = s_max(0.0f, spacing);
	const b8 changed = widget_ptr->desc.layout.direction != SE_UI_LAYOUT_HORIZONTAL || fabsf(widget_ptr->desc.layout.spacing - clamped_spacing) > 0.00001f;
	if (!changed) {
		return true;
	}
	widget_ptr->desc.layout.direction = SE_UI_LAYOUT_HORIZONTAL;
	widget_ptr->desc.layout.spacing = clamped_spacing;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_set_alignment(se_ui_handle ui, se_ui_widget_handle widget, se_ui_alignment horizontal, se_ui_alignment vertical) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (widget_ptr->desc.layout.align_horizontal == horizontal && widget_ptr->desc.layout.align_vertical == vertical) {
		return true;
	}
	widget_ptr->desc.layout.align_horizontal = horizontal;
	widget_ptr->desc.layout.align_vertical = vertical;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_set_padding(se_ui_handle ui, se_ui_widget_handle widget, se_ui_edge padding) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (se_ui_edge_equal(&widget_ptr->desc.layout.padding, &padding)) {
		return true;
	}
	widget_ptr->desc.layout.padding = padding;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_set_margin(se_ui_handle ui, se_ui_widget_handle widget, se_ui_edge margin) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (se_ui_edge_equal(&widget_ptr->desc.layout.margin, &margin)) {
		return true;
	}
	widget_ptr->desc.layout.margin = margin;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_set_anchor_rect(se_ui_handle ui, se_ui_widget_handle widget, s_vec2 anchor_min, s_vec2 anchor_max) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	const s_vec2 min_clamped = s_vec2(se_ui_clamp01(anchor_min.x), se_ui_clamp01(anchor_min.y));
	const s_vec2 max_clamped = s_vec2(se_ui_clamp01(anchor_max.x), se_ui_clamp01(anchor_max.y));
	const b8 changed = !widget_ptr->desc.layout.use_anchors ||
		!se_ui_vec2_equal(&widget_ptr->desc.layout.anchor_min, &min_clamped) ||
		!se_ui_vec2_equal(&widget_ptr->desc.layout.anchor_max, &max_clamped);
	if (!changed) {
		return true;
	}
	widget_ptr->desc.layout.use_anchors = true;
	widget_ptr->desc.layout.anchor_min = min_clamped;
	widget_ptr->desc.layout.anchor_max = max_clamped;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_set_min_size(se_ui_handle ui, se_ui_widget_handle widget, s_vec2 min_size) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	s_vec2 clamped = s_vec2(s_max(0.0f, min_size.x), s_max(0.0f, min_size.y));
	if (se_ui_vec2_equal(&widget_ptr->desc.layout.min_size, &clamped)) {
		return true;
	}
	widget_ptr->desc.layout.min_size = clamped;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_set_max_size(se_ui_handle ui, se_ui_widget_handle widget, s_vec2 max_size) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	s_vec2 clamped = s_vec2(s_max(0.0f, max_size.x), s_max(0.0f, max_size.y));
	if (se_ui_vec2_equal(&widget_ptr->desc.layout.max_size, &clamped)) {
		return true;
	}
	widget_ptr->desc.layout.max_size = clamped;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

void se_ui_style_apply_button_primary(se_ui_style* style) {
	if (!style) {
		return;
	}
	style->normal.background_color = s_vec4(0.13f, 0.35f, 0.64f, 0.95f);
	style->normal.border_color = s_vec4(0.22f, 0.52f, 0.86f, 1.0f);
	style->hovered.background_color = s_vec4(0.16f, 0.42f, 0.72f, 0.98f);
	style->hovered.border_color = s_vec4(0.30f, 0.62f, 0.92f, 1.0f);
	style->pressed.background_color = s_vec4(0.10f, 0.28f, 0.52f, 0.98f);
	style->pressed.border_color = s_vec4(0.18f, 0.46f, 0.76f, 1.0f);
	style->focused.border_color = s_vec4(0.38f, 0.74f, 0.98f, 1.0f);
}

void se_ui_style_apply_button_danger(se_ui_style* style) {
	if (!style) {
		return;
	}
	style->normal.background_color = s_vec4(0.58f, 0.16f, 0.18f, 0.95f);
	style->normal.border_color = s_vec4(0.78f, 0.24f, 0.28f, 1.0f);
	style->hovered.background_color = s_vec4(0.70f, 0.18f, 0.21f, 0.98f);
	style->hovered.border_color = s_vec4(0.90f, 0.30f, 0.34f, 1.0f);
	style->pressed.background_color = s_vec4(0.50f, 0.12f, 0.15f, 0.98f);
	style->pressed.border_color = s_vec4(0.70f, 0.18f, 0.22f, 1.0f);
	style->focused.border_color = s_vec4(0.96f, 0.42f, 0.42f, 1.0f);
}

void se_ui_style_apply_text_muted(se_ui_style* style) {
	if (!style) {
		return;
	}
	style->normal.text_color = s_vec4(0.62f, 0.68f, 0.76f, 1.0f);
	style->hovered.text_color = s_vec4(0.70f, 0.76f, 0.84f, 1.0f);
	style->pressed.text_color = s_vec4(0.56f, 0.62f, 0.70f, 1.0f);
	style->disabled.text_color = s_vec4(0.46f, 0.50f, 0.56f, 0.92f);
	style->focused.text_color = s_vec4(0.74f, 0.80f, 0.88f, 1.0f);
	style->normal.background_color.w = 0.0f;
	style->hovered.background_color.w = 0.0f;
	style->pressed.background_color.w = 0.0f;
	style->disabled.background_color.w = 0.0f;
	style->focused.background_color.w = 0.0f;
	style->normal.border_width = 0.0f;
	style->hovered.border_width = 0.0f;
	style->pressed.border_width = 0.0f;
	style->disabled.border_width = 0.0f;
	style->focused.border_width = 0.0f;
}

b8 se_ui_widget_use_style_preset(se_ui_handle ui, se_ui_widget_handle widget, se_ui_style_preset preset) {
	se_ui_style style = SE_UI_STYLE_DEFAULT;
	switch (preset) {
		case SE_UI_STYLE_PRESET_DEFAULT:
			style = SE_UI_STYLE_DEFAULT;
			break;
		case SE_UI_STYLE_PRESET_BUTTON_PRIMARY:
			style = SE_UI_STYLE_DEFAULT;
			se_ui_style_apply_button_primary(&style);
			break;
		case SE_UI_STYLE_PRESET_BUTTON_DANGER:
			style = SE_UI_STYLE_DEFAULT;
			se_ui_style_apply_button_danger(&style);
			break;
		case SE_UI_STYLE_PRESET_TEXT_MUTED:
			style = SE_UI_STYLE_DEFAULT;
			se_ui_style_apply_text_muted(&style);
			break;
		case SE_UI_STYLE_PRESET_SCROLL_ITEM_NORMAL:
			style = SE_UI_STYLE_DEFAULT;
			style.normal.background_color = s_vec4(0.12f, 0.14f, 0.18f, 0.92f);
			style.hovered.background_color = s_vec4(0.16f, 0.18f, 0.22f, 0.96f);
			style.pressed.background_color = s_vec4(0.10f, 0.12f, 0.16f, 0.96f);
			break;
		case SE_UI_STYLE_PRESET_SCROLL_ITEM_SELECTED:
			style = SE_UI_STYLE_DEFAULT;
			style.normal.background_color = s_vec4(0.16f, 0.26f, 0.38f, 0.95f);
			style.normal.border_color = s_vec4(0.28f, 0.64f, 0.90f, 1.0f);
			style.hovered.background_color = s_vec4(0.20f, 0.32f, 0.46f, 0.98f);
			style.hovered.border_color = s_vec4(0.34f, 0.70f, 0.95f, 1.0f);
			style.pressed.background_color = s_vec4(0.12f, 0.22f, 0.34f, 0.98f);
			style.pressed.border_color = s_vec4(0.22f, 0.56f, 0.84f, 1.0f);
			break;
		default:
			return false;
	}
	return se_ui_widget_set_style(ui, widget, &style);
}

b8 se_ui_theme_apply(se_ui_handle ui, se_ui_theme_preset preset) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!ctx || !root) {
		return false;
	}
	se_ui_style panel_style = SE_UI_STYLE_DEFAULT;
	se_ui_style button_style = SE_UI_STYLE_DEFAULT;
	se_ui_style textbox_style = SE_UI_STYLE_DEFAULT;
	se_ui_style scrollbox_style = SE_UI_STYLE_DEFAULT;
	se_ui_style text_style = SE_UI_STYLE_DEFAULT;
	if (preset == SE_UI_THEME_PRESET_LIGHT) {
		panel_style.normal.background_color = s_vec4(0.88f, 0.90f, 0.94f, 0.95f);
		panel_style.normal.border_color = s_vec4(0.68f, 0.72f, 0.78f, 1.0f);
		panel_style.hovered = panel_style.normal;
		panel_style.pressed = panel_style.normal;
		panel_style.focused = panel_style.normal;
		button_style = panel_style;
		se_ui_style_apply_button_primary(&button_style);
		textbox_style = panel_style;
		textbox_style.focused.border_color = s_vec4(0.24f, 0.54f, 0.88f, 1.0f);
		scrollbox_style = panel_style;
		text_style = panel_style;
		se_ui_style_apply_text_muted(&text_style);
	} else if (preset != SE_UI_THEME_PRESET_DEFAULT) {
		return false;
	}
	if (preset == SE_UI_THEME_PRESET_DEFAULT) {
		se_ui_style_apply_button_primary(&button_style);
		se_ui_style_apply_text_muted(&text_style);
	}

	b8 changed = false;
	for (sz i = 0; i < s_array_get_size(&ctx->ui_elements); ++i) {
		const se_ui_widget_handle handle = s_array_handle(&ctx->ui_elements, (u32)i);
		se_ui_widget* widget = se_ui_widget_from_handle(ctx, handle);
		if (!widget || widget->ui != ui) {
			continue;
		}
		se_ui_style target = SE_UI_STYLE_DEFAULT;
		switch (widget->type) {
			case SE_UI_WIDGET_PANEL:
				target = panel_style;
				break;
			case SE_UI_WIDGET_BUTTON:
				target = button_style;
				break;
			case SE_UI_WIDGET_TEXT:
				target = text_style;
				break;
			case SE_UI_WIDGET_TEXTBOX:
				target = textbox_style;
				break;
			case SE_UI_WIDGET_SCROLLBOX:
				target = scrollbox_style;
				break;
			default:
				target = SE_UI_STYLE_DEFAULT;
				break;
		}
		if (!se_ui_style_equal(&widget->desc.style, &target)) {
			widget->desc.style = target;
			changed = true;
		}
	}
	if (!changed) {
		return true;
	}
	se_ui_mark_visual_dirty_internal(root);
	se_ui_mark_text_dirty_internal(root);
	return true;
}

se_ui_widget_handle se_ui_scroll_item_add(se_ui_handle ui, se_ui_widget_handle scrollbox, const c8* text) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* scrollbox_widget = se_ui_widget_from_handle(ctx, scrollbox);
	if (!root || !scrollbox_widget || scrollbox_widget->ui != ui || scrollbox_widget->type != SE_UI_WIDGET_SCROLLBOX) {
		return S_HANDLE_NULL;
	}
	se_ui_button_desc item_desc = SE_UI_BUTTON_DESC_DEFAULTS;
	item_desc.text = text ? text : "";
	item_desc.widget.layout.align_horizontal = SE_UI_ALIGN_STRETCH;
	item_desc.widget.layout.align_vertical = SE_UI_ALIGN_CENTER;
	item_desc.widget.layout.margin = se_ui_edge_xy(0.0f, 0.002f);
	item_desc.widget.size = s_vec2(scrollbox_widget->desc.size.x * 0.92f, 0.034f);
	se_ui_widget_handle item = se_ui_button_add(ui, scrollbox, &item_desc);
	if (item == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_ui_scrollbox_refresh_item_styles(root, scrollbox_widget, scrollbox);
	return item;
}

b8 se_ui_scrollbox_set_selected(se_ui_handle ui, se_ui_widget_handle scrollbox, se_ui_widget_handle item) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!root) {
		return false;
	}
	return se_ui_scrollbox_set_selected_internal(root, scrollbox, item, true);
}

se_ui_widget_handle se_ui_scrollbox_get_selected(se_ui_handle ui, se_ui_widget_handle scrollbox) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, scrollbox);
	if (!widget || widget->ui != ui || widget->type != SE_UI_WIDGET_SCROLLBOX) {
		return S_HANDLE_NULL;
	}
	if (widget->selected_item == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_ui_widget* selected = se_ui_widget_from_handle(ctx, widget->selected_item);
	if (!selected || selected->parent != scrollbox) {
		return S_HANDLE_NULL;
	}
	return widget->selected_item;
}

b8 se_ui_scrollbox_enable_single_select(se_ui_handle ui, se_ui_widget_handle scrollbox, b8 enabled) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, scrollbox);
	if (!root || !widget || widget->ui != ui || widget->type != SE_UI_WIDGET_SCROLLBOX) {
		return false;
	}
	if (widget->single_select_enabled == enabled) {
		return true;
	}
	widget->single_select_enabled = enabled;
	if (!enabled) {
		widget->selected_item = S_HANDLE_NULL;
	}
	se_ui_scrollbox_refresh_item_styles(root, widget, scrollbox);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_scrollbox_set_item_styles(se_ui_handle ui, se_ui_widget_handle scrollbox, const se_ui_style* normal, const se_ui_style* selected) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget = se_ui_widget_from_handle(ctx, scrollbox);
	if (!root || !widget || widget->ui != ui || widget->type != SE_UI_WIDGET_SCROLLBOX) {
		return false;
	}
	se_ui_style normal_style = normal ? *normal : SE_UI_STYLE_DEFAULT;
	se_ui_style selected_style = selected ? *selected : SE_UI_STYLE_DEFAULT;
	const b8 changed = !widget->has_scroll_item_styles ||
		!se_ui_style_equal(&widget->scroll_item_style_normal, &normal_style) ||
		!se_ui_style_equal(&widget->scroll_item_style_selected, &selected_style);
	if (!changed) {
		return true;
	}
	widget->scroll_item_style_normal = normal_style;
	widget->scroll_item_style_selected = selected_style;
	widget->has_scroll_item_styles = true;
	se_ui_scrollbox_refresh_item_styles(root, widget, scrollbox);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_scrollbox_scroll_to_item(se_ui_handle ui, se_ui_widget_handle scrollbox, se_ui_widget_handle item) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* scrollbox_widget = se_ui_widget_from_handle(ctx, scrollbox);
	se_ui_widget* item_widget = se_ui_widget_from_handle(ctx, item);
	if (!root || !scrollbox_widget || !item_widget || scrollbox_widget->ui != ui || item_widget->ui != ui || scrollbox_widget->type != SE_UI_WIDGET_SCROLLBOX || item_widget->parent != scrollbox) {
		return false;
	}
	if (root->dirty_layout || root->dirty_structure) {
		se_ui_rebuild(root);
	}
	const f32 overflow = s_max(0.0f, scrollbox_widget->content_extent - scrollbox_widget->viewport_extent);
	if (overflow <= 0.0001f) {
		return true;
	}
	f32 value = scrollbox_widget->value;
	const se_box_2d viewport = scrollbox_widget->content_bounds;
	if (scrollbox_widget->desc.layout.direction == SE_UI_LAYOUT_VERTICAL) {
		if (item_widget->bounds.max.y > viewport.max.y) {
			value -= (item_widget->bounds.max.y - viewport.max.y) / overflow;
		}
		if (item_widget->bounds.min.y < viewport.min.y) {
			value += (viewport.min.y - item_widget->bounds.min.y) / overflow;
		}
	} else {
		if (item_widget->bounds.min.x < viewport.min.x) {
			value -= (viewport.min.x - item_widget->bounds.min.x) / overflow;
		}
		if (item_widget->bounds.max.x > viewport.max.x) {
			value += (item_widget->bounds.max.x - viewport.max.x) / overflow;
		}
	}
	return se_ui_widget_set_value(ui, scrollbox, se_ui_clamp01(value));
}

se_ui_widget_handle se_ui_widget_find(se_ui_handle ui, const c8* id) {
	se_context* ctx = se_current_context();
	if (!ctx || !id || id[0] == '\0') {
		return S_HANDLE_NULL;
	}
	for (sz i = 0; i < s_array_get_size(&ctx->ui_elements); ++i) {
		const se_ui_widget_handle handle = s_array_handle(&ctx->ui_elements, (u32)i);
		se_ui_widget* widget = se_ui_widget_from_handle(ctx, handle);
		if (!widget || widget->ui != ui) {
			continue;
		}
		if (strcmp(se_ui_chars_cstr(&widget->id), id) == 0) {
			return handle;
		}
	}
	return S_HANDLE_NULL;
}

b8 se_ui_widget_set_text_by_id(se_ui_handle ui, const c8* id, const c8* text) {
	const se_ui_widget_handle widget = se_ui_widget_find(ui, id);
	if (widget == S_HANDLE_NULL) {
		return false;
	}
	return se_ui_widget_set_text(ui, widget, text);
}

b8 se_ui_widget_set_visible_by_id(se_ui_handle ui, const c8* id, b8 visible) {
	const se_ui_widget_handle widget = se_ui_widget_find(ui, id);
	if (widget == S_HANDLE_NULL) {
		return false;
	}
	return se_ui_widget_set_visible(ui, widget, visible);
}

b8 se_ui_widget_exists(const se_ui_handle ui, const se_ui_widget_handle widget) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	return widget_ptr && widget_ptr->ui == ui;
}

b8 se_ui_widget_attach(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_widget_handle child) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!root) {
		return false;
	}
	if (root->dispatching) {
		se_ui_deferred_op op = { .type = SE_UI_DEFER_ATTACH, .parent = parent, .child = child };
		s_array_add(&root->deferred_ops, op);
		return true;
	}
	return se_ui_widget_attach_immediate(root, parent, child);
}

b8 se_ui_widget_detach(const se_ui_handle ui, const se_ui_widget_handle child) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!root) {
		return false;
	}
	if (root->dispatching) {
		se_ui_deferred_op op = { .type = SE_UI_DEFER_DETACH, .parent = S_HANDLE_NULL, .child = child };
		s_array_add(&root->deferred_ops, op);
		return true;
	}
	return se_ui_widget_detach_immediate(root, child);
}

b8 se_ui_widget_remove(const se_ui_handle ui, const se_ui_widget_handle widget) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	if (!root) {
		return false;
	}
	if (root->dispatching) {
		se_ui_deferred_op op = { .type = SE_UI_DEFER_REMOVE, .parent = S_HANDLE_NULL, .child = widget };
		s_array_add(&root->deferred_ops, op);
		return true;
	}
	se_ui_widget_destroy_recursive(root, widget);
	return true;
}

b8 se_ui_widget_set_layout(const se_ui_handle ui, const se_ui_widget_handle widget, const se_ui_layout* layout) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui || !layout) {
		return false;
	}
	if (se_ui_layout_equal(&widget_ptr->desc.layout, layout)) {
		return true;
	}
	widget_ptr->desc.layout = *layout;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_get_layout(const se_ui_handle ui, const se_ui_widget_handle widget, se_ui_layout* out_layout) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!out_layout || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	*out_layout = widget_ptr->desc.layout;
	return true;
}

b8 se_ui_widget_set_style(const se_ui_handle ui, const se_ui_widget_handle widget, const se_ui_style* style) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui || !style) {
		return false;
	}
	if (se_ui_style_equal(&widget_ptr->desc.style, style)) {
		return true;
	}
	widget_ptr->desc.style = *style;
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_get_style(const se_ui_handle ui, const se_ui_widget_handle widget, se_ui_style* out_style) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!out_style || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	*out_style = widget_ptr->desc.style;
	return true;
}

b8 se_ui_widget_set_position(const se_ui_handle ui, const se_ui_widget_handle widget, const s_vec2* position) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui || !position) {
		return false;
	}
	if (se_ui_vec2_equal(&widget_ptr->desc.position, position)) {
		return true;
	}
	widget_ptr->desc.position = *position;
	se_ui_mark_layout_dirty_internal(root);
	return true;
}

b8 se_ui_widget_set_size(const se_ui_handle ui, const se_ui_widget_handle widget, const s_vec2* size) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui || !size) {
		return false;
	}
	if (se_ui_vec2_equal(&widget_ptr->desc.size, size)) {
		return true;
	}
	widget_ptr->desc.size = *size;
	se_ui_mark_layout_dirty_internal(root);
	return true;
}

b8 se_ui_widget_get_bounds(const se_ui_handle ui, const se_ui_widget_handle widget, se_box_2d* out_bounds) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!out_bounds || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	*out_bounds = widget_ptr->bounds;
	return true;
}

b8 se_ui_widget_set_visible(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 visible) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (widget_ptr->desc.visible == visible) {
		return true;
	}
	widget_ptr->desc.visible = visible;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_is_visible(const se_ui_handle ui, const se_ui_widget_handle widget) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	return widget_ptr && widget_ptr->ui == ui && widget_ptr->desc.visible;
}

b8 se_ui_widget_set_enabled(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 enabled) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (widget_ptr->desc.enabled == enabled) {
		return true;
	}
	widget_ptr->desc.enabled = enabled;
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_is_enabled(const se_ui_handle ui, const se_ui_widget_handle widget) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	return widget_ptr && widget_ptr->ui == ui && widget_ptr->desc.enabled;
}

b8 se_ui_widget_set_interactable(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 interactable) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (widget_ptr->desc.interactable == interactable) {
		return true;
	}
	widget_ptr->desc.interactable = interactable;
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

b8 se_ui_widget_is_interactable(const se_ui_handle ui, const se_ui_widget_handle widget) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	return widget_ptr && widget_ptr->ui == ui && widget_ptr->desc.interactable;
}

b8 se_ui_widget_set_clipping(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 clip_children) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (widget_ptr->desc.layout.clip_children == clip_children) {
		return true;
	}
	widget_ptr->desc.layout.clip_children = clip_children;
	se_ui_mark_layout_dirty_internal(root);
	return true;
}

b8 se_ui_widget_is_clipping(const se_ui_handle ui, const se_ui_widget_handle widget) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	return widget_ptr && widget_ptr->ui == ui && widget_ptr->desc.layout.clip_children;
}

b8 se_ui_widget_set_z_order(const se_ui_handle ui, const se_ui_widget_handle widget, const i32 z_order) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (widget_ptr->desc.z_order == z_order) {
		return true;
	}
	widget_ptr->desc.z_order = z_order;
	se_ui_mark_structure_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

i32 se_ui_widget_get_z_order(const se_ui_handle ui, const se_ui_widget_handle widget) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr || widget_ptr->ui != ui) {
		return 0;
	}
	return widget_ptr->desc.z_order;
}

b8 se_ui_widget_set_text(const se_ui_handle ui, const se_ui_widget_handle widget, const c8* text) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (widget_ptr->type != SE_UI_WIDGET_TEXT && widget_ptr->type != SE_UI_WIDGET_BUTTON && widget_ptr->type != SE_UI_WIDGET_TEXTBOX) {
		return false;
	}
	if (!text) {
		text = "";
	}
	if (strlen(text) >= SE_TEXT_CHAR_COUNT) {
		return false;
	}
	if (strcmp(se_ui_chars_cstr(&widget_ptr->text), text) == 0) {
		return true;
	}
	se_ui_chars_set(&widget_ptr->text, text);
	widget_ptr->caret = se_ui_chars_length(&widget_ptr->text);
	se_ui_textbox_clear_selection(widget_ptr);
	se_ui_mark_text_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	return true;
}

sz se_ui_widget_get_text(const se_ui_handle ui, const se_ui_widget_handle widget, c8* out_text, const sz out_text_size) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr || widget_ptr->ui != ui) {
		return 0;
	}
	const c8* text = se_ui_chars_cstr(&widget_ptr->text);
	const sz len = strlen(text);
	if (!out_text || out_text_size == 0) {
		return len;
	}
	const sz copy_len = s_min(len, out_text_size - 1);
	memcpy(out_text, text, copy_len);
	out_text[copy_len] = '\0';
	return copy_len;
}

b8 se_ui_widget_set_value(const se_ui_handle ui, const se_ui_widget_handle widget, const f32 value) {
	se_context* ctx = se_current_context();
	se_ui_root* root = se_ui_root_from_handle(ctx, ui);
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!root || !widget_ptr || widget_ptr->ui != ui) {
		return false;
	}
	if (widget_ptr->type != SE_UI_WIDGET_SCROLLBOX) {
		return false;
	}
	const f32 clamped = se_ui_clamp01(value);
	if (fabsf(clamped - widget_ptr->value) <= 0.0001f) {
		return true;
	}
	widget_ptr->value = clamped;
	se_ui_mark_layout_dirty_internal(root);
	se_ui_mark_visual_dirty_internal(root);
	se_ui_fire_scroll(widget_ptr, widget, NULL);
	se_ui_fire_change(widget_ptr, widget);
	return true;
}

f32 se_ui_widget_get_value(const se_ui_handle ui, const se_ui_widget_handle widget) {
	se_context* ctx = se_current_context();
	se_ui_widget* widget_ptr = se_ui_widget_from_handle(ctx, widget);
	if (!widget_ptr || widget_ptr->ui != ui) {
		return 0.0f;
	}
	if (widget_ptr->type != SE_UI_WIDGET_SCROLLBOX) {
		return 0.0f;
	}
	return widget_ptr->value;
}

void se_ui_set_debug_overlay(const b8 enabled) {
	g_ui_debug_overlay_enabled = enabled;
}

b8 se_ui_is_debug_overlay_enabled(void) {
	return g_ui_debug_overlay_enabled;
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
