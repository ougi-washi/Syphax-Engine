// Syphax-Engine - Ougi Washi

#include "se_ui.h"

se_ui* se_ui_create(const se_ui_create_params* params) {
    s_assertf(params, "se_ui_create :: params is null");
    s_assertf(params->render_handle, "se_ui_create :: render_handle is null");
    s_assertf(params->children_count > 0, "se_ui_create :: children_count is 0");
    s_assertf(params->objects_count > 0, "se_ui_create :: objects_count is 0");
    s_assertf(params->fonts_count > 0, "se_ui_create :: fonts_count is 0");
    
    se_ui* new_ui = malloc(sizeof(se_ui));
    memset(new_ui, 0, sizeof(se_ui));
    new_ui->visible = params->visible;
    new_ui->layout = params->layout;
    new_ui->render_handle = params->render_handle;
    new_ui->window = params->window;
    new_ui->position = params->position;
    new_ui->size = params->size;
    new_ui->padding = params->padding;

    se_scene_handle_params scene_params = {0};
    scene_params.objects_2d_count = params->objects_count;
    scene_params.objects_3d_count = 0;
    scene_params.scenes_2d_count = 1; // TODO: maybe make this configurable
    scene_params.scenes_3d_count = 0;
    new_ui->scene_handle = se_scene_handle_create(params->render_handle, &scene_params);
    
    new_ui->scene_2d = se_scene_2d_create(new_ui->scene_handle, &se_vec2(1920, 1080), params->objects_count); // TODO: maybe expose size
    se_scene_2d_set_auto_resize(new_ui->scene_2d, params->window, &se_vec2(1., 1.)); // TODO: maybe expose option to enable/disable
    new_ui->text_handle = se_text_handle_create(params->render_handle, params->fonts_count);
    s_array_init(&new_ui->children, params->children_count);
    
    return new_ui;
}

void se_ui_render(se_ui* ui) {
    s_assertf(ui, "se_ui_render :: ui is null");
    s_assertf(ui->scene_2d, "se_ui_render :: scene_2d is null");

    if (!ui->visible) {
        return;
    }

    s_foreach(&ui->children, i) {
        se_ui_ptr* current_ui_ptr = s_array_get(&ui->children, i);
        if (!current_ui_ptr) {
            printf("se_ui_render :: children index %zu is null\n", i);
            continue;
        }
        se_ui* current_ui = *current_ui_ptr;
        if (!current_ui) {
            printf("se_ui_render :: children index %zu is null\n", i);
            continue;
        }
        se_ui_render(current_ui);
    }

    se_scene_2d_render(ui->scene_2d, ui->render_handle);
}

void se_ui_render_to_screen(se_ui* ui) {
    s_assertf(ui, "se_ui_render_to_screen :: ui is null");
    s_assertf(ui->scene_2d, "se_ui_render_to_screen :: scene_2d is null");
    s_assertf(ui->render_handle, "se_ui_render_to_screen :: render_handle is null");
    s_assertf(ui->window, "se_ui_render_to_screen :: window is null");
    s_foreach(&ui->children, i) {
        se_ui_ptr* current_ui_ptr = s_array_get(&ui->children, i);
        if (!current_ui_ptr) {
            printf("se_ui_render_to_screen :: children index %zu is null\n", i);
            continue;
        }
        se_ui* current_ui = *current_ui_ptr;
        if (!current_ui) {
            printf("se_ui_render_to_screen :: children index %zu is null\n", i);
            continue;
        }
        se_ui_render_to_screen(current_ui);
    }
    se_scene_2d_render_to_screen(ui->scene_2d, ui->render_handle, ui->window);
}

void se_ui_destroy(se_ui* ui) {
    s_assertf(ui, "se_ui_destroy :: ui is null");
    s_assertf(ui->scene_handle, "se_ui_destroy :: scene_handle is null");
    s_assertf(ui->scene_2d, "se_ui_destroy :: scene_2d is null");
    printf("se_ui_destroy :: ui: %p\n", ui);
    s_foreach(&ui->children, i) {
        se_ui_ptr* current_ui_ptr = s_array_get(&ui->children, i);
        if (!current_ui_ptr) {
            printf("se_ui_destroy :: children index %zu is null\n", i);
            continue;
        }
        se_ui* current_ui = *current_ui_ptr;
        if (!current_ui) {
            printf("se_ui_destroy :: children index %zu is null\n", i);
            continue;
        }
        se_ui_destroy(current_ui);
    }
    s_array_clear(&ui->children);

    se_scene_2d_destroy(ui->scene_handle, ui->scene_2d);
    se_scene_handle_cleanup(ui->scene_handle);
    se_text_handle_cleanup(ui->text_handle);
    free(ui);
    ui = NULL;
}

void se_ui_set_position(se_ui* ui, const se_vec2* position) {
    s_assertf(ui, "se_ui_set_position :: ui is null");
    s_assertf(position, "se_ui_set_position :: position is null");
    ui->position = *position;
    se_ui_update_objects(ui);
}

void se_ui_set_size(se_ui* ui, const se_vec2* size) {
    s_assertf(ui, "se_ui_set_size :: ui is null");
    s_assertf(size, "se_ui_set_size :: size is null");
    ui->size = *size;
    se_ui_update_objects(ui);
}

void se_ui_set_layout(se_ui* ui, const se_ui_layout layout) {
    s_assertf(ui, "se_ui_set_layout :: ui is null");
    ui->layout = layout;
    se_ui_update_objects(ui);
}

void se_ui_set_visible(se_ui* ui, const b8 visible) {
    s_assertf(ui, "se_ui_set_visible :: ui is null");
    ui->visible = visible;
}

void se_ui_update_objects(se_ui* ui) {
    s_assertf(ui, "se_ui_update_objects :: ui is null");
    se_scene_2d* scene_2d = ui->scene_2d;
    s_assertf(scene_2d, "se_ui_update_objects :: scene_2d is null");

    sz object_count = s_array_get_size(&scene_2d->objects);

    se_vec2 scale = {1, 1};
    se_vec2 position = {0, 0};
    
    if (object_count > 0) {
        if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
            scale.x = 1. / (object_count);
            scale.y *= ui->size.y;
        }
        else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
            scale.y = 1. / (object_count);
            scale.x *= ui->size.x;
        }
    }
  
    position = se_vec2(scale.x - 1, scale.y - 1);
    position.x += ui->position.x;
    position.y += ui->position.y;
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
        se_object_2d_set_scale(current_object_2d, &se_vec2(scale.x - ui->padding.x, scale.y - ui->padding.y));
        if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
            position.x += scale.x * 2;
        }
        else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
            position.y += scale.y * 2;
        }
    }
};

void se_ui_update_children(se_ui* ui) {
    s_assertf(ui, "se_ui_update_children :: ui is null");
    u32 child_count = s_array_get_size(&ui->children);
    se_vec2 scale = {1, 1};
    se_vec2 position = {0, 0};

    if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
        scale.x = 1. / (child_count);
        position.x = -scale.x * (child_count);
    }
    else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
        scale.y = 1. / (child_count);
        position.y = -scale.y * (child_count);
    }

    for (u32 i = 0; i < child_count; i++) {
        se_ui_ptr* current_ui_ptr = s_array_get(&ui->children, i);
        if (!current_ui_ptr) {
            printf("se_ui_update_children :: ui %p, children index %d is null\n", ui, i);
            continue;
        }
        se_ui* current_ui = *current_ui_ptr;
        if (!current_ui) {
            printf("se_ui_update_children :: ui %p children index %d is null\n", ui, i);
            continue;
        }
        if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
            position.x += scale.x * 2;
        }
        else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
            position.y += scale.y * 2;
        }
        se_ui_set_position(current_ui, &se_vec2(position.x + ui->position.x, position.y + ui->position.y));
        se_ui_set_size(current_ui, &se_vec2(scale.x * (ui->size.x ), scale.y * (ui->size.y )));
    }
}

se_object_2d_ptr se_ui_add_object(se_ui* ui, const c8* fragment_shader_path) {
    s_assertf(ui, "se_ui_add_object :: ui is null");
    s_assertf(fragment_shader_path, "se_ui_add_object :: fragment_shader_path is null");

    se_scene_2d* scene_2d = ui->scene_2d;
    s_assertf(scene_2d, "se_ui_add_object :: scene_2d is null");

    se_render_handle* render_handle = ui->render_handle;    
    s_assertf(render_handle, "se_ui_add_object :: render_handle is null");

    printf("se_ui_add_object :: ui: %p, fragment_shader_path: %s\n", ui, fragment_shader_path);

    se_vec2 scale = {1, 1};
    se_vec2 position = {0, 0};
    se_object_2d_ptr object_2d = se_object_2d_create(ui->scene_handle, fragment_shader_path, &position, &scale, 0);
    se_scene_2d_add_object(ui->scene_2d, object_2d);
    se_ui_update_objects(ui);
    return object_2d;
}

void se_ui_remove_object(se_ui* ui, se_object_2d* object) {
    s_assertf(ui, "se_ui_remove_object :: ui is null");
    s_assertf(object, "se_ui_remove_object :: object is null");
    se_scene_2d_remove_object(ui->scene_2d, object);
}

se_ui* se_ui_add_child(se_ui* parent_ui, const se_ui_create_params* params) {
    s_assertf(parent_ui, "se_ui_add_child :: parent_ui is null");
    s_assertf(params, "se_ui_add_child :: params is null");
    // TODO: maybe we need to force the child to have the sane window and render handle as the parent
    
    se_ui* new_ui = se_ui_create(params);
    s_array_add(&parent_ui->children, new_ui);
    se_ui_update_children(parent_ui);
    return new_ui;
}

