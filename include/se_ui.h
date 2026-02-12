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

typedef struct se_ui_element{
	se_ui_element_handle parent;
	se_ui_layout layout;
	s_vec2 position;
	s_vec2 size;
	s_vec2 padding;
	se_scene_2d_handle scene_2d;
	se_ui_text_handle text;
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
	c8 characters[SE_TEXT_CHAR_COUNT];
	c8 font_path[SE_MAX_PATH_LENGTH];
	f32 font_size; 
} se_ui_text;
typedef s_array(se_ui_text, se_ui_texts);
typedef se_ui_text_handle se_ui_text_ptr;

extern void se_ui_element_destroy(const se_ui_element_handle ui);
extern se_ui_element_handle se_ui_element_create(const se_window_handle window, const se_ui_element_params* params);
extern se_ui_element_handle se_ui_element_text_create(const se_window_handle window, const se_ui_element_params* params, const c8* text, const c8* font_path, const f32 font_size);
extern void se_ui_element_render(const se_ui_element_handle ui);
extern void se_ui_element_render_to_screen(const se_ui_element_handle ui, const se_window_handle window);
extern void se_ui_element_set_position(const se_ui_element_handle ui, const s_vec2* position);
extern void se_ui_element_set_size(const se_ui_element_handle ui, const s_vec2* size);
extern void se_ui_element_set_layout(const se_ui_element_handle ui, const se_ui_layout layout);
extern void se_ui_element_set_visible(const se_ui_element_handle ui, const b8 visible);
extern void se_ui_element_set_text(const se_ui_element_handle ui, const c8* text, const c8* font_path, const f32 font_size);
extern void se_ui_element_update_objects(const se_ui_element_handle ui);
extern void se_ui_element_update_children(const se_ui_element_handle ui);
extern se_object_2d_handle se_ui_element_add_object(const se_ui_element_handle ui, const c8* fragment_shader_path);
extern se_object_2d_handle se_ui_element_add_text(const se_ui_element_handle ui, const c8* text, const c8* font_path, const f32 font_size);
extern void se_ui_element_remove_object(const se_ui_element_handle ui, const se_object_2d_handle object);
extern se_ui_element_handle se_ui_element_add_child(const se_window_handle window, const se_ui_element_handle parent_ui, const se_ui_element_params* params);
extern void se_ui_element_detach_child(const se_ui_element_handle parent_ui, const se_ui_element_handle child);

#endif // SE_UI_H
