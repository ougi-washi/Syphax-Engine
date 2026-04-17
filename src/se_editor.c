// Syphax-Engine - Ougi Washi

#include "se_editor.h"

#include "syphax/s_array.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef s_array(se_editor_item, se_editor_items);
typedef s_array(se_editor_property, se_editor_properties);
typedef s_array(se_editor_command, se_editor_command_queue);
typedef s_array(se_editor_shortcut, se_editor_shortcuts);
typedef s_array(se_editor_shortcut_event, se_editor_shortcut_events);

struct se_editor {
	se_window_handle window;
	void* user_data;
	se_editor_collect_items_fn collect_items;
	se_editor_collect_properties_fn collect_properties;
	se_editor_apply_command_fn apply_command;

	se_editor_items items;
	se_editor_properties properties;
	se_editor_command_queue queue;
	se_editor_shortcuts shortcuts;
	se_editor_shortcut_events shortcut_events;
	se_editor_mode mode;
	se_key shortcut_sequence[SE_EDITOR_SHORTCUT_MAX_KEYS];
	u32 shortcut_sequence_count;
	se_key shortcut_repeat_key;
	u32 shortcut_repeat_modifiers;
	se_editor_mode shortcut_repeat_mode;
	f64 shortcut_repeat_next_time;
	c8 shortcut_repeat_action[SE_MAX_NAME_LENGTH];
	b8 shortcut_repeat_active : 1;
};

#define SE_EDITOR_SHORTCUT_REPEAT_DELAY 0.32
#define SE_EDITOR_SHORTCUT_REPEAT_INTERVAL 0.16

#define SE_EDITOR_NAME_ACTION "action"
#define SE_EDITOR_NAME_BOOL "bool"
#define SE_EDITOR_NAME_COMMAND "command"
#define SE_EDITOR_NAME_DOUBLE "double"
#define SE_EDITOR_NAME_FLOAT "float"
#define SE_EDITOR_NAME_HANDLE "handle"
#define SE_EDITOR_NAME_INSERT "insert"
#define SE_EDITOR_NAME_INT "int"
#define SE_EDITOR_NAME_ITEM "item"
#define SE_EDITOR_NAME_MAT3 "mat3"
#define SE_EDITOR_NAME_MAT4 "mat4"
#define SE_EDITOR_NAME_NONE "none"
#define SE_EDITOR_NAME_NORMAL "normal"
#define SE_EDITOR_NAME_POINTER "pointer"
#define SE_EDITOR_NAME_TEXT "text"
#define SE_EDITOR_NAME_U64 "u64"
#define SE_EDITOR_NAME_UINT "uint"
#define SE_EDITOR_NAME_UNKNOWN "unknown"
#define SE_EDITOR_NAME_VEC2 "vec2"
#define SE_EDITOR_NAME_VEC3 "vec3"
#define SE_EDITOR_NAME_VEC4 "vec4"

#define SE_EDITOR_ARRAY_POP_ALL(_arr) \
	do { \
		while (s_array_get_size((_arr)) > 0) { \
			s_array_remove((_arr), s_array_handle((_arr), (u32)(s_array_get_size((_arr)) - 1))); \
		} \
	} while (0)

static void se_editor_copy_text(c8* dst, sz dst_size, const c8* src) {
	if (!dst || dst_size == 0u) {
		return;
	}
	dst[0] = '\0';
	if (!src) {
		return;
	}
	snprintf(dst, dst_size, "%s", src);
}

static c8 se_editor_ascii_lower(c8 value) {
	if (value >= 'A' && value <= 'Z') {
		return (c8)(value - 'A' + 'a');
	}
	return value;
}

static b8 se_editor_ascii_is_alnum(c8 value) {
	if (value >= 'a' && value <= 'z') {
		return true;
	}
	if (value >= 'A' && value <= 'Z') {
		return true;
	}
	if (value >= '0' && value <= '9') {
		return true;
	}
	return false;
}

static void se_editor_normalize_name(const c8* src, c8* dst, sz dst_size) {
	sz out_index = 0u;
	if (!dst || dst_size == 0u) {
		return;
	}
	dst[0] = '\0';
	if (!src) {
		return;
	}
	for (sz i = 0u; src[i] != '\0' && out_index + 1u < dst_size; ++i) {
		const c8 c = src[i];
		if (!se_editor_ascii_is_alnum(c)) {
			continue;
		}
		dst[out_index++] = se_editor_ascii_lower(c);
	}
	dst[out_index] = '\0';
}

static void se_editor_clear_runtime_arrays(se_editor* editor) {
	if (!editor) {
		return;
	}
	SE_EDITOR_ARRAY_POP_ALL(&editor->items);
	SE_EDITOR_ARRAY_POP_ALL(&editor->properties);
}

static void se_editor_queue_clear(se_editor* editor) {
	if (!editor) {
		return;
	}
	SE_EDITOR_ARRAY_POP_ALL(&editor->queue);
}

static void se_editor_shortcut_events_clear(se_editor* editor) {
	if (!editor) {
		return;
	}
	SE_EDITOR_ARRAY_POP_ALL(&editor->shortcut_events);
}

static b8 se_editor_shortcut_key_valid(se_key key) {
	return key >= 0 && key < SE_KEY_COUNT;
}

static b8 se_editor_shortcut_key_modifier(se_key key) {
	return key == SE_KEY_LEFT_SHIFT || key == SE_KEY_RIGHT_SHIFT ||
		key == SE_KEY_LEFT_CONTROL || key == SE_KEY_RIGHT_CONTROL ||
		key == SE_KEY_LEFT_ALT || key == SE_KEY_RIGHT_ALT ||
		key == SE_KEY_LEFT_SUPER || key == SE_KEY_RIGHT_SUPER;
}

static b8 se_editor_shortcut_modifiers_down(se_window_handle window, u32 modifiers) {
	u32 active = 0u;
	if (se_window_is_key_down(window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(window, SE_KEY_RIGHT_SHIFT)) {
		active |= SE_EDITOR_SHORTCUT_MOD_SHIFT;
	}
	if (se_window_is_key_down(window, SE_KEY_LEFT_CONTROL) || se_window_is_key_down(window, SE_KEY_RIGHT_CONTROL)) {
		active |= SE_EDITOR_SHORTCUT_MOD_CONTROL;
	}
	if (se_window_is_key_down(window, SE_KEY_LEFT_ALT) || se_window_is_key_down(window, SE_KEY_RIGHT_ALT)) {
		active |= SE_EDITOR_SHORTCUT_MOD_ALT;
	}
	if (se_window_is_key_down(window, SE_KEY_LEFT_SUPER) || se_window_is_key_down(window, SE_KEY_RIGHT_SUPER)) {
		active |= SE_EDITOR_SHORTCUT_MOD_SUPER;
	}
	return active == modifiers;
}

static b8 se_editor_shortcut_repeat_matches(const se_editor* editor, const se_editor_shortcut* shortcut) {
	return editor && shortcut && editor->shortcut_repeat_active &&
		editor->shortcut_repeat_key == shortcut->keys[0] &&
		editor->shortcut_repeat_modifiers == shortcut->modifiers &&
		editor->shortcut_repeat_mode == shortcut->mode &&
		strcmp(editor->shortcut_repeat_action, shortcut->action) == 0;
}

static void se_editor_shortcut_repeat_clear(se_editor* editor) {
	if (!editor) return;
	editor->shortcut_repeat_key = SE_KEY_UNKNOWN;
	editor->shortcut_repeat_modifiers = SE_EDITOR_SHORTCUT_MOD_NONE;
	editor->shortcut_repeat_mode = SE_EDITOR_MODE_NORMAL;
	editor->shortcut_repeat_next_time = 0.0;
	editor->shortcut_repeat_action[0] = '\0';
	editor->shortcut_repeat_active = false;
}

static void se_editor_shortcut_repeat_set(se_editor* editor, const se_editor_shortcut* shortcut, f64 next_time) {
	if (!editor || !shortcut || shortcut->key_count == 0u) return;
	editor->shortcut_repeat_key = shortcut->keys[0];
	editor->shortcut_repeat_modifiers = shortcut->modifiers;
	editor->shortcut_repeat_mode = shortcut->mode;
	editor->shortcut_repeat_next_time = next_time;
	se_editor_copy_text(editor->shortcut_repeat_action, sizeof(editor->shortcut_repeat_action), shortcut->action);
	editor->shortcut_repeat_active = true;
}

static b8 se_editor_shortcut_repeat_ready(se_editor* editor, const se_editor_shortcut* shortcut) {
	f64 now = 0.0;
	if (!editor || !shortcut || shortcut->key_count != 1u || !shortcut->repeat) return false;
	now = se_window_get_time(editor->window);
	if (!se_window_is_key_down(editor->window, shortcut->keys[0])) {
		if (se_editor_shortcut_repeat_matches(editor, shortcut)) {
			se_editor_shortcut_repeat_clear(editor);
		}
		return false;
	}
	if (se_window_is_key_pressed(editor->window, shortcut->keys[0]) ||
		!se_editor_shortcut_repeat_matches(editor, shortcut)) {
		se_editor_shortcut_repeat_set(editor, shortcut, now + SE_EDITOR_SHORTCUT_REPEAT_DELAY);
		return se_window_is_key_pressed(editor->window, shortcut->keys[0]);
	}
	if (now >= editor->shortcut_repeat_next_time) {
		se_editor_shortcut_repeat_set(editor, shortcut, now + SE_EDITOR_SHORTCUT_REPEAT_INTERVAL);
		return true;
	}
	return false;
}

static se_key se_editor_shortcut_pressed_key(se_window_handle window) {
	for (i32 key = 0; key < SE_KEY_COUNT; ++key) {
		const se_key typed_key = (se_key)key;
		if (se_editor_shortcut_key_modifier(typed_key)) {
			continue;
		}
		if (se_window_is_key_pressed(window, typed_key)) {
			return typed_key;
		}
	}
	return SE_KEY_UNKNOWN;
}

static b8 se_editor_shortcut_parse_key_name(const c8* name, se_key* out_key) {
	typedef struct {
		const c8* name;
		se_key key;
	} se_editor_key_alias;
	c8 normalized[SE_MAX_NAME_LENGTH] = {0};
	static const se_editor_key_alias aliases[] = {
		{ "space", SE_KEY_SPACE },
		{ "tab", SE_KEY_TAB },
		{ "enter", SE_KEY_ENTER },
		{ "return", SE_KEY_ENTER },
		{ "escape", SE_KEY_ESCAPE },
		{ "esc", SE_KEY_ESCAPE },
		{ "backspace", SE_KEY_BACKSPACE },
		{ "insert", SE_KEY_INSERT },
		{ "ins", SE_KEY_INSERT },
		{ "delete", SE_KEY_DELETE },
		{ "del", SE_KEY_DELETE },
		{ "right", SE_KEY_RIGHT },
		{ "left", SE_KEY_LEFT },
		{ "down", SE_KEY_DOWN },
		{ "up", SE_KEY_UP },
		{ "pageup", SE_KEY_PAGE_UP },
		{ "pagedown", SE_KEY_PAGE_DOWN },
		{ "home", SE_KEY_HOME },
		{ "end", SE_KEY_END },
		{ "slash", SE_KEY_SLASH },
		{ "backslash", SE_KEY_BACKSLASH },
		{ "comma", SE_KEY_COMMA },
		{ "period", SE_KEY_PERIOD },
		{ "dot", SE_KEY_PERIOD },
		{ "minus", SE_KEY_MINUS },
		{ "equal", SE_KEY_EQUAL },
		{ "semicolon", SE_KEY_SEMICOLON },
		{ "apostrophe", SE_KEY_APOSTROPHE },
		{ "quote", SE_KEY_APOSTROPHE },
		{ "grave", SE_KEY_GRAVE_ACCENT },
		{ "backtick", SE_KEY_GRAVE_ACCENT }
	};
	if (!name || !out_key) {
		return false;
	}
	se_editor_normalize_name(name, normalized, sizeof(normalized));
	if (normalized[0] == '\0') {
		return false;
	}
	if (normalized[1] == '\0') {
		if (normalized[0] >= 'a' && normalized[0] <= 'z') {
			*out_key = (se_key)(SE_KEY_A + (normalized[0] - 'a'));
			return true;
		}
		if (normalized[0] >= '0' && normalized[0] <= '9') {
			*out_key = (se_key)(SE_KEY_0 + (normalized[0] - '0'));
			return true;
		}
	}
	if (normalized[0] == 'f' && normalized[1] >= '1' && normalized[1] <= '9') {
		i32 number = 0;
		for (u32 i = 1u; normalized[i] != '\0'; ++i) {
			if (normalized[i] < '0' || normalized[i] > '9') {
				number = 0;
				break;
			}
			number = number * 10 + (normalized[i] - '0');
		}
		if (number >= 1 && number <= 25) {
			*out_key = (se_key)(SE_KEY_F1 + number - 1);
			return true;
		}
	}
	for (u32 i = 0u; i < (u32)(sizeof(aliases) / sizeof(aliases[0])); ++i) {
		if (strcmp(normalized, aliases[i].name) == 0) {
			*out_key = aliases[i].key;
			return true;
		}
	}
	return false;
}

static b8 se_editor_shortcut_parse_token(const c8* token, se_key* out_key, u32* out_modifiers) {
	c8 part[SE_MAX_NAME_LENGTH] = {0};
	sz part_index = 0u;
	se_key key = SE_KEY_UNKNOWN;
	u32 modifiers = 0u;
	if (!token || !out_key || !out_modifiers) {
		return false;
	}
	for (sz i = 0u;; ++i) {
		const c8 c = token[i];
		if (c == '+' || c == '\0') {
			c8 normalized[SE_MAX_NAME_LENGTH] = {0};
			part[part_index] = '\0';
			se_editor_normalize_name(part, normalized, sizeof(normalized));
			if (strcmp(normalized, "shift") == 0) {
				modifiers |= SE_EDITOR_SHORTCUT_MOD_SHIFT;
			} else if (strcmp(normalized, "ctrl") == 0 || strcmp(normalized, "control") == 0) {
				modifiers |= SE_EDITOR_SHORTCUT_MOD_CONTROL;
			} else if (strcmp(normalized, "alt") == 0) {
				modifiers |= SE_EDITOR_SHORTCUT_MOD_ALT;
			} else if (strcmp(normalized, "super") == 0 || strcmp(normalized, "cmd") == 0 || strcmp(normalized, "meta") == 0) {
				modifiers |= SE_EDITOR_SHORTCUT_MOD_SUPER;
			} else if (normalized[0] != '\0') {
				if (key != SE_KEY_UNKNOWN || !se_editor_shortcut_parse_key_name(part, &key)) {
					return false;
				}
			}
			part_index = 0u;
			if (c == '\0') {
				break;
			}
			continue;
		}
		if (part_index + 1u >= sizeof(part)) {
			return false;
		}
		part[part_index++] = c;
	}
	if (key == SE_KEY_UNKNOWN) {
		return false;
	}
	*out_key = key;
	*out_modifiers = modifiers;
	return true;
}

static b8 se_editor_shortcut_add_event(se_editor* editor, const c8* action) {
	se_editor_shortcut_event event = {0};
	if (!editor || !action || action[0] == '\0') {
		return false;
	}
	event.mode = editor->mode;
	se_editor_copy_text(event.action, sizeof(event.action), action);
	s_array_add(&editor->shortcut_events, event);
	if (strcmp(action, "normal_mode") == 0) {
		editor->mode = SE_EDITOR_MODE_NORMAL;
	} else if (strcmp(action, "insert_mode") == 0) {
		editor->mode = SE_EDITOR_MODE_INSERT;
	} else if (strcmp(action, "command_mode") == 0) {
		editor->mode = SE_EDITOR_MODE_COMMAND;
	}
	return true;
}

static b8 se_editor_shortcut_sequence_matches(const se_editor_shortcut* shortcut, const se_key* keys, u32 key_count) {
	if (!shortcut || !keys || key_count == 0u || shortcut->key_count < key_count) {
		return false;
	}
	for (u32 i = 0u; i < key_count; ++i) {
		if (shortcut->keys[i] != keys[i]) {
			return false;
		}
	}
	return true;
}

static void se_editor_shortcut_sequence_push(se_editor* editor, se_key key) {
	if (!editor || !se_editor_shortcut_key_valid(key)) {
		return;
	}
	if (editor->shortcut_sequence_count >= SE_EDITOR_SHORTCUT_MAX_KEYS) {
		for (u32 i = 1u; i < SE_EDITOR_SHORTCUT_MAX_KEYS; ++i) {
			editor->shortcut_sequence[i - 1u] = editor->shortcut_sequence[i];
		}
		editor->shortcut_sequence_count = SE_EDITOR_SHORTCUT_MAX_KEYS - 1u;
	}
	editor->shortcut_sequence[editor->shortcut_sequence_count++] = key;
}

static b8 se_editor_shortcut_process_sequence(se_editor* editor) {
	b8 matched = false;
	b8 prefix = false;
	if (!editor || editor->shortcut_sequence_count == 0u) {
		return false;
	}
	for (sz i = 0u; i < s_array_get_size(&editor->shortcuts); ++i) {
		const s_handle handle = s_array_handle(&editor->shortcuts, (u32)i);
		const se_editor_shortcut* shortcut = s_array_get(&editor->shortcuts, handle);
		if (!shortcut || shortcut->mode != editor->mode || shortcut->key_count <= 1u) {
			continue;
		}
		if (!se_editor_shortcut_modifiers_down(editor->window, shortcut->modifiers)) {
			continue;
		}
		if (!se_editor_shortcut_sequence_matches(shortcut, editor->shortcut_sequence, editor->shortcut_sequence_count)) {
			continue;
		}
		if (shortcut->key_count == editor->shortcut_sequence_count) {
			se_editor_shortcut_add_event(editor, shortcut->action);
			matched = true;
		} else {
			prefix = true;
		}
	}
	if (matched) {
		editor->shortcut_sequence_count = 0u;
		return true;
	}
	return prefix;
}

static void se_editor_push_item(
	se_editor_items* items,
	se_editor_category category,
	s_handle handle,
	s_handle owner_handle,
	void* pointer,
	void* owner_pointer,
	u32 index,
	const c8* label) {
	se_editor_item item = {0};
	if (!items) {
		return;
	}
	item.category = category;
	item.handle = handle;
	item.owner_handle = owner_handle;
	item.pointer = pointer;
	item.owner_pointer = owner_pointer;
	item.index = index;
	se_editor_copy_text(item.label, sizeof(item.label), label);
	s_array_add(items, item);
}

static void se_editor_add_property_internal(se_editor_properties* properties, const se_editor_property_desc* desc) {
	se_editor_property property = {0};
	if (!properties || !desc || !desc->name || desc->name[0] == '\0') {
		return;
	}
	se_editor_copy_text(property.name, sizeof(property.name), desc->name);
	se_editor_copy_text(property.label, sizeof(property.label),
		(desc->label && desc->label[0] != '\0') ? desc->label : desc->name);
	property.value = desc->value;
	property.role = desc->role;
	property.step = desc->step > 0.0f ? desc->step : 0.0f;
	property.large_step = desc->large_step > 0.0f ? desc->large_step : 0.0f;
	property.editable = desc->editable;
	s_array_add(properties, property);
}

static se_editor_command se_editor_command_make_internal(se_editor_command_type type, const se_editor_item* item, const c8* name, const se_editor_value* value, const se_editor_value* aux_value) {
	se_editor_command command = {0};
	command.type = type;
	if (item) {
		command.item = *item;
	}
	if (name) {
		se_editor_copy_text(command.name, sizeof(command.name), name);
	}
	command.value = value ? *value : se_editor_value_none();
	command.aux_value = aux_value ? *aux_value : se_editor_value_none();
	return command;
}

static b8 se_editor_item_has_selector(const se_editor_item* item) {
	if (!item) {
		return false;
	}
	return item->handle != S_HANDLE_NULL ||
		item->owner_handle != S_HANDLE_NULL ||
		item->pointer != NULL ||
		item->owner_pointer != NULL ||
		item->label[0] != '\0' ||
		item->index != 0u;
}

static b8 se_editor_item_has_identity_selector(const se_editor_item* item) {
	if (!item) {
		return false;
	}
	return item->handle != S_HANDLE_NULL ||
		item->pointer != NULL ||
		item->label[0] != '\0' ||
		item->index != 0u;
}

static b8 se_editor_item_matches_selector(const se_editor_item* candidate, const se_editor_item* selector) {
	if (!candidate || !selector || candidate->category != selector->category) {
		return false;
	}
	if (selector->handle != S_HANDLE_NULL && candidate->handle != selector->handle) {
		return false;
	}
	if (selector->owner_handle != S_HANDLE_NULL && candidate->owner_handle != selector->owner_handle) {
		return false;
	}
	if (selector->pointer && candidate->pointer != selector->pointer) {
		return false;
	}
	if (selector->owner_pointer && candidate->owner_pointer != selector->owner_pointer) {
		return false;
	}
	if (selector->label[0] != '\0' && strcmp(candidate->label, selector->label) != 0) {
		return false;
	}
	if (selector->index != 0u && candidate->index != selector->index) {
		return false;
	}
	return true;
}

static b8 se_editor_item_matches_identity_selector(const se_editor_item* candidate, const se_editor_item* selector) {
	if (!candidate || !selector || candidate->category != selector->category) {
		return false;
	}
	if (selector->handle != S_HANDLE_NULL && candidate->handle != selector->handle) {
		return false;
	}
	if (selector->pointer && candidate->pointer != selector->pointer) {
		return false;
	}
	if (selector->label[0] != '\0' && strcmp(candidate->label, selector->label) != 0) {
		return false;
	}
	if (selector->index != 0u && candidate->index != selector->index) {
		return false;
	}
	return true;
}

b8 se_editor_add_item(
	se_editor* editor,
	se_editor_category category,
	s_handle handle,
	s_handle owner_handle,
	void* pointer,
	void* owner_pointer,
	u32 index,
	const c8* label
) {
	if (!editor || (u32)category >= (u32)SE_EDITOR_CATEGORY_COUNT || !label || label[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_editor_push_item(&editor->items, category, handle, owner_handle, pointer, owner_pointer, index, label);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_add_property(se_editor* editor, const se_editor_property_desc* desc) {
	if (!editor || !desc || !desc->name || desc->name[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_editor_add_property_internal(&editor->properties, desc);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_add_property_value(se_editor* editor, const c8* name, const se_editor_value* value, b8 editable) {
	se_editor_property_desc desc = SE_EDITOR_PROPERTY_DESC_DEFAULTS;
	if (!editor || !name || name[0] == '\0' || !value) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	desc.name = name;
	desc.label = name;
	desc.value = *value;
	desc.editable = editable;
	return se_editor_add_property(editor, &desc);
}

u32 se_editor_property_component_count(const se_editor_property* property) {
	if (!property) {
		return 0u;
	}
	switch (property->value.type) {
		case SE_EDITOR_VALUE_BOOL:
		case SE_EDITOR_VALUE_INT:
		case SE_EDITOR_VALUE_UINT:
		case SE_EDITOR_VALUE_U64:
		case SE_EDITOR_VALUE_FLOAT:
		case SE_EDITOR_VALUE_DOUBLE:
		case SE_EDITOR_VALUE_HANDLE:
		case SE_EDITOR_VALUE_POINTER:
		case SE_EDITOR_VALUE_TEXT:
			return 1u;
		case SE_EDITOR_VALUE_VEC2:
			return 2u;
		case SE_EDITOR_VALUE_VEC3:
			return 3u;
		case SE_EDITOR_VALUE_VEC4:
			return 4u;
		default:
			return 0u;
	}
}

const c8* se_editor_property_component_name(const se_editor_property* property, u32 component) {
	static const c8* scalar_name = "value";
	static const c8* vector_names[] = { "x", "y", "z", "w" };
	static const c8* color_names[] = { "r", "g", "b", "a" };
	const u32 count = se_editor_property_component_count(property);
	if (!property || count == 0u || component >= count) {
		return NULL;
	}
	if (count == 1u) {
		return scalar_name;
	}
	if (property->role == SE_EDITOR_PROPERTY_ROLE_COLOR) {
		return color_names[component];
	}
	return vector_names[component];
}

se_editor_value se_editor_value_none(void) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_NONE;
	return value;
}

se_editor_value se_editor_value_bool(b8 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_BOOL;
	value.bool_value = data;
	return value;
}

se_editor_value se_editor_value_int(i32 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_INT;
	value.int_value = data;
	return value;
}

se_editor_value se_editor_value_uint(u32 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_UINT;
	value.uint_value = data;
	return value;
}

se_editor_value se_editor_value_u64(u64 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_U64;
	value.u64_value = data;
	return value;
}

se_editor_value se_editor_value_float(f32 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_FLOAT;
	value.float_value = data;
	return value;
}

se_editor_value se_editor_value_double(f64 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_DOUBLE;
	value.double_value = data;
	return value;
}

se_editor_value se_editor_value_vec2(s_vec2 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_VEC2;
	value.vec2_value = data;
	return value;
}

se_editor_value se_editor_value_vec3(s_vec3 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_VEC3;
	value.vec3_value = data;
	return value;
}

se_editor_value se_editor_value_vec4(s_vec4 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_VEC4;
	value.vec4_value = data;
	return value;
}

se_editor_value se_editor_value_mat3(s_mat3 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_MAT3;
	value.mat3_value = data;
	return value;
}

se_editor_value se_editor_value_mat4(s_mat4 data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_MAT4;
	value.mat4_value = data;
	return value;
}

se_editor_value se_editor_value_handle(s_handle data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_HANDLE;
	value.handle_value = data;
	return value;
}

se_editor_value se_editor_value_pointer(void* data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_POINTER;
	value.pointer_value = data;
	return value;
}

se_editor_value se_editor_value_text(const c8* data) {
	se_editor_value value = {0};
	value.type = SE_EDITOR_VALUE_TEXT;
	se_editor_copy_text(value.text_value, sizeof(value.text_value), data ? data : "");
	return value;
}

b8 se_editor_value_as_bool(const se_editor_value* value, b8* out_value) {
	if (!value || !out_value) {
		return false;
	}
	switch (value->type) {
		case SE_EDITOR_VALUE_BOOL:
			*out_value = value->bool_value;
			return true;
		case SE_EDITOR_VALUE_INT:
			*out_value = value->int_value != 0;
			return true;
		case SE_EDITOR_VALUE_UINT:
			*out_value = value->uint_value != 0u;
			return true;
		case SE_EDITOR_VALUE_U64:
			*out_value = value->u64_value != 0u;
			return true;
		case SE_EDITOR_VALUE_FLOAT:
			*out_value = value->float_value != 0.0f;
			return true;
		case SE_EDITOR_VALUE_DOUBLE:
			*out_value = value->double_value != 0.0;
			return true;
		default:
			return false;
	}
}

b8 se_editor_value_as_i32(const se_editor_value* value, i32* out_value) {
	if (!value || !out_value) {
		return false;
	}
	switch (value->type) {
		case SE_EDITOR_VALUE_INT:
			*out_value = value->int_value;
			return true;
		case SE_EDITOR_VALUE_UINT:
			*out_value = (i32)value->uint_value;
			return true;
		case SE_EDITOR_VALUE_U64:
			*out_value = (i32)value->u64_value;
			return true;
		case SE_EDITOR_VALUE_BOOL:
			*out_value = value->bool_value ? 1 : 0;
			return true;
		case SE_EDITOR_VALUE_FLOAT:
			*out_value = (i32)value->float_value;
			return true;
		case SE_EDITOR_VALUE_DOUBLE:
			*out_value = (i32)value->double_value;
			return true;
		default:
			return false;
	}
}

b8 se_editor_value_as_u32(const se_editor_value* value, u32* out_value) {
	i32 i32_value = 0;
	if (!value || !out_value) {
		return false;
	}
	if (value->type == SE_EDITOR_VALUE_HANDLE) {
		*out_value = (u32)value->handle_value;
		return true;
	}
	if (!se_editor_value_as_i32(value, &i32_value)) {
		return false;
	}
	*out_value = (u32)s_max(i32_value, 0);
	return true;
}

b8 se_editor_value_as_u64(const se_editor_value* value, u64* out_value) {
	if (!value || !out_value) {
		return false;
	}
	switch (value->type) {
		case SE_EDITOR_VALUE_U64:
			*out_value = value->u64_value;
			return true;
		case SE_EDITOR_VALUE_UINT:
			*out_value = value->uint_value;
			return true;
		case SE_EDITOR_VALUE_INT:
			*out_value = (u64)s_max(value->int_value, 0);
			return true;
		case SE_EDITOR_VALUE_BOOL:
			*out_value = value->bool_value ? 1u : 0u;
			return true;
		case SE_EDITOR_VALUE_HANDLE:
			*out_value = (u64)value->handle_value;
			return true;
		default:
			return false;
	}
}

b8 se_editor_value_as_f32(const se_editor_value* value, f32* out_value) {
	if (!value || !out_value) {
		return false;
	}
	switch (value->type) {
		case SE_EDITOR_VALUE_FLOAT:
			*out_value = value->float_value;
			return true;
		case SE_EDITOR_VALUE_DOUBLE:
			*out_value = (f32)value->double_value;
			return true;
		case SE_EDITOR_VALUE_INT:
			*out_value = (f32)value->int_value;
			return true;
		case SE_EDITOR_VALUE_UINT:
			*out_value = (f32)value->uint_value;
			return true;
		case SE_EDITOR_VALUE_U64:
			*out_value = (f32)value->u64_value;
			return true;
		default:
			return false;
	}
}

b8 se_editor_value_as_f64(const se_editor_value* value, f64* out_value) {
	if (!value || !out_value) {
		return false;
	}
	switch (value->type) {
		case SE_EDITOR_VALUE_DOUBLE:
			*out_value = value->double_value;
			return true;
		case SE_EDITOR_VALUE_FLOAT:
			*out_value = value->float_value;
			return true;
		case SE_EDITOR_VALUE_INT:
			*out_value = (f64)value->int_value;
			return true;
		case SE_EDITOR_VALUE_UINT:
			*out_value = (f64)value->uint_value;
			return true;
		case SE_EDITOR_VALUE_U64:
			*out_value = (f64)value->u64_value;
			return true;
		default:
			return false;
	}
}

b8 se_editor_value_as_handle(const se_editor_value* value, s_handle* out_value) {
	u64 u64_value = 0u;
	if (!value || !out_value) {
		return false;
	}
	if (value->type == SE_EDITOR_VALUE_HANDLE) {
		*out_value = value->handle_value;
		return true;
	}
	if (!se_editor_value_as_u64(value, &u64_value)) {
		return false;
	}
	*out_value = (s_handle)u64_value;
	return true;
}

b8 se_editor_value_as_vec2(const se_editor_value* value, s_vec2* out_value) {
	if (!value || !out_value || value->type != SE_EDITOR_VALUE_VEC2) {
		return false;
	}
	*out_value = value->vec2_value;
	return true;
}

b8 se_editor_value_as_vec3(const se_editor_value* value, s_vec3* out_value) {
	if (!value || !out_value || value->type != SE_EDITOR_VALUE_VEC3) {
		return false;
	}
	*out_value = value->vec3_value;
	return true;
}

b8 se_editor_value_as_vec4(const se_editor_value* value, s_vec4* out_value) {
	if (!value || !out_value || value->type != SE_EDITOR_VALUE_VEC4) {
		return false;
	}
	*out_value = value->vec4_value;
	return true;
}

b8 se_editor_value_as_mat3(const se_editor_value* value, s_mat3* out_value) {
	if (!value || !out_value || value->type != SE_EDITOR_VALUE_MAT3) {
		return false;
	}
	*out_value = value->mat3_value;
	return true;
}

b8 se_editor_value_as_mat4(const se_editor_value* value, s_mat4* out_value) {
	if (!value || !out_value || value->type != SE_EDITOR_VALUE_MAT4) {
		return false;
	}
	*out_value = value->mat4_value;
	return true;
}

const c8* se_editor_value_as_text(const se_editor_value* value) {
	if (!value || value->type != SE_EDITOR_VALUE_TEXT) {
		return NULL;
	}
	return value->text_value;
}

static u32 se_editor_delta_units(const f32 delta) {
	const f32 abs_delta = fabsf(delta);
	u32 amount = (u32)(abs_delta + 0.5f);
	if (amount == 0u && abs_delta > 0.0f) {
		amount = 1u;
	}
	return amount;
}

b8 se_editor_value_adjust_component(se_editor_value* value, u32 component, f32 delta) {
	if (!value) {
		return false;
	}
	switch (value->type) {
		case SE_EDITOR_VALUE_INT: {
			const i32 amount = (i32)se_editor_delta_units(delta);
			if (component != 0u) {
				return false;
			}
			if (delta < 0.0f) {
				value->int_value -= amount;
			} else if (delta > 0.0f) {
				value->int_value += amount;
			}
			return true;
		}
		case SE_EDITOR_VALUE_UINT: {
			const u32 amount = se_editor_delta_units(delta);
			if (component != 0u) {
				return false;
			}
			if (delta < 0.0f) {
				value->uint_value = amount >= value->uint_value ? 0u : value->uint_value - amount;
			} else if (delta > 0.0f) {
				value->uint_value += amount;
			}
			return true;
		}
		case SE_EDITOR_VALUE_U64: {
			const u64 amount = (u64)se_editor_delta_units(delta);
			if (component != 0u) {
				return false;
			}
			if (delta < 0.0f) {
				value->u64_value = amount >= value->u64_value ? 0u : value->u64_value - amount;
			} else if (delta > 0.0f) {
				value->u64_value += amount;
			}
			return true;
		}
		case SE_EDITOR_VALUE_FLOAT:
			if (component != 0u) {
				return false;
			}
			value->float_value += delta;
			return true;
		case SE_EDITOR_VALUE_DOUBLE:
			if (component != 0u) {
				return false;
			}
			value->double_value += delta;
			return true;
		case SE_EDITOR_VALUE_VEC2:
			if (component >= 2u) {
				return false;
			}
			if (component == 0u) value->vec2_value.x += delta;
			else value->vec2_value.y += delta;
			return true;
		case SE_EDITOR_VALUE_VEC3:
			if (component >= 3u) {
				return false;
			}
			if (component == 0u) value->vec3_value.x += delta;
			else if (component == 1u) value->vec3_value.y += delta;
			else value->vec3_value.z += delta;
			return true;
		case SE_EDITOR_VALUE_VEC4:
			if (component >= 4u) {
				return false;
			}
			if (component == 0u) value->vec4_value.x += delta;
			else if (component == 1u) value->vec4_value.y += delta;
			else if (component == 2u) value->vec4_value.z += delta;
			else value->vec4_value.w += delta;
			return true;
		default:
			return false;
	}
}

b8 se_editor_json_write_file(const c8* path, s_json* root) {
	c8* text = NULL;
	b8 ok = false;
	if (!path || path[0] == '\0' || !root) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	text = s_json_stringify(root);
	if (!text) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return false;
	}
	ok = s_file_write(path, text, strlen(text));
	free(text);
	if (!ok) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_json_read_file(const c8* path, s_json** out_root) {
	c8* text = NULL;
	s_json* root = NULL;
	if (!path || path[0] == '\0' || !out_root) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_root = NULL;
	if (!s_file_read(path, &text, NULL)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	root = s_json_parse(text);
	free(text);
	if (!root) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	*out_root = root;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

se_editor_command se_editor_command_make(se_editor_command_type type, const se_editor_item* item, const c8* name, const se_editor_value* value, const se_editor_value* aux_value) {
	return se_editor_command_make_internal(type, item, name, value, aux_value);
}

se_editor_command se_editor_command_make_set(const se_editor_item* item, const c8* name, const se_editor_value* value) {
	return se_editor_command_make_internal(SE_EDITOR_COMMAND_SET, item, name, value, NULL);
}

se_editor_command se_editor_command_make_toggle(const se_editor_item* item, const c8* name) {
	return se_editor_command_make_internal(SE_EDITOR_COMMAND_TOGGLE, item, name, NULL, NULL);
}

se_editor_command se_editor_command_make_action(const se_editor_item* item, const c8* name, const se_editor_value* value, const se_editor_value* aux_value) {
	return se_editor_command_make_internal(SE_EDITOR_COMMAND_ACTION, item, name, value, aux_value);
}

const c8* se_editor_category_name(se_editor_category category) {
	switch (category) {
		case SE_EDITOR_CATEGORY_ITEM: return SE_EDITOR_NAME_ITEM;
		case SE_EDITOR_CATEGORY_COUNT:
		default: return SE_EDITOR_NAME_UNKNOWN;
	}
}

se_editor_category se_editor_category_from_name(const c8* name) {
	c8 normalized_name[SE_MAX_NAME_LENGTH] = {0};
	if (!name || name[0] == '\0') {
		return SE_EDITOR_CATEGORY_COUNT;
	}
	se_editor_normalize_name(name, normalized_name, sizeof(normalized_name));
	if (strcmp(normalized_name, SE_EDITOR_NAME_ITEM) == 0) {
		return SE_EDITOR_CATEGORY_ITEM;
	}
	return SE_EDITOR_CATEGORY_COUNT;
}

const c8* se_editor_value_type_name(se_editor_value_type type) {
	switch (type) {
		case SE_EDITOR_VALUE_NONE: return SE_EDITOR_NAME_NONE;
		case SE_EDITOR_VALUE_BOOL: return SE_EDITOR_NAME_BOOL;
		case SE_EDITOR_VALUE_INT: return SE_EDITOR_NAME_INT;
		case SE_EDITOR_VALUE_UINT: return SE_EDITOR_NAME_UINT;
		case SE_EDITOR_VALUE_U64: return SE_EDITOR_NAME_U64;
		case SE_EDITOR_VALUE_FLOAT: return SE_EDITOR_NAME_FLOAT;
		case SE_EDITOR_VALUE_DOUBLE: return SE_EDITOR_NAME_DOUBLE;
		case SE_EDITOR_VALUE_VEC2: return SE_EDITOR_NAME_VEC2;
		case SE_EDITOR_VALUE_VEC3: return SE_EDITOR_NAME_VEC3;
		case SE_EDITOR_VALUE_VEC4: return SE_EDITOR_NAME_VEC4;
		case SE_EDITOR_VALUE_MAT3: return SE_EDITOR_NAME_MAT3;
		case SE_EDITOR_VALUE_MAT4: return SE_EDITOR_NAME_MAT4;
		case SE_EDITOR_VALUE_HANDLE: return SE_EDITOR_NAME_HANDLE;
		case SE_EDITOR_VALUE_POINTER: return SE_EDITOR_NAME_POINTER;
		case SE_EDITOR_VALUE_TEXT: return SE_EDITOR_NAME_TEXT;
		default: return SE_EDITOR_NAME_UNKNOWN;
	}
}

const c8* se_editor_mode_name(se_editor_mode mode) {
	switch (mode) {
		case SE_EDITOR_MODE_NORMAL: return SE_EDITOR_NAME_NORMAL;
		case SE_EDITOR_MODE_INSERT: return SE_EDITOR_NAME_INSERT;
		case SE_EDITOR_MODE_COMMAND: return SE_EDITOR_NAME_COMMAND;
		case SE_EDITOR_MODE_COUNT:
		default: return SE_EDITOR_NAME_UNKNOWN;
	}
}

se_editor* se_editor_create(const se_editor_config* config) {
	se_editor* editor = calloc(1, sizeof(*editor));
	se_editor_config cfg = SE_EDITOR_CONFIG_DEFAULTS;
	if (!editor) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	if (config) {
		cfg = *config;
	}
	s_array_init(&editor->items);
	s_array_init(&editor->properties);
	s_array_init(&editor->queue);
	s_array_init(&editor->shortcuts);
	s_array_init(&editor->shortcut_events);
	editor->window = cfg.window;
	editor->user_data = cfg.user_data;
	editor->collect_items = cfg.collect_items;
	editor->collect_properties = cfg.collect_properties;
	editor->apply_command = cfg.apply_command;
	editor->mode = SE_EDITOR_MODE_NORMAL;
	se_editor_shortcut_repeat_clear(editor);
	se_set_last_error(SE_RESULT_OK);
	return editor;
}

void se_editor_destroy(se_editor* editor) {
	if (!editor) {
		return;
	}
	s_array_clear(&editor->shortcut_events);
	s_array_clear(&editor->shortcuts);
	s_array_clear(&editor->queue);
	s_array_clear(&editor->properties);
	s_array_clear(&editor->items);
	free(editor);
}

void se_editor_reset(se_editor* editor) {
	if (!editor) {
		return;
	}
	se_editor_queue_clear(editor);
	se_editor_clear_runtime_arrays(editor);
	SE_EDITOR_ARRAY_POP_ALL(&editor->shortcut_events);
	SE_EDITOR_ARRAY_POP_ALL(&editor->shortcuts);
	editor->mode = SE_EDITOR_MODE_NORMAL;
	editor->shortcut_sequence_count = 0u;
	se_editor_shortcut_repeat_clear(editor);
}

void se_editor_set_window(se_editor* editor, se_window_handle window) {
	if (!editor) {
		return;
	}
	editor->window = window;
	editor->shortcut_sequence_count = 0u;
	se_editor_shortcut_repeat_clear(editor);
}

se_window_handle se_editor_get_window(const se_editor* editor) {
	if (!editor) {
		return S_HANDLE_NULL;
	}
	return editor->window;
}

void se_editor_set_user_data(se_editor* editor, void* user_data) {
	if (!editor) {
		return;
	}
	editor->user_data = user_data;
}

void* se_editor_get_user_data(const se_editor* editor) {
	if (!editor) {
		return NULL;
	}
	return editor->user_data;
}

void se_editor_set_collect_items(se_editor* editor, se_editor_collect_items_fn fn) {
	if (!editor) {
		return;
	}
	editor->collect_items = fn;
}

void se_editor_set_collect_properties(se_editor* editor, se_editor_collect_properties_fn fn) {
	if (!editor) {
		return;
	}
	editor->collect_properties = fn;
}

void se_editor_set_apply_command(se_editor* editor, se_editor_apply_command_fn fn) {
	if (!editor) {
		return;
	}
	editor->apply_command = fn;
}

b8 se_editor_collect_counts(se_editor* editor, se_editor_counts* out_counts) {
	const se_editor_item* items = NULL;
	sz item_count = 0u;
	if (!editor || !out_counts) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	memset(out_counts, 0, sizeof(*out_counts));
	if (!se_editor_collect_items(editor, &items, &item_count)) {
		return false;
	}
	(void)items;
	out_counts->items = (u32)item_count;
	out_counts->queued_commands = (u32)s_array_get_size(&editor->queue);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_collect_items(se_editor* editor, const se_editor_item** out_items, sz* out_count) {
	if (!editor || !out_items || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_editor_clear_runtime_arrays(editor);
	if (editor->collect_items && !editor->collect_items(editor, editor->user_data)) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
		}
		return false;
	}
	*out_items = s_array_get_data(&editor->items);
	*out_count = s_array_get_size(&editor->items);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_collect_properties(se_editor* editor, const se_editor_item* item, const se_editor_property** out_properties, sz* out_count) {
	if (!editor || !item || !out_properties || !out_count || (u32)item->category >= (u32)SE_EDITOR_CATEGORY_COUNT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	SE_EDITOR_ARRAY_POP_ALL(&editor->properties);
	if (!editor->collect_properties || !editor->collect_properties(editor, item, editor->user_data)) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
		}
		return false;
	}
	*out_properties = s_array_get_data(&editor->properties);
	*out_count = s_array_get_size(&editor->properties);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_validate_item(se_editor* editor, const se_editor_item* item) {
	const se_editor_item* items = NULL;
	sz item_count = 0u;
	b8 should_retry_without_owner = false;
	if (!editor || !item || (u32)item->category >= (u32)SE_EDITOR_CATEGORY_COUNT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_editor_collect_items(editor, &items, &item_count)) {
		return false;
	}
	if (!items || item_count == 0u) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (!se_editor_item_has_selector(item)) {
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	should_retry_without_owner =
		(item->owner_handle != S_HANDLE_NULL || item->owner_pointer != NULL) &&
		se_editor_item_has_identity_selector(item);
	for (sz i = 0; i < item_count; ++i) {
		if (se_editor_item_matches_selector(&items[i], item)) {
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	if (should_retry_without_owner) {
		for (sz i = 0; i < item_count; ++i) {
			if (se_editor_item_matches_identity_selector(&items[i], item)) {
				se_set_last_error(SE_RESULT_OK);
				return true;
			}
		}
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

b8 se_editor_find_item(se_editor* editor, se_editor_category category, s_handle handle, se_editor_item* out_item) {
	const se_editor_item* items = NULL;
	sz item_count = 0u;
	if (!editor || !out_item || (u32)category >= (u32)SE_EDITOR_CATEGORY_COUNT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_editor_collect_items(editor, &items, &item_count)) {
		return false;
	}
	for (sz i = 0; i < item_count; ++i) {
		if (!items) {
			break;
		}
		if (items[i].category != category) {
			continue;
		}
		if (handle == S_HANDLE_NULL || items[i].handle == handle) {
			*out_item = items[i];
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

b8 se_editor_find_item_by_label(se_editor* editor, se_editor_category category, const c8* label, se_editor_item* out_item) {
	const se_editor_item* items = NULL;
	sz item_count = 0u;
	if (!editor || !label || label[0] == '\0' || !out_item || (u32)category >= (u32)SE_EDITOR_CATEGORY_COUNT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_editor_collect_items(editor, &items, &item_count)) {
		return false;
	}
	for (sz i = 0; i < item_count; ++i) {
		if (!items) {
			break;
		}
		if (items[i].category != category) {
			continue;
		}
		if (strcmp(items[i].label, label) == 0) {
			*out_item = items[i];
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

b8 se_editor_find_property(const se_editor_property* properties, sz property_count, const c8* name, const se_editor_property** out_property) {
	if (!properties || !name || name[0] == '\0' || !out_property) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (property_count == 0u) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	for (sz i = 0; i < property_count; ++i) {
		if (strcmp(properties[i].name, name) == 0) {
			*out_property = &properties[i];
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

void se_editor_set_mode(se_editor* editor, se_editor_mode mode) {
	if (!editor || (u32)mode >= (u32)SE_EDITOR_MODE_COUNT) {
		return;
	}
	editor->mode = mode;
	editor->shortcut_sequence_count = 0u;
	se_editor_shortcut_repeat_clear(editor);
}

se_editor_mode se_editor_get_mode(const se_editor* editor) {
	if (!editor) {
		return SE_EDITOR_MODE_NORMAL;
	}
	return editor->mode;
}

b8 se_editor_bind_shortcut(se_editor* editor, se_editor_mode mode, const se_key* keys, u32 key_count, u32 modifiers, const c8* action) {
	se_editor_shortcut shortcut = {0};
	if (!editor || (u32)mode >= (u32)SE_EDITOR_MODE_COUNT || !keys ||
		key_count == 0u || key_count > SE_EDITOR_SHORTCUT_MAX_KEYS ||
		!action || action[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (u32 i = 0u; i < key_count; ++i) {
		if (!se_editor_shortcut_key_valid(keys[i])) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}
		shortcut.keys[i] = keys[i];
	}
	shortcut.mode = mode;
	shortcut.key_count = key_count;
	shortcut.modifiers = modifiers;
	se_editor_copy_text(shortcut.action, sizeof(shortcut.action), action);

	for (sz i = 0u; i < s_array_get_size(&editor->shortcuts); ++i) {
		const s_handle handle = s_array_handle(&editor->shortcuts, (u32)i);
		se_editor_shortcut* existing = s_array_get(&editor->shortcuts, handle);
		b8 same_keys = existing && existing->key_count == shortcut.key_count && existing->modifiers == shortcut.modifiers;
		for (u32 key_index = 0u; same_keys && key_index < shortcut.key_count; ++key_index) {
			if (existing->keys[key_index] != shortcut.keys[key_index]) {
				same_keys = false;
			}
		}
		if (existing && existing->mode == mode && strcmp(existing->action, shortcut.action) == 0 && same_keys) {
			*existing = shortcut;
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	s_array_add(&editor->shortcuts, shortcut);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_bind_key(se_editor* editor, se_editor_mode mode, se_key key, u32 modifiers, const c8* action) {
	return se_editor_bind_shortcut(editor, mode, &key, 1u, modifiers, action);
}

b8 se_editor_bind_shortcut_text(se_editor* editor, se_editor_mode mode, const c8* keys, const c8* action) {
	se_key parsed_keys[SE_EDITOR_SHORTCUT_MAX_KEYS] = {0};
	u32 key_count = 0u;
	u32 modifiers = 0u;
	sz index = 0u;
	if (!editor || !keys || keys[0] == '\0' || !action || action[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	while (keys[index] != '\0') {
		c8 token[SE_MAX_NAME_LENGTH] = {0};
		sz token_index = 0u;
		se_key key = SE_KEY_UNKNOWN;
		u32 token_modifiers = 0u;
		while (keys[index] == ' ' || keys[index] == '\t' || keys[index] == ',') {
			index++;
		}
		if (keys[index] == '\0') {
			break;
		}
		while (keys[index] != '\0' && keys[index] != ' ' && keys[index] != '\t' && keys[index] != ',') {
			if (token_index + 1u >= sizeof(token)) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				return false;
			}
			token[token_index++] = keys[index++];
		}
		token[token_index] = '\0';
		if (key_count >= SE_EDITOR_SHORTCUT_MAX_KEYS ||
			!se_editor_shortcut_parse_token(token, &key, &token_modifiers)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}
		parsed_keys[key_count++] = key;
		modifiers |= token_modifiers;
	}
	return se_editor_bind_shortcut(editor, mode, parsed_keys, key_count, modifiers, action);
}

b8 se_editor_bind_default_shortcuts(se_editor* editor) {
	se_key gg[2] = { SE_KEY_G, SE_KEY_G };
	b8 ok = true;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_H, SE_EDITOR_SHORTCUT_MOD_NONE, "move_left") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_J, SE_EDITOR_SHORTCUT_MOD_NONE, "move_down") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_K, SE_EDITOR_SHORTCUT_MOD_NONE, "move_up") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_L, SE_EDITOR_SHORTCUT_MOD_NONE, "move_right") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_LEFT, SE_EDITOR_SHORTCUT_MOD_NONE, "move_left") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_DOWN, SE_EDITOR_SHORTCUT_MOD_NONE, "move_down") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_UP, SE_EDITOR_SHORTCUT_MOD_NONE, "move_up") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_RIGHT, SE_EDITOR_SHORTCUT_MOD_NONE, "move_right") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_TAB, SE_EDITOR_SHORTCUT_MOD_NONE, "next") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_TAB, SE_EDITOR_SHORTCUT_MOD_SHIFT, "previous") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_I, SE_EDITOR_SHORTCUT_MOD_NONE, "insert_mode") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_INSERT, SE_KEY_ESCAPE, SE_EDITOR_SHORTCUT_MOD_NONE, "normal_mode") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_SLASH, SE_EDITOR_SHORTCUT_MOD_NONE, "command_mode") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_COMMAND, SE_KEY_ESCAPE, SE_EDITOR_SHORTCUT_MOD_NONE, "normal_mode") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_S, SE_EDITOR_SHORTCUT_MOD_CONTROL, "save") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_O, SE_EDITOR_SHORTCUT_MOD_CONTROL, "load") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_Q, SE_EDITOR_SHORTCUT_MOD_CONTROL, "quit") && ok;
	ok = se_editor_bind_shortcut(editor, SE_EDITOR_MODE_NORMAL, gg, 2u, SE_EDITOR_SHORTCUT_MOD_NONE, "first") && ok;
	ok = se_editor_bind_key(editor, SE_EDITOR_MODE_NORMAL, SE_KEY_G, SE_EDITOR_SHORTCUT_MOD_SHIFT, "last") && ok;
	ok = se_editor_set_shortcut_repeat(editor, "move_left", true) && ok;
	ok = se_editor_set_shortcut_repeat(editor, "move_down", true) && ok;
	ok = se_editor_set_shortcut_repeat(editor, "move_up", true) && ok;
	ok = se_editor_set_shortcut_repeat(editor, "move_right", true) && ok;
	if (!ok && se_get_last_error() == SE_RESULT_OK) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
	}
	return ok;
}

b8 se_editor_set_shortcut_repeat(se_editor* editor, const c8* action, b8 repeat) {
	b8 changed = false;
	if (!editor || !action || action[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = 0u; i < s_array_get_size(&editor->shortcuts); ++i) {
		const s_handle handle = s_array_handle(&editor->shortcuts, (u32)i);
		se_editor_shortcut* shortcut = s_array_get(&editor->shortcuts, handle);
		if (shortcut && strcmp(shortcut->action, action) == 0) {
			shortcut->repeat = repeat;
			if (!repeat && se_editor_shortcut_repeat_matches(editor, shortcut)) {
				se_editor_shortcut_repeat_clear(editor);
			}
			changed = true;
		}
	}
	se_set_last_error(changed ? SE_RESULT_OK : SE_RESULT_NOT_FOUND);
	return changed;
}

b8 se_editor_unbind_shortcut(se_editor* editor, const c8* action) {
	b8 removed = false;
	if (!editor || !action || action[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = s_array_get_size(&editor->shortcuts); i-- > 0u;) {
		const s_handle handle = s_array_handle(&editor->shortcuts, (u32)i);
		se_editor_shortcut* shortcut = s_array_get(&editor->shortcuts, handle);
		if (shortcut && strcmp(shortcut->action, action) == 0) {
			if (se_editor_shortcut_repeat_matches(editor, shortcut)) {
				se_editor_shortcut_repeat_clear(editor);
			}
			s_array_remove(&editor->shortcuts, handle);
			removed = true;
		}
	}
	se_set_last_error(removed ? SE_RESULT_OK : SE_RESULT_NOT_FOUND);
	return removed;
}

void se_editor_clear_shortcuts(se_editor* editor) {
	if (!editor) {
		return;
	}
	SE_EDITOR_ARRAY_POP_ALL(&editor->shortcuts);
	editor->shortcut_sequence_count = 0u;
	se_editor_shortcut_repeat_clear(editor);
}

b8 se_editor_get_shortcuts(const se_editor* editor, const se_editor_shortcut** out_shortcuts, sz* out_count) {
	if (!editor || !out_shortcuts || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_shortcuts = s_array_get_data((se_editor_shortcuts*)&editor->shortcuts);
	*out_count = s_array_get_size(&editor->shortcuts);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_update_shortcuts(se_editor* editor) {
	se_key pressed = SE_KEY_UNKNOWN;
	if (!editor || editor->window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_editor_shortcut_events_clear(editor);
	for (sz i = 0u; i < s_array_get_size(&editor->shortcuts); ++i) {
		const s_handle handle = s_array_handle(&editor->shortcuts, (u32)i);
		const se_editor_shortcut* shortcut = s_array_get(&editor->shortcuts, handle);
		if (!shortcut || shortcut->mode != editor->mode || shortcut->key_count != 1u) {
			continue;
		}
		if (!se_editor_shortcut_modifiers_down(editor->window, shortcut->modifiers)) {
			continue;
		}
		if (shortcut->repeat
			? se_editor_shortcut_repeat_ready(editor, shortcut)
			: se_window_is_key_pressed(editor->window, shortcut->keys[0])) {
			se_editor_shortcut_add_event(editor, shortcut->action);
		}
	}
	pressed = se_editor_shortcut_pressed_key(editor->window);
	if (pressed != SE_KEY_UNKNOWN) {
		se_editor_shortcut_sequence_push(editor, pressed);
		if (!se_editor_shortcut_process_sequence(editor)) {
			editor->shortcut_sequence[0] = pressed;
			editor->shortcut_sequence_count = 1u;
			if (!se_editor_shortcut_process_sequence(editor)) {
				editor->shortcut_sequence_count = 0u;
			}
		}
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_poll_shortcut(se_editor* editor, se_editor_shortcut_event* out_event) {
	s_handle handle = S_HANDLE_NULL;
	se_editor_shortcut_event* event = NULL;
	if (!editor || !out_event) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (s_array_get_size(&editor->shortcut_events) == 0u) {
		se_set_last_error(SE_RESULT_OK);
		return false;
	}
	handle = s_array_handle(&editor->shortcut_events, 0u);
	event = s_array_get(&editor->shortcut_events, handle);
	if (!event) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_event = *event;
	s_array_remove_ordered(&editor->shortcut_events, handle);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_editor_clear_shortcut_events(se_editor* editor) {
	se_editor_shortcut_events_clear(editor);
}

b8 se_editor_apply_command(se_editor* editor, const se_editor_command* command) {
	b8 ok = false;
	if (!editor || !command || command->name[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_editor_validate_item(editor, &command->item)) {
		return false;
	}
	if (!editor->apply_command) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	ok = editor->apply_command(editor, command, editor->user_data);
	if (ok) {
		se_set_last_error(SE_RESULT_OK);
	} else if (se_get_last_error() == SE_RESULT_OK) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
	}
	return ok;
}

b8 se_editor_apply_commands(se_editor* editor, const se_editor_command* commands, sz command_count, u32* out_applied) {
	u32 applied = 0u;
	b8 ok = true;
	if (!editor || (!commands && command_count > 0u)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = 0; i < command_count; ++i) {
		if (se_editor_apply_command(editor, &commands[i])) {
			applied++;
		} else {
			ok = false;
		}
	}
	if (out_applied) {
		*out_applied = applied;
	}
	if (ok) {
		se_set_last_error(SE_RESULT_OK);
	}
	return ok;
}

b8 se_editor_apply_set(se_editor* editor, const se_editor_item* item, const c8* property_name, const se_editor_value* value) {
	se_editor_command command = {0};
	if (!editor || !item || !property_name || property_name[0] == '\0' || !value) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	command = se_editor_command_make_set(item, property_name, value);
	return se_editor_apply_command(editor, &command);
}

b8 se_editor_apply_toggle(se_editor* editor, const se_editor_item* item, const c8* property_name) {
	se_editor_command command = {0};
	if (!editor || !item || !property_name || property_name[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	command = se_editor_command_make_toggle(item, property_name);
	return se_editor_apply_command(editor, &command);
}

b8 se_editor_apply_action(se_editor* editor, const se_editor_item* item, const c8* action_name, const se_editor_value* value, const se_editor_value* aux_value) {
	se_editor_command command = {0};
	if (!editor || !item || !action_name || action_name[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	command = se_editor_command_make_action(item, action_name, value, aux_value);
	return se_editor_apply_command(editor, &command);
}

b8 se_editor_queue_command(se_editor* editor, const se_editor_command* command) {
	if (!editor || !command || command->name[0] == '\0' || (u32)command->item.category >= (u32)SE_EDITOR_CATEGORY_COUNT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_editor_validate_item(editor, &command->item)) {
		return false;
	}
	s_array_add(&editor->queue, *command);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_editor_clear_queue(se_editor* editor) {
	if (!editor) {
		return;
	}
	se_editor_queue_clear(editor);
}

b8 se_editor_get_queue(const se_editor* editor, const se_editor_command** out_commands, sz* out_count) {
	if (!editor || !out_commands || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_commands = s_array_get_data((se_editor_command_queue*)&editor->queue);
	*out_count = s_array_get_size(&editor->queue);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_apply_queue(se_editor* editor, u32* out_applied, b8 clear_after_apply) {
	u32 applied = 0u;
	b8 ok = true;
	if (!editor) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&editor->queue); ++i) {
		const s_handle handle = s_array_handle(&editor->queue, (u32)i);
		se_editor_command* command = s_array_get(&editor->queue, handle);
		if (!command) {
			ok = false;
			continue;
		}
		if (se_editor_apply_command(editor, command)) {
			applied++;
		} else {
			ok = false;
		}
	}
	if (clear_after_apply) {
		se_editor_queue_clear(editor);
	}
	if (out_applied) {
		*out_applied = applied;
	}
	if (ok) {
		se_set_last_error(SE_RESULT_OK);
	}
	return ok;
}
