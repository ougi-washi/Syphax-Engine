// Syphax-Engine - Ougi Washi

#include "se_ui.h"

se_ui* se_ui_create(se_render_handle* render_handle, const u32 objects_count, const u32 fonts_count) {
    s_assertf(render_handle, "se_ui_create :: render_handle is null");
    s_assertf(objects_count > 0, "se_ui_create :: objects_count is 0");
    s_assertf(fonts_count > 0, "se_ui_create :: fonts_count is 0");
    se_ui* new_ui = malloc(sizeof(se_ui));
    memset(new_ui, 0, sizeof(se_ui));
    new_ui->render_handle = render_handle;
    se_scene_handle_params scene_params = {0};
    scene_params.objects_2d_count = objects_count;
    scene_params.objects_3d_count = 0;
    scene_params.scenes_2d_count = 1; // TODO: maybe make this configurable
    scene_params.scenes_3d_count = 0;
    new_ui->scene_handle = se_scene_handle_create(render_handle, &scene_params);
    // TODO: maybe the size should be configurable
    new_ui->scene_2d = se_scene_2d_create(new_ui->scene_handle, &se_vec2(1920, 1080), objects_count);
    new_ui->text_handle = se_text_handle_create(render_handle, fonts_count);
    s_array_init(&new_ui->objects, objects_count);
    return new_ui;
}

void se_ui_render(se_ui* ui, se_render_handle* render_handle) {
    s_assertf(ui, "se_ui_render :: ui is null");
    s_assertf(ui->scene_2d, "se_ui_render :: scene_2d is null");
    s_assertf(render_handle, "se_ui_render :: render_handle is null");
    se_scene_2d_render(ui->scene_2d, render_handle);
}

void se_ui_destroy(se_ui* ui) {
    s_assertf(ui, "se_ui_destroy :: ui is null");
    s_assertf(ui->scene_handle, "se_ui_destroy :: scene_handle is null");
    s_assertf(ui->scene_2d, "se_ui_destroy :: scene_2d is null");
    se_scene_handle_cleanup(ui->scene_handle);
    se_text_handle_cleanup(ui->text_handle);
    free(ui);
    ui = NULL;
}

se_ui_object* se_ui_add_object(se_ui* ui, const se_ui_object_params* params) {
    s_assertf(ui, "se_ui_add_object :: ui is null");
    s_assertf(params, "se_ui_add_object :: params is null");
    s_assertf(ui->scene_2d, "se_ui_add_object :: scene_2d is null");
    s_assertf(ui->render_handle, "se_ui_add_object :: render_handle is null");
    se_ui_object* new_object = s_array_increment(&ui->objects);
    // TODO shader
    se_object_2d* object_2d = se_object_2d_create(ui->scene_handle, "", &se_vec2(0, 0), &params->size, 0);
    
    return new_object;
}
void se_ui_remove_object(se_ui* ui, se_ui_object* object);
