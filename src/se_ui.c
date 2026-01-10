// Syphax-Engine - Ougi Washi

#include "se_ui.h"

se_ui* se_ui_create(se_render_handle* render_handle, se_window* window, const u32 objects_count, const u32 fonts_count, const se_ui_layout layout) {
    s_assertf(render_handle, "se_ui_create :: render_handle is null");
    s_assertf(objects_count > 0, "se_ui_create :: objects_count is 0");
    s_assertf(fonts_count > 0, "se_ui_create :: fonts_count is 0");
    se_ui* new_ui = malloc(sizeof(se_ui));
    memset(new_ui, 0, sizeof(se_ui));
    new_ui->layout = layout;
    new_ui->render_handle = render_handle;
    new_ui->window = window;
    se_scene_handle_params scene_params = {0};
    scene_params.objects_2d_count = objects_count;
    scene_params.objects_3d_count = 0;
    scene_params.scenes_2d_count = 1; // TODO: maybe make this configurable
    scene_params.scenes_3d_count = 0;
    new_ui->scene_handle = se_scene_handle_create(render_handle, &scene_params);
    // TODO: maybe the size should be configurable
    new_ui->scene_2d = se_scene_2d_create(new_ui->scene_handle, &se_vec2(1920, 1080), objects_count);
    se_scene_2d_set_auto_resize(new_ui->scene_2d, window, &se_vec2(1., 1.));
    new_ui->text_handle = se_text_handle_create(render_handle, fonts_count);
    return new_ui;
}

void se_ui_render(se_ui* ui) {
    s_assertf(ui, "se_ui_render :: ui is null");
    s_assertf(ui->scene_2d, "se_ui_render :: scene_2d is null");
    se_scene_2d_render(ui->scene_2d, ui->render_handle);
}

void se_ui_render_to_screen(se_ui* ui) {
    s_assertf(ui, "se_ui_render_to_screen :: ui is null");
    s_assertf(ui->scene_2d, "se_ui_render_to_screen :: scene_2d is null");
    s_assertf(ui->render_handle, "se_ui_render_to_screen :: render_handle is null");
    s_assertf(ui->window, "se_ui_render_to_screen :: window is null");
    se_scene_2d_render_to_screen(ui->scene_2d, ui->render_handle, ui->window);
}

void se_ui_destroy(se_ui* ui) {
    s_assertf(ui, "se_ui_destroy :: ui is null");
    s_assertf(ui->scene_handle, "se_ui_destroy :: scene_handle is null");
    s_assertf(ui->scene_2d, "se_ui_destroy :: scene_2d is null");
    se_scene_2d_destroy(ui->scene_handle, ui->scene_2d);
    se_scene_handle_cleanup(ui->scene_handle);
    se_text_handle_cleanup(ui->text_handle);
    free(ui);
    ui = NULL;
}

se_object_2d_ptr se_ui_add_object(se_ui* ui, const c8* fragment_shader_path, const se_vec2* padding) {
    s_assertf(ui, "se_ui_add_object :: ui is null");
    s_assertf(fragment_shader_path, "se_ui_add_object :: fragment_shader_path is null");
    s_assertf(padding, "se_ui_add_object :: padding is null");

    se_scene_2d* scene_2d = ui->scene_2d;
    s_assertf(scene_2d, "se_ui_add_object :: scene_2d is null");

    se_render_handle* render_handle = ui->render_handle;    
    s_assertf(render_handle, "se_ui_add_object :: render_handle is null");

    printf("se_ui_add_object :: ui: %p, fragment_shader_path: %s\n", ui, fragment_shader_path);

    sz object_count = s_array_get_size(&scene_2d->objects);

    se_vec2 scale = {1, 1};
    se_vec2 position = {0, 0};
    if (object_count > 0) {
        if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
            scale.x = 1. / (object_count + 1);
            position.x = -scale.x;
        }
        else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
            scale.y = 1. / (object_count + 1);
            position.y = -scale.y;
        }
    }
    
    scale.x -= padding->x;
    scale.y -= padding->y;
    
    s_foreach(&scene_2d->objects, i) {
        se_object_2d_ptr* current_object_2d_ptr = s_array_get(&scene_2d->objects, i);
        if (!current_object_2d_ptr) {
            printf("se_ui_add_object :: one of the previous objects is null\n");
            continue;
        }
        se_object_2d* current_object_2d = *current_object_2d_ptr;
        if (!current_object_2d) {
            printf("se_ui_add_object :: one of the previous objects is null\n");
            continue;
        }
        se_object_2d_set_position(current_object_2d, &position);
        se_object_2d_set_scale(current_object_2d, &scale);
        if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
            position.x += scale.x * (i + 1) * 2.;
        }
        else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
            position.y += scale.y * (i + 1) * 2.;
        }
    }
   
    se_object_2d_ptr object_2d = se_object_2d_create(ui->scene_handle, fragment_shader_path, &position, &scale, 0);
    se_scene_2d_add_object(ui->scene_2d, object_2d);
    return object_2d;
}

void se_ui_remove_object(se_ui* ui, se_object_2d* object) {
    s_assertf(ui, "se_ui_remove_object :: ui is null");
    s_assertf(object, "se_ui_remove_object :: object is null");
    se_scene_2d_remove_object(ui->scene_2d, object);
}

