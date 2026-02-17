<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_ui.h

Source header: `include/se_ui.h`

## Overview

Immediate mode style UI tree, widgets, layout, and events.

This page is generated from `include/se_ui.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/ui.md)

## Functions

### `se_ui_add_button_impl`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_add_button_impl(se_ui_widget_handle parent, se_ui_button_args args);
```

</div>

One-call button creation from parent. Usage: se_ui_add_button(parent, { .on_click_fn = ..., .on_click_data = ... });

### `se_ui_add_panel_impl`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_add_panel_impl(se_ui_widget_handle parent, se_ui_panel_args args);
```

</div>

One-call panel creation from parent. Usage: se_ui_add_panel(parent, { .size = s_vec2(...) });

### `se_ui_add_root_impl`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_add_root_impl(se_ui_handle ui, se_ui_panel_args args);
```

</div>

One-call root creation. Usage: se_ui_add_root(ui, { .size = s_vec2(1.0f, 1.0f) });

### `se_ui_add_scrollbox_impl`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_add_scrollbox_impl(se_ui_widget_handle parent, se_ui_scrollbox_args args);
```

</div>

One-call scrollbox creation from parent. Usage: se_ui_add_scrollbox(parent, { .on_scroll_fn = ... });

### `se_ui_add_spacer_impl`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_add_spacer_impl(se_ui_widget_handle parent, se_ui_panel_args args);
```

</div>

One-call spacer creation from parent. Usage: se_ui_add_spacer(parent, { .size = s_vec2(...) });

### `se_ui_add_text_impl`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_add_text_impl(se_ui_widget_handle parent, se_ui_text_args args);
```

</div>

One-call text creation from parent. Usage: se_ui_add_text(parent, { .text = "..." });

### `se_ui_add_textbox_impl`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_add_textbox_impl(se_ui_widget_handle parent, se_ui_textbox_args args);
```

</div>

One-call textbox creation from parent. Usage: se_ui_add_textbox(parent, { .on_change_fn = ... });

### `se_ui_button_add`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_button_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_button_desc* desc);
```

</div>

No inline description found in header comments.

### `se_ui_button_create`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_button_create(se_ui_handle ui, se_ui_widget_handle parent, const c8* text);
```

</div>

Adds a button with defaults and label text.

### `se_ui_clear_focus`

<div class="api-signature">

```c
extern void se_ui_clear_focus(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_create`

<div class="api-signature">

```c
extern se_ui_handle se_ui_create(const se_window_handle window, const u16 widget_capacity);
```

</div>

No inline description found in header comments.

### `se_ui_destroy`

<div class="api-signature">

```c
extern void se_ui_destroy(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_destroy_all`

<div class="api-signature">

```c
extern void se_ui_destroy_all(void);
```

</div>

No inline description found in header comments.

### `se_ui_draw`

<div class="api-signature">

```c
extern void se_ui_draw(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_focus_widget`

<div class="api-signature">

```c
extern b8 se_ui_focus_widget(const se_ui_handle ui, const se_ui_widget_handle widget);
```

</div>

No inline description found in header comments.

### `se_ui_get_default_layout`

<div class="api-signature">

```c
extern void se_ui_get_default_layout(se_ui_layout* out_layout);
```

</div>

No inline description found in header comments.

### `se_ui_get_default_style`

<div class="api-signature">

```c
extern void se_ui_get_default_style(se_ui_style* out_style);
```

</div>

No inline description found in header comments.

### `se_ui_get_default_widget_desc`

<div class="api-signature">

```c
extern void se_ui_get_default_widget_desc(se_ui_widget_desc* out_desc);
```

</div>

No inline description found in header comments.

### `se_ui_get_focused_widget`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_get_focused_widget(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_get_hovered_widget`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_get_hovered_widget(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_get_scene`

<div class="api-signature">

```c
extern se_scene_2d_handle se_ui_get_scene(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_get_window`

<div class="api-signature">

```c
extern se_window_handle se_ui_get_window(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_is_debug_overlay_enabled`

<div class="api-signature">

```c
extern b8 se_ui_is_debug_overlay_enabled(void);
```

</div>

No inline description found in header comments.

### `se_ui_is_dirty`

<div class="api-signature">

```c
extern b8 se_ui_is_dirty(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_mark_layout_dirty`

<div class="api-signature">

```c
extern void se_ui_mark_layout_dirty(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_mark_structure_dirty`

<div class="api-signature">

```c
extern void se_ui_mark_structure_dirty(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_mark_text_dirty`

<div class="api-signature">

```c
extern void se_ui_mark_text_dirty(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_mark_visual_dirty`

<div class="api-signature">

```c
extern void se_ui_mark_visual_dirty(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_minimap_ui_to_world`

<div class="api-signature">

```c
extern b8 se_ui_minimap_ui_to_world(const se_box_2d* world_rect, const se_box_2d* ui_rect, const s_vec2* ui_point, s_vec2* out_world_point);
```

</div>

No inline description found in header comments.

### `se_ui_minimap_world_to_ui`

<div class="api-signature">

```c
extern b8 se_ui_minimap_world_to_ui(const se_box_2d* world_rect, const se_box_2d* ui_rect, const s_vec2* world_point, s_vec2* out_ui_point);
```

</div>

No inline description found in header comments.

### `se_ui_panel_add`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_panel_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_panel_desc* desc);
```

</div>

No inline description found in header comments.

### `se_ui_panel_create`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_panel_create(se_ui_handle ui, se_ui_widget_handle parent, s_vec2 size);
```

</div>

Adds a panel with default style/behavior and explicit size.

### `se_ui_scroll_item_add`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_scroll_item_add(se_ui_handle ui, se_ui_widget_handle scrollbox, const c8* text);
```

</div>

Adds a selectable item entry to a scrollbox.

### `se_ui_scrollbox_add`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_scrollbox_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_scrollbox_desc* desc);
```

</div>

No inline description found in header comments.

### `se_ui_scrollbox_create`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_scrollbox_create(se_ui_handle ui, se_ui_widget_handle parent, s_vec2 size);
```

</div>

Adds a scrollbox with defaults and explicit size.

### `se_ui_scrollbox_enable_single_select`

<div class="api-signature">

```c
extern b8 se_ui_scrollbox_enable_single_select(se_ui_handle ui, se_ui_widget_handle scrollbox, b8 enabled);
```

</div>

Enables/disables single-selection behavior for scrollbox item children.

### `se_ui_scrollbox_get_selected`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_scrollbox_get_selected(se_ui_handle ui, se_ui_widget_handle scrollbox);
```

</div>

Returns the currently selected scrollbox item, or null.

### `se_ui_scrollbox_scroll_to_item`

<div class="api-signature">

```c
extern b8 se_ui_scrollbox_scroll_to_item(se_ui_handle ui, se_ui_widget_handle scrollbox, se_ui_widget_handle item);
```

</div>

Scrolls viewport to make a specific child item visible.

### `se_ui_scrollbox_set_item_styles`

<div class="api-signature">

```c
extern b8 se_ui_scrollbox_set_item_styles(se_ui_handle ui, se_ui_widget_handle scrollbox, const se_ui_style* normal, const se_ui_style* selected);
```

</div>

Sets normal and selected style templates for scrollbox item children.

### `se_ui_scrollbox_set_selected`

<div class="api-signature">

```c
extern b8 se_ui_scrollbox_set_selected(se_ui_handle ui, se_ui_widget_handle scrollbox, se_ui_widget_handle item);
```

</div>

Selects one item in a scrollbox.

### `se_ui_set_debug_overlay`

<div class="api-signature">

```c
extern void se_ui_set_debug_overlay(const b8 enabled);
```

</div>

No inline description found in header comments.

### `se_ui_spacer_create`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_spacer_create(se_ui_handle ui, se_ui_widget_handle parent, s_vec2 size);
```

</div>

Adds a non-visual spacer panel with explicit size.

### `se_ui_style_apply_button_danger`

<div class="api-signature">

```c
extern void se_ui_style_apply_button_danger(se_ui_style* style);
```

</div>

Applies danger button style values in-place.

### `se_ui_style_apply_button_primary`

<div class="api-signature">

```c
extern void se_ui_style_apply_button_primary(se_ui_style* style);
```

</div>

Applies primary button style values in-place.

### `se_ui_style_apply_text_muted`

<div class="api-signature">

```c
extern void se_ui_style_apply_text_muted(se_ui_style* style);
```

</div>

Applies muted text style values in-place.

### `se_ui_text_add`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_text_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_text_desc* desc);
```

</div>

No inline description found in header comments.

### `se_ui_text_create`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_text_create(se_ui_handle ui, se_ui_widget_handle parent, const c8* text);
```

</div>

Adds a text widget with defaults and label text.

### `se_ui_textbox_add`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_textbox_add(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_textbox_desc* desc);
```

</div>

No inline description found in header comments.

### `se_ui_textbox_create`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_textbox_create(se_ui_handle ui, se_ui_widget_handle parent, const c8* placeholder);
```

</div>

Adds a textbox with defaults and placeholder text.

### `se_ui_theme_apply`

<div class="api-signature">

```c
extern b8 se_ui_theme_apply(se_ui_handle ui, se_ui_theme_preset preset);
```

</div>

Applies a theme preset across all widgets in a UI root.

### `se_ui_tick`

<div class="api-signature">

```c
extern void se_ui_tick(const se_ui_handle ui);
```

</div>

No inline description found in header comments.

### `se_ui_widget_attach`

<div class="api-signature">

```c
extern b8 se_ui_widget_attach(const se_ui_handle ui, const se_ui_widget_handle parent, const se_ui_widget_handle child);
```

</div>

No inline description found in header comments.

### `se_ui_widget_detach`

<div class="api-signature">

```c
extern b8 se_ui_widget_detach(const se_ui_handle ui, const se_ui_widget_handle child);
```

</div>

No inline description found in header comments.

### `se_ui_widget_exists`

<div class="api-signature">

```c
extern b8 se_ui_widget_exists(const se_ui_handle ui, const se_ui_widget_handle widget);
```

</div>

No inline description found in header comments.

### `se_ui_widget_find`

<div class="api-signature">

```c
extern se_ui_widget_handle se_ui_widget_find(se_ui_handle ui, const c8* id);
```

</div>

Finds a widget by stable id string inside one UI root.

### `se_ui_widget_get_bounds`

<div class="api-signature">

```c
extern b8 se_ui_widget_get_bounds(const se_ui_handle ui, const se_ui_widget_handle widget, se_box_2d* out_bounds);
```

</div>

No inline description found in header comments.

### `se_ui_widget_get_layout`

<div class="api-signature">

```c
extern b8 se_ui_widget_get_layout(const se_ui_handle ui, const se_ui_widget_handle widget, se_ui_layout* out_layout);
```

</div>

No inline description found in header comments.

### `se_ui_widget_get_style`

<div class="api-signature">

```c
extern b8 se_ui_widget_get_style(const se_ui_handle ui, const se_ui_widget_handle widget, se_ui_style* out_style);
```

</div>

No inline description found in header comments.

### `se_ui_widget_get_text`

<div class="api-signature">

```c
extern sz se_ui_widget_get_text(const se_ui_handle ui, const se_ui_widget_handle widget, c8* out_text, const sz out_text_size);
```

</div>

No inline description found in header comments.

### `se_ui_widget_get_value`

<div class="api-signature">

```c
extern f32 se_ui_widget_get_value(const se_ui_handle ui, const se_ui_widget_handle widget);
```

</div>

No inline description found in header comments.

### `se_ui_widget_get_z_order`

<div class="api-signature">

```c
extern i32 se_ui_widget_get_z_order(const se_ui_handle ui, const se_ui_widget_handle widget);
```

</div>

No inline description found in header comments.

### `se_ui_widget_is_clipping`

<div class="api-signature">

```c
extern b8 se_ui_widget_is_clipping(const se_ui_handle ui, const se_ui_widget_handle widget);
```

</div>

No inline description found in header comments.

### `se_ui_widget_is_enabled`

<div class="api-signature">

```c
extern b8 se_ui_widget_is_enabled(const se_ui_handle ui, const se_ui_widget_handle widget);
```

</div>

No inline description found in header comments.

### `se_ui_widget_is_interactable`

<div class="api-signature">

```c
extern b8 se_ui_widget_is_interactable(const se_ui_handle ui, const se_ui_widget_handle widget);
```

</div>

No inline description found in header comments.

### `se_ui_widget_is_visible`

<div class="api-signature">

```c
extern b8 se_ui_widget_is_visible(const se_ui_handle ui, const se_ui_widget_handle widget);
```

</div>

No inline description found in header comments.

### `se_ui_widget_on_change`

<div class="api-signature">

```c
extern b8 se_ui_widget_on_change(se_ui_handle ui, se_ui_widget_handle widget, se_ui_change_callback cb, void* user_data);
```

</div>

Sets value/text change callback and user data for a widget.

### `se_ui_widget_on_click`

<div class="api-signature">

```c
extern b8 se_ui_widget_on_click(se_ui_handle ui, se_ui_widget_handle widget, se_ui_click_callback cb, void* user_data);
```

</div>

Sets click callback and user data for a widget.

### `se_ui_widget_on_hover`

<div class="api-signature">

```c
extern b8 se_ui_widget_on_hover(se_ui_handle ui, se_ui_widget_handle widget, se_ui_hover_callback on_start, se_ui_hover_callback on_end, void* user_data);
```

</div>

Sets hover callbacks and user data for a widget.

### `se_ui_widget_on_press`

<div class="api-signature">

```c
extern b8 se_ui_widget_on_press(se_ui_handle ui, se_ui_widget_handle widget, se_ui_press_callback on_press, se_ui_press_callback on_release, void* user_data);
```

</div>

Sets press/release callbacks and user data for a widget.

### `se_ui_widget_on_scroll`

<div class="api-signature">

```c
extern b8 se_ui_widget_on_scroll(se_ui_handle ui, se_ui_widget_handle widget, se_ui_scroll_callback cb, void* user_data);
```

</div>

Sets scroll callback and user data for a widget.

### `se_ui_widget_on_submit`

<div class="api-signature">

```c
extern b8 se_ui_widget_on_submit(se_ui_handle ui, se_ui_widget_handle widget, se_ui_submit_callback cb, void* user_data);
```

</div>

Sets submit callback and user data for a widget.

### `se_ui_widget_remove`

<div class="api-signature">

```c
extern b8 se_ui_widget_remove(const se_ui_handle ui, const se_ui_widget_handle widget);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_alignment`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_alignment(se_ui_handle ui, se_ui_widget_handle widget, se_ui_alignment horizontal, se_ui_alignment vertical);
```

</div>

Sets per-widget horizontal/vertical alignment in parent layout.

### `se_ui_widget_set_anchor_rect`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_anchor_rect(se_ui_handle ui, se_ui_widget_handle widget, s_vec2 anchor_min, s_vec2 anchor_max);
```

</div>

Enables anchors and sets anchor rectangle.

### `se_ui_widget_set_callbacks`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_callbacks(se_ui_handle ui, se_ui_widget_handle widget, const se_ui_callbacks* callbacks);
```

</div>

Replaces the full callback set for a widget. Returns false for unsupported callback types.

### `se_ui_widget_set_clipping`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_clipping(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 clip_children);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_enabled`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_enabled(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 enabled);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_interactable`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_interactable(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 interactable);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_layout`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_layout(const se_ui_handle ui, const se_ui_widget_handle widget, const se_ui_layout* layout);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_margin`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_margin(se_ui_handle ui, se_ui_widget_handle widget, se_ui_edge margin);
```

</div>

Sets per-widget layout margin.

### `se_ui_widget_set_max_size`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_max_size(se_ui_handle ui, se_ui_widget_handle widget, s_vec2 max_size);
```

</div>

Sets per-widget maximum size constraints (half extents in NDC units).

### `se_ui_widget_set_min_size`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_min_size(se_ui_handle ui, se_ui_widget_handle widget, s_vec2 min_size);
```

</div>

Sets per-widget minimum size constraints (half extents in NDC units).

### `se_ui_widget_set_padding`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_padding(se_ui_handle ui, se_ui_widget_handle widget, se_ui_edge padding);
```

</div>

Sets per-widget layout padding.

### `se_ui_widget_set_position`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_position(const se_ui_handle ui, const se_ui_widget_handle widget, const s_vec2* position);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_size`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_size(const se_ui_handle ui, const se_ui_widget_handle widget, const s_vec2* size);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_stack_horizontal`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_stack_horizontal(se_ui_handle ui, se_ui_widget_handle widget, f32 spacing);
```

</div>

Configures horizontal stack layout and spacing for a container.

### `se_ui_widget_set_stack_vertical`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_stack_vertical(se_ui_handle ui, se_ui_widget_handle widget, f32 spacing);
```

</div>

Configures vertical stack layout and spacing for a container.

### `se_ui_widget_set_style`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_style(const se_ui_handle ui, const se_ui_widget_handle widget, const se_ui_style* style);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_text`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_text(const se_ui_handle ui, const se_ui_widget_handle widget, const c8* text);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_text_by_id`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_text_by_id(se_ui_handle ui, const c8* id, const c8* text);
```

</div>

Sets text for a widget found by id.

### `se_ui_widget_set_user_data`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_user_data(se_ui_handle ui, se_ui_widget_handle widget, void* user_data);
```

</div>

Updates the callback user-data pointer on an existing widget.

### `se_ui_widget_set_value`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_value(const se_ui_handle ui, const se_ui_widget_handle widget, const f32 value);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_visible`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_visible(const se_ui_handle ui, const se_ui_widget_handle widget, const b8 visible);
```

</div>

No inline description found in header comments.

### `se_ui_widget_set_visible_by_id`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_visible_by_id(se_ui_handle ui, const c8* id, b8 visible);
```

</div>

Sets visibility for a widget found by id.

### `se_ui_widget_set_z_order`

<div class="api-signature">

```c
extern b8 se_ui_widget_set_z_order(const se_ui_handle ui, const se_ui_widget_handle widget, const i32 z_order);
```

</div>

No inline description found in header comments.

### `se_ui_widget_use_style_preset`

<div class="api-signature">

```c
extern b8 se_ui_widget_use_style_preset(se_ui_handle ui, se_ui_widget_handle widget, se_ui_style_preset preset);
```

</div>

Applies a predefined style preset to a widget.

### `se_ui_world_to_ndc`

<div class="api-signature">

```c
extern b8 se_ui_world_to_ndc(const se_camera_handle camera, const s_vec3* world, s_vec2* out_ndc);
```

</div>

No inline description found in header comments.

### `se_ui_world_to_window`

<div class="api-signature">

```c
extern b8 se_ui_world_to_window(const se_camera_handle camera, const se_window_handle window, const s_vec3* world, s_vec2* out_window_pixel);
```

</div>

No inline description found in header comments.

## Enums

### `se_ui_alignment`

<div class="api-signature">

```c
typedef enum { SE_UI_ALIGN_START = 0, SE_UI_ALIGN_CENTER, SE_UI_ALIGN_END, SE_UI_ALIGN_STRETCH } se_ui_alignment;
```

</div>

No inline description found in header comments.

### `se_ui_layout_direction`

<div class="api-signature">

```c
typedef enum { SE_UI_LAYOUT_VERTICAL = 0, SE_UI_LAYOUT_HORIZONTAL } se_ui_layout_direction;
```

</div>

No inline description found in header comments.

### `se_ui_style_preset`

<div class="api-signature">

```c
typedef enum { SE_UI_STYLE_PRESET_DEFAULT = 0, SE_UI_STYLE_PRESET_BUTTON_PRIMARY, SE_UI_STYLE_PRESET_BUTTON_DANGER, SE_UI_STYLE_PRESET_TEXT_MUTED, SE_UI_STYLE_PRESET_SCROLL_ITEM_NORMAL, SE_UI_STYLE_PRESET_SCROLL_ITEM_SELECTED } se_ui_style_preset;
```

</div>

No inline description found in header comments.

### `se_ui_theme_preset`

<div class="api-signature">

```c
typedef enum { SE_UI_THEME_PRESET_DEFAULT = 0, SE_UI_THEME_PRESET_LIGHT } se_ui_theme_preset;
```

</div>

No inline description found in header comments.

### `se_ui_widget_type`

<div class="api-signature">

```c
typedef enum { SE_UI_WIDGET_PANEL = 0, SE_UI_WIDGET_BUTTON, SE_UI_WIDGET_TEXT, SE_UI_WIDGET_TEXTBOX, SE_UI_WIDGET_SCROLLBOX } se_ui_widget_type;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_ui_button_args`

<div class="api-signature">

```c
typedef struct { const c8* id; s_vec2 position; s_vec2 size; const c8* text; const c8* font_path; f32 font_size; se_ui_hover_callback on_hover_start_fn; void* on_hover_start_data; se_ui_hover_callback on_hover_end_fn; void* on_hover_end_data; se_ui_focus_callback on_focus_fn; void* on_focus_data; se_ui_focus_callback on_blur_fn; void* on_blur_data; se_ui_press_callback on_press_fn; void* on_press_data; se_ui_press_callback on_release_fn; void* on_release_data; se_ui_click_callback on_click_fn; void* on_click_data; se_ui_change_callback on_change_fn; void* on_change_data; se_ui_submit_callback on_submit_fn; void* on_submit_data; se_ui_scroll_callback on_scroll_fn; void* on_scroll_data; void* user_data; } se_ui_button_args;
```

</div>

No inline description found in header comments.

### `se_ui_button_desc`

<div class="api-signature">

```c
typedef struct { se_ui_widget_desc widget; const c8* text; const c8* font_path; f32 font_size; se_ui_callbacks callbacks; } se_ui_button_desc;
```

</div>

No inline description found in header comments.

### `se_ui_callbacks`

<div class="api-signature">

```c
typedef struct { se_ui_hover_callback on_hover_start; void* on_hover_start_data; se_ui_hover_callback on_hover_end; void* on_hover_end_data; se_ui_focus_callback on_focus; void* on_focus_data; se_ui_focus_callback on_blur; void* on_blur_data; se_ui_press_callback on_press; void* on_press_data; se_ui_press_callback on_release; void* on_release_data; se_ui_click_callback on_click; void* on_click_data; se_ui_change_callback on_change; void* on_change_data; se_ui_submit_callback on_submit; void* on_submit_data; se_ui_scroll_callback on_scroll; void* on_scroll_data; void* user_data; } se_ui_callbacks;
```

</div>

No inline description found in header comments.

### `se_ui_change_callback`

<div class="api-signature">

```c
typedef void (*se_ui_change_callback)(const se_ui_change_event* event, void* user_data);
```

</div>

No inline description found in header comments.

### `se_ui_change_event`

<div class="api-signature">

```c
typedef struct { se_ui_handle ui; se_ui_widget_handle widget; f32 value; const c8* text; } se_ui_change_event;
```

</div>

No inline description found in header comments.

### `se_ui_click_callback`

<div class="api-signature">

```c
typedef void (*se_ui_click_callback)(const se_ui_click_event* event, void* user_data);
```

</div>

No inline description found in header comments.

### `se_ui_click_event`

<div class="api-signature">

```c
typedef struct { se_ui_handle ui; se_ui_widget_handle widget; s_vec2 pointer_ndc; se_mouse_button button; } se_ui_click_event;
```

</div>

No inline description found in header comments.

### `se_ui_edge`

<div class="api-signature">

```c
typedef struct { f32 left; f32 right; f32 top; f32 bottom; } se_ui_edge;
```

</div>

No inline description found in header comments.

### `se_ui_focus_callback`

<div class="api-signature">

```c
typedef void (*se_ui_focus_callback)(const se_ui_focus_event* event, void* user_data);
```

</div>

No inline description found in header comments.

### `se_ui_focus_event`

<div class="api-signature">

```c
typedef struct { se_ui_handle ui; se_ui_widget_handle widget; } se_ui_focus_event;
```

</div>

No inline description found in header comments.

### `se_ui_hover_callback`

<div class="api-signature">

```c
typedef void (*se_ui_hover_callback)(const se_ui_hover_event* event, void* user_data);
```

</div>

No inline description found in header comments.

### `se_ui_hover_event`

<div class="api-signature">

```c
typedef struct { se_ui_handle ui; se_ui_widget_handle widget; s_vec2 pointer_ndc; } se_ui_hover_event;
```

</div>

No inline description found in header comments.

### `se_ui_layout`

<div class="api-signature">

```c
typedef struct { se_ui_layout_direction direction; se_ui_alignment align_horizontal; se_ui_alignment align_vertical; f32 spacing; s_vec2 anchor_min; s_vec2 anchor_max; se_ui_edge margin; se_ui_edge padding; s_vec2 min_size; s_vec2 max_size; b8 use_anchors : 1; b8 clip_children : 1; } se_ui_layout;
```

</div>

No inline description found in header comments.

### `se_ui_panel_args`

<div class="api-signature">

```c
typedef struct { const c8* id; s_vec2 position; s_vec2 size; se_ui_hover_callback on_hover_start_fn; void* on_hover_start_data; se_ui_hover_callback on_hover_end_fn; void* on_hover_end_data; se_ui_focus_callback on_focus_fn; void* on_focus_data; se_ui_focus_callback on_blur_fn; void* on_blur_data; se_ui_press_callback on_press_fn; void* on_press_data; se_ui_press_callback on_release_fn; void* on_release_data; se_ui_click_callback on_click_fn; void* on_click_data; se_ui_change_callback on_change_fn; void* on_change_data; se_ui_submit_callback on_submit_fn; void* on_submit_data; se_ui_scroll_callback on_scroll_fn; void* on_scroll_data; void* user_data; } se_ui_panel_args;
```

</div>

No inline description found in header comments.

### `se_ui_panel_desc`

<div class="api-signature">

```c
typedef struct { se_ui_widget_desc widget; } se_ui_panel_desc;
```

</div>

No inline description found in header comments.

### `se_ui_press_callback`

<div class="api-signature">

```c
typedef void (*se_ui_press_callback)(const se_ui_press_event* event, void* user_data);
```

</div>

No inline description found in header comments.

### `se_ui_press_event`

<div class="api-signature">

```c
typedef struct { se_ui_handle ui; se_ui_widget_handle widget; s_vec2 pointer_ndc; se_mouse_button button; } se_ui_press_event;
```

</div>

No inline description found in header comments.

### `se_ui_scroll_callback`

<div class="api-signature">

```c
typedef void (*se_ui_scroll_callback)(const se_ui_scroll_event* event, void* user_data);
```

</div>

No inline description found in header comments.

### `se_ui_scroll_event`

<div class="api-signature">

```c
typedef struct { se_ui_handle ui; se_ui_widget_handle widget; f32 value; s_vec2 pointer_ndc; } se_ui_scroll_event;
```

</div>

No inline description found in header comments.

### `se_ui_scrollbox_args`

<div class="api-signature">

```c
typedef struct { const c8* id; s_vec2 position; s_vec2 size; f32 value; f32 wheel_step; se_ui_hover_callback on_hover_start_fn; void* on_hover_start_data; se_ui_hover_callback on_hover_end_fn; void* on_hover_end_data; se_ui_focus_callback on_focus_fn; void* on_focus_data; se_ui_focus_callback on_blur_fn; void* on_blur_data; se_ui_press_callback on_press_fn; void* on_press_data; se_ui_press_callback on_release_fn; void* on_release_data; se_ui_click_callback on_click_fn; void* on_click_data; se_ui_change_callback on_change_fn; void* on_change_data; se_ui_submit_callback on_submit_fn; void* on_submit_data; se_ui_scroll_callback on_scroll_fn; void* on_scroll_data; void* user_data; } se_ui_scrollbox_args;
```

</div>

No inline description found in header comments.

### `se_ui_scrollbox_desc`

<div class="api-signature">

```c
typedef struct { se_ui_widget_desc widget; f32 value; f32 wheel_step; se_ui_callbacks callbacks; } se_ui_scrollbox_desc;
```

</div>

No inline description found in header comments.

### `se_ui_style`

<div class="api-signature">

```c
typedef struct { se_ui_style_state normal; se_ui_style_state hovered; se_ui_style_state pressed; se_ui_style_state disabled; se_ui_style_state focused; } se_ui_style;
```

</div>

No inline description found in header comments.

### `se_ui_style_state`

<div class="api-signature">

```c
typedef struct { s_vec4 background_color; s_vec4 border_color; s_vec4 text_color; f32 border_width; } se_ui_style_state;
```

</div>

No inline description found in header comments.

### `se_ui_submit_callback`

<div class="api-signature">

```c
typedef void (*se_ui_submit_callback)(const se_ui_submit_event* event, void* user_data);
```

</div>

No inline description found in header comments.

### `se_ui_submit_event`

<div class="api-signature">

```c
typedef struct { se_ui_handle ui; se_ui_widget_handle widget; const c8* text; } se_ui_submit_event;
```

</div>

No inline description found in header comments.

### `se_ui_text_args`

<div class="api-signature">

```c
typedef struct { const c8* id; s_vec2 position; s_vec2 size; const c8* text; const c8* font_path; f32 font_size; se_ui_hover_callback on_hover_start_fn; void* on_hover_start_data; se_ui_hover_callback on_hover_end_fn; void* on_hover_end_data; se_ui_focus_callback on_focus_fn; void* on_focus_data; se_ui_focus_callback on_blur_fn; void* on_blur_data; se_ui_press_callback on_press_fn; void* on_press_data; se_ui_press_callback on_release_fn; void* on_release_data; se_ui_click_callback on_click_fn; void* on_click_data; se_ui_change_callback on_change_fn; void* on_change_data; se_ui_submit_callback on_submit_fn; void* on_submit_data; se_ui_scroll_callback on_scroll_fn; void* on_scroll_data; void* user_data; } se_ui_text_args;
```

</div>

No inline description found in header comments.

### `se_ui_text_desc`

<div class="api-signature">

```c
typedef struct { se_ui_widget_desc widget; const c8* text; const c8* font_path; f32 font_size; } se_ui_text_desc;
```

</div>

No inline description found in header comments.

### `se_ui_textbox_args`

<div class="api-signature">

```c
typedef struct { const c8* id; s_vec2 position; s_vec2 size; const c8* text; const c8* placeholder; const c8* font_path; f32 font_size; u32 max_length; b8 set_submit_on_enter : 1; b8 submit_on_enter : 1; se_ui_hover_callback on_hover_start_fn; void* on_hover_start_data; se_ui_hover_callback on_hover_end_fn; void* on_hover_end_data; se_ui_focus_callback on_focus_fn; void* on_focus_data; se_ui_focus_callback on_blur_fn; void* on_blur_data; se_ui_press_callback on_press_fn; void* on_press_data; se_ui_press_callback on_release_fn; void* on_release_data; se_ui_click_callback on_click_fn; void* on_click_data; se_ui_change_callback on_change_fn; void* on_change_data; se_ui_submit_callback on_submit_fn; void* on_submit_data; se_ui_scroll_callback on_scroll_fn; void* on_scroll_data; void* user_data; } se_ui_textbox_args;
```

</div>

No inline description found in header comments.

### `se_ui_textbox_desc`

<div class="api-signature">

```c
typedef struct { se_ui_widget_desc widget; const c8* text; const c8* placeholder; const c8* font_path; f32 font_size; u32 max_length; b8 submit_on_enter : 1; se_ui_callbacks callbacks; } se_ui_textbox_desc;
```

</div>

No inline description found in header comments.

### `se_ui_widget_desc`

<div class="api-signature">

```c
typedef struct { s_vec2 position; s_vec2 size; const c8* id; b8 visible : 1; b8 enabled : 1; b8 interactable : 1; i32 z_order; se_ui_layout layout; se_ui_style style; } se_ui_widget_desc;
```

</div>

No inline description found in header comments.
