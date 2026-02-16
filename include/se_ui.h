// Syphax-Engine - Ougi Washi

#ifndef SE_UI_H
#define SE_UI_H

#include "se_scene.h"
#include "se_text.h"
#include "se_window.h"

typedef enum {
	SE_UI_LAYOUT_VERTICAL = 0,
	SE_UI_LAYOUT_HORIZONTAL
} se_ui_layout_direction;

typedef enum {
	SE_UI_ALIGN_START = 0,
	SE_UI_ALIGN_CENTER,
	SE_UI_ALIGN_END,
	SE_UI_ALIGN_STRETCH
} se_ui_alignment;

typedef enum {
	SE_UI_WIDGET_PANEL = 0,
	SE_UI_WIDGET_BUTTON,
	SE_UI_WIDGET_TEXT,
	SE_UI_WIDGET_TEXTBOX,
	SE_UI_WIDGET_SCROLLBOX
} se_ui_widget_type;

typedef enum {
	SE_UI_STYLE_PRESET_DEFAULT = 0,
	SE_UI_STYLE_PRESET_BUTTON_PRIMARY,
	SE_UI_STYLE_PRESET_BUTTON_DANGER,
	SE_UI_STYLE_PRESET_TEXT_MUTED,
	SE_UI_STYLE_PRESET_SCROLL_ITEM_NORMAL,
	SE_UI_STYLE_PRESET_SCROLL_ITEM_SELECTED
} se_ui_style_preset;

typedef enum {
	SE_UI_THEME_PRESET_DEFAULT = 0,
	SE_UI_THEME_PRESET_LIGHT
} se_ui_theme_preset;

typedef struct {
	f32 left;
	f32 right;
	f32 top;
	f32 bottom;
} se_ui_edge;

typedef struct {
	s_vec4 background_color;
	s_vec4 border_color;
	s_vec4 text_color;
	f32 border_width;
} se_ui_style_state;

typedef struct {
	se_ui_style_state normal;
	se_ui_style_state hovered;
	se_ui_style_state pressed;
	se_ui_style_state disabled;
	se_ui_style_state focused;
} se_ui_style;

typedef struct {
	se_ui_layout_direction direction;
	se_ui_alignment align_horizontal;
	se_ui_alignment align_vertical;
	f32 spacing;
	s_vec2 anchor_min;
	s_vec2 anchor_max;
	se_ui_edge margin;
	se_ui_edge padding;
	s_vec2 min_size;
	s_vec2 max_size;
	b8 use_anchors : 1;
	b8 clip_children : 1;
} se_ui_layout;

typedef struct {
	s_vec2 position;
	s_vec2 size;
	const c8* id;
	b8 visible : 1;
	b8 enabled : 1;
	b8 interactable : 1;
	i32 z_order;
	se_ui_layout layout;
	se_ui_style style;
} se_ui_widget_desc;

typedef struct {
	se_ui_handle ui;
	se_ui_widget_handle widget;
	s_vec2 pointer_ndc;
	se_mouse_button button;
} se_ui_click_event;

typedef struct {
	se_ui_handle ui;
	se_ui_widget_handle widget;
	s_vec2 pointer_ndc;
} se_ui_hover_event;

typedef struct {
	se_ui_handle ui;
	se_ui_widget_handle widget;
} se_ui_focus_event;

typedef struct {
	se_ui_handle ui;
	se_ui_widget_handle widget;
	f32 value;
	const c8* text;
} se_ui_change_event;

typedef struct {
	se_ui_handle ui;
	se_ui_widget_handle widget;
	const c8* text;
} se_ui_submit_event;

typedef struct {
	se_ui_handle ui;
	se_ui_widget_handle widget;
	f32 value;
	s_vec2 pointer_ndc;
} se_ui_scroll_event;

typedef struct {
	se_ui_handle ui;
	se_ui_widget_handle widget;
	s_vec2 pointer_ndc;
	se_mouse_button button;
} se_ui_press_event;

typedef void (*se_ui_click_callback)(const se_ui_click_event* event, void* user_data);
typedef void (*se_ui_hover_callback)(const se_ui_hover_event* event, void* user_data);
typedef void (*se_ui_focus_callback)(const se_ui_focus_event* event, void* user_data);
typedef void (*se_ui_change_callback)(const se_ui_change_event* event, void* user_data);
typedef void (*se_ui_submit_callback)(const se_ui_submit_event* event, void* user_data);
typedef void (*se_ui_scroll_callback)(const se_ui_scroll_event* event, void* user_data);
typedef void (*se_ui_press_callback)(const se_ui_press_event* event, void* user_data);

typedef struct {
	se_ui_hover_callback on_hover_start;
	se_ui_hover_callback on_hover_end;
	se_ui_focus_callback on_focus;
	se_ui_focus_callback on_blur;
	se_ui_press_callback on_press;
	se_ui_press_callback on_release;
	se_ui_click_callback on_click;
	se_ui_change_callback on_change;
	se_ui_submit_callback on_submit;
	se_ui_scroll_callback on_scroll;
	void* user_data;
} se_ui_callbacks;

typedef struct {
	se_ui_widget_desc widget;
} se_ui_panel_desc;

typedef struct {
	se_ui_widget_desc widget;
	const c8* text;
	const c8* font_path;
	f32 font_size;
} se_ui_text_desc;

typedef struct {
	se_ui_widget_desc widget;
	const c8* text;
	const c8* font_path;
	f32 font_size;
	se_ui_callbacks callbacks;
} se_ui_button_desc;

typedef struct {
	se_ui_widget_desc widget;
	const c8* text;
	const c8* placeholder;
	const c8* font_path;
	f32 font_size;
	u32 max_length;
	b8 submit_on_enter : 1;
	se_ui_callbacks callbacks;
} se_ui_textbox_desc;

typedef struct {
	se_ui_widget_desc widget;
	f32 value;
	f32 wheel_step;
	se_ui_callbacks callbacks;
} se_ui_scrollbox_desc;

#define SE_UI_EDGE_ZERO ((se_ui_edge){ .left = 0.0f, .right = 0.0f, .top = 0.0f, .bottom = 0.0f })
#define se_ui_edge_all(v) ((se_ui_edge){ (v), (v), (v), (v) })
#define se_ui_edge_xy(x, y) ((se_ui_edge){ (x), (x), (y), (y) })
#define SE_UI_FONT_DEFAULT SE_RESOURCE_PUBLIC("fonts/ithaca.ttf")

#define SE_UI_STYLE_STATE_DEFAULT \
	((se_ui_style_state){ \
		.background_color = s_vec4(0.14f, 0.16f, 0.20f, 0.86f), \
		.border_color = s_vec4(0.28f, 0.32f, 0.38f, 1.00f), \
		.text_color = s_vec4(0.92f, 0.94f, 0.97f, 1.00f), \
		.border_width = 0.003f \
	})

#define SE_UI_STYLE_DEFAULT \
	((se_ui_style){ \
		.normal = SE_UI_STYLE_STATE_DEFAULT, \
		.hovered = (se_ui_style_state){ .background_color = s_vec4(0.18f, 0.20f, 0.24f, 0.94f), .border_color = s_vec4(0.33f, 0.38f, 0.45f, 1.00f), .text_color = s_vec4(0.95f, 0.96f, 0.98f, 1.00f), .border_width = 0.003f }, \
		.pressed = (se_ui_style_state){ .background_color = s_vec4(0.10f, 0.12f, 0.16f, 0.95f), .border_color = s_vec4(0.18f, 0.22f, 0.28f, 1.00f), .text_color = s_vec4(0.90f, 0.92f, 0.95f, 1.00f), .border_width = 0.003f }, \
		.disabled = (se_ui_style_state){ .background_color = s_vec4(0.10f, 0.10f, 0.12f, 0.55f), .border_color = s_vec4(0.16f, 0.16f, 0.18f, 0.80f), .text_color = s_vec4(0.62f, 0.64f, 0.68f, 0.90f), .border_width = 0.002f }, \
		.focused = (se_ui_style_state){ .background_color = s_vec4(0.13f, 0.18f, 0.24f, 0.96f), .border_color = s_vec4(0.22f, 0.66f, 0.55f, 1.00f), .text_color = s_vec4(0.95f, 0.98f, 1.00f, 1.00f), .border_width = 0.004f } \
	})

#define SE_UI_LAYOUT_DEFAULT \
	((se_ui_layout){ \
		.direction = SE_UI_LAYOUT_VERTICAL, \
		.align_horizontal = SE_UI_ALIGN_STRETCH, \
		.align_vertical = SE_UI_ALIGN_START, \
		.spacing = 0.010f, \
		.anchor_min = s_vec2(0.0f, 0.0f), \
		.anchor_max = s_vec2(1.0f, 1.0f), \
		.margin = SE_UI_EDGE_ZERO, \
		.padding = SE_UI_EDGE_ZERO, \
		.min_size = s_vec2(0.0f, 0.0f), \
		.max_size = s_vec2(0.0f, 0.0f), \
		.use_anchors = false, \
		.clip_children = false \
	})

#define SE_UI_WIDGET_DESC_DEFAULTS \
	((se_ui_widget_desc){ \
		.position = s_vec2(0.0f, 0.0f), \
		.size = s_vec2(0.20f, 0.08f), \
		.id = NULL, \
		.visible = true, \
		.enabled = true, \
		.interactable = false, \
		.z_order = 0, \
		.layout = SE_UI_LAYOUT_DEFAULT, \
		.style = SE_UI_STYLE_DEFAULT \
	})

#define SE_UI_PANEL_DESC_DEFAULTS ((se_ui_panel_desc){ .widget = SE_UI_WIDGET_DESC_DEFAULTS })
#define SE_UI_TEXT_DESC_DEFAULTS ((se_ui_text_desc){ .widget = SE_UI_WIDGET_DESC_DEFAULTS, .text = "", .font_path = SE_UI_FONT_DEFAULT, .font_size = 28.0f })
#define SE_UI_BUTTON_DESC_DEFAULTS ((se_ui_button_desc){ .widget = SE_UI_WIDGET_DESC_DEFAULTS, .text = "Button", .font_path = SE_UI_FONT_DEFAULT, .font_size = 28.0f, .callbacks = {0} })
#define SE_UI_TEXTBOX_DESC_DEFAULTS ((se_ui_textbox_desc){ .widget = SE_UI_WIDGET_DESC_DEFAULTS, .text = "", .placeholder = "", .font_path = SE_UI_FONT_DEFAULT, .font_size = 24.0f, .max_length = 256, .submit_on_enter = true, .callbacks = {0} })
#define SE_UI_SCROLLBOX_DESC_DEFAULTS ((se_ui_scrollbox_desc){ .widget = SE_UI_WIDGET_DESC_DEFAULTS, .value = 0.0f, .wheel_step = 0.08f, .callbacks = {0} })

extern void se_ui_get_default_style(se_ui_style* out_style);
extern void se_ui_get_default_layout(se_ui_layout* out_layout);
extern void se_ui_get_default_widget_desc(se_ui_widget_desc* out_desc);

extern se_ui_handle se_ui_create(const se_window_handle window, const u16 widget_capacity);
extern void se_ui_destroy(const se_ui_handle ui);
extern void se_ui_destroy_all(void);
extern void se_ui_tick(const se_ui_handle ui);
extern void se_ui_draw(const se_ui_handle ui);

extern void se_ui_mark_layout_dirty(const se_ui_handle ui);
extern void se_ui_mark_visual_dirty(const se_ui_handle ui);
extern void se_ui_mark_text_dirty(const se_ui_handle ui);
extern void se_ui_mark_structure_dirty(const se_ui_handle ui);
extern b8 se_ui_is_dirty(const se_ui_handle ui);

extern se_scene_2d_handle se_ui_get_scene(const se_ui_handle ui);
extern se_window_handle se_ui_get_window(const se_ui_handle ui);
extern se_ui_widget_handle se_ui_get_hovered_widget(const se_ui_handle ui);
extern se_ui_widget_handle se_ui_get_focused_widget(const se_ui_handle ui);
extern b8 se_ui_focus_widget(const se_ui_handle ui, const se_ui_widget_handle widget);
extern void se_ui_clear_focus(const se_ui_handle ui);

extern se_ui_widget_handle se_ui_panel_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_panel_desc* desc);
extern se_ui_widget_handle se_ui_button_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_button_desc* desc);
extern se_ui_widget_handle se_ui_text_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_text_desc* desc);
extern se_ui_widget_handle se_ui_textbox_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_textbox_desc* desc);
extern se_ui_widget_handle se_ui_scrollbox_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_scrollbox_desc* desc);

/* Adds a panel with default style/behavior and explicit size. */
extern se_ui_widget_handle se_ui_panel_create(se_ui_handle ui, se_ui_widget_handle parent, s_vec2 size);
/* Adds a text widget with defaults and label text. */
extern se_ui_widget_handle se_ui_text_create(se_ui_handle ui, se_ui_widget_handle parent, const c8* text);
/* Adds a button with defaults and label text. */
extern se_ui_widget_handle se_ui_button_create(se_ui_handle ui, se_ui_widget_handle parent, const c8* text);
/* Adds a textbox with defaults and placeholder text. */
extern se_ui_widget_handle se_ui_textbox_create(se_ui_handle ui, se_ui_widget_handle parent, const c8* placeholder);
/* Adds a scrollbox with defaults and explicit size. */
extern se_ui_widget_handle se_ui_scrollbox_create(se_ui_handle ui, se_ui_widget_handle parent, s_vec2 size);
/* Adds a non-visual spacer panel with explicit size. */
extern se_ui_widget_handle se_ui_spacer_create(se_ui_handle ui, se_ui_widget_handle parent, s_vec2 size);

/* Replaces the full callback set for a widget. Returns false for unsupported callback types. */
extern b8 se_ui_widget_set_callbacks(se_ui_handle ui, se_ui_widget_handle widget, const se_ui_callbacks* callbacks);
/* Updates the callback user-data pointer on an existing widget. */
extern b8 se_ui_widget_set_user_data(se_ui_handle ui, se_ui_widget_handle widget, void* user_data);
/* Sets click callback and user data for a widget. */
extern b8 se_ui_widget_on_click(se_ui_handle ui, se_ui_widget_handle widget, se_ui_click_callback cb, void* user_data);
/* Sets hover callbacks and user data for a widget. */
extern b8 se_ui_widget_on_hover(se_ui_handle ui, se_ui_widget_handle widget, se_ui_hover_callback on_start, se_ui_hover_callback on_end, void* user_data);
/* Sets press/release callbacks and user data for a widget. */
extern b8 se_ui_widget_on_press(se_ui_handle ui, se_ui_widget_handle widget, se_ui_press_callback on_press, se_ui_press_callback on_release, void* user_data);
/* Sets value/text change callback and user data for a widget. */
extern b8 se_ui_widget_on_change(se_ui_handle ui, se_ui_widget_handle widget, se_ui_change_callback cb, void* user_data);
/* Sets submit callback and user data for a widget. */
extern b8 se_ui_widget_on_submit(se_ui_handle ui, se_ui_widget_handle widget, se_ui_submit_callback cb, void* user_data);
/* Sets scroll callback and user data for a widget. */
extern b8 se_ui_widget_on_scroll(se_ui_handle ui, se_ui_widget_handle widget, se_ui_scroll_callback cb, void* user_data);

/* Configures vertical stack layout and spacing for a container. */
extern b8 se_ui_widget_set_stack_vertical(se_ui_handle ui, se_ui_widget_handle widget, f32 spacing);
/* Configures horizontal stack layout and spacing for a container. */
extern b8 se_ui_widget_set_stack_horizontal(se_ui_handle ui, se_ui_widget_handle widget, f32 spacing);
/* Sets per-widget horizontal/vertical alignment in parent layout. */
extern b8 se_ui_widget_set_alignment(se_ui_handle ui, se_ui_widget_handle widget, se_ui_alignment horizontal, se_ui_alignment vertical);
/* Sets per-widget layout padding. */
extern b8 se_ui_widget_set_padding(se_ui_handle ui, se_ui_widget_handle widget, se_ui_edge padding);
/* Sets per-widget layout margin. */
extern b8 se_ui_widget_set_margin(se_ui_handle ui, se_ui_widget_handle widget, se_ui_edge margin);
/* Enables anchors and sets anchor rectangle. */
extern b8 se_ui_widget_set_anchor_rect(se_ui_handle ui, se_ui_widget_handle widget, s_vec2 anchor_min, s_vec2 anchor_max);
/* Sets per-widget minimum size constraints (half extents in NDC units). */
extern b8 se_ui_widget_set_min_size(se_ui_handle ui, se_ui_widget_handle widget, s_vec2 min_size);
/* Sets per-widget maximum size constraints (half extents in NDC units). */
extern b8 se_ui_widget_set_max_size(se_ui_handle ui, se_ui_widget_handle widget, s_vec2 max_size);

/* Applies primary button style values in-place. */
extern void se_ui_style_apply_button_primary(se_ui_style* style);
/* Applies danger button style values in-place. */
extern void se_ui_style_apply_button_danger(se_ui_style* style);
/* Applies muted text style values in-place. */
extern void se_ui_style_apply_text_muted(se_ui_style* style);
/* Applies a predefined style preset to a widget. */
extern b8 se_ui_widget_use_style_preset(se_ui_handle ui, se_ui_widget_handle widget, se_ui_style_preset preset);
/* Applies a theme preset across all widgets in a UI root. */
extern b8 se_ui_theme_apply(se_ui_handle ui, se_ui_theme_preset preset);

/* Adds a selectable item entry to a scrollbox. */
extern se_ui_widget_handle se_ui_scroll_item_add(se_ui_handle ui, se_ui_widget_handle scrollbox, const c8* text);
/* Selects one item in a scrollbox. */
extern b8 se_ui_scrollbox_set_selected(se_ui_handle ui, se_ui_widget_handle scrollbox, se_ui_widget_handle item);
/* Returns the currently selected scrollbox item, or null. */
extern se_ui_widget_handle se_ui_scrollbox_get_selected(se_ui_handle ui, se_ui_widget_handle scrollbox);
/* Enables/disables single-selection behavior for scrollbox item children. */
extern b8 se_ui_scrollbox_enable_single_select(se_ui_handle ui, se_ui_widget_handle scrollbox, b8 enabled);
/* Sets normal and selected style templates for scrollbox item children. */
extern b8 se_ui_scrollbox_set_item_styles(se_ui_handle ui, se_ui_widget_handle scrollbox, const se_ui_style* normal, const se_ui_style* selected);
/* Scrolls viewport to make a specific child item visible. */
extern b8 se_ui_scrollbox_scroll_to_item(se_ui_handle ui, se_ui_widget_handle scrollbox, se_ui_widget_handle item);

/* Finds a widget by stable id string inside one UI root. */
extern se_ui_widget_handle se_ui_widget_find(se_ui_handle ui, const c8* id);
/* Sets text for a widget found by id. */
extern b8 se_ui_widget_set_text_by_id(se_ui_handle ui, const c8* id, const c8* text);
/* Sets visibility for a widget found by id. */
extern b8 se_ui_widget_set_visible_by_id(se_ui_handle ui, const c8* id, b8 visible);

extern b8 se_ui_widget_exists(const se_ui_handle ui, const se_ui_widget_handle widget);
extern b8 se_ui_widget_attach(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_widget_handle child);
extern b8 se_ui_widget_detach(const se_ui_handle ui, const se_ui_widget_handle child);
extern b8 se_ui_widget_remove(const se_ui_handle ui, const se_ui_widget_handle widget);

extern b8 se_ui_widget_set_layout(const se_ui_handle ui, const se_ui_widget_handle widget, const se_ui_layout* layout);
extern b8 se_ui_widget_get_layout(const se_ui_handle ui, const se_ui_widget_handle widget, se_ui_layout* out_layout);
extern b8 se_ui_widget_set_style(const se_ui_handle ui, const se_ui_widget_handle widget, const se_ui_style* style);
extern b8 se_ui_widget_get_style(const se_ui_handle ui, const se_ui_widget_handle widget, se_ui_style* out_style);

extern b8 se_ui_widget_set_position(const se_ui_handle ui, const se_ui_widget_handle widget, const s_vec2* position);
extern b8 se_ui_widget_set_size(const se_ui_handle ui, const se_ui_widget_handle widget, const s_vec2* size);
extern b8 se_ui_widget_get_bounds(const se_ui_handle ui, const se_ui_widget_handle widget, se_box_2d* out_bounds);

extern b8 se_ui_widget_set_visible(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 visible);
extern b8 se_ui_widget_is_visible(const se_ui_handle ui, const se_ui_widget_handle widget);
extern b8 se_ui_widget_set_enabled(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 enabled);
extern b8 se_ui_widget_is_enabled(const se_ui_handle ui, const se_ui_widget_handle widget);
extern b8 se_ui_widget_set_interactable(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 interactable);
extern b8 se_ui_widget_is_interactable(const se_ui_handle ui, const se_ui_widget_handle widget);
extern b8 se_ui_widget_set_clipping(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 clip_children);
extern b8 se_ui_widget_is_clipping(const se_ui_handle ui, const se_ui_widget_handle widget);
extern b8 se_ui_widget_set_z_order(const se_ui_handle ui, const se_ui_widget_handle widget, const i32 z_order);
extern i32 se_ui_widget_get_z_order(const se_ui_handle ui, const se_ui_widget_handle widget);

extern b8 se_ui_widget_set_text(const se_ui_handle ui, const se_ui_widget_handle widget, const c8* text);
extern sz se_ui_widget_get_text(const se_ui_handle ui, const se_ui_widget_handle widget, c8* out_text, const sz out_text_size);
extern b8 se_ui_widget_set_value(const se_ui_handle ui, const se_ui_widget_handle widget, const f32 value);
extern f32 se_ui_widget_get_value(const se_ui_handle ui, const se_ui_widget_handle widget);

extern void se_ui_set_debug_overlay(const b8 enabled);
extern b8 se_ui_is_debug_overlay_enabled(void);

extern b8 se_ui_world_to_window(const se_camera_handle camera, const se_window_handle window, const s_vec3* world, s_vec2* out_window_pixel);
extern b8 se_ui_world_to_ndc(const se_camera_handle camera, const s_vec3* world, s_vec2* out_ndc);
extern b8 se_ui_minimap_world_to_ui(const se_box_2d* world_rect, const se_box_2d* ui_rect, const s_vec2* world_point, s_vec2* out_ui_point);
extern b8 se_ui_minimap_ui_to_world(const se_box_2d* world_rect, const se_box_2d* ui_rect, const s_vec2* ui_point, s_vec2* out_world_point);

#define se_ui_create_root(ui) se_ui_panel_create((ui), S_HANDLE_NULL, s_vec2(1.0f, 1.0f))
#define se_ui_label(ui, parent, text) se_ui_text_create((ui), (parent), (text))

static inline b8 se_ui_widget_apply_vstack(se_ui_handle ui, se_ui_widget_handle widget, f32 spacing, se_ui_edge padding) {
	return se_ui_widget_set_stack_vertical(ui, widget, spacing) && se_ui_widget_set_padding(ui, widget, padding);
}

static inline b8 se_ui_widget_apply_hstack(se_ui_handle ui, se_ui_widget_handle widget, f32 spacing, se_ui_edge padding) {
	return se_ui_widget_set_stack_horizontal(ui, widget, spacing) && se_ui_widget_set_padding(ui, widget, padding);
}

static inline se_ui_widget_handle se_ui_button_create_with_click(
	se_ui_handle ui, se_ui_widget_handle parent, const c8* label, se_ui_click_callback on_click, void* user_data) {
	const se_ui_widget_handle button = se_ui_button_create(ui, parent, label);
	if (button != S_HANDLE_NULL && on_click) {
		(void)se_ui_widget_on_click(ui, button, on_click, user_data);
	}
	return button;
}

static inline se_ui_widget_handle se_ui_textbox_create_with_callbacks(
	se_ui_handle ui, se_ui_widget_handle parent, const c8* placeholder,
	se_ui_change_callback on_change, se_ui_submit_callback on_submit, void* user_data) {
	const se_ui_widget_handle textbox = se_ui_textbox_create(ui, parent, placeholder);
	if (textbox == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	if (on_change) {
		(void)se_ui_widget_on_change(ui, textbox, on_change, user_data);
	}
	if (on_submit) {
		(void)se_ui_widget_on_submit(ui, textbox, on_submit, user_data);
	}
	return textbox;
}

static inline se_ui_widget_handle se_ui_scrollbox_create_with_scroll(
	se_ui_handle ui, se_ui_widget_handle parent, s_vec2 size, se_ui_scroll_callback on_scroll, void* user_data) {
	const se_ui_widget_handle scrollbox = se_ui_scrollbox_create(ui, parent, size);
	if (scrollbox != S_HANDLE_NULL && on_scroll) {
		(void)se_ui_widget_on_scroll(ui, scrollbox, on_scroll, user_data);
	}
	return scrollbox;
}

#define se_ui_vstack(ui, widget, spacing, padding) se_ui_widget_apply_vstack((ui), (widget), (spacing), (padding))
#define se_ui_hstack(ui, widget, spacing, padding) se_ui_widget_apply_hstack((ui), (widget), (spacing), (padding))
#define se_ui_button(ui, parent, label, on_click, user_data) se_ui_button_create_with_click((ui), (parent), (label), (on_click), (user_data))
#define se_ui_textbox(ui, parent, placeholder, on_change, on_submit, user_data) se_ui_textbox_create_with_callbacks((ui), (parent), (placeholder), (on_change), (on_submit), (user_data))
#define se_ui_scrollbox(ui, parent, size, on_scroll, user_data) se_ui_scrollbox_create_with_scroll((ui), (parent), (size), (on_scroll), (user_data))

#endif // SE_UI_H
