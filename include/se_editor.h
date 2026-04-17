// Syphax-Engine - Ougi Washi

#ifndef SE_EDITOR_H
#define SE_EDITOR_H

#include "se_window.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct se_editor se_editor;
typedef struct s_json s_json;

typedef enum {
	SE_EDITOR_CATEGORY_ITEM = 0,
	SE_EDITOR_CATEGORY_COUNT
} se_editor_category;

#define SE_EDITOR_SHORTCUT_MAX_KEYS 4u

typedef enum {
	SE_EDITOR_VALUE_NONE = 0,
	SE_EDITOR_VALUE_BOOL,
	SE_EDITOR_VALUE_INT,
	SE_EDITOR_VALUE_UINT,
	SE_EDITOR_VALUE_U64,
	SE_EDITOR_VALUE_FLOAT,
	SE_EDITOR_VALUE_DOUBLE,
	SE_EDITOR_VALUE_VEC2,
	SE_EDITOR_VALUE_VEC3,
	SE_EDITOR_VALUE_VEC4,
	SE_EDITOR_VALUE_MAT3,
	SE_EDITOR_VALUE_MAT4,
	SE_EDITOR_VALUE_HANDLE,
	SE_EDITOR_VALUE_POINTER,
	SE_EDITOR_VALUE_TEXT
} se_editor_value_type;

typedef struct {
	se_editor_value_type type;
	union {
		b8 bool_value;
		i32 int_value;
		u32 uint_value;
		u64 u64_value;
		f32 float_value;
		f64 double_value;
		s_vec2 vec2_value;
		s_vec3 vec3_value;
		s_vec4 vec4_value;
		s_mat3 mat3_value;
		s_mat4 mat4_value;
		s_handle handle_value;
		void* pointer_value;
		c8 text_value[SE_MAX_PATH_LENGTH];
	};
} se_editor_value;

typedef enum {
	SE_EDITOR_PROPERTY_ROLE_NONE = 0,
	SE_EDITOR_PROPERTY_ROLE_POSITION,
	SE_EDITOR_PROPERTY_ROLE_ROTATION,
	SE_EDITOR_PROPERTY_ROLE_SCALE,
	SE_EDITOR_PROPERTY_ROLE_COLOR
} se_editor_property_role;

typedef struct {
	se_editor_category category;
	s_handle handle;
	s_handle owner_handle;
	void* pointer;
	void* owner_pointer;
	u32 index;
	c8 label[SE_MAX_NAME_LENGTH];
} se_editor_item;

typedef struct {
	const c8* name;
	const c8* label;
	se_editor_value value;
	se_editor_property_role role;
	f32 step;
	f32 large_step;
	b8 editable : 1;
} se_editor_property_desc;

#define SE_EDITOR_PROPERTY_DESC_DEFAULTS ((se_editor_property_desc){ \
	.name = NULL, \
	.label = NULL, \
	.value = {0}, \
	.role = SE_EDITOR_PROPERTY_ROLE_NONE, \
	.step = 0.0f, \
	.large_step = 0.0f, \
	.editable = false \
})

typedef struct {
	c8 name[SE_MAX_NAME_LENGTH];
	c8 label[SE_MAX_NAME_LENGTH];
	se_editor_value value;
	se_editor_property_role role;
	f32 step;
	f32 large_step;
	b8 editable : 1;
} se_editor_property;

typedef enum {
	SE_EDITOR_COMMAND_SET = 0,
	SE_EDITOR_COMMAND_TOGGLE,
	SE_EDITOR_COMMAND_ACTION
} se_editor_command_type;

typedef enum {
	SE_EDITOR_MODE_NORMAL = 0,
	SE_EDITOR_MODE_INSERT,
	SE_EDITOR_MODE_COMMAND,
	SE_EDITOR_MODE_COUNT
} se_editor_mode;

typedef enum {
	SE_EDITOR_SHORTCUT_MOD_NONE = 0,
	SE_EDITOR_SHORTCUT_MOD_SHIFT = 1u << 0,
	SE_EDITOR_SHORTCUT_MOD_CONTROL = 1u << 1,
	SE_EDITOR_SHORTCUT_MOD_ALT = 1u << 2,
	SE_EDITOR_SHORTCUT_MOD_SUPER = 1u << 3
} se_editor_shortcut_mod;

typedef struct {
	se_editor_command_type type;
	se_editor_item item;
	c8 name[SE_MAX_NAME_LENGTH];
	se_editor_value value;
	se_editor_value aux_value;
} se_editor_command;

typedef struct {
	se_editor_mode mode;
	se_key keys[SE_EDITOR_SHORTCUT_MAX_KEYS];
	u32 key_count;
	u32 modifiers;
	b8 repeat : 1;
	c8 action[SE_MAX_NAME_LENGTH];
} se_editor_shortcut;

typedef struct {
	se_editor_mode mode;
	c8 action[SE_MAX_NAME_LENGTH];
} se_editor_shortcut_event;

typedef b8 (*se_editor_collect_items_fn)(se_editor* editor, void* user_data);
typedef b8 (*se_editor_collect_properties_fn)(se_editor* editor, const se_editor_item* item, void* user_data);
typedef b8 (*se_editor_apply_command_fn)(se_editor* editor, const se_editor_command* command, void* user_data);

typedef struct {
	se_window_handle window;
	void* user_data;
	se_editor_collect_items_fn collect_items;
	se_editor_collect_properties_fn collect_properties;
	se_editor_apply_command_fn apply_command;
} se_editor_config;

#define SE_EDITOR_CONFIG_DEFAULTS ((se_editor_config){ \
	.window = S_HANDLE_NULL, \
	.user_data = NULL, \
	.collect_items = NULL, \
	.collect_properties = NULL, \
	.apply_command = NULL \
})

typedef struct {
	u32 items;
	u32 queued_commands;
} se_editor_counts;

extern const c8* se_editor_category_name(se_editor_category category);
/* Accepts case-insensitive names and ignores separators such as '_', '-', and spaces. */
extern se_editor_category se_editor_category_from_name(const c8* name);
extern const c8* se_editor_value_type_name(se_editor_value_type type);
extern const c8* se_editor_mode_name(se_editor_mode mode);

extern se_editor_value se_editor_value_none(void);
extern se_editor_value se_editor_value_bool(b8 value);
extern se_editor_value se_editor_value_int(i32 value);
extern se_editor_value se_editor_value_uint(u32 value);
extern se_editor_value se_editor_value_u64(u64 value);
extern se_editor_value se_editor_value_float(f32 value);
extern se_editor_value se_editor_value_double(f64 value);
extern se_editor_value se_editor_value_vec2(s_vec2 value);
extern se_editor_value se_editor_value_vec3(s_vec3 value);
extern se_editor_value se_editor_value_vec4(s_vec4 value);
extern se_editor_value se_editor_value_mat3(s_mat3 value);
extern se_editor_value se_editor_value_mat4(s_mat4 value);
extern se_editor_value se_editor_value_handle(s_handle value);
extern se_editor_value se_editor_value_pointer(void* value);
extern se_editor_value se_editor_value_text(const c8* value);
extern b8 se_editor_value_as_bool(const se_editor_value* value, b8* out_value);
extern b8 se_editor_value_as_i32(const se_editor_value* value, i32* out_value);
extern b8 se_editor_value_as_u32(const se_editor_value* value, u32* out_value);
extern b8 se_editor_value_as_u64(const se_editor_value* value, u64* out_value);
extern b8 se_editor_value_as_f32(const se_editor_value* value, f32* out_value);
extern b8 se_editor_value_as_f64(const se_editor_value* value, f64* out_value);
extern b8 se_editor_value_as_handle(const se_editor_value* value, s_handle* out_value);
extern b8 se_editor_value_as_vec2(const se_editor_value* value, s_vec2* out_value);
extern b8 se_editor_value_as_vec3(const se_editor_value* value, s_vec3* out_value);
extern b8 se_editor_value_as_vec4(const se_editor_value* value, s_vec4* out_value);
extern b8 se_editor_value_as_mat3(const se_editor_value* value, s_mat3* out_value);
extern b8 se_editor_value_as_mat4(const se_editor_value* value, s_mat4* out_value);
extern const c8* se_editor_value_as_text(const se_editor_value* value);
extern b8 se_editor_value_adjust_component(se_editor_value* value, u32 component, f32 delta);

extern se_editor_command se_editor_command_make(se_editor_command_type type, const se_editor_item* item, const c8* name, const se_editor_value* value, const se_editor_value* aux_value);
extern se_editor_command se_editor_command_make_set(const se_editor_item* item, const c8* name, const se_editor_value* value);
extern se_editor_command se_editor_command_make_toggle(const se_editor_item* item, const c8* name);
extern se_editor_command se_editor_command_make_action(const se_editor_item* item, const c8* name, const se_editor_value* value, const se_editor_value* aux_value);

extern se_editor* se_editor_create(const se_editor_config* config);
extern void se_editor_destroy(se_editor* editor);
extern void se_editor_reset(se_editor* editor);

extern void se_editor_set_window(se_editor* editor, se_window_handle window);
extern se_window_handle se_editor_get_window(const se_editor* editor);
extern void se_editor_set_user_data(se_editor* editor, void* user_data);
extern void* se_editor_get_user_data(const se_editor* editor);
extern void se_editor_set_collect_items(se_editor* editor, se_editor_collect_items_fn fn);
extern void se_editor_set_collect_properties(se_editor* editor, se_editor_collect_properties_fn fn);
extern void se_editor_set_apply_command(se_editor* editor, se_editor_apply_command_fn fn);

extern b8 se_editor_json_write_file(const c8* path, s_json* root);
extern b8 se_editor_json_read_file(const c8* path, s_json** out_root);

extern b8 se_editor_collect_counts(se_editor* editor, se_editor_counts* out_counts);
extern b8 se_editor_collect_items(se_editor* editor, const se_editor_item** out_items, sz* out_count);
extern b8 se_editor_collect_properties(se_editor* editor, const se_editor_item* item, const se_editor_property** out_properties, sz* out_count);
extern b8 se_editor_add_item(se_editor* editor, se_editor_category category, s_handle handle, s_handle owner_handle, void* pointer, void* owner_pointer, u32 index, const c8* label);
extern b8 se_editor_add_property(se_editor* editor, const se_editor_property_desc* desc);
extern b8 se_editor_add_property_value(se_editor* editor, const c8* name, const se_editor_value* value, b8 editable);
extern u32 se_editor_property_component_count(const se_editor_property* property);
extern const c8* se_editor_property_component_name(const se_editor_property* property, u32 component);
extern b8 se_editor_validate_item(se_editor* editor, const se_editor_item* item);
extern b8 se_editor_find_item(se_editor* editor, se_editor_category category, s_handle handle, se_editor_item* out_item);
extern b8 se_editor_find_item_by_label(se_editor* editor, se_editor_category category, const c8* label, se_editor_item* out_item);
extern b8 se_editor_find_property(const se_editor_property* properties, sz property_count, const c8* name, const se_editor_property** out_property);

extern void se_editor_set_mode(se_editor* editor, se_editor_mode mode);
extern se_editor_mode se_editor_get_mode(const se_editor* editor);
extern b8 se_editor_bind_shortcut(se_editor* editor, se_editor_mode mode, const se_key* keys, u32 key_count, u32 modifiers, const c8* action);
extern b8 se_editor_bind_key(se_editor* editor, se_editor_mode mode, se_key key, u32 modifiers, const c8* action);
extern b8 se_editor_bind_shortcut_text(se_editor* editor, se_editor_mode mode, const c8* keys, const c8* action);
extern b8 se_editor_bind_default_shortcuts(se_editor* editor);
extern b8 se_editor_set_shortcut_repeat(se_editor* editor, const c8* action, b8 repeat);
extern b8 se_editor_unbind_shortcut(se_editor* editor, const c8* action);
extern void se_editor_clear_shortcuts(se_editor* editor);
extern b8 se_editor_get_shortcuts(const se_editor* editor, const se_editor_shortcut** out_shortcuts, sz* out_count);
extern b8 se_editor_update_shortcuts(se_editor* editor);
extern b8 se_editor_poll_shortcut(se_editor* editor, se_editor_shortcut_event* out_event);
extern void se_editor_clear_shortcut_events(se_editor* editor);

extern b8 se_editor_apply_command(se_editor* editor, const se_editor_command* command);
extern b8 se_editor_apply_commands(se_editor* editor, const se_editor_command* commands, sz command_count, u32* out_applied);
extern b8 se_editor_apply_set(se_editor* editor, const se_editor_item* item, const c8* property_name, const se_editor_value* value);
extern b8 se_editor_apply_toggle(se_editor* editor, const se_editor_item* item, const c8* property_name);
extern b8 se_editor_apply_action(se_editor* editor, const se_editor_item* item, const c8* action_name, const se_editor_value* value, const se_editor_value* aux_value);

extern b8 se_editor_queue_command(se_editor* editor, const se_editor_command* command);
extern void se_editor_clear_queue(se_editor* editor);
extern b8 se_editor_get_queue(const se_editor* editor, const se_editor_command** out_commands, sz* out_count);
extern b8 se_editor_apply_queue(se_editor* editor, u32* out_applied, b8 clear_after_apply);

#ifdef __cplusplus
}
#endif

#endif // SE_EDITOR_H
