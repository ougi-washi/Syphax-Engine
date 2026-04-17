<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_editor.h

Source header: `include/se_editor.h`

## Overview

Generic editor values, item/property collection, commands, shortcuts, and JSON helpers.

This page is generated from `include/se_editor.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/utilities.md)

## Functions

### `se_editor_add_item`

<div class="api-signature">

```c
extern b8 se_editor_add_item(se_editor* editor, se_editor_category category, s_handle handle, s_handle owner_handle, void* pointer, void* owner_pointer, u32 index, const c8* label);
```

</div>

No inline description found in header comments.

### `se_editor_add_property`

<div class="api-signature">

```c
extern b8 se_editor_add_property(se_editor* editor, const se_editor_property_desc* desc);
```

</div>

No inline description found in header comments.

### `se_editor_add_property_value`

<div class="api-signature">

```c
extern b8 se_editor_add_property_value(se_editor* editor, const c8* name, const se_editor_value* value, b8 editable);
```

</div>

No inline description found in header comments.

### `se_editor_apply_action`

<div class="api-signature">

```c
extern b8 se_editor_apply_action(se_editor* editor, const se_editor_item* item, const c8* action_name, const se_editor_value* value, const se_editor_value* aux_value);
```

</div>

No inline description found in header comments.

### `se_editor_apply_command`

<div class="api-signature">

```c
extern b8 se_editor_apply_command(se_editor* editor, const se_editor_command* command);
```

</div>

No inline description found in header comments.

### `se_editor_apply_commands`

<div class="api-signature">

```c
extern b8 se_editor_apply_commands(se_editor* editor, const se_editor_command* commands, sz command_count, u32* out_applied);
```

</div>

No inline description found in header comments.

### `se_editor_apply_queue`

<div class="api-signature">

```c
extern b8 se_editor_apply_queue(se_editor* editor, u32* out_applied, b8 clear_after_apply);
```

</div>

No inline description found in header comments.

### `se_editor_apply_set`

<div class="api-signature">

```c
extern b8 se_editor_apply_set(se_editor* editor, const se_editor_item* item, const c8* property_name, const se_editor_value* value);
```

</div>

No inline description found in header comments.

### `se_editor_apply_toggle`

<div class="api-signature">

```c
extern b8 se_editor_apply_toggle(se_editor* editor, const se_editor_item* item, const c8* property_name);
```

</div>

No inline description found in header comments.

### `se_editor_bind_default_shortcuts`

<div class="api-signature">

```c
extern b8 se_editor_bind_default_shortcuts(se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_bind_key`

<div class="api-signature">

```c
extern b8 se_editor_bind_key(se_editor* editor, se_editor_mode mode, se_key key, u32 modifiers, const c8* action);
```

</div>

No inline description found in header comments.

### `se_editor_bind_shortcut`

<div class="api-signature">

```c
extern b8 se_editor_bind_shortcut(se_editor* editor, se_editor_mode mode, const se_key* keys, u32 key_count, u32 modifiers, const c8* action);
```

</div>

No inline description found in header comments.

### `se_editor_bind_shortcut_text`

<div class="api-signature">

```c
extern b8 se_editor_bind_shortcut_text(se_editor* editor, se_editor_mode mode, const c8* keys, const c8* action);
```

</div>

No inline description found in header comments.

### `se_editor_category_from_name`

<div class="api-signature">

```c
extern se_editor_category se_editor_category_from_name(const c8* name);
```

</div>

Accepts case-insensitive names and ignores separators such as '_', '-', and spaces.

### `se_editor_category_name`

<div class="api-signature">

```c
extern const c8* se_editor_category_name(se_editor_category category);
```

</div>

No inline description found in header comments.

### `se_editor_clear_queue`

<div class="api-signature">

```c
extern void se_editor_clear_queue(se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_clear_shortcut_events`

<div class="api-signature">

```c
extern void se_editor_clear_shortcut_events(se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_clear_shortcuts`

<div class="api-signature">

```c
extern void se_editor_clear_shortcuts(se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_collect_counts`

<div class="api-signature">

```c
extern b8 se_editor_collect_counts(se_editor* editor, se_editor_counts* out_counts);
```

</div>

No inline description found in header comments.

### `se_editor_collect_items`

<div class="api-signature">

```c
extern b8 se_editor_collect_items(se_editor* editor, const se_editor_item** out_items, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_editor_collect_properties`

<div class="api-signature">

```c
extern b8 se_editor_collect_properties(se_editor* editor, const se_editor_item* item, const se_editor_property** out_properties, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_editor_command_make`

<div class="api-signature">

```c
extern se_editor_command se_editor_command_make(se_editor_command_type type, const se_editor_item* item, const c8* name, const se_editor_value* value, const se_editor_value* aux_value);
```

</div>

No inline description found in header comments.

### `se_editor_command_make_action`

<div class="api-signature">

```c
extern se_editor_command se_editor_command_make_action(const se_editor_item* item, const c8* name, const se_editor_value* value, const se_editor_value* aux_value);
```

</div>

No inline description found in header comments.

### `se_editor_command_make_set`

<div class="api-signature">

```c
extern se_editor_command se_editor_command_make_set(const se_editor_item* item, const c8* name, const se_editor_value* value);
```

</div>

No inline description found in header comments.

### `se_editor_command_make_toggle`

<div class="api-signature">

```c
extern se_editor_command se_editor_command_make_toggle(const se_editor_item* item, const c8* name);
```

</div>

No inline description found in header comments.

### `se_editor_create`

<div class="api-signature">

```c
extern se_editor* se_editor_create(const se_editor_config* config);
```

</div>

No inline description found in header comments.

### `se_editor_destroy`

<div class="api-signature">

```c
extern void se_editor_destroy(se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_find_item`

<div class="api-signature">

```c
extern b8 se_editor_find_item(se_editor* editor, se_editor_category category, s_handle handle, se_editor_item* out_item);
```

</div>

No inline description found in header comments.

### `se_editor_find_item_by_label`

<div class="api-signature">

```c
extern b8 se_editor_find_item_by_label(se_editor* editor, se_editor_category category, const c8* label, se_editor_item* out_item);
```

</div>

No inline description found in header comments.

### `se_editor_find_property`

<div class="api-signature">

```c
extern b8 se_editor_find_property(const se_editor_property* properties, sz property_count, const c8* name, const se_editor_property** out_property);
```

</div>

No inline description found in header comments.

### `se_editor_get_mode`

<div class="api-signature">

```c
extern se_editor_mode se_editor_get_mode(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_queue`

<div class="api-signature">

```c
extern b8 se_editor_get_queue(const se_editor* editor, const se_editor_command** out_commands, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_editor_get_shortcuts`

<div class="api-signature">

```c
extern b8 se_editor_get_shortcuts(const se_editor* editor, const se_editor_shortcut** out_shortcuts, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_editor_get_user_data`

<div class="api-signature">

```c
extern void* se_editor_get_user_data(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_window`

<div class="api-signature">

```c
extern se_window_handle se_editor_get_window(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_json_read_file`

<div class="api-signature">

```c
extern b8 se_editor_json_read_file(const c8* path, s_json** out_root);
```

</div>

No inline description found in header comments.

### `se_editor_json_write_file`

<div class="api-signature">

```c
extern b8 se_editor_json_write_file(const c8* path, s_json* root);
```

</div>

No inline description found in header comments.

### `se_editor_mode_name`

<div class="api-signature">

```c
extern const c8* se_editor_mode_name(se_editor_mode mode);
```

</div>

No inline description found in header comments.

### `se_editor_poll_shortcut`

<div class="api-signature">

```c
extern b8 se_editor_poll_shortcut(se_editor* editor, se_editor_shortcut_event* out_event);
```

</div>

No inline description found in header comments.

### `se_editor_property_component_count`

<div class="api-signature">

```c
extern u32 se_editor_property_component_count(const se_editor_property* property);
```

</div>

No inline description found in header comments.

### `se_editor_property_component_name`

<div class="api-signature">

```c
extern const c8* se_editor_property_component_name(const se_editor_property* property, u32 component);
```

</div>

No inline description found in header comments.

### `se_editor_queue_command`

<div class="api-signature">

```c
extern b8 se_editor_queue_command(se_editor* editor, const se_editor_command* command);
```

</div>

No inline description found in header comments.

### `se_editor_reset`

<div class="api-signature">

```c
extern void se_editor_reset(se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_set_apply_command`

<div class="api-signature">

```c
extern void se_editor_set_apply_command(se_editor* editor, se_editor_apply_command_fn fn);
```

</div>

No inline description found in header comments.

### `se_editor_set_collect_items`

<div class="api-signature">

```c
extern void se_editor_set_collect_items(se_editor* editor, se_editor_collect_items_fn fn);
```

</div>

No inline description found in header comments.

### `se_editor_set_collect_properties`

<div class="api-signature">

```c
extern void se_editor_set_collect_properties(se_editor* editor, se_editor_collect_properties_fn fn);
```

</div>

No inline description found in header comments.

### `se_editor_set_mode`

<div class="api-signature">

```c
extern void se_editor_set_mode(se_editor* editor, se_editor_mode mode);
```

</div>

No inline description found in header comments.

### `se_editor_set_shortcut_repeat`

<div class="api-signature">

```c
extern b8 se_editor_set_shortcut_repeat(se_editor* editor, const c8* action, b8 repeat);
```

</div>

No inline description found in header comments.

### `se_editor_set_user_data`

<div class="api-signature">

```c
extern void se_editor_set_user_data(se_editor* editor, void* user_data);
```

</div>

No inline description found in header comments.

### `se_editor_set_window`

<div class="api-signature">

```c
extern void se_editor_set_window(se_editor* editor, se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_editor_unbind_shortcut`

<div class="api-signature">

```c
extern b8 se_editor_unbind_shortcut(se_editor* editor, const c8* action);
```

</div>

No inline description found in header comments.

### `se_editor_update_shortcuts`

<div class="api-signature">

```c
extern b8 se_editor_update_shortcuts(se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_validate_item`

<div class="api-signature">

```c
extern b8 se_editor_validate_item(se_editor* editor, const se_editor_item* item);
```

</div>

No inline description found in header comments.

### `se_editor_value_adjust_component`

<div class="api-signature">

```c
extern b8 se_editor_value_adjust_component(se_editor_value* value, u32 component, f32 delta);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_bool`

<div class="api-signature">

```c
extern b8 se_editor_value_as_bool(const se_editor_value* value, b8* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_f32`

<div class="api-signature">

```c
extern b8 se_editor_value_as_f32(const se_editor_value* value, f32* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_f64`

<div class="api-signature">

```c
extern b8 se_editor_value_as_f64(const se_editor_value* value, f64* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_handle`

<div class="api-signature">

```c
extern b8 se_editor_value_as_handle(const se_editor_value* value, s_handle* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_i32`

<div class="api-signature">

```c
extern b8 se_editor_value_as_i32(const se_editor_value* value, i32* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_mat3`

<div class="api-signature">

```c
extern b8 se_editor_value_as_mat3(const se_editor_value* value, s_mat3* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_mat4`

<div class="api-signature">

```c
extern b8 se_editor_value_as_mat4(const se_editor_value* value, s_mat4* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_text`

<div class="api-signature">

```c
extern const c8* se_editor_value_as_text(const se_editor_value* value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_u32`

<div class="api-signature">

```c
extern b8 se_editor_value_as_u32(const se_editor_value* value, u32* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_u64`

<div class="api-signature">

```c
extern b8 se_editor_value_as_u64(const se_editor_value* value, u64* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_vec2`

<div class="api-signature">

```c
extern b8 se_editor_value_as_vec2(const se_editor_value* value, s_vec2* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_vec3`

<div class="api-signature">

```c
extern b8 se_editor_value_as_vec3(const se_editor_value* value, s_vec3* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_as_vec4`

<div class="api-signature">

```c
extern b8 se_editor_value_as_vec4(const se_editor_value* value, s_vec4* out_value);
```

</div>

No inline description found in header comments.

### `se_editor_value_bool`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_bool(b8 value);
```

</div>

No inline description found in header comments.

### `se_editor_value_double`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_double(f64 value);
```

</div>

No inline description found in header comments.

### `se_editor_value_float`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_float(f32 value);
```

</div>

No inline description found in header comments.

### `se_editor_value_handle`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_handle(s_handle value);
```

</div>

No inline description found in header comments.

### `se_editor_value_int`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_int(i32 value);
```

</div>

No inline description found in header comments.

### `se_editor_value_mat3`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_mat3(s_mat3 value);
```

</div>

No inline description found in header comments.

### `se_editor_value_mat4`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_mat4(s_mat4 value);
```

</div>

No inline description found in header comments.

### `se_editor_value_none`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_none(void);
```

</div>

No inline description found in header comments.

### `se_editor_value_pointer`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_pointer(void* value);
```

</div>

No inline description found in header comments.

### `se_editor_value_text`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_text(const c8* value);
```

</div>

No inline description found in header comments.

### `se_editor_value_type_name`

<div class="api-signature">

```c
extern const c8* se_editor_value_type_name(se_editor_value_type type);
```

</div>

No inline description found in header comments.

### `se_editor_value_u64`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_u64(u64 value);
```

</div>

No inline description found in header comments.

### `se_editor_value_uint`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_uint(u32 value);
```

</div>

No inline description found in header comments.

### `se_editor_value_vec2`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_vec2(s_vec2 value);
```

</div>

No inline description found in header comments.

### `se_editor_value_vec3`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_vec3(s_vec3 value);
```

</div>

No inline description found in header comments.

### `se_editor_value_vec4`

<div class="api-signature">

```c
extern se_editor_value se_editor_value_vec4(s_vec4 value);
```

</div>

No inline description found in header comments.

## Enums

### `se_editor_category`

<div class="api-signature">

```c
typedef enum { SE_EDITOR_CATEGORY_ITEM = 0, SE_EDITOR_CATEGORY_COUNT } se_editor_category;
```

</div>

No inline description found in header comments.

### `se_editor_command_type`

<div class="api-signature">

```c
typedef enum { SE_EDITOR_COMMAND_SET = 0, SE_EDITOR_COMMAND_TOGGLE, SE_EDITOR_COMMAND_ACTION } se_editor_command_type;
```

</div>

No inline description found in header comments.

### `se_editor_mode`

<div class="api-signature">

```c
typedef enum { SE_EDITOR_MODE_NORMAL = 0, SE_EDITOR_MODE_INSERT, SE_EDITOR_MODE_COMMAND, SE_EDITOR_MODE_COUNT } se_editor_mode;
```

</div>

No inline description found in header comments.

### `se_editor_property_role`

<div class="api-signature">

```c
typedef enum { SE_EDITOR_PROPERTY_ROLE_NONE = 0, SE_EDITOR_PROPERTY_ROLE_POSITION, SE_EDITOR_PROPERTY_ROLE_ROTATION, SE_EDITOR_PROPERTY_ROLE_SCALE, SE_EDITOR_PROPERTY_ROLE_COLOR } se_editor_property_role;
```

</div>

No inline description found in header comments.

### `se_editor_shortcut_mod`

<div class="api-signature">

```c
typedef enum { SE_EDITOR_SHORTCUT_MOD_NONE = 0, SE_EDITOR_SHORTCUT_MOD_SHIFT = 1u << 0, SE_EDITOR_SHORTCUT_MOD_CONTROL = 1u << 1, SE_EDITOR_SHORTCUT_MOD_ALT = 1u << 2, SE_EDITOR_SHORTCUT_MOD_SUPER = 1u << 3 } se_editor_shortcut_mod;
```

</div>

No inline description found in header comments.

### `se_editor_value_type`

<div class="api-signature">

```c
typedef enum { SE_EDITOR_VALUE_NONE = 0, SE_EDITOR_VALUE_BOOL, SE_EDITOR_VALUE_INT, SE_EDITOR_VALUE_UINT, SE_EDITOR_VALUE_U64, SE_EDITOR_VALUE_FLOAT, SE_EDITOR_VALUE_DOUBLE, SE_EDITOR_VALUE_VEC2, SE_EDITOR_VALUE_VEC3, SE_EDITOR_VALUE_VEC4, SE_EDITOR_VALUE_MAT3, SE_EDITOR_VALUE_MAT4, SE_EDITOR_VALUE_HANDLE, SE_EDITOR_VALUE_POINTER, SE_EDITOR_VALUE_TEXT } se_editor_value_type;
```

</div>

No inline description found in header comments.

## Typedefs

### `s_json`

<div class="api-signature">

```c
typedef struct s_json s_json;
```

</div>

No inline description found in header comments.

### `se_editor`

<div class="api-signature">

```c
typedef struct se_editor se_editor;
```

</div>

No inline description found in header comments.

### `se_editor_apply_command_fn`

<div class="api-signature">

```c
typedef b8 (*se_editor_apply_command_fn)(se_editor* editor, const se_editor_command* command, void* user_data);
```

</div>

No inline description found in header comments.

### `se_editor_collect_items_fn`

<div class="api-signature">

```c
typedef b8 (*se_editor_collect_items_fn)(se_editor* editor, void* user_data);
```

</div>

No inline description found in header comments.

### `se_editor_collect_properties_fn`

<div class="api-signature">

```c
typedef b8 (*se_editor_collect_properties_fn)(se_editor* editor, const se_editor_item* item, void* user_data);
```

</div>

No inline description found in header comments.

### `se_editor_command`

<div class="api-signature">

```c
typedef struct { se_editor_command_type type; se_editor_item item; c8 name[SE_MAX_NAME_LENGTH]; se_editor_value value; se_editor_value aux_value; } se_editor_command;
```

</div>

No inline description found in header comments.

### `se_editor_config`

<div class="api-signature">

```c
typedef struct { se_window_handle window; void* user_data; se_editor_collect_items_fn collect_items; se_editor_collect_properties_fn collect_properties; se_editor_apply_command_fn apply_command; } se_editor_config;
```

</div>

No inline description found in header comments.

### `se_editor_counts`

<div class="api-signature">

```c
typedef struct { u32 items; u32 queued_commands; } se_editor_counts;
```

</div>

No inline description found in header comments.

### `se_editor_item`

<div class="api-signature">

```c
typedef struct { se_editor_category category; s_handle handle; s_handle owner_handle; void* pointer; void* owner_pointer; u32 index; c8 label[SE_MAX_NAME_LENGTH]; } se_editor_item;
```

</div>

No inline description found in header comments.

### `se_editor_property`

<div class="api-signature">

```c
typedef struct { c8 name[SE_MAX_NAME_LENGTH]; c8 label[SE_MAX_NAME_LENGTH]; se_editor_value value; se_editor_property_role role; f32 step; f32 large_step; b8 editable : 1; } se_editor_property;
```

</div>

No inline description found in header comments.

### `se_editor_property_desc`

<div class="api-signature">

```c
typedef struct { const c8* name; const c8* label; se_editor_value value; se_editor_property_role role; f32 step; f32 large_step; b8 editable : 1; } se_editor_property_desc;
```

</div>

No inline description found in header comments.

### `se_editor_shortcut`

<div class="api-signature">

```c
typedef struct { se_editor_mode mode; se_key keys[SE_EDITOR_SHORTCUT_MAX_KEYS]; u32 key_count; u32 modifiers; b8 repeat : 1; c8 action[SE_MAX_NAME_LENGTH]; } se_editor_shortcut;
```

</div>

No inline description found in header comments.

### `se_editor_shortcut_event`

<div class="api-signature">

```c
typedef struct { se_editor_mode mode; c8 action[SE_MAX_NAME_LENGTH]; } se_editor_shortcut_event;
```

</div>

No inline description found in header comments.

### `se_editor_value`

<div class="api-signature">

```c
typedef struct { se_editor_value_type type; union { b8 bool_value; i32 int_value; u32 uint_value; u64 u64_value; f32 float_value; f64 double_value; s_vec2 vec2_value; s_vec3 vec3_value; s_vec4 vec4_value; s_mat3 mat3_value; s_mat4 mat4_value; s_handle handle_value; void* pointer_value; c8 text_value[SE_MAX_PATH_LENGTH]; }; } se_editor_value;
```

</div>

No inline description found in header comments.
