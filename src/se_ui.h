// Syphax-Engine - Ougi Washi

#ifndef SE_UI_H
#define SE_UI_H

#include "se_scene.h"
#include "se_text.h"

typedef enum {
    SE_UI_LAYOUT_HORIZONTAL,
    SE_UI_LAYOUT_VERTICAL
} se_ui_layout;

typedef struct {
    se_ui_layout layout;
    se_vec2 size;
    se_vec2 spacing;
    se_vec2 padding;
    b8 visible;
} se_ui_object_params;
#define SE_UI_OBJECT_PARAMS_DEFAULT { .layout = SE_UI_LAYOUT_HORIZONTAL, .size = se_vec2(1., 1.), .spacing = se_vec2(0., 0.), .padding = se_vec2(0., 0.), .visible = true }

typedef struct {
    se_ui_layout layout;
    se_render_object* render_object;
    b8 visible;
} se_ui_object;
typedef s_array(se_ui_object, se_ui_objects);

typedef struct {
    se_render_handle* render_handle;
    se_scene_handle* scene_handle;
    se_scene_2d* scene_2d;
    se_text_handle* text_handle;
    se_ui_objects objects;
} se_ui;

extern se_ui* se_ui_create(se_render_handle* render_handle, const u32 objects_count, const u32 fonts_count);
extern void se_ui_render(se_ui* ui, se_render_handle* render_handle);
extern void se_ui_destroy(se_ui* ui);
extern se_ui_object* se_ui_add_object(se_ui* ui, const se_ui_object_params* params);
extern void se_ui_remove_object(se_ui* ui, se_ui_object* object);

#endif // SE_UI_H
