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
    se_ui_layout layout;
    se_vec2 position;
    se_vec2 size;
    se_vec2 padding;
    se_render_handle* render_handle;
    se_scene_handle* scene_handle;
    se_window* window;
    se_scene_2d* scene_2d;
    se_text_handle* text_handle;
    b8 visible : 1;
    s_array(struct se_ui*, children);
} se_ui;
typedef se_ui* se_ui_ptr;

typedef struct {
    se_render_handle* render_handle;
    se_window* window;
    u32 children_count;
    u32 objects_count;
    u32 fonts_count;
    se_ui_layout layout;
    se_vec2 position;
    se_vec2 size;
    se_vec2 padding;
    b8 visible : 1;
} se_ui_create_params;
#define SE_UI_CREATE_PARAMS_DEFAULTS { .render_handle = NULL, .window = NULL, .children_count = 8, .objects_count = 8, .fonts_count = 1, .layout = SE_UI_LAYOUT_HORIZONTAL, .position = se_vec2(0, 0), .size = se_vec2(1, 1), .padding = se_vec2(.05, .05), .visible = 1 }

extern se_ui* se_ui_create(const se_ui_create_params* params);
extern void se_ui_render(se_ui* ui);
extern void se_ui_render_to_screen(se_ui* ui);
extern void se_ui_destroy(se_ui* ui);
extern void se_ui_set_position(se_ui* ui, const se_vec2* position);
extern void se_ui_set_size(se_ui* ui, const se_vec2* size);
extern void se_ui_set_layout(se_ui* ui, const se_ui_layout layout);
extern void se_ui_set_visible(se_ui* ui, const b8 visible);
extern void se_ui_update_objects(se_ui* ui);
extern void se_ui_update_children(se_ui* ui);
extern se_object_2d_ptr se_ui_add_object(se_ui* ui, const c8* fragment_shader_path);
extern void se_ui_remove_object(se_ui* ui, se_object_2d_ptr object);
extern se_ui* se_ui_add_child(se_ui* parent_ui, const se_ui_create_params* params);

#endif // SE_UI_H
