// Syphax-Engine - Ougi Washi

#ifndef SE_EDITOR_H
#define SE_EDITOR_H

#include "se_backend.h"
#include "se_debug.h"
#include "se_navigation.h"
#include "se_physics.h"
#include "se_scene.h"
#include "se_simulation.h"
#include "se_ui.h"
#include "se_vfx.h"
#include "se_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct se_editor se_editor;
typedef struct s_json s_json;

typedef enum {
	SE_EDITOR_CATEGORY_BACKEND = 0,
	SE_EDITOR_CATEGORY_DEBUG,
	SE_EDITOR_CATEGORY_WINDOW,
	SE_EDITOR_CATEGORY_INPUT,
	SE_EDITOR_CATEGORY_CAMERA,
	SE_EDITOR_CATEGORY_SCENE_2D,
	SE_EDITOR_CATEGORY_OBJECT_2D,
	SE_EDITOR_CATEGORY_SCENE_3D,
	SE_EDITOR_CATEGORY_OBJECT_3D,
	SE_EDITOR_CATEGORY_UI,
	SE_EDITOR_CATEGORY_UI_WIDGET,
	SE_EDITOR_CATEGORY_VFX_2D,
	SE_EDITOR_CATEGORY_VFX_3D,
	SE_EDITOR_CATEGORY_SIMULATION,
	SE_EDITOR_CATEGORY_MODEL,
	SE_EDITOR_CATEGORY_SHADER,
	SE_EDITOR_CATEGORY_TEXTURE,
	SE_EDITOR_CATEGORY_FRAMEBUFFER,
	SE_EDITOR_CATEGORY_RENDER_BUFFER,
	SE_EDITOR_CATEGORY_FONT,
	SE_EDITOR_CATEGORY_AUDIO,
	SE_EDITOR_CATEGORY_AUDIO_CLIP,
	SE_EDITOR_CATEGORY_AUDIO_STREAM,
	SE_EDITOR_CATEGORY_AUDIO_CAPTURE,
	SE_EDITOR_CATEGORY_NAVIGATION,
	SE_EDITOR_CATEGORY_PHYSICS_2D,
	SE_EDITOR_CATEGORY_PHYSICS_2D_BODY,
	SE_EDITOR_CATEGORY_PHYSICS_3D,
	SE_EDITOR_CATEGORY_PHYSICS_3D_BODY,
	SE_EDITOR_CATEGORY_CUSTOM,
	SE_EDITOR_CATEGORY_COUNT
} se_editor_category;

typedef u64 se_editor_category_mask;

#define SE_EDITOR_CATEGORY_MASK_ALL ((se_editor_category_mask)~(se_editor_category_mask)0)
#define SE_EDITOR_SHORTCUT_MAX_KEYS 4u

static inline se_editor_category_mask se_editor_category_to_mask(const se_editor_category category) {
	if ((u32)category >= (u32)SE_EDITOR_CATEGORY_COUNT || (u32)category >= 64u) {
		return 0u;
	}
	return ((se_editor_category_mask)1u) << (u32)category;
}

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
	c8 name[SE_MAX_NAME_LENGTH];
	se_editor_value value;
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

typedef b8 (*se_editor_collect_custom_items_fn)(se_editor* editor, se_editor_category_mask category_mask, void* user_data);
typedef b8 (*se_editor_collect_custom_properties_fn)(se_editor* editor, const se_editor_item* item, void* user_data);
typedef b8 (*se_editor_apply_custom_command_fn)(se_editor* editor, const se_editor_command* command, void* user_data);

typedef struct {
	se_context* context;
	se_window_handle window;
	se_input_handle* input;
	se_audio_engine* audio;
	se_navigation_grid* navigation;
	se_physics_world_2d_handle physics_2d;
	se_physics_world_3d_handle physics_3d;
	se_simulation_handle focused_simulation;
	se_vfx_2d_handle focused_vfx_2d;
	se_vfx_3d_handle focused_vfx_3d;
	void* custom_user_data;
	se_editor_collect_custom_items_fn collect_custom_items;
	se_editor_collect_custom_properties_fn collect_custom_properties;
	se_editor_apply_custom_command_fn apply_custom_command;
} se_editor_config;

#define SE_EDITOR_CONFIG_DEFAULTS ((se_editor_config){ \
	.context = NULL, \
	.window = S_HANDLE_NULL, \
	.input = NULL, \
	.audio = NULL, \
	.navigation = NULL, \
	.physics_2d = SE_PHYSICS_WORLD_2D_HANDLE_NULL, \
	.physics_3d = SE_PHYSICS_WORLD_3D_HANDLE_NULL, \
	.focused_simulation = S_HANDLE_NULL, \
	.focused_vfx_2d = S_HANDLE_NULL, \
	.focused_vfx_3d = S_HANDLE_NULL, \
	.custom_user_data = NULL, \
	.collect_custom_items = NULL, \
	.collect_custom_properties = NULL, \
	.apply_custom_command = NULL \
})

typedef struct {
	u32 windows;
	u32 inputs;
	u32 cameras;
	u32 scenes_2d;
	u32 objects_2d;
	u32 scenes_3d;
	u32 objects_3d;
	u32 uis;
	u32 ui_widgets;
	u32 vfx_2d;
	u32 vfx_3d;
	u32 simulations;
	u32 models;
	u32 shaders;
	u32 textures;
	u32 framebuffers;
	u32 render_buffers;
	u32 fonts;
	u32 audio_clips;
	u32 audio_streams;
	u32 audio_captures;
	u32 physics_2d_bodies;
	u32 physics_3d_bodies;
	u32 custom_items;
	u32 queued_commands;
} se_editor_counts;

typedef struct {
	se_backend_info backend_info;
	se_capabilities capabilities;
	se_debug_frame_timing frame_timing;
	u64 trace_event_count;
	u32 trace_stat_count_total;
	u32 trace_stat_count_last_frame;
	u32 trace_thread_count_last_frame;
	f64 trace_cpu_ms_last_frame;
	f64 trace_gpu_ms_last_frame;
	se_window_diagnostics window_diagnostics;
	se_input_diagnostics input_diagnostics;
	se_simulation_diagnostics simulation_diagnostics;
	se_vfx_diagnostics vfx_2d_diagnostics;
	se_vfx_diagnostics vfx_3d_diagnostics;
	se_audio_band_levels audio_clip_bands;
	se_audio_band_levels audio_capture_bands;
	b8 has_trace_stats : 1;
	b8 has_frame_timing : 1;
	b8 has_window_diagnostics : 1;
	b8 has_input_diagnostics : 1;
	b8 has_simulation_diagnostics : 1;
	b8 has_vfx_2d_diagnostics : 1;
	b8 has_vfx_3d_diagnostics : 1;
	b8 has_audio_clip_bands : 1;
	b8 has_audio_capture_bands : 1;
} se_editor_diagnostics;

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

extern se_editor_command se_editor_command_make(se_editor_command_type type, const se_editor_item* item, const c8* name, const se_editor_value* value, const se_editor_value* aux_value);
extern se_editor_command se_editor_command_make_set(const se_editor_item* item, const c8* name, const se_editor_value* value);
extern se_editor_command se_editor_command_make_toggle(const se_editor_item* item, const c8* name);
extern se_editor_command se_editor_command_make_action(const se_editor_item* item, const c8* name, const se_editor_value* value, const se_editor_value* aux_value);

extern se_editor* se_editor_create(const se_editor_config* config);
extern void se_editor_destroy(se_editor* editor);
extern void se_editor_reset(se_editor* editor);
extern void se_editor_refresh_defaults(se_editor* editor);

extern void se_editor_set_context(se_editor* editor, se_context* context);
extern se_context* se_editor_get_context(const se_editor* editor);

extern void se_editor_set_window(se_editor* editor, se_window_handle window);
extern se_window_handle se_editor_get_window(const se_editor* editor);

extern void se_editor_set_input(se_editor* editor, se_input_handle* input);
extern se_input_handle* se_editor_get_input(const se_editor* editor);

extern void se_editor_set_audio_engine(se_editor* editor, se_audio_engine* audio);
extern se_audio_engine* se_editor_get_audio_engine(const se_editor* editor);

extern void se_editor_set_navigation(se_editor* editor, se_navigation_grid* navigation);
extern se_navigation_grid* se_editor_get_navigation(const se_editor* editor);

extern void se_editor_set_physics_2d(se_editor* editor, se_physics_world_2d_handle world);
extern se_physics_world_2d_handle se_editor_get_physics_2d(const se_editor* editor);

extern void se_editor_set_physics_3d(se_editor* editor, se_physics_world_3d_handle world);
extern se_physics_world_3d_handle se_editor_get_physics_3d(const se_editor* editor);

extern void se_editor_set_focused_simulation(se_editor* editor, se_simulation_handle simulation);
extern se_simulation_handle se_editor_get_focused_simulation(const se_editor* editor);

extern void se_editor_set_focused_vfx_2d(se_editor* editor, se_vfx_2d_handle vfx);
extern se_vfx_2d_handle se_editor_get_focused_vfx_2d(const se_editor* editor);

extern void se_editor_set_focused_vfx_3d(se_editor* editor, se_vfx_3d_handle vfx);
extern se_vfx_3d_handle se_editor_get_focused_vfx_3d(const se_editor* editor);

extern void se_editor_set_custom_user_data(se_editor* editor, void* user_data);
extern void* se_editor_get_custom_user_data(const se_editor* editor);
extern void se_editor_set_collect_custom_items(se_editor* editor, se_editor_collect_custom_items_fn fn);
extern void se_editor_set_collect_custom_properties(se_editor* editor, se_editor_collect_custom_properties_fn fn);
extern void se_editor_set_apply_custom_command(se_editor* editor, se_editor_apply_custom_command_fn fn);

extern b8 se_editor_json_write_file(const c8* path, s_json* root);
extern b8 se_editor_json_read_file(const c8* path, s_json** out_root);

extern b8 se_editor_track_audio_clip(se_editor* editor, se_audio_clip* clip, const c8* label);
extern b8 se_editor_untrack_audio_clip(se_editor* editor, se_audio_clip* clip);
extern b8 se_editor_track_audio_stream(se_editor* editor, se_audio_stream* stream, const c8* label);
extern b8 se_editor_untrack_audio_stream(se_editor* editor, se_audio_stream* stream);
extern b8 se_editor_track_audio_capture(se_editor* editor, se_audio_capture* capture, const c8* label);
extern b8 se_editor_untrack_audio_capture(se_editor* editor, se_audio_capture* capture);

extern b8 se_editor_collect_counts(se_editor* editor, se_editor_counts* out_counts);
extern b8 se_editor_collect_items(se_editor* editor, se_editor_category_mask category_mask, const se_editor_item** out_items, sz* out_count);
extern b8 se_editor_collect_properties(se_editor* editor, const se_editor_item* item, const se_editor_property** out_properties, sz* out_count);
extern b8 se_editor_collect_diagnostics(se_editor* editor, se_editor_diagnostics* out_diagnostics);
extern b8 se_editor_add_item(se_editor* editor, se_editor_category category, s_handle handle, s_handle owner_handle, void* pointer, void* owner_pointer, u32 index, const c8* label);
extern b8 se_editor_add_property_value(se_editor* editor, const c8* name, const se_editor_value* value, b8 editable);
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
