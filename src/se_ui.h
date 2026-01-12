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

typedef struct se_ui{
    struct se_ui_handle* ui_handle;
    se_ui_layout layout;
    se_vec2 position;
    se_vec2 size;
    se_vec2 padding;
    se_scene_2d* scene_2d;
    b8 visible : 1;
    s_array(struct se_ui*, children);
} se_ui_element;
typedef s_array(se_ui_element, se_ui_elements);
typedef se_ui_element* se_ui_element_ptr;

typedef struct {
    se_ui_layout layout;
    se_vec2 position;
    se_vec2 size;
    se_vec2 padding;
    b8 visible : 1;
} se_ui_element_params;
#define SE_UI_ELEMENT_PARAMS_DEFAULTS { .layout = SE_UI_LAYOUT_HORIZONTAL, .position = se_vec2(0, 0), .size = se_vec2(1, 1), .padding = se_vec2(.05, .05), .visible = 1 }

typedef struct se_ui_handle {
    se_window* window;
    se_render_handle* render_handle;
    se_scene_handle* scene_handle;
    se_text_handle* text_handle;
    se_ui_elements ui_elements;

    u16 objects_per_element_count;
    u16 children_per_element_count;
} se_ui_handle;

typedef struct {
    u16 elements_count;
    u16 children_per_element_count;
    u16 objects_per_element_count;
    u16 fonts_count;
} se_ui_handle_params;
#define SE_UI_HANDLE_PARAMS_DEFAULTS { .elements_count = 8, .children_per_element_count = 8, .objects_per_element_count = 16, .fonts_count = 0 }

extern se_ui_handle* se_ui_handle_create(se_window* window, se_render_handle* render_handle, const se_ui_handle_params* params);
extern void se_ui_handle_cleanup(se_ui_handle* ui_handle);

extern se_ui_element* se_ui_element_create(se_ui_handle* ui_handle, const se_ui_element_params* params);
extern void se_ui_element_render(se_ui_element* ui);
extern void se_ui_element_render_to_screen(se_ui_element* ui);
extern void se_ui_element_destroy(se_ui_element* ui);
extern void se_ui_element_set_position(se_ui_element* ui, const se_vec2* position);
extern void se_ui_element_set_size(se_ui_element* ui, const se_vec2* size);
extern void se_ui_element_set_layout(se_ui_element* ui, const se_ui_layout layout);
extern void se_ui_element_set_visible(se_ui_element* ui, const b8 visible);
extern void se_ui_element_update_objects(se_ui_element* ui);
extern void se_ui_element_update_children(se_ui_element* ui);
extern se_object_2d_ptr se_ui_element_add_object(se_ui_element* ui, const c8* fragment_shader_path);
extern void se_ui_element_remove_object(se_ui_element* ui, se_object_2d_ptr object);
extern se_ui_element* se_ui_element_add_child(se_ui_element* parent_ui, const se_ui_element_params* params);

#endif // SE_UI_H
