// Syphax-Engine - Ougi Washi

#include "se_ui.h"

se_ui_handle *se_ui_handle_create(se_window *window, se_render_handle *render_handle, const se_ui_handle_params *params) {
  s_assertf(window, "se_ui_handle_create :: window is null");
  s_assertf(render_handle, "se_ui_handle_create :: render_handle is null");
  s_assertf(params, "se_ui_handle_create :: params is null");

  se_ui_handle *ui_handle = malloc(sizeof(se_ui_handle));
  memset(ui_handle, 0, sizeof(se_ui_handle));
  ui_handle->window = window;
  ui_handle->render_handle = render_handle;
  s_array_init(&ui_handle->ui_elements, params->elements_count);
  ui_handle->objects_per_element_count = params->objects_per_element_count;
  ui_handle->children_per_element_count = params->children_per_element_count;

  se_scene_handle_params scene_params = {0};
  scene_params.objects_2d_count = params->elements_count * params->objects_per_element_count + 1;
  scene_params.objects_3d_count = 0;
  scene_params.scenes_2d_count = params->elements_count * params->children_per_element_count + 1;
  scene_params.scenes_3d_count = 0;
  ui_handle->scene_handle = se_scene_handle_create(render_handle, &scene_params);

  if (params->fonts_count > 0) {
    ui_handle->text_handle = se_text_handle_create(render_handle, params->fonts_count);
    s_array_init(&ui_handle->ui_texts, params->texts_count);
  }
  printf("se_ui_handle_create :: created ui handle %p\n", ui_handle);
  return ui_handle;
}

void se_ui_handle_cleanup(se_ui_handle *ui_handle) {
  s_assertf(ui_handle, "se_ui_handle_cleanup :: ui_handle is null");
  printf("se_ui_handle_cleanup :: ui_handle: %p\n", ui_handle);
  s_foreach(&ui_handle->ui_elements, i) {
    se_ui_element *current_ui = s_array_get(&ui_handle->ui_elements, i);
    se_ui_element_destroy(current_ui);
  }
  s_array_clear(&ui_handle->ui_elements);
  se_scene_handle_cleanup(ui_handle->scene_handle);
  if (ui_handle->text_handle) { // Text handle is optional
    se_text_handle_cleanup(ui_handle->text_handle);
  }
  s_array_clear(&ui_handle->ui_texts);
  free(ui_handle);
  ui_handle = NULL;
  printf("se_ui_handle_cleanup :: ui_handle cleanup done\n");
}

se_ui_element *se_ui_element_create(se_ui_handle *ui_handle, const se_ui_element_params *params) {
  s_assertf(ui_handle, "se_ui_element_create :: ui_handle is null");
  s_assertf(params, "se_ui_element_create :: params is null");

  se_ui_element *new_ui = s_array_increment(&ui_handle->ui_elements);
  new_ui->ui_handle = ui_handle;
  new_ui->visible = params->visible;
  new_ui->layout = params->layout;
  new_ui->position = params->position;
  new_ui->size = params->size;
  new_ui->padding = params->padding;

  new_ui->scene_2d = se_scene_2d_create(ui_handle->scene_handle, &se_vec2(1920, 1080), ui_handle->objects_per_element_count); // TODO: maybe expose size
  se_scene_2d_set_auto_resize(new_ui->scene_2d, ui_handle->window, &se_vec2(1., 1.)); // TODO: maybe expose option to enable/disable
  s_array_init(&new_ui->children, ui_handle->children_per_element_count);

  return new_ui;
}

se_ui_element *se_ui_element_text_create(se_ui_handle *ui_handle, const se_ui_element_params *params, const c8 *text, const c8 *font_path, const f32 font_size) {
  s_assertf(ui_handle, "se_ui_element_text_create :: ui_handle is null");
  s_assertf(params, "se_ui_element_text_create :: params is null");
  s_assertf(text, "se_ui_element_text_create :: text is null");
  s_assertf(font_path, "se_ui_element_text_create :: font_path is null");

  se_ui_element *new_ui = se_ui_element_create(ui_handle, params);
  se_ui_element_set_text(new_ui, text, font_path, font_size);
  return new_ui;
}

void se_ui_element_render(se_ui_element *ui) {
  s_assertf(ui, "se_ui_element_render :: ui is null");
  s_assertf(ui->ui_handle, "se_ui_element_render :: ui_handle is null");
  s_assertf(ui->scene_2d, "se_ui_element_render :: scene_2d is null");

  se_ui_handle *ui_handle = ui->ui_handle;
  s_assertf(ui_handle, "se_ui_element_render :: ui_handle is null");

  if (!ui->visible) {
    return;
  }

  s_foreach(&ui->children, i) {
    se_ui_element_ptr *current_ui_ptr = s_array_get(&ui->children, i);
    if (!current_ui_ptr) {
      printf("se_ui_element_render :: children index %zu is null\n", i);
      continue;
    }
    se_ui_element *current_ui = *current_ui_ptr;
    if (!current_ui) {
      printf("se_ui_element_render :: children index %zu is null\n", i);
      continue;
    }
    se_ui_element_render(current_ui);
  }

  se_scene_2d_bind(ui->scene_2d);
  se_scene_2d_render_raw(ui->scene_2d, ui->ui_handle->render_handle);
  if (ui->text) {
    se_font *curr_font = se_font_load(ui_handle->text_handle, ui->text->font_path, ui->text->font_size); // TODO: store font instead of path
    // TODO: add/store parameters, add allignment
    se_text_render(ui_handle->text_handle, curr_font, ui->text->characters, &se_vec2(ui->position.x + ui->padding.x, ui->position.y - ui->padding.y), .1, .08f); 
  }
  se_scene_2d_unbind(ui->scene_2d);
}

void se_ui_element_render_to_screen(se_ui_element *ui) {
  s_assertf(ui, "se_ui_element_render_to_screen :: ui is null");
  s_assertf(ui->scene_2d, "se_ui_element_render_to_screen :: scene_2d is null");
  se_ui_handle *ui_handle = ui->ui_handle;
  s_assertf(ui_handle, "se_ui_element_render_to_screen :: ui_handle is null");
  s_assertf(ui_handle->render_handle, "se_ui_element_render_to_screen :: render_handle is null");
  s_assertf(ui_handle->window, "se_ui_element_render_to_screen :: window is null");

  s_foreach(&ui->children, i) {
    se_ui_element_ptr *current_ui_ptr = s_array_get(&ui->children, i);
    if (!current_ui_ptr) {
      printf("se_ui_element_render_to_screen :: children index %zu is null\n", i);
      continue;
    }
    se_ui_element *current_ui = *current_ui_ptr;
    if (!current_ui) {
      printf("se_ui_element_render_to_screen :: children index %zu is null\n", i);
      continue;
    }
    se_ui_element_render_to_screen(current_ui);
  }
  se_scene_2d_render_to_screen(ui->scene_2d, ui_handle->render_handle,
                               ui_handle->window);
}

void se_ui_element_destroy(se_ui_element *ui) {
  s_assertf(ui, "se_ui_element_destroy :: ui is null");
  s_assertf(ui->scene_2d, "se_ui_element_destroy :: scene_2d is null");
  se_ui_handle *ui_handle = ui->ui_handle;
  s_assertf(ui_handle, "se_ui_element_destroy :: ui_handle is null");
  printf("se_ui_element_destroy :: ui: %p, ui_handle: %p\n", ui, ui_handle);
  s_foreach(&ui->children, i) {
    se_ui_element_ptr *current_ui_ptr = s_array_get(&ui->children, i);
    if (!current_ui_ptr) {
      printf("se_ui_element_destroy :: children index %zu is null\n", i);
      continue;
    }
    se_ui_element *current_ui = *current_ui_ptr;
    if (!current_ui) {
      printf("se_ui_element_destroy :: children index %zu is null\n", i);
      continue;
    }
    se_ui_element_destroy(current_ui);
  }
  s_array_clear(&ui->children);
  se_scene_2d_destroy(ui_handle->scene_handle, ui->scene_2d);
  s_array_remove(&ui_handle->ui_elements, ui);
}

void se_ui_element_set_position(se_ui_element *ui, const se_vec2 *position) {
  s_assertf(ui, "se_ui_element_set_position :: ui is null");
  s_assertf(position, "se_ui_element_set_position :: position is null");
  ui->position = *position;
  se_ui_element_update_objects(ui);
}

void se_ui_element_set_size(se_ui_element *ui, const se_vec2 *size) {
  s_assertf(ui, "se_ui_element_set_size :: ui is null");
  s_assertf(size, "se_ui_element_set_size :: size is null");
  ui->size = *size;
  se_ui_element_update_objects(ui);
}

void se_ui_element_set_layout(se_ui_element *ui, const se_ui_layout layout) {
  s_assertf(ui, "se_ui_set_layout :: ui is null");
  ui->layout = layout;
  se_ui_element_update_objects(ui);
}

void se_ui_element_set_visible(se_ui_element *ui, const b8 visible) {
  s_assertf(ui, "se_ui_element_set_visible :: ui is null");
  ui->visible = visible;
}

void se_ui_element_set_text(se_ui_element *ui, const c8 *text, const c8 *font_path, const f32 font_size) {
  s_assertf(ui, "se_ui_element_set_text :: ui is null");
  s_assertf(text, "se_ui_element_set_text :: text is null");
  s_assertf(font_path, "se_ui_element_set_text :: font_path is null");
  se_ui_handle *ui_handle = ui->ui_handle;
  s_assertf(ui_handle, "se_ui_element_set_text :: ui_handle is null");

  if (strlen(text) > SE_MAX_UI_TEXT_LENGTH) {
    printf("se_ui_element_set_text :: text is too long, max length is %d\n", SE_MAX_UI_TEXT_LENGTH);
    return;
  }

  if (!ui->text) {
    if (s_array_get_capacity(&ui_handle->ui_texts) <= s_array_get_size(&ui_handle->ui_texts)) {
      printf("se_ui_element_set_text :: error, ui_handle->ui_texts is full, allocate more space\n");
      return;
    }
    ui->text = s_array_increment(&ui_handle->ui_texts);
  }

  strcpy(ui->text->characters, text);
  strcpy(ui->text->font_path, font_path);
  ui->text->font_size = font_size;
}

void se_ui_element_update_objects(se_ui_element *ui) {
  s_assertf(ui, "se_ui_element_update_objects :: ui is null");
  se_scene_2d *scene_2d = ui->scene_2d;
  s_assertf(scene_2d, "se_ui_element_update_objects :: scene_2d is null");

  sz object_count = s_array_get_size(&scene_2d->objects);

  se_vec2 scale = {1, 1};
  se_vec2 position = {0, 0};

  if (object_count > 0) {
    if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
      scale.x = 1. / (object_count);
      scale.y *= ui->size.y;
    } else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
      scale.y = 1. / (object_count);
      scale.x *= ui->size.x;
    }
  }

  position = se_vec2(scale.x - 1, scale.y - 1);
  position.x += ui->position.x;
  position.y += ui->position.y;
  s_foreach(&scene_2d->objects, i) {
    se_object_2d_ptr *current_object_2d_ptr = s_array_get(&scene_2d->objects, i);
    if (!current_object_2d_ptr) {
      printf("se_ui_element_add_object :: one of the previous objects is null\n");
      continue;
    }
    se_object_2d *current_object_2d = *current_object_2d_ptr;
    if (!current_object_2d) {
      printf("se_ui_element_add_object :: one of the previous objects is null\n");
      continue;
    }
    se_object_2d_set_position(current_object_2d, &position);
    se_object_2d_set_scale(current_object_2d, &se_vec2(scale.x - ui->padding.x, scale.y - ui->padding.y));
    if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
      position.x += scale.x * 2;
    } else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
      position.y += scale.y * 2;
    }
  }
};

void se_ui_element_update_children(se_ui_element *ui) {
  s_assertf(ui, "se_ui_element_update_children :: ui is null");
  u32 child_count = s_array_get_size(&ui->children);
  se_vec2 scale = {1, 1};
  se_vec2 position = {0, 0};

  if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
    scale.x = 1. / (child_count);
    position.x = -scale.x * (child_count);
  } else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
    scale.y = 1. / (child_count);
    position.y = -scale.y * (child_count);
  }

  for (u32 i = 0; i < child_count; i++) {
    se_ui_element_ptr *current_ui_ptr = s_array_get(&ui->children, i);
    if (!current_ui_ptr) {
      printf("se_ui_element_update_children :: ui %p, children index %d is null\n", ui, i);
      continue;
    }
    se_ui_element *current_ui = *current_ui_ptr;
    if (!current_ui) {
      printf("se_ui_element_update_children :: ui %p children index %d is null\n", ui, i);
      continue;
    }
    if (ui->layout == SE_UI_LAYOUT_HORIZONTAL) {
      position.x += scale.x * 2;
    } else if (ui->layout == SE_UI_LAYOUT_VERTICAL) {
      position.y += scale.y * 2;
    }
    se_ui_element_set_position(current_ui, &se_vec2(position.x + ui->position.x, position.y + ui->position.y));
    se_ui_element_set_size(current_ui, &se_vec2(scale.x * (ui->size.x), scale.y * (ui->size.y)));
  }
}

se_object_2d_ptr se_ui_element_add_object(se_ui_element *ui, const c8 *fragment_shader_path) {
  s_assertf(ui, "se_ui_element_add_object :: ui is null");
  s_assertf(fragment_shader_path, "se_ui_element_add_object :: fragment_shader_path is null");

  se_scene_2d *scene_2d = ui->scene_2d;
  s_assertf(scene_2d, "se_ui_element_add_object :: scene_2d is null");

  se_ui_handle *ui_handle = ui->ui_handle;
  s_assertf(ui_handle, "se_ui_element_add_object :: ui_handle is null");

  se_render_handle *render_handle = ui_handle->render_handle;
  s_assertf(render_handle, "se_ui_element_add_object :: render_handle is null");

  se_scene_handle *scene_handle = ui_handle->scene_handle;
  s_assertf(scene_handle, "se_ui_element_add_object :: scene_handle is null");

  printf("se_ui_element_add_object :: ui: %p, fragment_shader_path: %s\n", ui,
         fragment_shader_path);

  se_mat3 transform = mat3_identity();
  se_object_2d_ptr object_2d =
      se_object_2d_create(scene_handle, fragment_shader_path, &transform, 0);
  se_scene_2d_add_object(ui->scene_2d, object_2d);
  se_ui_element_update_objects(ui);
  return object_2d;
}

void se_ui_element_remove_object(se_ui_element *ui, se_object_2d *object) {
  s_assertf(ui, "se_ui_element_remove_object :: ui is null");
  s_assertf(object, "se_ui_element_remove_object :: object is null");
  se_scene_2d_remove_object(ui->scene_2d, object);
}

se_ui_element *se_ui_element_add_child(se_ui_element *parent_ui, const se_ui_element_params *params) {
  s_assertf(parent_ui, "se_ui_element_add_child :: parent_ui is null");
  s_assertf(params, "se_ui_element_add_child :: params is null");
  // TODO: maybe we need to force the child to have the sane window and render
  // handle as the parent

  se_ui_element *new_ui = se_ui_element_create(parent_ui->ui_handle, params);
  s_array_add(&parent_ui->children, new_ui);
  se_ui_element_update_children(parent_ui);
  return new_ui;
}
