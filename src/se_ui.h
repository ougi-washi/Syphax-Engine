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

typedef struct {
    se_ui_layout layout;
    se_vec2 position;
    se_vec2 size;
    se_render_handle* render_handle;
    se_scene_handle* scene_handle;
    se_window* window;
    se_scene_2d* scene_2d;
    se_text_handle* text_handle;

    s_array(struct se_ui, children);
} se_ui;

extern se_ui* se_ui_create(se_render_handle* render_handle, se_window* window, const u32 objects_count, const u32 fonts_count, const se_ui_layout layout);
extern void se_ui_render(se_ui* ui);
extern void se_ui_render_to_screen(se_ui* ui);
extern void se_ui_destroy(se_ui* ui);
extern se_object_2d_ptr se_ui_add_object(se_ui* ui, const c8* fragment_shader_path, const se_vec2* padding);
extern void se_ui_remove_object(se_ui* ui, se_object_2d_ptr object);

#endif // SE_UI_H
