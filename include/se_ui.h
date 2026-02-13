// Syphax-Engine - Ougi Washi

#ifndef SE_UI_H
#define SE_UI_H

#include "se_scene.h"
#include "se_text.h"
#include "se_window.h"

typedef enum {
	SE_UI_LAYOUT_HORIZONTAL,
	SE_UI_LAYOUT_VERTICAL
} se_ui_layout;

typedef enum {
	SE_UI_ALIGN_START = 0,
	SE_UI_ALIGN_CENTER,
	SE_UI_ALIGN_END
} se_ui_alignment;

typedef enum {
	SE_UI_WIDGET_NONE = 0,
	SE_UI_WIDGET_PANEL,
	SE_UI_WIDGET_LABEL,
	SE_UI_WIDGET_BUTTON,
	SE_UI_WIDGET_SLIDER,
	SE_UI_WIDGET_CHECKBOX,
	SE_UI_WIDGET_IMAGE
} se_ui_widget_type;

typedef struct {
	f32 left;
	f32 right;
	f32 top;
	f32 bottom;
} se_ui_margin;

typedef struct {
	s_vec2 anchor_min;
	s_vec2 anchor_max;
	se_ui_margin margin;
	s_vec2 min_size;
	s_vec2 max_size;
	se_ui_alignment align_x;
	se_ui_alignment align_y;
	b8 stretch_x : 1;
	b8 stretch_y : 1;
} se_ui_layout_rules;

typedef struct {
	s_vec4 panel_color;
	s_vec4 text_color;
	s_vec4 accent_color;
	f32 corner_radius;
	f32 border_width;
} se_ui_theme;

typedef void (*se_ui_interaction_callback)(se_ui_element_handle ui, void* user_data);

typedef struct se_ui_element{
	se_ui_element_handle parent;
	se_ui_layout layout;
	s_vec2 position;
	s_vec2 size;
	s_vec2 padding;
	se_ui_layout_rules layout_rules;
	se_ui_theme theme;
	i32 z_order;
	se_ui_widget_type widget_type;
	f32 slider_min;
	f32 slider_max;
	f32 slider_value;
	se_texture_handle image_texture;
	se_ui_interaction_callback on_click;
	void* on_click_user_data;
	se_scene_2d_handle scene_2d;
	se_ui_text_handle text;
	b8 clip_enabled : 1;
	b8 interactable : 1;
	b8 consume_input : 1;
	b8 checkbox_value : 1;
	b8 use_custom_theme : 1;
	b8 hovered : 1;
	b8 pressed : 1;
	b8 visible : 1;
	s_array(se_ui_element_handle, children);
} se_ui_element;
typedef s_array(se_ui_element, se_ui_elements);
typedef se_ui_element_handle se_ui_element_ptr;

typedef struct {
	se_ui_layout layout;
	s_vec2 position;
	s_vec2 size;
	s_vec2 padding;
	b8 visible : 1;
} se_ui_element_params;

typedef struct se_ui_text {
	s_array(c8, characters);
	s_array(c8, font_path);
	f32 font_size; 
} se_ui_text;
typedef s_array(se_ui_text, se_ui_texts);
typedef se_ui_text_handle se_ui_text_ptr;

extern void se_ui_element_destroy(const se_ui_element_handle ui);
extern se_ui_element_handle se_ui_element_create(const se_window_handle window, const se_ui_element_params* params);
extern se_ui_element_handle se_ui_element_text_create(const se_window_handle window, const se_ui_element_params* params, const c8* text, const c8* font_path, const f32 font_size);
extern void se_ui_element_render(const se_ui_element_handle ui);
extern void se_ui_element_render_to_screen(const se_ui_element_handle ui, const se_window_handle window);
extern void se_ui_element_rebuild_tree(const se_ui_element_handle ui);
extern void se_ui_element_set_position(const se_ui_element_handle ui, const s_vec2* position);
extern void se_ui_element_set_size(const se_ui_element_handle ui, const s_vec2* size);
extern void se_ui_element_set_layout(const se_ui_element_handle ui, const se_ui_layout layout);
extern void se_ui_element_set_visible(const se_ui_element_handle ui, const b8 visible);
extern void se_ui_element_set_layout_rules(const se_ui_element_handle ui, const se_ui_layout_rules* rules);
extern void se_ui_element_set_theme(const se_ui_element_handle ui, const se_ui_theme* theme);
extern void se_ui_element_set_z_order(const se_ui_element_handle ui, const i32 z_order);
extern i32 se_ui_element_get_z_order(const se_ui_element_handle ui);
extern void se_ui_element_set_clipping(const se_ui_element_handle ui, const b8 enabled);
extern void se_ui_element_set_interaction(const se_ui_element_handle ui, const b8 interactable, const b8 consume_input, se_ui_interaction_callback on_click, void* user_data);
extern b8 se_ui_element_hit_test(const se_ui_element_handle root, const s_vec2* point_ndc, se_ui_element_handle* out_ui);
extern b8 se_ui_element_dispatch_pointer(const se_ui_element_handle root, const s_vec2* point_ndc, const b8 pressed, const b8 released);
extern void se_ui_element_set_text(const se_ui_element_handle ui, const c8* text, const c8* font_path, const f32 font_size);
extern void se_ui_element_update_objects(const se_ui_element_handle ui);
extern void se_ui_element_update_children(const se_ui_element_handle ui);
extern se_object_2d_handle se_ui_element_add_object(const se_ui_element_handle ui, const c8* fragment_shader_path);
extern se_object_2d_handle se_ui_element_add_text(const se_ui_element_handle ui, const c8* text, const c8* font_path, const f32 font_size);
extern void se_ui_element_remove_object(const se_ui_element_handle ui, const se_object_2d_handle object);
extern se_ui_element_handle se_ui_element_add_child(const se_window_handle window, const se_ui_element_handle parent_ui, const se_ui_element_params* params);
extern void se_ui_element_detach_child(const se_ui_element_handle parent_ui, const se_ui_element_handle child);

extern void se_ui_set_theme(const se_ui_theme* theme);
extern void se_ui_get_theme(se_ui_theme* out_theme);
extern void se_ui_begin_batch(void);
extern void se_ui_end_batch(void);

extern se_ui_element_handle se_ui_panel_create(const se_window_handle window, const se_ui_element_params* params);
extern se_ui_element_handle se_ui_label_create(const se_window_handle window, const se_ui_element_params* params, const c8* text, const c8* font_path, const f32 font_size);
extern se_ui_element_handle se_ui_button_create(const se_window_handle window, const se_ui_element_params* params, const c8* text, const c8* font_path, const f32 font_size, se_ui_interaction_callback on_click, void* user_data);
extern se_ui_element_handle se_ui_slider_create(const se_window_handle window, const se_ui_element_params* params, const f32 min_value, const f32 max_value, const f32 value);
extern b8 se_ui_slider_set_value(const se_ui_element_handle ui, const f32 value);
extern f32 se_ui_slider_get_value(const se_ui_element_handle ui);
extern se_ui_element_handle se_ui_checkbox_create(const se_window_handle window, const se_ui_element_params* params, const b8 checked);
extern b8 se_ui_checkbox_set_checked(const se_ui_element_handle ui, const b8 checked);
extern b8 se_ui_checkbox_is_checked(const se_ui_element_handle ui);
extern se_ui_element_handle se_ui_image_create(const se_window_handle window, const se_ui_element_params* params, const se_texture_handle texture);

extern b8 se_ui_world_to_window(const se_camera_handle camera, const se_window_handle window, const s_vec3* world, s_vec2* out_window_pixel);
extern b8 se_ui_world_to_ndc(const se_camera_handle camera, const s_vec3* world, s_vec2* out_ndc);
extern b8 se_ui_minimap_world_to_ui(const se_box_2d* world_rect, const se_box_2d* ui_rect, const s_vec2* world_point, s_vec2* out_ui_point);
extern b8 se_ui_minimap_ui_to_world(const se_box_2d* world_rect, const se_box_2d* ui_rect, const s_vec2* ui_point, s_vec2* out_world_point);

extern void se_ui_set_debug_overlay(const b8 enabled);
extern b8 se_ui_is_debug_overlay_enabled(void);
extern void se_ui_render_debug_overlay(const se_ui_element_handle root, const se_window_handle window);

#endif // SE_UI_H
