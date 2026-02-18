// Syphax-Engine - Ougi Washi

#include "se_editor.h"

#include "se_graphics.h"
#include "se_ext.h"
#include "se_texture.h"

#include "syphax/s_array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef s_array(se_editor_item, se_editor_items);
typedef s_array(se_editor_property, se_editor_properties);
typedef s_array(se_editor_command, se_editor_command_queue);

typedef struct {
	se_audio_clip* clip;
	c8 label[SE_MAX_NAME_LENGTH];
} se_editor_audio_clip_ref;
typedef s_array(se_editor_audio_clip_ref, se_editor_audio_clip_refs);

typedef struct {
	se_audio_stream* stream;
	c8 label[SE_MAX_NAME_LENGTH];
} se_editor_audio_stream_ref;
typedef s_array(se_editor_audio_stream_ref, se_editor_audio_stream_refs);

typedef struct {
	se_audio_capture* capture;
	c8 label[SE_MAX_NAME_LENGTH];
} se_editor_audio_capture_ref;
typedef s_array(se_editor_audio_capture_ref, se_editor_audio_capture_refs);

typedef struct {
	u64 event_count;
	u32 stat_count_total;
	u32 stat_count_last_frame;
	u32 thread_count_last_frame;
	f64 cpu_ms_last_frame;
	f64 gpu_ms_last_frame;
	b8 valid : 1;
} se_editor_trace_summary;

struct se_editor {
	se_context* context;
	se_window_handle window;
	se_input_handle* input;
	se_audio_engine* audio;
	se_navigation_grid* navigation;
	se_physics_world_2d* physics_2d;
	se_physics_world_3d* physics_3d;
	se_simulation_handle focused_simulation;
	se_vfx_2d_handle focused_vfx_2d;
	se_vfx_3d_handle focused_vfx_3d;

	se_editor_audio_clip_refs clips;
	se_editor_audio_stream_refs streams;
	se_editor_audio_capture_refs captures;

	se_editor_items items;
	se_editor_properties properties;
	se_editor_command_queue queue;
};

#define SE_EDITOR_ARRAY_POP_ALL(_arr) \
	do { \
		while (s_array_get_size((_arr)) > 0) { \
			s_array_remove((_arr), s_array_handle((_arr), (u32)(s_array_get_size((_arr)) - 1))); \
		} \
	} while (0)

/* Editor name literals (property/action/category keys) */
#define SE_EDITOR_NAME_ACTION_COUNT "action_count"
#define SE_EDITOR_NAME_ADD_EMITTER "add_emitter"
#define SE_EDITOR_NAME_ADD_OBJECT "add_object"
#define SE_EDITOR_NAME_ADD_POST_PROCESS_BUFFER "add_post_process_buffer"
#define SE_EDITOR_NAME_ALIGN_HORIZONTAL "align_horizontal"
#define SE_EDITOR_NAME_ALIGN_VERTICAL "align_vertical"
#define SE_EDITOR_NAME_ALIVE_PARTICLES "alive_particles"
#define SE_EDITOR_NAME_ANCHOR_MAX "anchor_max"
#define SE_EDITOR_NAME_ANCHOR_MIN "anchor_min"
#define SE_EDITOR_NAME_ANCHORS_ENABLED "anchors_enabled"
#define SE_EDITOR_NAME_ANGULAR_DAMPING "angular_damping"
#define SE_EDITOR_NAME_ANGULAR_VELOCITY "angular_velocity"
#define SE_EDITOR_NAME_APPLY_FORCE "apply_force"
#define SE_EDITOR_NAME_APPLY_IMPULSE "apply_impulse"
#define SE_EDITOR_NAME_ASPECT "aspect"
#define SE_EDITOR_NAME_ATLAS_HEIGHT "atlas_height"
#define SE_EDITOR_NAME_ATLAS_TEXTURE "atlas_texture"
#define SE_EDITOR_NAME_ATLAS_WIDTH "atlas_width"
#define SE_EDITOR_NAME_ATTACH_TO "attach_to"
#define SE_EDITOR_NAME_ATTACH_WINDOW_TEXT "attach_window_text"
#define SE_EDITOR_NAME_AUDIO "audio"
#define SE_EDITOR_NAME_AUDIO_CAPTURE "audio_capture"
#define SE_EDITOR_NAME_AUDIO_CLIP "audio_clip"
#define SE_EDITOR_NAME_AUDIO_STREAM "audio_stream"
#define SE_EDITOR_NAME_AUTO_RESIZE "auto_resize"
#define SE_EDITOR_NAME_BACKGROUND "background"
#define SE_EDITOR_NAME_BACKEND "backend"
#define SE_EDITOR_NAME_BAND_ENERGY "band_energy"
#define SE_EDITOR_NAME_BAND_HIGH "band_high"
#define SE_EDITOR_NAME_BAND_LOW "band_low"
#define SE_EDITOR_NAME_BAND_MID "band_mid"
#define SE_EDITOR_NAME_BEGIN_FRAME "begin_frame"
#define SE_EDITOR_NAME_BIND "bind"
#define SE_EDITOR_NAME_BLOCKED_CELLS "blocked_cells"
#define SE_EDITOR_NAME_BLOCKED_RATIO "blocked_ratio"
#define SE_EDITOR_NAME_BODY_COUNT "body_count"
#define SE_EDITOR_NAME_BOOL "bool"
#define SE_EDITOR_NAME_BORDER "border"
#define SE_EDITOR_NAME_BORDER_WIDTH "border_width"
#define SE_EDITOR_NAME_BOUNDS_MAX "bounds_max"
#define SE_EDITOR_NAME_BOUNDS_MIN "bounds_min"
#define SE_EDITOR_NAME_CAMERA "camera"
#define SE_EDITOR_NAME_CAMERA_U "camera[%u]"
#define SE_EDITOR_NAME_CAPABILITIES_VALID "capabilities_valid"
#define SE_EDITOR_NAME_CAPABILITY_COMPUTE "capability_compute"
#define SE_EDITOR_NAME_CAPABILITY_FLOAT_RENDER_TARGETS "capability_float_render_targets"
#define SE_EDITOR_NAME_CAPABILITY_INSTANCING "capability_instancing"
#define SE_EDITOR_NAME_CAPABILITY_MAX_MRT_COUNT "capability_max_mrt_count"
#define SE_EDITOR_NAME_CAPABILITY_MAX_TEXTURE_SIZE "capability_max_texture_size"
#define SE_EDITOR_NAME_CAPABILITY_MAX_TEXTURE_UNITS "capability_max_texture_units"
#define SE_EDITOR_NAME_CATEGORY_MASK "category_mask"
#define SE_EDITOR_NAME_CELL_SIZE "cell_size"
#define SE_EDITOR_NAME_CHANNELS "channels"
#define SE_EDITOR_NAME_CHARACTERS_COUNT "characters_count"
#define SE_EDITOR_NAME_CHECK_EXIT_KEYS "check_exit_keys"
#define SE_EDITOR_NAME_CLAMP_TARGET "clamp_target"
#define SE_EDITOR_NAME_CLEANUP "cleanup"
#define SE_EDITOR_NAME_CLEAR "clear"
#define SE_EDITOR_NAME_CLEAR_DEBUG_MARKERS "clear_debug_markers"
#define SE_EDITOR_NAME_CLEAR_EVENTS "clear_events"
#define SE_EDITOR_NAME_CLEAR_FOCUS "clear_focus"
#define SE_EDITOR_NAME_CLEAR_INPUT_STATE "clear_input_state"
#define SE_EDITOR_NAME_CLIP_CHILDREN "clip_children"
#define SE_EDITOR_NAME_CLIPPING "clipping"
#define SE_EDITOR_NAME_CLOSE "close"
#define SE_EDITOR_NAME_CLOSE_UNTRACK "close_untrack"
#define SE_EDITOR_NAME_COMPONENT_REGISTRY_COUNT "component_registry_count"
#define SE_EDITOR_NAME_CONTACT_COUNT "contact_count"
#define SE_EDITOR_NAME_CONTENT_SCALE_X "content_scale_x"
#define SE_EDITOR_NAME_CONTENT_SCALE_Y "content_scale_y"
#define SE_EDITOR_NAME_CONTEXT "context"
#define SE_EDITOR_NAME_CONTEXT_CLEAR "context_clear"
#define SE_EDITOR_NAME_CONTEXT_COUNT "context_count"
#define SE_EDITOR_NAME_CONTEXT_CREATE "context_create"
#define SE_EDITOR_NAME_CONTEXT_POP "context_pop"
#define SE_EDITOR_NAME_CONTEXT_PUSH "context_push"
#define SE_EDITOR_NAME_CONTEXT_SET_ENABLED "context_set_enabled"
#define SE_EDITOR_NAME_CULLING "culling"
#define SE_EDITOR_NAME_CURRENT_TICK "current_tick"
#define SE_EDITOR_NAME_CURSOR "cursor"
#define SE_EDITOR_NAME_CURSOR_MODE "cursor_mode"
#define SE_EDITOR_NAME_DEBUG "debug"
#define SE_EDITOR_NAME_DEBUG_MARKER_COUNT "debug_marker_count"
#define SE_EDITOR_NAME_DEBUG_OVERLAY "debug_overlay"
#define SE_EDITOR_NAME_DELTA_TIME "delta_time"
#define SE_EDITOR_NAME_DEPTH_BUFFER "depth_buffer"
#define SE_EDITOR_NAME_DESTROY "destroy"
#define SE_EDITOR_NAME_DETACH "detach"
#define SE_EDITOR_NAME_DIRTY "dirty"
#define SE_EDITOR_NAME_DISCARD_CPU_DATA "discard_cpu_data"
#define SE_EDITOR_NAME_DISABLED "disabled"
#define SE_EDITOR_NAME_DOLLY "dolly"
#define SE_EDITOR_NAME_DOUBLE "double"
#define SE_EDITOR_NAME_DRAW "draw"
#define SE_EDITOR_NAME_EMITTER_BURST "emitter_burst"
#define SE_EDITOR_NAME_EMITTER_CLEAR_TRACKS "emitter_clear_tracks"
#define SE_EDITOR_NAME_EMITTER_COUNT "emitter_count"
#define SE_EDITOR_NAME_EMITTER_REMOVE "emitter_remove"
#define SE_EDITOR_NAME_EMITTER_SET_BLEND_MODE "emitter_set_blend_mode"
#define SE_EDITOR_NAME_EMITTER_SET_MODEL "emitter_set_model"
#define SE_EDITOR_NAME_EMITTER_SET_SHADER "emitter_set_shader"
#define SE_EDITOR_NAME_EMITTER_SET_SPAWN "emitter_set_spawn"
#define SE_EDITOR_NAME_EMITTER_SET_TEXTURE "emitter_set_texture"
#define SE_EDITOR_NAME_EMITTER_START "emitter_start"
#define SE_EDITOR_NAME_EMITTER_STOP "emitter_stop"
#define SE_EDITOR_NAME_EMIT_TEXT "emit_text"
#define SE_EDITOR_NAME_ENABLED "enabled"
#define SE_EDITOR_NAME_END_FRAME "end_frame"
#define SE_EDITOR_NAME_ENTITY_CAPACITY "entity_capacity"
#define SE_EDITOR_NAME_ENTITY_COUNT "entity_count"
#define SE_EDITOR_NAME_ENTITY_CREATE "entity_create"
#define SE_EDITOR_NAME_ENTITY_DESTROY "entity_destroy"
#define SE_EDITOR_NAME_EVENT_QUEUE_COUNT "event_queue_count"
#define SE_EDITOR_NAME_EVENT_REGISTRY_COUNT "event_registry_count"
#define SE_EDITOR_NAME_EXPIRED_PARTICLES "expired_particles"
#define SE_EDITOR_NAME_EXT_COMPUTE_SUPPORTED "ext_compute_supported"
#define SE_EDITOR_NAME_EXTENSION_APIS_SUPPORTED "extension_apis_supported"
#define SE_EDITOR_NAME_EXT_FLOAT_RENDER_TARGET_SUPPORTED "ext_float_render_target_supported"
#define SE_EDITOR_NAME_EXT_INSTANCING_SUPPORTED "ext_instancing_supported"
#define SE_EDITOR_NAME_EXT_MULTI_RENDER_TARGET_SUPPORTED "ext_multi_render_target_supported"
#define SE_EDITOR_NAME_EXT_REQUIRE_COMPUTE "ext_require_compute"
#define SE_EDITOR_NAME_EXT_REQUIRE_FLOAT_RENDER_TARGET "ext_require_float_render_target"
#define SE_EDITOR_NAME_EXT_REQUIRE_INSTANCING "ext_require_instancing"
#define SE_EDITOR_NAME_EXT_REQUIRE_MULTI_RENDER_TARGET "ext_require_multi_render_target"
#define SE_EDITOR_NAME_FAR "far"
#define SE_EDITOR_NAME_FIRST_CHARACTER "first_character"
#define SE_EDITOR_NAME_FIXED_DT "fixed_dt"
#define SE_EDITOR_NAME_FLOAT "float"
#define SE_EDITOR_NAME_FOCUS "focus"
#define SE_EDITOR_NAME_FOCUSED "focused"
#define SE_EDITOR_NAME_FOCUSED_WIDGET "focused_widget"
#define SE_EDITOR_NAME_FOCUS_WIDGET "focus_widget"
#define SE_EDITOR_NAME_FONT "font"
#define SE_EDITOR_NAME_FONT_U "font[%u]"
#define SE_EDITOR_NAME_FORCE "force"
#define SE_EDITOR_NAME_FORWARD "forward"
#define SE_EDITOR_NAME_FOV "fov"
#define SE_EDITOR_NAME_FPS "fps"
#define SE_EDITOR_NAME_FRAGMENT_PATH "fragment_path"
#define SE_EDITOR_NAME_FRAGMENT_SHADER "fragment_shader"
#define SE_EDITOR_NAME_FRAME_BEGIN "frame_begin"
#define SE_EDITOR_NAME_FRAMEBUFFER "framebuffer"
#define SE_EDITOR_NAME_FRAMEBUFFER_HEIGHT "framebuffer_height"
#define SE_EDITOR_NAME_FRAMEBUFFER_U "framebuffer[%u]"
#define SE_EDITOR_NAME_FRAMEBUFFER_WIDTH "framebuffer_width"
#define SE_EDITOR_NAME_FRAME_COUNT "frame_count"
#define SE_EDITOR_NAME_FRAME_END "frame_end"
#define SE_EDITOR_NAME_FRAME_MS "frame_ms"
#define SE_EDITOR_NAME_FRAMES_PRESENTED "frames_presented"
#define SE_EDITOR_NAME_FRICTION "friction"
#define SE_EDITOR_NAME_GRAVITY "gravity"
#define SE_EDITOR_NAME_HANDLE "handle"
#define SE_EDITOR_NAME_HEIGHT "height"
#define SE_EDITOR_NAME_HOVERED "hovered"
#define SE_EDITOR_NAME_HOVERED_PREFIX "hovered_"
#define SE_EDITOR_NAME_HOVERED_WIDGET "hovered_widget"
#define SE_EDITOR_NAME_ID "id"
#define SE_EDITOR_NAME_INACTIVE_SLOT_COUNT "inactive_slot_count"
#define SE_EDITOR_NAME_INDEX_COUNT "index_count"
#define SE_EDITOR_NAME_INERTIA "inertia"
#define SE_EDITOR_NAME_INPUT "input"
#define SE_EDITOR_NAME_INPUT_MS "input_ms"
#define SE_EDITOR_NAME_INSTANCE_COUNT "instance_count"
#define SE_EDITOR_NAME_INSTANCE_REMOVE "instance_remove"
#define SE_EDITOR_NAME_INSTANCES_DIRTY "instances_dirty"
#define SE_EDITOR_NAME_INSTANCE_SET_ACTIVE "instance_set_active"
#define SE_EDITOR_NAME_INSTANCE_SET_BUFFER "instance_set_buffer"
#define SE_EDITOR_NAME_INSTANCE_SET_TRANSFORM "instance_set_transform"
#define SE_EDITOR_NAME_INT "int"
#define SE_EDITOR_NAME_INTERACTABLE "interactable"
#define SE_EDITOR_NAME_INV_INERTIA "inv_inertia"
#define SE_EDITOR_NAME_INV_MASS "inv_mass"
#define SE_EDITOR_NAME_JSON_LOAD_FILE "json_load_file"
#define SE_EDITOR_NAME_JSON_SAVE_FILE "json_save_file"
#define SE_EDITOR_NAME_KEY_EVENTS "key_events"
#define SE_EDITOR_NAME_LAST_PRESENT_DURATION "last_present_duration"
#define SE_EDITOR_NAME_LAST_SLEEP_DURATION "last_sleep_duration"
#define SE_EDITOR_NAME_LAYOUT_DIRECTION "layout_direction"
#define SE_EDITOR_NAME_LAYOUT_SPACING "layout_spacing"
#define SE_EDITOR_NAME_LEVEL "level"
#define SE_EDITOR_NAME_LINEAR_DAMPING "linear_damping"
#define SE_EDITOR_NAME_LOAD_MAPPINGS "load_mappings"
#define SE_EDITOR_NAME_LOOPING "looping"
#define SE_EDITOR_NAME_MARGIN "margin"
#define SE_EDITOR_NAME_MARK_LAYOUT_DIRTY "mark_layout_dirty"
#define SE_EDITOR_NAME_MARK_STRUCTURE_DIRTY "mark_structure_dirty"
#define SE_EDITOR_NAME_MARK_TEXT_DIRTY "mark_text_dirty"
#define SE_EDITOR_NAME_MARK_VISUAL_DIRTY "mark_visual_dirty"
#define SE_EDITOR_NAME_MASS "mass"
#define SE_EDITOR_NAME_MASTER_VOLUME "master_volume"
#define SE_EDITOR_NAME_MAT3 "mat3"
#define SE_EDITOR_NAME_MAT4 "mat4"
#define SE_EDITOR_NAME_MAX_EVENT_PAYLOAD_BYTES "max_event_payload_bytes"
#define SE_EDITOR_NAME_MAX_EVENTS "max_events"
#define SE_EDITOR_NAME_MAX_SIZE "max_size"
#define SE_EDITOR_NAME_MESH_COUNT "mesh_count"
#define SE_EDITOR_NAME_MESHES_WITH_CPU_DATA "meshes_with_cpu_data"
#define SE_EDITOR_NAME_MESHES_WITH_GPU_DATA "meshes_with_gpu_data"
#define SE_EDITOR_NAME_MIN_SIZE "min_size"
#define SE_EDITOR_NAME_MODEL "model"
#define SE_EDITOR_NAME_MODEL_U "model[%u]"
#define SE_EDITOR_NAME_MOUSE_BUTTON_EVENTS "mouse_button_events"
#define SE_EDITOR_NAME_MOUSE_DELTA_NDC "mouse_delta_ndc"
#define SE_EDITOR_NAME_MOUSE_DX "mouse_dx"
#define SE_EDITOR_NAME_MOUSE_DY "mouse_dy"
#define SE_EDITOR_NAME_MOUSE_MOVE_EVENTS "mouse_move_events"
#define SE_EDITOR_NAME_MOUSE_NDC "mouse_ndc"
#define SE_EDITOR_NAME_MOUSE_X "mouse_x"
#define SE_EDITOR_NAME_MOUSE_Y "mouse_y"
#define SE_EDITOR_NAME_MUSIC_VOLUME "music_volume"
#define SE_EDITOR_NAME_NAVIGATION "navigation"
#define SE_EDITOR_NAME_NAVIGATION_MS "navigation_ms"
#define SE_EDITOR_NAME_NEAR "near"
#define SE_EDITOR_NAME_NEEDS_RELOAD "needs_reload"
#define SE_EDITOR_NAME_NONE "none"
#define SE_EDITOR_NAME_NORMAL "normal"
#define SE_EDITOR_NAME_NORMAL_PREFIX "normal_"
#define SE_EDITOR_NAME_OBJECT2 "object2"
#define SE_EDITOR_NAME_OBJECT2D "object2d"
#define SE_EDITOR_NAME_OBJECT2D_U "object2d[%u]"
#define SE_EDITOR_NAME_OBJECT3 "object3"
#define SE_EDITOR_NAME_OBJECT3D "object3d"
#define SE_EDITOR_NAME_OBJECT3D_U "object3d[%u]"
#define SE_EDITOR_NAME_OBJECT_COUNT "object_count"
#define SE_EDITOR_NAME_OCCUPANCY_CELLS "occupancy_cells"
#define SE_EDITOR_NAME_ORBIT "orbit"
#define SE_EDITOR_NAME_ORIGIN "origin"
#define SE_EDITOR_NAME_ORTHO_HEIGHT "ortho_height"
#define SE_EDITOR_NAME_OTHER_MS "other_ms"
#define SE_EDITOR_NAME_OUTPUT "output"
#define SE_EDITOR_NAME_OUTPUT_AUTO_RESIZE "output_auto_resize"
#define SE_EDITOR_NAME_OUTPUT_RATIO "output_ratio"
#define SE_EDITOR_NAME_OUTPUT_SIZE "output_size"
#define SE_EDITOR_NAME_OVERLAY_ENABLED "overlay_enabled"
#define SE_EDITOR_NAME_PADDING "padding"
#define SE_EDITOR_NAME_PAN "pan"
#define SE_EDITOR_NAME_PAN_LOCAL "pan_local"
#define SE_EDITOR_NAME_PAN_WORLD "pan_world"
#define SE_EDITOR_NAME_PATH "path"
#define SE_EDITOR_NAME_PENDING_EVENT_COUNT "pending_event_count"
#define SE_EDITOR_NAME_PHYSICS2D "physics2d"
#define SE_EDITOR_NAME_PHYSICS2D_BODY "physics2d_body"
#define SE_EDITOR_NAME_PHYSICS2DBODY "physics2dbody"
#define SE_EDITOR_NAME_PHYSICS2D_BODY_U "physics2d_body[%u]"
#define SE_EDITOR_NAME_PHYSICS2DWORLD "physics2dworld"
#define SE_EDITOR_NAME_PHYSICS3D "physics3d"
#define SE_EDITOR_NAME_PHYSICS3D_BODY "physics3d_body"
#define SE_EDITOR_NAME_PHYSICS3DBODY "physics3dbody"
#define SE_EDITOR_NAME_PHYSICS3D_BODY_U "physics3d_body[%u]"
#define SE_EDITOR_NAME_PHYSICS3DWORLD "physics3dworld"
#define SE_EDITOR_NAME_PLATFORM_BACKEND "platform_backend"
#define SE_EDITOR_NAME_PLAY "play"
#define SE_EDITOR_NAME_PLAYING "playing"
#define SE_EDITOR_NAME_POINTER "pointer"
#define SE_EDITOR_NAME_POLL_EVENTS "poll_events"
#define SE_EDITOR_NAME_PORTABILITY_PROFILE "portability_profile"
#define SE_EDITOR_NAME_POSITION "position"
#define SE_EDITOR_NAME_POST_PROCESS_COUNT "post_process_count"
#define SE_EDITOR_NAME_PRESENT "present"
#define SE_EDITOR_NAME_PRESENT_FRAME "present_frame"
#define SE_EDITOR_NAME_PREV_FRAMEBUFFER "prev_framebuffer"
#define SE_EDITOR_NAME_PREV_TEXTURE "prev_texture"
#define SE_EDITOR_NAME_PROGRAM "program"
#define SE_EDITOR_NAME_PROJECTION_MATRIX "projection_matrix"
#define SE_EDITOR_NAME_PRESSED "pressed"
#define SE_EDITOR_NAME_PRESSED_PREFIX "pressed_"
#define SE_EDITOR_NAME_QUEUE_CAPACITY "queue_capacity"
#define SE_EDITOR_NAME_QUEUE_SIZE "queue_size"
#define SE_EDITOR_NAME_RATIO "ratio"
#define SE_EDITOR_NAME_RAW_MOUSE_MOTION "raw_mouse_motion"
#define SE_EDITOR_NAME_RAW_MOUSE_MOTION_ENABLED "raw_mouse_motion_enabled"
#define SE_EDITOR_NAME_RAW_MOUSE_MOTION_SUPPORTED "raw_mouse_motion_supported"
#define SE_EDITOR_NAME_READY_EVENT_COUNT "ready_event_count"
#define SE_EDITOR_NAME_RECORD_CLEAR "record_clear"
#define SE_EDITOR_NAME_RECORDED_COUNT "recorded_count"
#define SE_EDITOR_NAME_RECORDING_ENABLED "recording_enabled"
#define SE_EDITOR_NAME_RECORD_START "record_start"
#define SE_EDITOR_NAME_RECORD_STOP "record_stop"
#define SE_EDITOR_NAME_RELOAD_CHANGED_SHADERS "reload_changed_shaders"
#define SE_EDITOR_NAME_RELOAD_IF_CHANGED "reload_if_changed"
#define SE_EDITOR_NAME_REMOVE "remove"
#define SE_EDITOR_NAME_REMOVE_OBJECT "remove_object"
#define SE_EDITOR_NAME_REMOVE_POST_PROCESS_BUFFER "remove_post_process_buffer"
#define SE_EDITOR_NAME_RENDER "render"
#define SE_EDITOR_NAME_RENDER_BACKEND "render_backend"
#define SE_EDITOR_NAME_RENDER_BUFFER "render_buffer"
#define SE_EDITOR_NAME_RENDER_BUFFER_U "render_buffer[%u]"
#define SE_EDITOR_NAME_RENDER_CLEAR "render_clear"
#define SE_EDITOR_NAME_RENDER_GENERATION "render_generation"
#define SE_EDITOR_NAME_RENDER_HAS_CONTEXT "render_has_context"
#define SE_EDITOR_NAME_RENDER_INIT "render_init"
#define SE_EDITOR_NAME_RENDER_QUAD "render_quad"
#define SE_EDITOR_NAME_RENDER_RAW "render_raw"
#define SE_EDITOR_NAME_RENDER_SCREEN "render_screen"
#define SE_EDITOR_NAME_RENDER_SET_BACKGROUND_COLOR "render_set_background_color"
#define SE_EDITOR_NAME_RENDER_SET_BLENDING "render_set_blending"
#define SE_EDITOR_NAME_RENDER_SHUTDOWN "render_shutdown"
#define SE_EDITOR_NAME_RENDER_TO_BUFFER "render_to_buffer"
#define SE_EDITOR_NAME_RENDER_TO_SCREEN "render_to_screen"
#define SE_EDITOR_NAME_RENDER_UNBIND_FRAMEBUFFER "render_unbind_framebuffer"
#define SE_EDITOR_NAME_REPLAY_ENABLED "replay_enabled"
#define SE_EDITOR_NAME_REPLAY_START "replay_start"
#define SE_EDITOR_NAME_REPLAY_STOP "replay_stop"
#define SE_EDITOR_NAME_RESET "reset"
#define SE_EDITOR_NAME_RESET_DIAGNOSTICS "reset_diagnostics"
#define SE_EDITOR_NAME_RESTITUTION "restitution"
#define SE_EDITOR_NAME_RIGHT "right"
#define SE_EDITOR_NAME_ROTATE "rotate"
#define SE_EDITOR_NAME_ROTATION "rotation"
#define SE_EDITOR_NAME_SAVE_MAPPINGS "save_mappings"
#define SE_EDITOR_NAME_SCALE "scale"
#define SE_EDITOR_NAME_SCENE "scene"
#define SE_EDITOR_NAME_SCENE2 "scene2"
#define SE_EDITOR_NAME_SCENE2D "scene2d"
#define SE_EDITOR_NAME_SCENE2D_MS "scene2d_ms"
#define SE_EDITOR_NAME_SCENE2D_U "scene2d[%u]"
#define SE_EDITOR_NAME_SCENE3 "scene3"
#define SE_EDITOR_NAME_SCENE3D "scene3d"
#define SE_EDITOR_NAME_SCENE3D_MS "scene3d_ms"
#define SE_EDITOR_NAME_SCENE3D_U "scene3d[%u]"
#define SE_EDITOR_NAME_SCROLLBOX_SELECTED "scrollbox_selected"
#define SE_EDITOR_NAME_SCROLLBOX_SET_SELECTED "scrollbox_set_selected"
#define SE_EDITOR_NAME_SCROLLBOX_SINGLE_SELECT "scrollbox_single_select"
#define SE_EDITOR_NAME_SCROLL_DELTA "scroll_delta"
#define SE_EDITOR_NAME_SCROLL_DX "scroll_dx"
#define SE_EDITOR_NAME_SCROLL_DY "scroll_dy"
#define SE_EDITOR_NAME_SCROLL_EVENTS "scroll_events"
#define SE_EDITOR_NAME_SCROLL_TO_ITEM "scroll_to_item"
#define SE_EDITOR_NAME_SEEK "seek"
#define SE_EDITOR_NAME_SET_ASPECT_FROM_WINDOW "set_aspect_from_window"
#define SE_EDITOR_NAME_SET_AUTO_RESIZE "set_auto_resize"
#define SE_EDITOR_NAME_SET_CELL_BLOCKED "set_cell_blocked"
#define SE_EDITOR_NAME_SET_CIRCLE_BLOCKED "set_circle_blocked"
#define SE_EDITOR_NAME_SET_DYNAMIC_CELL_BLOCKED "set_dynamic_cell_blocked"
#define SE_EDITOR_NAME_SET_DYNAMIC_RECT_BLOCKED "set_dynamic_rect_blocked"
#define SE_EDITOR_NAME_SET_EXIT_KEY "set_exit_key"
#define SE_EDITOR_NAME_SET_RECT_BLOCKED "set_rect_blocked"
#define SE_EDITOR_NAME_SET_SELECTED "set_selected"
#define SE_EDITOR_NAME_SET_STACK_HORIZONTAL "set_stack_horizontal"
#define SE_EDITOR_NAME_SET_STACK_VERTICAL "set_stack_vertical"
#define SE_EDITOR_NAME_SFX_VOLUME "sfx_volume"
#define SE_EDITOR_NAME_SHADER "shader"
#define SE_EDITOR_NAME_SHADER_PROFILE "shader_profile"
#define SE_EDITOR_NAME_SHADER_U "shader[%u]"
#define SE_EDITOR_NAME_SHAPE_COUNT "shape_count"
#define SE_EDITOR_NAME_SHAPES_PER_BODY "shapes_per_body"
#define SE_EDITOR_NAME_SHOULD_CLOSE "should_close"
#define SE_EDITOR_NAME_SIMULATION "simulation"
#define SE_EDITOR_NAME_SIMULATION_U "simulation[%u]"
#define SE_EDITOR_NAME_SIZE "size"
#define SE_EDITOR_NAME_SMOOTH_POSITION "smooth_position"
#define SE_EDITOR_NAME_SMOOTH_TARGET "smooth_target"
#define SE_EDITOR_NAME_SNAPSHOT_LOAD_FILE "snapshot_load_file"
#define SE_EDITOR_NAME_SNAPSHOT_SAVE_FILE "snapshot_save_file"
#define SE_EDITOR_NAME_SOLVER_ITERATIONS "solver_iterations"
#define SE_EDITOR_NAME_SPAWNED_PARTICLES "spawned_particles"
#define SE_EDITOR_NAME_STEP "step"
#define SE_EDITOR_NAME_STOP "stop"
#define SE_EDITOR_NAME_STOP_ALL "stop_all"
#define SE_EDITOR_NAME_STOP_UNTRACK "stop_untrack"
#define SE_EDITOR_NAME_STYLE_PRESET "style_preset"
#define SE_EDITOR_NAME_TARGET "target"
#define SE_EDITOR_NAME_TARGET_FPS "target_fps"
#define SE_EDITOR_NAME_TEXT "text"
#define SE_EDITOR_NAME_TEXT_MS "text_ms"
#define SE_EDITOR_NAME_TEXTURE "texture"
#define SE_EDITOR_NAME_TEXTURE_ID "texture_id"
#define SE_EDITOR_NAME_TEXTURE_SIZE "texture_size"
#define SE_EDITOR_NAME_TEXTURE_U "texture[%u]"
#define SE_EDITOR_NAME_THEME_PRESET "theme_preset"
#define SE_EDITOR_NAME_TICK "tick"
#define SE_EDITOR_NAME_TICK_WINDOW "tick_window"
#define SE_EDITOR_NAME_TIME "time"
#define SE_EDITOR_NAME_TOP_CONTEXT "top_context"
#define SE_EDITOR_NAME_TORQUE "torque"
#define SE_EDITOR_NAME_TRACE_CLEAR "trace_clear"
#define SE_EDITOR_NAME_TRACE_CPU_MS_LAST_FRAME "trace_cpu_ms_last_frame"
#define SE_EDITOR_NAME_TRACE_EVENT_COUNT "trace_event_count"
#define SE_EDITOR_NAME_TRACE_GPU_MS_LAST_FRAME "trace_gpu_ms_last_frame"
#define SE_EDITOR_NAME_TRACE_STAT_COUNT_LAST_FRAME "trace_stat_count_last_frame"
#define SE_EDITOR_NAME_TRACE_STAT_COUNT_TOTAL "trace_stat_count_total"
#define SE_EDITOR_NAME_TRACE_THREAD_COUNT_LAST_FRAME "trace_thread_count_last_frame"
#define SE_EDITOR_NAME_TRANSFORM "transform"
#define SE_EDITOR_NAME_TRANSLATE "translate"
#define SE_EDITOR_NAME_TYPE "type"
#define SE_EDITOR_NAME_U64 "u64"
#define SE_EDITOR_NAME_UI "ui"
#define SE_EDITOR_NAME_UI_MS "ui_ms"
#define SE_EDITOR_NAME_UINT "uint"
#define SE_EDITOR_NAME_UI_U "ui[%u]"
#define SE_EDITOR_NAME_UI_WIDGET "ui_widget"
#define SE_EDITOR_NAME_UIWIDGET "uiwidget"
#define SE_EDITOR_NAME_UI_WIDGET_U "ui_widget[%u]"
#define SE_EDITOR_NAME_UNBIND "unbind"
#define SE_EDITOR_NAME_UNBIND_ALL "unbind_all"
#define SE_EDITOR_NAME_UNIFORM "uniform:"
#define SE_EDITOR_NAME_UNIFORM_COUNT "uniform_count"
#define SE_EDITOR_NAME_UNKNOWN "unknown"
#define SE_EDITOR_NAME_UNLOAD "unload"
#define SE_EDITOR_NAME_UNTRACK "untrack"
#define SE_EDITOR_NAME_UP "up"
#define SE_EDITOR_NAME_UPDATE "update"
#define SE_EDITOR_NAME_UPDATE_UNIFORMS "update_uniforms"
#define SE_EDITOR_NAME_USE "use"
#define SE_EDITOR_NAME_USED_EVENT_PAYLOAD_BYTES "used_event_payload_bytes"
#define SE_EDITOR_NAME_USE_ORTHOGRAPHIC "use_orthographic"
#define SE_EDITOR_NAME_VALUE "value"
#define SE_EDITOR_NAME_VEC2 "vec2"
#define SE_EDITOR_NAME_VEC3 "vec3"
#define SE_EDITOR_NAME_VEC4 "vec4"
#define SE_EDITOR_NAME_VELOCITY "velocity"
#define SE_EDITOR_NAME_VERTEX_COUNT "vertex_count"
#define SE_EDITOR_NAME_VERTEX_PATH "vertex_path"
#define SE_EDITOR_NAME_VERTEX_SHADER "vertex_shader"
#define SE_EDITOR_NAME_VFX2D "vfx2d"
#define SE_EDITOR_NAME_VFX2D_U "vfx2d[%u]"
#define SE_EDITOR_NAME_VFX3D "vfx3d"
#define SE_EDITOR_NAME_VFX3D_U "vfx3d[%u]"
#define SE_EDITOR_NAME_VIEW_MATRIX "view_matrix"
#define SE_EDITOR_NAME_VIEW_PROJECTION_MATRIX "view_projection_matrix"
#define SE_EDITOR_NAME_VISIBLE "visible"
#define SE_EDITOR_NAME_VOLUME "volume"
#define SE_EDITOR_NAME_VSYNC "vsync"
#define SE_EDITOR_NAME_WIDGET "widget"
#define SE_EDITOR_NAME_WIDTH "width"
#define SE_EDITOR_NAME_WINDOW "window"
#define SE_EDITOR_NAME_WINDOW_PRESENT_MS "window_present_ms"
#define SE_EDITOR_NAME_WINDOW_U "window[%u]"
#define SE_EDITOR_NAME_WINDOW_UPDATE_MS "window_update_ms"
#define SE_EDITOR_NAME_Z_ORDER "z_order"
#define SE_EDITOR_NAME_STYLE_FIELD_FORMAT "%s_%s"
#define SE_EDITOR_NAME_DISABLED_PREFIX "disabled_"
#define SE_EDITOR_NAME_FOCUSED_PREFIX "focused_"

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

static i32 se_editor_clamp_i32(i32 value, i32 min_value, i32 max_value) {
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}

static b8 se_editor_mask_has(se_editor_category_mask mask, se_editor_category category) {
	if ((u32)category >= (u32)SE_EDITOR_CATEGORY_COUNT || (u32)category >= 64u) {
		return false;
	}
	return (mask & (((se_editor_category_mask)1u) << (u32)category)) != 0u;
}

static se_context* se_editor_context(const se_editor* editor) {
	if (!editor) {
		return NULL;
	}
	if (editor->context) {
		return editor->context;
	}
	return se_current_context();
}

static b8 se_editor_context_has_window(const se_context* context, se_window_handle handle) {
	if (!context || handle == S_HANDLE_NULL) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&context->windows); ++i) {
		if (s_array_handle(&context->windows, (u32)i) == handle) {
			return true;
		}
	}
	return false;
}

static b8 se_editor_context_has_simulation(const se_context* context, se_simulation_handle handle) {
	if (!context || handle == S_HANDLE_NULL) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&context->simulations); ++i) {
		if (s_array_handle(&context->simulations, (u32)i) == handle) {
			return true;
		}
	}
	return false;
}

static b8 se_editor_context_has_vfx_2d(const se_context* context, se_vfx_2d_handle handle) {
	if (!context || handle == S_HANDLE_NULL) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&context->vfx_2ds); ++i) {
		if (s_array_handle(&context->vfx_2ds, (u32)i) == handle) {
			return true;
		}
	}
	return false;
}

static b8 se_editor_context_has_vfx_3d(const se_context* context, se_vfx_3d_handle handle) {
	if (!context || handle == S_HANDLE_NULL) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&context->vfx_3ds); ++i) {
		if (s_array_handle(&context->vfx_3ds, (u32)i) == handle) {
			return true;
		}
	}
	return false;
}

static void se_editor_refresh_defaults_internal(se_editor* editor) {
	se_context* context = se_editor_context(editor);
	if (!editor || !context) {
		return;
	}
	if (editor->window != S_HANDLE_NULL && !se_editor_context_has_window(context, editor->window)) {
		editor->window = S_HANDLE_NULL;
	}
	if (editor->window == S_HANDLE_NULL && s_array_get_size(&context->windows) > 0) {
		editor->window = s_array_handle(&context->windows, 0u);
	}
	if (editor->focused_simulation != S_HANDLE_NULL && !se_editor_context_has_simulation(context, editor->focused_simulation)) {
		editor->focused_simulation = S_HANDLE_NULL;
	}
	if (editor->focused_simulation == S_HANDLE_NULL && s_array_get_size(&context->simulations) > 0) {
		editor->focused_simulation = s_array_handle(&context->simulations, 0u);
	}
	if (editor->focused_vfx_2d != S_HANDLE_NULL && !se_editor_context_has_vfx_2d(context, editor->focused_vfx_2d)) {
		editor->focused_vfx_2d = S_HANDLE_NULL;
	}
	if (editor->focused_vfx_2d == S_HANDLE_NULL && s_array_get_size(&context->vfx_2ds) > 0) {
		editor->focused_vfx_2d = s_array_handle(&context->vfx_2ds, 0u);
	}
	if (editor->focused_vfx_3d != S_HANDLE_NULL && !se_editor_context_has_vfx_3d(context, editor->focused_vfx_3d)) {
		editor->focused_vfx_3d = S_HANDLE_NULL;
	}
	if (editor->focused_vfx_3d == S_HANDLE_NULL && s_array_get_size(&context->vfx_3ds) > 0) {
		editor->focused_vfx_3d = s_array_handle(&context->vfx_3ds, 0u);
	}
}

static b8 se_editor_trace_summary_collect(se_editor_trace_summary* out_summary) {
	se_editor_trace_summary summary = {0};
	const se_debug_trace_event* events = NULL;
	sz event_count = 0;
	u32 total_stats = 0u;
	u32 last_frame_stats = 0u;
	se_debug_trace_stat* stats = NULL;
	u32 stats_count = 0u;

	if (!out_summary) {
		return false;
	}
	if (se_debug_get_trace_events(&events, &event_count)) {
		summary.event_count = (u64)event_count;
		summary.valid = true;
	}
	if (se_debug_get_trace_stats(NULL, 0u, &total_stats, false)) {
		summary.stat_count_total = total_stats;
		summary.valid = true;
	}
	if (se_debug_get_trace_stats(NULL, 0u, &last_frame_stats, true)) {
		summary.stat_count_last_frame = last_frame_stats;
		summary.valid = true;
	}
	if (last_frame_stats > 0u) {
		stats = (se_debug_trace_stat*)calloc(last_frame_stats, sizeof(*stats));
		if (stats && se_debug_get_trace_stats(stats, last_frame_stats, &stats_count, true)) {
			for (u32 i = 0; i < stats_count; ++i) {
				b8 seen_thread = false;
				const se_debug_trace_stat* stat = &stats[i];
				if (stat->channel == (u32)SE_DEBUG_TRACE_CHANNEL_CPU) {
					summary.cpu_ms_last_frame += stat->total_ms;
				} else if (stat->channel == (u32)SE_DEBUG_TRACE_CHANNEL_GPU) {
					summary.gpu_ms_last_frame += stat->total_ms;
				}
				for (u32 j = 0; j < i; ++j) {
					if (stats[j].thread_id == stat->thread_id) {
						seen_thread = true;
						break;
					}
				}
				if (!seen_thread) {
					summary.thread_count_last_frame++;
				}
			}
			summary.valid = true;
		}
	}
	free(stats);
	*out_summary = summary;
	return summary.valid;
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

static void se_editor_push_item(
	se_editor_items* items,
	se_editor_category category,
	s_handle handle,
	s_handle owner_handle,
	void* pointer,
	void* owner_pointer,
	u32 index,
	const c8* label) {
	if (!items) {
		return;
	}
	se_editor_item item = {0};
	item.category = category;
	item.handle = handle;
	item.owner_handle = owner_handle;
	item.pointer = pointer;
	item.owner_pointer = owner_pointer;
	item.index = index;
	se_editor_copy_text(item.label, sizeof(item.label), label);
	s_array_add(items, item);
}

static void se_editor_add_property(se_editor_properties* properties, const c8* name, const se_editor_value* value, b8 editable) {
	if (!properties || !name || !value) {
		return;
	}
	se_editor_property property = {0};
	se_editor_copy_text(property.name, sizeof(property.name), name);
	property.value = *value;
	property.editable = editable;
	s_array_add(properties, property);
}

static void se_editor_add_property_bool(se_editor_properties* properties, const c8* name, b8 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_BOOL;
	editor_value.bool_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_int(se_editor_properties* properties, const c8* name, i32 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_INT;
	editor_value.int_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_uint(se_editor_properties* properties, const c8* name, u32 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_UINT;
	editor_value.uint_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_u64(se_editor_properties* properties, const c8* name, u64 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_U64;
	editor_value.u64_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_float(se_editor_properties* properties, const c8* name, f32 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_FLOAT;
	editor_value.float_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_double(se_editor_properties* properties, const c8* name, f64 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_DOUBLE;
	editor_value.double_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_vec2(se_editor_properties* properties, const c8* name, s_vec2 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_VEC2;
	editor_value.vec2_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_vec3(se_editor_properties* properties, const c8* name, s_vec3 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_VEC3;
	editor_value.vec3_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_vec4(se_editor_properties* properties, const c8* name, s_vec4 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_VEC4;
	editor_value.vec4_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_mat3(se_editor_properties* properties, const c8* name, s_mat3 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_MAT3;
	editor_value.mat3_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_mat4(se_editor_properties* properties, const c8* name, s_mat4 value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_MAT4;
	editor_value.mat4_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_handle(se_editor_properties* properties, const c8* name, s_handle value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_HANDLE;
	editor_value.handle_value = value;
	se_editor_add_property(properties, name, &editor_value, editable);
}

static void se_editor_add_property_text(se_editor_properties* properties, const c8* name, const c8* value, b8 editable) {
	se_editor_value editor_value = {0};
	editor_value.type = SE_EDITOR_VALUE_TEXT;
	se_editor_copy_text(editor_value.text_value, sizeof(editor_value.text_value), value ? value : "");
	se_editor_add_property(properties, name, &editor_value, editable);
}

static b8 se_editor_value_as_bool(const se_editor_value* value, b8* out_value) {
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

static b8 se_editor_value_as_i32(const se_editor_value* value, i32* out_value) {
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

static b8 se_editor_value_as_u32(const se_editor_value* value, u32* out_value) {
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

static b8 se_editor_value_as_u64(const se_editor_value* value, u64* out_value) {
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

static b8 se_editor_value_as_f32(const se_editor_value* value, f32* out_value) {
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

static b8 se_editor_value_as_f64(const se_editor_value* value, f64* out_value) {
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

static b8 se_editor_value_as_handle(const se_editor_value* value, s_handle* out_value) {
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

static b8 se_editor_value_as_vec2(const se_editor_value* value, s_vec2* out_value) {
	if (!value || !out_value || value->type != SE_EDITOR_VALUE_VEC2) {
		return false;
	}
	*out_value = value->vec2_value;
	return true;
}

static b8 se_editor_value_as_vec3(const se_editor_value* value, s_vec3* out_value) {
	if (!value || !out_value || value->type != SE_EDITOR_VALUE_VEC3) {
		return false;
	}
	*out_value = value->vec3_value;
	return true;
}

static b8 se_editor_value_as_vec4(const se_editor_value* value, s_vec4* out_value) {
	if (!value || !out_value || value->type != SE_EDITOR_VALUE_VEC4) {
		return false;
	}
	*out_value = value->vec4_value;
	return true;
}

static b8 se_editor_value_as_mat3(const se_editor_value* value, s_mat3* out_value) {
	if (!value || !out_value || value->type != SE_EDITOR_VALUE_MAT3) {
		return false;
	}
	*out_value = value->mat3_value;
	return true;
}

static b8 se_editor_value_as_mat4(const se_editor_value* value, s_mat4* out_value) {
	if (!value || !out_value || value->type != SE_EDITOR_VALUE_MAT4) {
		return false;
	}
	*out_value = value->mat4_value;
	return true;
}

static const c8* se_editor_value_as_text(const se_editor_value* value) {
	if (!value || value->type != SE_EDITOR_VALUE_TEXT) {
		return NULL;
	}
	return value->text_value;
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
	if (value) {
		command.value = *value;
	} else {
		command.value.type = SE_EDITOR_VALUE_NONE;
	}
	if (aux_value) {
		command.aux_value = *aux_value;
	} else {
		command.aux_value.type = SE_EDITOR_VALUE_NONE;
	}
	return command;
}

static b8 se_editor_command_read_bool(const se_editor_command* command, b8 current_value, b8* out_value) {
	if (!command || !out_value) {
		return false;
	}
	if (command->type == SE_EDITOR_COMMAND_TOGGLE) {
		*out_value = !current_value;
		return true;
	}
	return se_editor_value_as_bool(&command->value, out_value);
}

static se_window_handle se_editor_resolve_window(const se_editor* editor, s_handle candidate) {
	if (candidate != S_HANDLE_NULL) {
		return (se_window_handle)candidate;
	}
	if (editor) {
		return editor->window;
	}
	return S_HANDLE_NULL;
}

static se_window* se_editor_window_ptr(se_editor* editor, se_window_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->windows, handle);
}

static se_camera* se_editor_camera_ptr(se_editor* editor, se_camera_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->cameras, handle);
}

static se_scene_2d* se_editor_scene_2d_ptr(se_editor* editor, se_scene_2d_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->scenes_2d, handle);
}

static se_object_2d* se_editor_object_2d_ptr(se_editor* editor, se_object_2d_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->objects_2d, handle);
}

static se_scene_3d* se_editor_scene_3d_ptr(se_editor* editor, se_scene_3d_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->scenes_3d, handle);
}

static se_object_3d* se_editor_object_3d_ptr(se_editor* editor, se_object_3d_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->objects_3d, handle);
}

static se_model* se_editor_model_ptr(se_editor* editor, se_model_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->models, handle);
}

static se_shader* se_editor_shader_ptr(se_editor* editor, se_shader_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->shaders, handle);
}

static se_texture* se_editor_texture_ptr(se_editor* editor, se_texture_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->textures, handle);
}

static se_framebuffer* se_editor_framebuffer_ptr(se_editor* editor, se_framebuffer_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->framebuffers, handle);
}

static se_render_buffer* se_editor_render_buffer_ptr(se_editor* editor, se_render_buffer_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->render_buffers, handle);
}

static se_font* se_editor_font_ptr(se_editor* editor, se_font_handle handle) {
	se_context* context = se_editor_context(editor);
	if (!context || handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&context->fonts, handle);
}

static se_ui_handle se_editor_find_widget_owner(se_editor* editor, se_ui_widget_handle widget) {
	se_context* context = se_editor_context(editor);
	if (!context || widget == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	for (sz i = 0; i < s_array_get_size(&context->ui_roots); ++i) {
		const se_ui_handle ui = s_array_handle(&context->ui_roots, (u32)i);
		if (se_ui_widget_exists(ui, widget)) {
			return ui;
		}
	}
	return S_HANDLE_NULL;
}

static void se_editor_camera_refresh_axes(se_camera* camera) {
	if (!camera) {
		return;
	}
	s_vec3 forward = s_vec3_sub(&camera->target, &camera->position);
	if (s_vec3_length(&forward) <= 0.00001f) {
		return;
	}
	forward = s_vec3_normalize(&forward);
	s_vec3 right = s_vec3_cross(&forward, &camera->up);
	if (s_vec3_length(&right) > 0.00001f) {
		camera->right = s_vec3_normalize(&right);
	}
	s_vec3 up = s_vec3_cross(&camera->right, &forward);
	if (s_vec3_length(&up) > 0.00001f) {
		camera->up = s_vec3_normalize(&up);
	}
}

static void se_editor_physics_update_mass_2d(se_physics_body_2d* body) {
	if (!body) {
		return;
	}
	if (body->type == SE_PHYSICS_BODY_DYNAMIC && body->mass > 0.000001f) {
		body->inv_mass = 1.0f / body->mass;
	} else {
		body->inv_mass = 0.0f;
	}
}

static void se_editor_physics_update_mass_3d(se_physics_body_3d* body) {
	if (!body) {
		return;
	}
	if (body->type == SE_PHYSICS_BODY_DYNAMIC && body->mass > 0.000001f) {
		body->inv_mass = 1.0f / body->mass;
	} else {
		body->inv_mass = 0.0f;
	}
}

static void se_editor_add_ui_style_state_properties(se_editor_properties* properties, const c8* prefix, const se_ui_style_state* state, b8 editable) {
	c8 name[SE_MAX_NAME_LENGTH] = {0};
	if (!properties || !prefix || !state) {
		return;
	}
	snprintf(name, sizeof(name), SE_EDITOR_NAME_STYLE_FIELD_FORMAT, prefix, SE_EDITOR_NAME_BACKGROUND);
	se_editor_add_property_vec4(properties, name, state->background_color, editable);
	snprintf(name, sizeof(name), SE_EDITOR_NAME_STYLE_FIELD_FORMAT, prefix, SE_EDITOR_NAME_BORDER);
	se_editor_add_property_vec4(properties, name, state->border_color, editable);
	snprintf(name, sizeof(name), SE_EDITOR_NAME_STYLE_FIELD_FORMAT, prefix, SE_EDITOR_NAME_TEXT);
	se_editor_add_property_vec4(properties, name, state->text_color, editable);
	snprintf(name, sizeof(name), SE_EDITOR_NAME_STYLE_FIELD_FORMAT, prefix, SE_EDITOR_NAME_BORDER_WIDTH);
	se_editor_add_property_float(properties, name, state->border_width, editable);
}

static b8 se_editor_ui_style_resolve_state(se_ui_style* style, const c8* name, se_ui_style_state** out_state, const c8** out_suffix) {
	if (!style || !name || !out_state || !out_suffix) {
		return false;
	}
	if (strncmp(name, SE_EDITOR_NAME_NORMAL_PREFIX, sizeof(SE_EDITOR_NAME_NORMAL_PREFIX) - 1u) == 0) {
		*out_state = &style->normal;
		*out_suffix = name + (sizeof(SE_EDITOR_NAME_NORMAL_PREFIX) - 1u);
		return true;
	}
	if (strncmp(name, SE_EDITOR_NAME_HOVERED_PREFIX, sizeof(SE_EDITOR_NAME_HOVERED_PREFIX) - 1u) == 0) {
		*out_state = &style->hovered;
		*out_suffix = name + (sizeof(SE_EDITOR_NAME_HOVERED_PREFIX) - 1u);
		return true;
	}
	if (strncmp(name, SE_EDITOR_NAME_PRESSED_PREFIX, sizeof(SE_EDITOR_NAME_PRESSED_PREFIX) - 1u) == 0) {
		*out_state = &style->pressed;
		*out_suffix = name + (sizeof(SE_EDITOR_NAME_PRESSED_PREFIX) - 1u);
		return true;
	}
	if (strncmp(name, SE_EDITOR_NAME_DISABLED_PREFIX, sizeof(SE_EDITOR_NAME_DISABLED_PREFIX) - 1u) == 0) {
		*out_state = &style->disabled;
		*out_suffix = name + (sizeof(SE_EDITOR_NAME_DISABLED_PREFIX) - 1u);
		return true;
	}
	if (strncmp(name, SE_EDITOR_NAME_FOCUSED_PREFIX, sizeof(SE_EDITOR_NAME_FOCUSED_PREFIX) - 1u) == 0) {
		*out_state = &style->focused;
		*out_suffix = name + (sizeof(SE_EDITOR_NAME_FOCUSED_PREFIX) - 1u);
		return true;
	}
	return false;
}

static b8 se_editor_ui_style_apply_property(se_ui_style* style, const c8* property_name, const se_editor_value* value) {
	se_ui_style_state* state = NULL;
	const c8* suffix = NULL;
	s_vec4 vec4_value = {0};
	f32 f32_value = 0.0f;
	if (!style || !property_name || !value) {
		return false;
	}
	if (!se_editor_ui_style_resolve_state(style, property_name, &state, &suffix)) {
		return false;
	}
	if (strcmp(suffix, SE_EDITOR_NAME_BACKGROUND) == 0) {
		if (!se_editor_value_as_vec4(value, &vec4_value)) {
			return false;
		}
		state->background_color = vec4_value;
		return true;
	}
	if (strcmp(suffix, SE_EDITOR_NAME_BORDER) == 0) {
		if (!se_editor_value_as_vec4(value, &vec4_value)) {
			return false;
		}
		state->border_color = vec4_value;
		return true;
	}
	if (strcmp(suffix, SE_EDITOR_NAME_TEXT) == 0) {
		if (!se_editor_value_as_vec4(value, &vec4_value)) {
			return false;
		}
		state->text_color = vec4_value;
		return true;
	}
	if (strcmp(suffix, SE_EDITOR_NAME_BORDER_WIDTH) == 0) {
		if (!se_editor_value_as_f32(value, &f32_value)) {
			return false;
		}
		state->border_width = f32_value;
		return true;
	}
	return false;
}

static u64 se_editor_navigation_count_blocked_cells(const se_navigation_grid* grid) {
	u64 blocked_count = 0u;
	if (!grid) {
		return 0u;
	}
	for (sz i = 0; i < s_array_get_size(&grid->occupancy); ++i) {
		const u8* cell = s_array_get((se_navigation_occupancy*)&grid->occupancy, s_array_handle(&grid->occupancy, (u32)i));
		if (cell && *cell != 0u) {
			blocked_count++;
		}
	}
	return blocked_count;
}

static b8 se_editor_collect_backend_properties(se_editor* editor) {
	(void)editor;
	const se_backend_info backend_info = se_get_backend_info();
	const se_capabilities capabilities = se_capabilities_get();
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_RENDER_BACKEND, (i32)backend_info.render_backend, false);
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_PLATFORM_BACKEND, (i32)backend_info.platform_backend, false);
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_SHADER_PROFILE, (i32)backend_info.shader_profile, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_PORTABILITY_PROFILE, (u32)backend_info.portability_profile, false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_EXTENSION_APIS_SUPPORTED, backend_info.extension_apis_supported, false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_CAPABILITIES_VALID, capabilities.valid, false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_CAPABILITY_INSTANCING, capabilities.instancing, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_CAPABILITY_MAX_MRT_COUNT, capabilities.max_mrt_count, false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_CAPABILITY_FLOAT_RENDER_TARGETS, capabilities.float_render_targets, false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_CAPABILITY_COMPUTE, capabilities.compute_available, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_CAPABILITY_MAX_TEXTURE_SIZE, capabilities.max_texture_size, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_CAPABILITY_MAX_TEXTURE_UNITS, capabilities.max_texture_units, false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_EXT_INSTANCING_SUPPORTED, se_ext_is_supported(SE_EXT_FEATURE_INSTANCING), false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_EXT_MULTI_RENDER_TARGET_SUPPORTED, se_ext_is_supported(SE_EXT_FEATURE_MULTI_RENDER_TARGET), false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_EXT_FLOAT_RENDER_TARGET_SUPPORTED, se_ext_is_supported(SE_EXT_FEATURE_FLOAT_RENDER_TARGET), false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_EXT_COMPUTE_SUPPORTED, se_ext_is_supported(SE_EXT_FEATURE_COMPUTE), false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_RENDER_HAS_CONTEXT, se_render_has_context(), false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_RENDER_GENERATION, se_render_get_generation(), false);
	return true;
}

static b8 se_editor_collect_debug_properties(se_editor* editor) {
	se_debug_frame_timing timing = {0};
	se_editor_trace_summary trace_summary = {0};
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_LEVEL, (i32)se_debug_get_level(), true);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_CATEGORY_MASK, se_debug_get_category_mask(), true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_OVERLAY_ENABLED, se_debug_is_overlay_enabled(), true);
	if (se_editor_trace_summary_collect(&trace_summary)) {
		se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_TRACE_EVENT_COUNT, trace_summary.event_count, false);
		se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_TRACE_STAT_COUNT_TOTAL, trace_summary.stat_count_total, false);
		se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_TRACE_STAT_COUNT_LAST_FRAME, trace_summary.stat_count_last_frame, false);
		se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_TRACE_THREAD_COUNT_LAST_FRAME, trace_summary.thread_count_last_frame, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_TRACE_CPU_MS_LAST_FRAME, trace_summary.cpu_ms_last_frame, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_TRACE_GPU_MS_LAST_FRAME, trace_summary.gpu_ms_last_frame, false);
	}
	if (se_debug_get_last_frame_timing(&timing)) {
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_FRAME_MS, timing.frame_ms, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_WINDOW_UPDATE_MS, timing.window_update_ms, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_INPUT_MS, timing.input_ms, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_SCENE2D_MS, timing.scene2d_ms, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_SCENE3D_MS, timing.scene3d_ms, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_TEXT_MS, timing.text_ms, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_UI_MS, timing.ui_ms, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_NAVIGATION_MS, timing.navigation_ms, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_WINDOW_PRESENT_MS, timing.window_present_ms, false);
		se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_OTHER_MS, timing.other_ms, false);
	}
	return true;
}

static b8 se_editor_collect_window_properties(se_editor* editor, const se_editor_item* item) {
	se_window* window = se_editor_window_ptr(editor, (se_window_handle)item->handle);
	se_window_handle window_handle = (se_window_handle)item->handle;
	u32 framebuffer_width = 0u;
	u32 framebuffer_height = 0u;
	f32 content_scale_x = 1.0f;
	f32 content_scale_y = 1.0f;
	s_vec2 mouse_ndc = s_vec2(0.0f, 0.0f);
	s_vec2 mouse_delta_ndc = s_vec2(0.0f, 0.0f);
	s_vec2 scroll_delta = s_vec2(0.0f, 0.0f);
	if (!window) {
		return false;
	}
	se_window_diagnostics diagnostics = {0};
	se_window_get_diagnostics(window_handle, &diagnostics);
	se_window_get_framebuffer_size(window_handle, &framebuffer_width, &framebuffer_height);
	se_window_get_content_scale(window_handle, &content_scale_x, &content_scale_y);
	se_window_get_mouse_position_normalized(window_handle, &mouse_ndc);
	se_window_get_mouse_delta_normalized(window_handle, &mouse_delta_ndc);
	se_window_get_scroll_delta(window_handle, &scroll_delta);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_WIDTH, window->width, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_HEIGHT, window->height, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_FRAMEBUFFER_WIDTH, framebuffer_width, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_FRAMEBUFFER_HEIGHT, framebuffer_height, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_TARGET_FPS, window->target_fps, true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_VSYNC, se_window_is_vsync_enabled(window_handle), true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_SHOULD_CLOSE, se_window_should_close(window_handle), true);
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_CURSOR_MODE, (i32)se_window_get_cursor_mode(window_handle), true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_RAW_MOUSE_MOTION_SUPPORTED, se_window_is_raw_mouse_motion_supported(window_handle), false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_RAW_MOUSE_MOTION_ENABLED, se_window_is_raw_mouse_motion_enabled(window_handle), true);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_DELTA_TIME, se_window_get_delta_time(window_handle), false);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_FPS, se_window_get_fps(window_handle), false);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_TIME, se_window_get_time(window_handle), false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_ASPECT, se_window_get_aspect(window_handle), false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_CONTENT_SCALE_X, content_scale_x, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_CONTENT_SCALE_Y, content_scale_y, false);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_MOUSE_NDC, mouse_ndc, false);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_MOUSE_DELTA_NDC, mouse_delta_ndc, false);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_SCROLL_DELTA, scroll_delta, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_FRAME_COUNT, window->frame_count, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_FRAMES_PRESENTED, diagnostics.frames_presented, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_KEY_EVENTS, diagnostics.key_events, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_MOUSE_BUTTON_EVENTS, diagnostics.mouse_button_events, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_MOUSE_MOVE_EVENTS, diagnostics.mouse_move_events, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_SCROLL_EVENTS, diagnostics.scroll_events, false);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_LAST_PRESENT_DURATION, diagnostics.last_present_duration, false);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_LAST_SLEEP_DURATION, diagnostics.last_sleep_duration, false);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_MOUSE_X, window->mouse_x, false);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_MOUSE_Y, window->mouse_y, false);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_MOUSE_DX, window->mouse_dx, false);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_MOUSE_DY, window->mouse_dy, false);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_SCROLL_DX, window->scroll_dx, false);
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_SCROLL_DY, window->scroll_dy, false);
	return true;
}

static b8 se_editor_collect_input_properties(se_editor* editor) {
	se_input_diagnostics diagnostics = {0};
	const se_input_event_record* events = NULL;
	sz event_count = 0u;
	i32 top_context = -1;
	if (!editor->input) {
		return false;
	}
	se_input_get_diagnostics(editor->input, &diagnostics);
	(void)se_input_get_event_queue(editor->input, &events, &event_count);
	(void)se_input_context_peek(editor->input, &top_context);
	(void)events;
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_ENABLED, se_input_is_enabled(editor->input), true);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_ACTION_COUNT, diagnostics.action_count, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_CONTEXT_COUNT, diagnostics.context_count, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_QUEUE_SIZE, diagnostics.queue_size, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_QUEUE_CAPACITY, diagnostics.queue_capacity, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_EVENT_QUEUE_COUNT, (u64)event_count, false);
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_TOP_CONTEXT, top_context, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_RECORDED_COUNT, diagnostics.recorded_count, false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_RECORDING_ENABLED, diagnostics.recording_enabled, false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_REPLAY_ENABLED, diagnostics.replay_enabled, false);
	return true;
}

static b8 se_editor_collect_camera_properties(se_editor* editor, const se_editor_item* item) {
	se_camera* camera = se_editor_camera_ptr(editor, (se_camera_handle)item->handle);
	if (!camera) {
		return false;
	}
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_POSITION, camera->position, true);
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_TARGET, camera->target, true);
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_UP, camera->up, true);
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_RIGHT, camera->right, true);
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_FORWARD, se_camera_get_forward_vector((se_camera_handle)item->handle), false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_FOV, camera->fov, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_NEAR, camera->near, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_FAR, camera->far, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_ASPECT, camera->aspect, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_ORTHO_HEIGHT, camera->ortho_height, true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_USE_ORTHOGRAPHIC, camera->use_orthographic, true);
	se_editor_add_property_mat4(&editor->properties, SE_EDITOR_NAME_VIEW_MATRIX, se_camera_get_view_matrix((se_camera_handle)item->handle), false);
	se_editor_add_property_mat4(&editor->properties, SE_EDITOR_NAME_PROJECTION_MATRIX, se_camera_get_projection_matrix((se_camera_handle)item->handle), false);
	se_editor_add_property_mat4(&editor->properties, SE_EDITOR_NAME_VIEW_PROJECTION_MATRIX, se_camera_get_view_projection_matrix((se_camera_handle)item->handle), false);
	return true;
}

static b8 se_editor_collect_scene_2d_properties(se_editor* editor, const se_editor_item* item) {
	se_scene_2d* scene = se_editor_scene_2d_ptr(editor, (se_scene_2d_handle)item->handle);
	s_vec2 size = s_vec2(0.0f, 0.0f);
	se_framebuffer* framebuffer = NULL;
	if (!scene) {
		return false;
	}
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_OBJECT_COUNT, (u64)s_array_get_size(&scene->objects), false);
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_OUTPUT, scene->output, false);
	if (scene->output != S_HANDLE_NULL) {
		se_framebuffer_get_size(scene->output, &size);
		framebuffer = se_editor_framebuffer_ptr(editor, scene->output);
		se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_OUTPUT_SIZE, size, true);
		if (framebuffer) {
			se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_OUTPUT_RATIO, framebuffer->ratio, true);
			se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_OUTPUT_AUTO_RESIZE, framebuffer->auto_resize, true);
		}
	}
	return true;
}

static b8 se_editor_collect_object_2d_properties(se_editor* editor, const se_editor_item* item) {
	se_object_2d* object = se_editor_object_2d_ptr(editor, (se_object_2d_handle)item->handle);
	if (!object) {
		return false;
	}
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_VISIBLE, object->is_visible, true);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_POSITION, se_object_2d_get_position((se_object_2d_handle)item->handle), true);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_SCALE, se_object_2d_get_scale((se_object_2d_handle)item->handle), true);
	se_editor_add_property_mat3(&editor->properties, SE_EDITOR_NAME_TRANSFORM, se_object_2d_get_transform((se_object_2d_handle)item->handle), true);
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_SHADER, se_object_2d_get_shader((se_object_2d_handle)item->handle), true);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_INSTANCE_COUNT, (u64)se_object_2d_get_instance_count((se_object_2d_handle)item->handle), false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_INACTIVE_SLOT_COUNT, (u64)se_object_2d_get_inactive_slot_count((se_object_2d_handle)item->handle), false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_INSTANCES_DIRTY, se_object_2d_are_instances_dirty((se_object_2d_handle)item->handle), true);
	return true;
}

static b8 se_editor_collect_scene_3d_properties(se_editor* editor, const se_editor_item* item) {
	se_scene_3d* scene = se_editor_scene_3d_ptr(editor, (se_scene_3d_handle)item->handle);
	const se_scene_debug_marker* markers = NULL;
	sz marker_count = 0;
	s_vec2 size = s_vec2(0.0f, 0.0f);
	se_framebuffer* framebuffer = NULL;
	if (!scene) {
		return false;
	}
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_OBJECT_COUNT, (u64)s_array_get_size(&scene->objects), false);
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_CAMERA, se_scene_3d_get_camera((se_scene_3d_handle)item->handle), true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_CULLING, scene->enable_culling, true);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_POST_PROCESS_COUNT, (u64)s_array_get_size(&scene->post_process), false);
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_OUTPUT, scene->output, false);
	if (scene->output != S_HANDLE_NULL) {
		se_framebuffer_get_size(scene->output, &size);
		framebuffer = se_editor_framebuffer_ptr(editor, scene->output);
		se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_OUTPUT_SIZE, size, true);
		if (framebuffer) {
			se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_OUTPUT_RATIO, framebuffer->ratio, true);
			se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_OUTPUT_AUTO_RESIZE, framebuffer->auto_resize, true);
		}
	}
	if (se_scene_3d_get_debug_markers((se_scene_3d_handle)item->handle, &markers, &marker_count)) {
		se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_DEBUG_MARKER_COUNT, (u64)marker_count, false);
	}
	return true;
}

static b8 se_editor_collect_object_3d_properties(se_editor* editor, const se_editor_item* item) {
	se_object_3d* object = se_editor_object_3d_ptr(editor, (se_object_3d_handle)item->handle);
	if (!object) {
		return false;
	}
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_VISIBLE, object->is_visible, true);
	se_editor_add_property_mat4(&editor->properties, SE_EDITOR_NAME_TRANSFORM, se_object_3d_get_transform((se_object_3d_handle)item->handle), true);
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_MODEL, object->model, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_INSTANCE_COUNT, (u64)se_object_3d_get_instance_count((se_object_3d_handle)item->handle), false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_INACTIVE_SLOT_COUNT, (u64)se_object_3d_get_inactive_slot_count((se_object_3d_handle)item->handle), false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_INSTANCES_DIRTY, se_object_3d_are_instances_dirty((se_object_3d_handle)item->handle), true);
	return true;
}

static b8 se_editor_collect_ui_properties(se_editor* editor, const se_editor_item* item) {
	se_ui_handle ui = (se_ui_handle)item->handle;
	if (ui == S_HANDLE_NULL) {
		return false;
	}
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_WINDOW, se_ui_get_window(ui), false);
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_SCENE, se_ui_get_scene(ui), false);
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_HOVERED_WIDGET, se_ui_get_hovered_widget(ui), false);
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_FOCUSED_WIDGET, se_ui_get_focused_widget(ui), true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_DIRTY, se_ui_is_dirty(ui), false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_DEBUG_OVERLAY, se_ui_is_debug_overlay_enabled(), true);
	return true;
}

static b8 se_editor_collect_ui_widget_properties(se_editor* editor, const se_editor_item* item) {
	se_ui_widget_handle widget = (se_ui_widget_handle)item->handle;
	se_ui_handle ui = (se_ui_handle)item->owner_handle;
	se_box_2d bounds = {0};
	se_ui_layout layout = {0};
	se_ui_style style = {0};
	c8 text[SE_MAX_PATH_LENGTH] = {0};
	if (widget == S_HANDLE_NULL) {
		return false;
	}
	if (ui == S_HANDLE_NULL) {
		ui = se_editor_find_widget_owner(editor, widget);
	}
	if (ui == S_HANDLE_NULL || !se_ui_widget_exists(ui, widget)) {
		return false;
	}

	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_UI, ui, false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_VISIBLE, se_ui_widget_is_visible(ui, widget), true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_ENABLED, se_ui_widget_is_enabled(ui, widget), true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_INTERACTABLE, se_ui_widget_is_interactable(ui, widget), true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_CLIPPING, se_ui_widget_is_clipping(ui, widget), true);
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_Z_ORDER, se_ui_widget_get_z_order(ui, widget), true);
	if (se_ui_widget_get_bounds(ui, widget, &bounds)) {
		se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_BOUNDS_MIN, bounds.min, false);
		se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_BOUNDS_MAX, bounds.max, false);
	}
	if (se_ui_widget_get_layout(ui, widget, &layout)) {
		se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_LAYOUT_SPACING, layout.spacing, true);
		se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_LAYOUT_DIRECTION, (i32)layout.direction, true);
		se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_ALIGN_HORIZONTAL, (i32)layout.align_horizontal, true);
		se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_ALIGN_VERTICAL, (i32)layout.align_vertical, true);
		se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_ANCHOR_MIN, layout.anchor_min, true);
		se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_ANCHOR_MAX, layout.anchor_max, true);
		se_editor_add_property_vec4(&editor->properties, SE_EDITOR_NAME_MARGIN, s_vec4(layout.margin.left, layout.margin.right, layout.margin.top, layout.margin.bottom), true);
		se_editor_add_property_vec4(&editor->properties, SE_EDITOR_NAME_PADDING, s_vec4(layout.padding.left, layout.padding.right, layout.padding.top, layout.padding.bottom), true);
		se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_MIN_SIZE, layout.min_size, true);
		se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_MAX_SIZE, layout.max_size, true);
		se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_ANCHORS_ENABLED, layout.use_anchors, true);
		se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_CLIP_CHILDREN, layout.clip_children, true);
	}
	if (se_ui_widget_get_style(ui, widget, &style)) {
		se_editor_add_ui_style_state_properties(&editor->properties, SE_EDITOR_NAME_NORMAL, &style.normal, true);
		se_editor_add_ui_style_state_properties(&editor->properties, SE_EDITOR_NAME_HOVERED, &style.hovered, true);
		se_editor_add_ui_style_state_properties(&editor->properties, SE_EDITOR_NAME_PRESSED, &style.pressed, true);
		se_editor_add_ui_style_state_properties(&editor->properties, SE_EDITOR_NAME_DISABLED, &style.disabled, true);
		se_editor_add_ui_style_state_properties(&editor->properties, SE_EDITOR_NAME_FOCUSED, &style.focused, true);
	}
	(void)se_ui_widget_get_text(ui, widget, text, sizeof(text));
	se_editor_add_property_text(&editor->properties, SE_EDITOR_NAME_TEXT, text, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_VALUE, se_ui_widget_get_value(ui, widget), true);
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_SCROLLBOX_SELECTED, se_ui_scrollbox_get_selected(ui, widget), true);
	return true;
}

static b8 se_editor_collect_vfx_2d_properties(se_editor* editor, const se_editor_item* item) {
	se_vfx_2d_handle vfx = (se_vfx_2d_handle)item->handle;
	se_vfx_diagnostics diagnostics = {0};
	se_framebuffer_handle fb = S_HANDLE_NULL;
	u32 texture_id = 0u;
	if (vfx == S_HANDLE_NULL) {
		return false;
	}
	if (se_vfx_2d_get_diagnostics(vfx, &diagnostics)) {
		se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_EMITTER_COUNT, diagnostics.emitter_count, false);
		se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_ALIVE_PARTICLES, diagnostics.alive_particles, false);
		se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_SPAWNED_PARTICLES, diagnostics.spawned_particles, false);
		se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_EXPIRED_PARTICLES, diagnostics.expired_particles, false);
	}
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_FOCUSED, editor->focused_vfx_2d == vfx, true);
	if (se_vfx_2d_get_framebuffer(vfx, &fb)) {
		se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_FRAMEBUFFER, fb, false);
	}
	if (se_vfx_2d_get_texture_id(vfx, &texture_id)) {
		se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_TEXTURE_ID, texture_id, false);
	}
	return true;
}

static b8 se_editor_collect_vfx_3d_properties(se_editor* editor, const se_editor_item* item) {
	se_vfx_3d_handle vfx = (se_vfx_3d_handle)item->handle;
	se_vfx_diagnostics diagnostics = {0};
	se_framebuffer_handle fb = S_HANDLE_NULL;
	u32 texture_id = 0u;
	if (vfx == S_HANDLE_NULL) {
		return false;
	}
	if (se_vfx_3d_get_diagnostics(vfx, &diagnostics)) {
		se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_EMITTER_COUNT, diagnostics.emitter_count, false);
		se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_ALIVE_PARTICLES, diagnostics.alive_particles, false);
		se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_SPAWNED_PARTICLES, diagnostics.spawned_particles, false);
		se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_EXPIRED_PARTICLES, diagnostics.expired_particles, false);
	}
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_FOCUSED, editor->focused_vfx_3d == vfx, true);
	if (se_vfx_3d_get_framebuffer(vfx, &fb)) {
		se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_FRAMEBUFFER, fb, false);
	}
	if (se_vfx_3d_get_texture_id(vfx, &texture_id)) {
		se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_TEXTURE_ID, texture_id, false);
	}
	return true;
}

static b8 se_editor_collect_simulation_properties(se_editor* editor, const se_editor_item* item) {
	se_simulation_handle simulation = (se_simulation_handle)item->handle;
	se_simulation_diagnostics diagnostics = {0};
	if (simulation == S_HANDLE_NULL) {
		return false;
	}
	se_simulation_get_diagnostics(simulation, &diagnostics);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_ENTITY_CAPACITY, diagnostics.entity_capacity, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_ENTITY_COUNT, diagnostics.entity_count, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_COMPONENT_REGISTRY_COUNT, diagnostics.component_registry_count, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_EVENT_REGISTRY_COUNT, diagnostics.event_registry_count, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_PENDING_EVENT_COUNT, diagnostics.pending_event_count, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_READY_EVENT_COUNT, diagnostics.ready_event_count, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_MAX_EVENTS, diagnostics.max_events, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_USED_EVENT_PAYLOAD_BYTES, diagnostics.used_event_payload_bytes, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_MAX_EVENT_PAYLOAD_BYTES, diagnostics.max_event_payload_bytes, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_CURRENT_TICK, diagnostics.current_tick, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_FIXED_DT, diagnostics.fixed_dt, false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_FOCUSED, editor->focused_simulation == simulation, true);
	return true;
}

static b8 se_editor_collect_model_properties(se_editor* editor, const se_editor_item* item) {
	se_model* model = se_editor_model_ptr(editor, (se_model_handle)item->handle);
	u64 vertex_count = 0u;
	u64 index_count = 0u;
	u32 meshes_with_cpu_data = 0u;
	u32 meshes_with_gpu_data = 0u;
	if (!model) {
		return false;
	}
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_MESH_COUNT, (u64)s_array_get_size(&model->meshes), false);
	for (sz i = 0; i < s_array_get_size(&model->meshes); ++i) {
		const se_mesh* mesh = s_array_get(&model->meshes, s_array_handle(&model->meshes, (u32)i));
		if (!mesh) {
			continue;
		}
		if (se_mesh_has_cpu_data(mesh)) {
			meshes_with_cpu_data++;
			vertex_count += (u64)s_array_get_size(&mesh->cpu.vertices);
			index_count += (u64)s_array_get_size(&mesh->cpu.indices);
		}
		if (se_mesh_has_gpu_data(mesh)) {
			meshes_with_gpu_data++;
		}
	}
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_VERTEX_COUNT, vertex_count, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_INDEX_COUNT, index_count, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_MESHES_WITH_CPU_DATA, meshes_with_cpu_data, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_MESHES_WITH_GPU_DATA, meshes_with_gpu_data, false);
	return true;
}

static b8 se_editor_collect_shader_properties(se_editor* editor, const se_editor_item* item) {
	se_shader* shader = se_editor_shader_ptr(editor, (se_shader_handle)item->handle);
	if (!shader) {
		return false;
	}
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_PROGRAM, shader->program, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_VERTEX_SHADER, shader->vertex_shader, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_FRAGMENT_SHADER, shader->fragment_shader, false);
	se_editor_add_property_text(&editor->properties, SE_EDITOR_NAME_VERTEX_PATH, shader->vertex_path, false);
	se_editor_add_property_text(&editor->properties, SE_EDITOR_NAME_FRAGMENT_PATH, shader->fragment_path, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_UNIFORM_COUNT, (u64)s_array_get_size(&shader->uniforms), false);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_NEEDS_RELOAD, shader->needs_reload, true);
	return true;
}

static b8 se_editor_collect_texture_properties(se_editor* editor, const se_editor_item* item) {
	se_texture* texture = se_editor_texture_ptr(editor, (se_texture_handle)item->handle);
	if (!texture) {
		return false;
	}
	se_editor_add_property_text(&editor->properties, SE_EDITOR_NAME_PATH, texture->path, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_ID, texture->id, false);
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_WIDTH, texture->width, false);
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_HEIGHT, texture->height, false);
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_CHANNELS, texture->channels, false);
	return true;
}

static b8 se_editor_collect_framebuffer_properties(se_editor* editor, const se_editor_item* item) {
	se_framebuffer* framebuffer = se_editor_framebuffer_ptr(editor, (se_framebuffer_handle)item->handle);
	u32 texture_id = 0u;
	if (!framebuffer) {
		return false;
	}
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_FRAMEBUFFER, framebuffer->framebuffer, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_TEXTURE, framebuffer->texture, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_DEPTH_BUFFER, framebuffer->depth_buffer, false);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_SIZE, framebuffer->size, true);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_RATIO, framebuffer->ratio, true);
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_AUTO_RESIZE, framebuffer->auto_resize, true);
	if (se_framebuffer_get_texture_id((se_framebuffer_handle)item->handle, &texture_id)) {
		se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_TEXTURE_ID, texture_id, false);
	}
	return true;
}

static b8 se_editor_collect_render_buffer_properties(se_editor* editor, const se_editor_item* item) {
	se_render_buffer* render_buffer = se_editor_render_buffer_ptr(editor, (se_render_buffer_handle)item->handle);
	if (!render_buffer) {
		return false;
	}
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_FRAMEBUFFER, render_buffer->framebuffer, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_TEXTURE, render_buffer->texture, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_PREV_FRAMEBUFFER, render_buffer->prev_framebuffer, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_PREV_TEXTURE, render_buffer->prev_texture, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_DEPTH_BUFFER, render_buffer->depth_buffer, false);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_TEXTURE_SIZE, render_buffer->texture_size, false);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_SCALE, render_buffer->scale, true);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_POSITION, render_buffer->position, true);
	se_editor_add_property_handle(&editor->properties, SE_EDITOR_NAME_SHADER, render_buffer->shader, true);
	return true;
}

static b8 se_editor_collect_font_properties(se_editor* editor, const se_editor_item* item) {
	se_font* font = se_editor_font_ptr(editor, (se_font_handle)item->handle);
	if (!font) {
		return false;
	}
	se_editor_add_property_text(&editor->properties, SE_EDITOR_NAME_PATH, s_array_get_data(&font->path), false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_SIZE, font->size, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_ATLAS_TEXTURE, font->atlas_texture, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_ATLAS_WIDTH, font->atlas_width, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_ATLAS_HEIGHT, font->atlas_height, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_FIRST_CHARACTER, font->first_character, false);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_CHARACTERS_COUNT, font->characters_count, false);
	return true;
}

static b8 se_editor_collect_audio_properties(se_editor* editor) {
	if (!editor->audio) {
		return false;
	}
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_MASTER_VOLUME, se_audio_bus_get_volume(editor->audio, SE_AUDIO_BUS_MASTER), true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_MUSIC_VOLUME, se_audio_bus_get_volume(editor->audio, SE_AUDIO_BUS_MUSIC), true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_SFX_VOLUME, se_audio_bus_get_volume(editor->audio, SE_AUDIO_BUS_SFX), true);
	return true;
}

static b8 se_editor_collect_audio_clip_properties(se_editor* editor, const se_editor_item* item) {
	se_audio_band_levels bands = {0};
	se_audio_clip* clip = (se_audio_clip*)item->pointer;
	(void)editor;
	if (!clip) {
		return false;
	}
	se_editor_add_property_bool(&editor->properties, SE_EDITOR_NAME_PLAYING, se_audio_clip_is_playing(clip), false);
	if (se_audio_clip_get_bands(clip, &bands, 2048u)) {
		se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_BAND_LOW, bands.low, false);
		se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_BAND_MID, bands.mid, false);
		se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_BAND_HIGH, bands.high, false);
		se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_BAND_ENERGY, bands.low + bands.mid + bands.high, false);
	}
	return true;
}

static b8 se_editor_collect_audio_stream_properties(se_editor* editor, const se_editor_item* item) {
	se_audio_stream* stream = (se_audio_stream*)item->pointer;
	(void)editor;
	if (!stream) {
		return false;
	}
	se_editor_add_property_double(&editor->properties, SE_EDITOR_NAME_CURSOR, se_audio_stream_get_cursor(stream), true);
	return true;
}

static b8 se_editor_collect_audio_capture_properties(se_editor* editor, const se_editor_item* item) {
	se_audio_capture* capture = (se_audio_capture*)item->pointer;
	se_audio_band_levels bands = {0};
	(void)editor;
	if (!capture) {
		return false;
	}
	if (se_audio_capture_get_bands(capture, &bands)) {
		se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_BAND_LOW, bands.low, false);
		se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_BAND_MID, bands.mid, false);
		se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_BAND_HIGH, bands.high, false);
	}
	return true;
}

static b8 se_editor_collect_navigation_properties(se_editor* editor) {
	u64 blocked_cell_count = 0u;
	u64 occupancy_cells = 0u;
	if (!editor->navigation) {
		return false;
	}
	occupancy_cells = (u64)s_array_get_size(&editor->navigation->occupancy);
	blocked_cell_count = se_editor_navigation_count_blocked_cells(editor->navigation);
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_WIDTH, editor->navigation->width, false);
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_HEIGHT, editor->navigation->height, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_CELL_SIZE, editor->navigation->cell_size, true);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_ORIGIN, editor->navigation->origin, true);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_OCCUPANCY_CELLS, occupancy_cells, false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_BLOCKED_CELLS, blocked_cell_count, false);
	se_editor_add_property_double(
		&editor->properties,
		SE_EDITOR_NAME_BLOCKED_RATIO,
		occupancy_cells > 0u ? (f64)blocked_cell_count / (f64)occupancy_cells : 0.0,
		false);
	return true;
}

static b8 se_editor_collect_physics_2d_properties(se_editor* editor) {
	if (!editor->physics_2d) {
		return false;
	}
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_GRAVITY, editor->physics_2d->gravity, true);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_SOLVER_ITERATIONS, editor->physics_2d->solver_iterations, true);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_SHAPES_PER_BODY, editor->physics_2d->shapes_per_body, true);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_BODY_COUNT, (u64)s_array_get_size(&editor->physics_2d->bodies), false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_CONTACT_COUNT, (u64)s_array_get_size(&editor->physics_2d->contacts), false);
	return true;
}

static b8 se_editor_collect_physics_2d_body_properties(se_editor* editor, const se_editor_item* item) {
	se_physics_body_2d* body = (se_physics_body_2d*)item->pointer;
	(void)editor;
	if (!body || !body->is_valid) {
		return false;
	}
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_TYPE, (i32)body->type, true);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_POSITION, body->position, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_ROTATION, body->rotation, true);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_VELOCITY, body->velocity, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_ANGULAR_VELOCITY, body->angular_velocity, true);
	se_editor_add_property_vec2(&editor->properties, SE_EDITOR_NAME_FORCE, body->force, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_TORQUE, body->torque, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_MASS, body->mass, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_INV_MASS, body->inv_mass, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_INERTIA, body->inertia, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_INV_INERTIA, body->inv_inertia, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_RESTITUTION, body->restitution, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_FRICTION, body->friction, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_LINEAR_DAMPING, body->linear_damping, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_ANGULAR_DAMPING, body->angular_damping, true);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_SHAPE_COUNT, (u64)s_array_get_size(&body->shapes), false);
	return true;
}

static b8 se_editor_collect_physics_3d_properties(se_editor* editor) {
	if (!editor->physics_3d) {
		return false;
	}
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_GRAVITY, editor->physics_3d->gravity, true);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_SOLVER_ITERATIONS, editor->physics_3d->solver_iterations, true);
	se_editor_add_property_uint(&editor->properties, SE_EDITOR_NAME_SHAPES_PER_BODY, editor->physics_3d->shapes_per_body, true);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_BODY_COUNT, (u64)s_array_get_size(&editor->physics_3d->bodies), false);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_CONTACT_COUNT, (u64)s_array_get_size(&editor->physics_3d->contacts), false);
	return true;
}

static b8 se_editor_collect_physics_3d_body_properties(se_editor* editor, const se_editor_item* item) {
	se_physics_body_3d* body = (se_physics_body_3d*)item->pointer;
	(void)editor;
	if (!body || !body->is_valid) {
		return false;
	}
	se_editor_add_property_int(&editor->properties, SE_EDITOR_NAME_TYPE, (i32)body->type, true);
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_POSITION, body->position, true);
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_ROTATION, body->rotation, true);
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_VELOCITY, body->velocity, true);
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_ANGULAR_VELOCITY, body->angular_velocity, true);
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_FORCE, body->force, false);
	se_editor_add_property_vec3(&editor->properties, SE_EDITOR_NAME_TORQUE, body->torque, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_MASS, body->mass, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_INV_MASS, body->inv_mass, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_INERTIA, body->inertia, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_INV_INERTIA, body->inv_inertia, false);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_RESTITUTION, body->restitution, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_FRICTION, body->friction, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_LINEAR_DAMPING, body->linear_damping, true);
	se_editor_add_property_float(&editor->properties, SE_EDITOR_NAME_ANGULAR_DAMPING, body->angular_damping, true);
	se_editor_add_property_u64(&editor->properties, SE_EDITOR_NAME_SHAPE_COUNT, (u64)s_array_get_size(&body->shapes), false);
	return true;
}

static b8 se_editor_apply_backend_command(se_editor* editor, const se_editor_command* command) {
	s_vec4 vec4_value = s_vec4(0.0f, 0.0f, 0.0f, 1.0f);
	b8 bool_value = false;
	(void)editor;
	if (!command) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_RENDER_SET_BACKGROUND_COLOR) == 0) {
		if (!se_editor_value_as_vec4(&command->value, &vec4_value)) {
			return false;
		}
		se_render_set_background_color(vec4_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_RENDER_SET_BLENDING) == 0) {
		if (!se_editor_value_as_bool(&command->value, &bool_value)) {
			return false;
		}
		se_render_set_blending(bool_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RELOAD_CHANGED_SHADERS) == 0) {
		se_context_reload_changed_shaders();
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EXT_REQUIRE_INSTANCING) == 0) {
		return se_ext_require(SE_EXT_FEATURE_INSTANCING);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EXT_REQUIRE_MULTI_RENDER_TARGET) == 0) {
		return se_ext_require(SE_EXT_FEATURE_MULTI_RENDER_TARGET);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EXT_REQUIRE_FLOAT_RENDER_TARGET) == 0) {
		return se_ext_require(SE_EXT_FEATURE_FLOAT_RENDER_TARGET);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EXT_REQUIRE_COMPUTE) == 0) {
		return se_ext_require(SE_EXT_FEATURE_COMPUTE);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_CLEAR) == 0) {
		se_render_clear();
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_UNBIND_FRAMEBUFFER) == 0) {
		se_render_unbind_framebuffer();
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_INIT) == 0) {
		return se_render_init();
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_SHUTDOWN) == 0) {
		se_render_shutdown();
		return true;
	}
	return false;
}

static b8 se_editor_apply_debug_command(se_editor* editor, const se_editor_command* command) {
	i32 i32_value = 0;
	u32 u32_value = 0;
	b8 bool_value = false;
	(void)editor;
	if (strcmp(command->name, SE_EDITOR_NAME_LEVEL) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		se_debug_set_level((se_debug_level)i32_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_CATEGORY_MASK) == 0) {
		if (!se_editor_value_as_u32(&command->value, &u32_value)) {
			return false;
		}
		se_debug_set_category_mask(u32_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_OVERLAY_ENABLED) == 0) {
		if (!se_editor_command_read_bool(command, se_debug_is_overlay_enabled(), &bool_value)) {
			return false;
		}
		se_debug_set_overlay_enabled(bool_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_TRACE_CLEAR) == 0) {
		se_debug_clear_trace_events();
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_FRAME_BEGIN) == 0) {
		se_debug_frame_begin();
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_FRAME_END) == 0) {
		se_debug_frame_end();
		return true;
	}
	return false;
}

static b8 se_editor_apply_window_command(se_editor* editor, const se_editor_command* command) {
	se_window_handle window_handle = se_editor_resolve_window(editor, command->item.handle);
	se_window* window = se_editor_window_ptr(editor, window_handle);
	u32 u32_value = 0;
	i32 i32_value = 0;
	b8 bool_value = false;
	const c8* text_value = NULL;
	s_vec4 clear_color = s_vec4(0.0f, 0.0f, 0.0f, 1.0f);
	if (!window) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_TARGET_FPS) == 0) {
		if (!se_editor_value_as_u32(&command->value, &u32_value)) {
			return false;
		}
		se_window_set_target_fps(window_handle, (u16)u32_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_VSYNC) == 0) {
		if (!se_editor_command_read_bool(command, se_window_is_vsync_enabled(window_handle), &bool_value)) {
			return false;
		}
		se_window_set_vsync(window_handle, bool_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SHOULD_CLOSE) == 0) {
		if (!se_editor_command_read_bool(command, se_window_should_close(window_handle), &bool_value)) {
			return false;
		}
		se_window_set_should_close(window_handle, bool_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_CURSOR_MODE) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		se_window_set_cursor_mode(window_handle, (se_window_cursor_mode)i32_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_RAW_MOUSE_MOTION) == 0 || strcmp(command->name, SE_EDITOR_NAME_RAW_MOUSE_MOTION_ENABLED) == 0) {
		if (!se_editor_command_read_bool(command, se_window_is_raw_mouse_motion_enabled(window_handle), &bool_value)) {
			return false;
		}
		se_window_set_raw_mouse_motion(window_handle, bool_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_BEGIN_FRAME) == 0) {
		se_window_begin_frame(window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_END_FRAME) == 0) {
		se_window_end_frame(window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_TICK) == 0) {
		se_window_tick(window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UPDATE) == 0) {
		se_window_update(window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_QUAD) == 0) {
		se_window_render_quad(window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_SCREEN) == 0) {
		se_window_render_screen(window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_PRESENT) == 0) {
		se_window_present(window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_PRESENT_FRAME) == 0) {
		if (!se_editor_value_as_vec4(&command->value, &clear_color)) {
			clear_color = s_vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		se_window_present_frame(window_handle, &clear_color);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RESET_DIAGNOSTICS) == 0) {
		se_window_reset_diagnostics(window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_EXIT_KEY) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		se_window_set_exit_key(window_handle, (se_key)i32_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CHECK_EXIT_KEYS) == 0) {
		se_window_check_exit_keys(window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMIT_TEXT) == 0) {
		text_value = se_editor_value_as_text(&command->value);
		if (!text_value) {
			return false;
		}
		se_window_emit_text(window_handle, text_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CLEAR_INPUT_STATE) == 0) {
		se_window_clear_input_state(window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_POLL_EVENTS) == 0) {
		se_window_poll_events();
		return true;
	}
	return false;
}

static b8 se_editor_apply_input_command(se_editor* editor, const se_editor_command* command) {
	b8 bool_value = false;
	i32 i32_value = 0;
	const c8* text_value = NULL;
	if (!editor->input) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ENABLED) == 0) {
		if (!se_editor_command_read_bool(command, se_input_is_enabled(editor->input), &bool_value)) {
			return false;
		}
		se_input_set_enabled(editor->input, bool_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_TICK) == 0) {
		se_input_tick(editor->input);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UNBIND_ALL) == 0) {
		se_input_unbind_all(editor->input);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CLEAR_EVENTS) == 0) {
		se_input_clear_event_queue(editor->input);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RECORD_START) == 0) {
		se_input_record_start(editor->input);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RECORD_STOP) == 0) {
		se_input_record_stop(editor->input);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RECORD_CLEAR) == 0) {
		se_input_record_clear(editor->input);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_REPLAY_START) == 0) {
		if (!se_editor_value_as_bool(&command->value, &bool_value)) {
			bool_value = false;
		}
		return se_input_replay_start(editor->input, bool_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_REPLAY_STOP) == 0) {
		se_input_replay_stop(editor->input);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CONTEXT_PUSH) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		return se_input_context_push(editor->input, i32_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CONTEXT_POP) == 0) {
		return se_input_context_pop(editor->input, NULL);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CONTEXT_CLEAR) == 0) {
		se_input_context_clear_stack(editor->input);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ATTACH_WINDOW_TEXT) == 0) {
		se_input_attach_window_text(editor->input);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CONTEXT_SET_ENABLED) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		if (!se_editor_value_as_bool(&command->aux_value, &bool_value)) {
			bool_value = true;
		}
		return se_input_context_set_enabled(editor->input, i32_value, bool_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CONTEXT_CREATE) == 0) {
		const c8* context_name = se_editor_value_as_text(&command->aux_value);
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		if (!context_name) {
			context_name = SE_EDITOR_NAME_CONTEXT;
		}
		return se_input_context_create(editor->input, i32_value, context_name, true);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMIT_TEXT) == 0) {
		text_value = se_editor_value_as_text(&command->value);
		if (!text_value) {
			return false;
		}
		se_input_emit_text(editor->input, text_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SAVE_MAPPINGS) == 0) {
		text_value = se_editor_value_as_text(&command->value);
		if (!text_value) {
			return false;
		}
		return se_input_save_mappings(editor->input, text_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_LOAD_MAPPINGS) == 0) {
		text_value = se_editor_value_as_text(&command->value);
		if (!text_value) {
			return false;
		}
		return se_input_load_mappings(editor->input, text_value);
	}
	return false;
}

static b8 se_editor_apply_camera_command(se_editor* editor, const se_editor_command* command) {
	se_camera_handle camera_handle = (se_camera_handle)command->item.handle;
	se_camera* camera = se_editor_camera_ptr(editor, camera_handle);
	se_window_handle window_handle = S_HANDLE_NULL;
	s_vec3 vec3_value = {0};
	s_vec2 vec2_value = {0};
	s_vec4 vec4_value = {0};
	f32 f32_value = 0.0f;
	b8 bool_value = false;
	s_handle handle_value = S_HANDLE_NULL;
	if (!camera) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_POSITION) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		camera->position = vec3_value;
		se_editor_camera_refresh_axes(camera);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_TARGET) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		camera->target = vec3_value;
		se_editor_camera_refresh_axes(camera);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_UP) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		camera->up = vec3_value;
		se_editor_camera_refresh_axes(camera);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_RIGHT) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		camera->right = vec3_value;
		se_editor_camera_refresh_axes(camera);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_FOV) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		camera->fov = f32_value;
		if (camera->use_orthographic) {
			se_camera_set_orthographic(camera_handle, camera->ortho_height, camera->near, camera->far);
		} else {
			se_camera_set_perspective(camera_handle, camera->fov, camera->near, camera->far);
		}
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_NEAR) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		camera->near = f32_value;
		if (camera->use_orthographic) {
			se_camera_set_orthographic(camera_handle, camera->ortho_height, camera->near, camera->far);
		} else {
			se_camera_set_perspective(camera_handle, camera->fov, camera->near, camera->far);
		}
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_FAR) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		camera->far = f32_value;
		if (camera->use_orthographic) {
			se_camera_set_orthographic(camera_handle, camera->ortho_height, camera->near, camera->far);
		} else {
			se_camera_set_perspective(camera_handle, camera->fov, camera->near, camera->far);
		}
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ASPECT) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		camera->aspect = f32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ORTHO_HEIGHT) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		camera->ortho_height = f32_value;
		if (camera->use_orthographic) {
			se_camera_set_orthographic(camera_handle, camera->ortho_height, camera->near, camera->far);
		}
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_USE_ORTHOGRAPHIC) == 0) {
		if (!se_editor_command_read_bool(command, camera->use_orthographic, &bool_value)) {
			return false;
		}
		if (bool_value) {
			se_camera_set_orthographic(camera_handle, camera->ortho_height, camera->near, camera->far);
		} else {
			se_camera_set_perspective(camera_handle, camera->fov, camera->near, camera->far);
		}
		camera->use_orthographic = bool_value;
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_ASPECT_FROM_WINDOW) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		window_handle = (se_window_handle)handle_value;
		if (window_handle == S_HANDLE_NULL) {
			return false;
		}
		se_camera_set_aspect_from_window(camera_handle, window_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ORBIT) == 0) {
		s_vec3 pivot = camera->target;
		if (!se_editor_value_as_vec4(&command->value, &vec4_value)) {
			return false;
		}
		(void)se_editor_value_as_vec3(&command->aux_value, &pivot);
		se_camera_orbit(camera_handle, &pivot, vec4_value.x, vec4_value.y, vec4_value.z, vec4_value.w);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_PAN_WORLD) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_camera_pan_world(camera_handle, &vec3_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_PAN_LOCAL) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_camera_pan_local(camera_handle, vec3_value.x, vec3_value.y, vec3_value.z);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DOLLY) == 0) {
		s_vec3 pivot = camera->target;
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		(void)se_editor_value_as_vec3(&command->aux_value, &pivot);
		se_camera_dolly(camera_handle, &pivot, vec3_value.x, vec3_value.y, vec3_value.z);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CLAMP_TARGET) == 0) {
		s_vec3 max_bounds = {0};
		if (!se_editor_value_as_vec3(&command->value, &vec3_value) || !se_editor_value_as_vec3(&command->aux_value, &max_bounds)) {
			return false;
		}
		se_camera_clamp_target(camera_handle, &vec3_value, &max_bounds);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SMOOTH_TARGET) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		if (!se_editor_value_as_vec2(&command->aux_value, &vec2_value)) {
			vec2_value = s_vec2(12.0f, 1.0f / 60.0f);
		}
		se_camera_smooth_target(camera_handle, &vec3_value, vec2_value.x, vec2_value.y);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SMOOTH_POSITION) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		if (!se_editor_value_as_vec2(&command->aux_value, &vec2_value)) {
			vec2_value = s_vec2(12.0f, 1.0f / 60.0f);
		}
		se_camera_smooth_position(camera_handle, &vec3_value, vec2_value.x, vec2_value.y);
		return true;
	}
	return false;
}

static b8 se_editor_apply_scene_2d_command(se_editor* editor, const se_editor_command* command) {
	se_scene_2d_handle scene_handle = (se_scene_2d_handle)command->item.handle;
	se_scene_2d* scene = se_editor_scene_2d_ptr(editor, scene_handle);
	s_vec2 vec2_value = {0};
	s_handle handle_value = S_HANDLE_NULL;
	if (!scene) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_OUTPUT_SIZE) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value) || scene->output == S_HANDLE_NULL) {
			return false;
		}
		se_framebuffer_set_size(scene->output, &vec2_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_OUTPUT_RATIO) == 0) {
		se_framebuffer* framebuffer = NULL;
		if (!se_editor_value_as_vec2(&command->value, &vec2_value) || scene->output == S_HANDLE_NULL) {
			return false;
		}
		framebuffer = se_editor_framebuffer_ptr(editor, scene->output);
		if (!framebuffer) {
			return false;
		}
		framebuffer->ratio = vec2_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_OUTPUT_AUTO_RESIZE) == 0) {
		se_framebuffer* framebuffer = NULL;
		b8 bool_value = false;
		if (scene->output == S_HANDLE_NULL) {
			return false;
		}
		framebuffer = se_editor_framebuffer_ptr(editor, scene->output);
		if (!framebuffer || !se_editor_command_read_bool(command, framebuffer->auto_resize, &bool_value)) {
			return false;
		}
		framebuffer->auto_resize = bool_value;
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_AUTO_RESIZE) == 0) {
		if (!se_editor_value_as_vec2(&command->aux_value, &vec2_value)) {
			if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
				vec2_value = s_vec2(1.0f, 1.0f);
			}
			handle_value = editor->window;
		} else if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		if (handle_value == S_HANDLE_NULL) {
			return false;
		}
		se_scene_2d_set_auto_resize(scene_handle, (se_window_handle)handle_value, &vec2_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_BIND) == 0) {
		se_scene_2d_bind(scene_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UNBIND) == 0) {
		se_scene_2d_unbind(scene_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_RAW) == 0) {
		se_scene_2d_render_raw(scene_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_TO_BUFFER) == 0) {
		se_scene_2d_render_to_buffer(scene_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_TO_SCREEN) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		if (handle_value == S_HANDLE_NULL) {
			return false;
		}
		se_scene_2d_render_to_screen(scene_handle, (se_window_handle)handle_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DRAW) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		if (handle_value == S_HANDLE_NULL) {
			return false;
		}
		se_scene_2d_draw(scene_handle, (se_window_handle)handle_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ADD_OBJECT) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		se_scene_2d_add_object(scene_handle, (se_object_2d_handle)handle_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_REMOVE_OBJECT) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		se_scene_2d_remove_object(scene_handle, (se_object_2d_handle)handle_value);
		return true;
	}
	return false;
}

static b8 se_editor_apply_object_2d_command(se_editor* editor, const se_editor_command* command) {
	se_object_2d_handle object_handle = (se_object_2d_handle)command->item.handle;
	se_object_2d* object = se_editor_object_2d_ptr(editor, object_handle);
	s_vec2 vec2_value = {0};
	s_mat3 mat3_value = s_mat3_identity;
	s_mat4 mat4_value = s_mat4_identity;
	s_handle handle_value = S_HANDLE_NULL;
	i32 instance_id = 0;
	b8 bool_value = false;
	if (!object) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_VISIBLE) == 0) {
		if (!se_editor_command_read_bool(command, object->is_visible, &bool_value)) {
			return false;
		}
		object->is_visible = bool_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_POSITION) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		se_object_2d_set_position(object_handle, &vec2_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SCALE) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		se_object_2d_set_scale(object_handle, &vec2_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_TRANSFORM) == 0) {
		if (!se_editor_value_as_mat3(&command->value, &mat3_value)) {
			return false;
		}
		se_object_2d_set_transform(object_handle, &mat3_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SHADER) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		se_object_2d_set_shader(object_handle, (se_shader_handle)handle_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_INSTANCES_DIRTY) == 0) {
		if (!se_editor_command_read_bool(command, se_object_2d_are_instances_dirty(object_handle), &bool_value)) {
			return false;
		}
		se_object_2d_set_instances_dirty(object_handle, bool_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UPDATE_UNIFORMS) == 0) {
		se_object_2d_update_uniforms(object_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_INSTANCE_REMOVE) == 0) {
		if (!se_editor_value_as_i32(&command->value, &instance_id)) {
			return false;
		}
		return se_object_2d_remove_instance(object_handle, instance_id);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_INSTANCE_SET_ACTIVE) == 0) {
		if (!se_editor_value_as_i32(&command->value, &instance_id) || !se_editor_value_as_bool(&command->aux_value, &bool_value)) {
			return false;
		}
		return se_object_2d_set_instance_active(object_handle, instance_id, bool_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_INSTANCE_SET_TRANSFORM) == 0) {
		if (!se_editor_value_as_i32(&command->value, &instance_id) || !se_editor_value_as_mat3(&command->aux_value, &mat3_value)) {
			return false;
		}
		se_object_2d_set_instance_transform(object_handle, instance_id, &mat3_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_INSTANCE_SET_BUFFER) == 0) {
		if (!se_editor_value_as_i32(&command->value, &instance_id) || !se_editor_value_as_mat4(&command->aux_value, &mat4_value)) {
			return false;
		}
		se_object_2d_set_instance_buffer(object_handle, instance_id, &mat4_value);
		return true;
	}
	return false;
}

static b8 se_editor_apply_scene_3d_command(se_editor* editor, const se_editor_command* command) {
	se_scene_3d_handle scene_handle = (se_scene_3d_handle)command->item.handle;
	se_scene_3d* scene = se_editor_scene_3d_ptr(editor, scene_handle);
	s_vec2 vec2_value = {0};
	s_handle handle_value = S_HANDLE_NULL;
	b8 bool_value = false;
	if (!scene) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_CAMERA) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		se_scene_3d_set_camera(scene_handle, (se_camera_handle)handle_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_CULLING) == 0) {
		if (!se_editor_command_read_bool(command, scene->enable_culling, &bool_value)) {
			return false;
		}
		se_scene_3d_set_culling(scene_handle, bool_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_OUTPUT_SIZE) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value) || scene->output == S_HANDLE_NULL) {
			return false;
		}
		se_framebuffer_set_size(scene->output, &vec2_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_OUTPUT_RATIO) == 0) {
		se_framebuffer* framebuffer = NULL;
		if (!se_editor_value_as_vec2(&command->value, &vec2_value) || scene->output == S_HANDLE_NULL) {
			return false;
		}
		framebuffer = se_editor_framebuffer_ptr(editor, scene->output);
		if (!framebuffer) {
			return false;
		}
		framebuffer->ratio = vec2_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_OUTPUT_AUTO_RESIZE) == 0) {
		se_framebuffer* framebuffer = NULL;
		if (scene->output == S_HANDLE_NULL) {
			return false;
		}
		framebuffer = se_editor_framebuffer_ptr(editor, scene->output);
		if (!framebuffer || !se_editor_command_read_bool(command, framebuffer->auto_resize, &bool_value)) {
			return false;
		}
		framebuffer->auto_resize = bool_value;
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_AUTO_RESIZE) == 0) {
		if (!se_editor_value_as_vec2(&command->aux_value, &vec2_value)) {
			if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
				vec2_value = s_vec2(1.0f, 1.0f);
			}
			handle_value = editor->window;
		} else if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		if (handle_value == S_HANDLE_NULL) {
			return false;
		}
		se_scene_3d_set_auto_resize(scene_handle, (se_window_handle)handle_value, &vec2_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_TO_BUFFER) == 0) {
		se_scene_3d_render_to_buffer(scene_handle);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER_TO_SCREEN) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		if (handle_value == S_HANDLE_NULL) {
			return false;
		}
		se_scene_3d_render_to_screen(scene_handle, (se_window_handle)handle_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DRAW) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		if (handle_value == S_HANDLE_NULL) {
			return false;
		}
		se_scene_3d_draw(scene_handle, (se_window_handle)handle_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ADD_OBJECT) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		se_scene_3d_add_object(scene_handle, (se_object_3d_handle)handle_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_REMOVE_OBJECT) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		se_scene_3d_remove_object(scene_handle, (se_object_3d_handle)handle_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ADD_POST_PROCESS_BUFFER) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		se_scene_3d_add_post_process_buffer(scene_handle, (se_render_buffer_handle)handle_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_REMOVE_POST_PROCESS_BUFFER) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		se_scene_3d_remove_post_process_buffer(scene_handle, (se_render_buffer_handle)handle_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CLEAR_DEBUG_MARKERS) == 0) {
		se_scene_3d_clear_debug_markers(scene_handle);
		return true;
	}
	return false;
}

static b8 se_editor_apply_object_3d_command(se_editor* editor, const se_editor_command* command) {
	se_object_3d_handle object_handle = (se_object_3d_handle)command->item.handle;
	se_object_3d* object = se_editor_object_3d_ptr(editor, object_handle);
	s_mat4 mat4_value = s_mat4_identity;
	i32 instance_id = 0;
	b8 bool_value = false;
	if (!object) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_VISIBLE) == 0) {
		if (!se_editor_command_read_bool(command, object->is_visible, &bool_value)) {
			return false;
		}
		object->is_visible = bool_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_TRANSFORM) == 0) {
		if (!se_editor_value_as_mat4(&command->value, &mat4_value)) {
			return false;
		}
		se_object_3d_set_transform(object_handle, &mat4_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_INSTANCES_DIRTY) == 0) {
		if (!se_editor_command_read_bool(command, se_object_3d_are_instances_dirty(object_handle), &bool_value)) {
			return false;
		}
		se_object_3d_set_instances_dirty(object_handle, bool_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_INSTANCE_REMOVE) == 0) {
		if (!se_editor_value_as_i32(&command->value, &instance_id)) {
			return false;
		}
		return se_object_3d_remove_instance(object_handle, instance_id);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_INSTANCE_SET_ACTIVE) == 0) {
		if (!se_editor_value_as_i32(&command->value, &instance_id) || !se_editor_value_as_bool(&command->aux_value, &bool_value)) {
			return false;
		}
		return se_object_3d_set_instance_active(object_handle, instance_id, bool_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_INSTANCE_SET_TRANSFORM) == 0) {
		if (!se_editor_value_as_i32(&command->value, &instance_id) || !se_editor_value_as_mat4(&command->aux_value, &mat4_value)) {
			return false;
		}
		se_object_3d_set_instance_transform(object_handle, instance_id, &mat4_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_INSTANCE_SET_BUFFER) == 0) {
		if (!se_editor_value_as_i32(&command->value, &instance_id) || !se_editor_value_as_mat4(&command->aux_value, &mat4_value)) {
			return false;
		}
		se_object_3d_set_instance_buffer(object_handle, instance_id, &mat4_value);
		return true;
	}
	return false;
}

static b8 se_editor_apply_ui_command(se_editor* editor, const se_editor_command* command) {
	se_ui_handle ui = (se_ui_handle)command->item.handle;
	s_handle handle_value = S_HANDLE_NULL;
	i32 i32_value = 0;
	if (ui == S_HANDLE_NULL) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_FOCUSED_WIDGET) == 0 || strcmp(command->name, SE_EDITOR_NAME_FOCUS_WIDGET) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		if ((se_ui_widget_handle)handle_value == S_HANDLE_NULL) {
			se_ui_clear_focus(ui);
			return true;
		}
		return se_ui_focus_widget(ui, (se_ui_widget_handle)handle_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_DEBUG_OVERLAY) == 0) {
		b8 overlay = se_ui_is_debug_overlay_enabled();
		if (!se_editor_command_read_bool(command, overlay, &overlay)) {
			return false;
		}
		se_ui_set_debug_overlay(overlay);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_THEME_PRESET) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		return se_ui_theme_apply(ui, (se_ui_theme_preset)i32_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_TICK) == 0) {
		se_ui_tick(ui);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DRAW) == 0) {
		se_ui_draw(ui);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CLEAR_FOCUS) == 0) {
		se_ui_clear_focus(ui);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_MARK_LAYOUT_DIRTY) == 0) {
		se_ui_mark_layout_dirty(ui);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_MARK_VISUAL_DIRTY) == 0) {
		se_ui_mark_visual_dirty(ui);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_MARK_TEXT_DIRTY) == 0) {
		se_ui_mark_text_dirty(ui);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_MARK_STRUCTURE_DIRTY) == 0) {
		se_ui_mark_structure_dirty(ui);
		return true;
	}
	return false;
}

static b8 se_editor_apply_ui_widget_command(se_editor* editor, const se_editor_command* command) {
	se_ui_widget_handle widget = (se_ui_widget_handle)command->item.handle;
	se_ui_handle ui = (se_ui_handle)command->item.owner_handle;
	b8 bool_value = false;
	i32 i32_value = 0;
	f32 f32_value = 0.0f;
	s_vec2 vec2_value = {0};
	s_vec4 vec4_value = {0};
	s_handle handle_value = S_HANDLE_NULL;
	const c8* text_value = NULL;
	se_ui_layout layout = {0};
	se_ui_style style = {0};
	se_ui_style_state* style_state = NULL;
	const c8* style_suffix = NULL;
	if (widget == S_HANDLE_NULL) {
		return false;
	}
	if (ui == S_HANDLE_NULL) {
		ui = se_editor_find_widget_owner(editor, widget);
	}
	if (ui == S_HANDLE_NULL || !se_ui_widget_exists(ui, widget)) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_VISIBLE) == 0) {
		if (!se_editor_command_read_bool(command, se_ui_widget_is_visible(ui, widget), &bool_value)) {
			return false;
		}
		return se_ui_widget_set_visible(ui, widget, bool_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ENABLED) == 0) {
		if (!se_editor_command_read_bool(command, se_ui_widget_is_enabled(ui, widget), &bool_value)) {
			return false;
		}
		return se_ui_widget_set_enabled(ui, widget, bool_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_INTERACTABLE) == 0) {
		if (!se_editor_command_read_bool(command, se_ui_widget_is_interactable(ui, widget), &bool_value)) {
			return false;
		}
		return se_ui_widget_set_interactable(ui, widget, bool_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_CLIPPING) == 0 || strcmp(command->name, SE_EDITOR_NAME_CLIP_CHILDREN) == 0) {
		if (!se_editor_command_read_bool(command, se_ui_widget_is_clipping(ui, widget), &bool_value)) {
			return false;
		}
		return se_ui_widget_set_clipping(ui, widget, bool_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_Z_ORDER) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		return se_ui_widget_set_z_order(ui, widget, i32_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_TEXT) == 0) {
		text_value = se_editor_value_as_text(&command->value);
		if (!text_value) {
			return false;
		}
		return se_ui_widget_set_text(ui, widget, text_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_VALUE) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		return se_ui_widget_set_value(ui, widget, f32_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_POSITION) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		return se_ui_widget_set_position(ui, widget, &vec2_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SIZE) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		return se_ui_widget_set_size(ui, widget, &vec2_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_LAYOUT_SPACING) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value) || !se_ui_widget_get_layout(ui, widget, &layout)) {
			return false;
		}
		layout.spacing = f32_value;
		return se_ui_widget_set_layout(ui, widget, &layout);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_LAYOUT_DIRECTION) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value) || !se_ui_widget_get_layout(ui, widget, &layout)) {
			return false;
		}
		layout.direction = (se_ui_layout_direction)i32_value;
		return se_ui_widget_set_layout(ui, widget, &layout);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ALIGN_HORIZONTAL) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		return se_ui_widget_set_alignment(ui, widget, (se_ui_alignment)i32_value, se_ui_widget_get_layout(ui, widget, &layout) ? layout.align_vertical : SE_UI_ALIGN_START);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ALIGN_VERTICAL) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		return se_ui_widget_set_alignment(ui, widget, se_ui_widget_get_layout(ui, widget, &layout) ? layout.align_horizontal : SE_UI_ALIGN_START, (se_ui_alignment)i32_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_MARGIN) == 0) {
		if (!se_editor_value_as_vec4(&command->value, &vec4_value)) {
			return false;
		}
		return se_ui_widget_set_margin(ui, widget, (se_ui_edge){ .left = vec4_value.x, .right = vec4_value.y, .top = vec4_value.z, .bottom = vec4_value.w });
	}
	if (strcmp(command->name, SE_EDITOR_NAME_PADDING) == 0) {
		if (!se_editor_value_as_vec4(&command->value, &vec4_value)) {
			return false;
		}
		return se_ui_widget_set_padding(ui, widget, (se_ui_edge){ .left = vec4_value.x, .right = vec4_value.y, .top = vec4_value.z, .bottom = vec4_value.w });
	}
	if (strcmp(command->name, SE_EDITOR_NAME_MIN_SIZE) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		return se_ui_widget_set_min_size(ui, widget, vec2_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_MAX_SIZE) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		return se_ui_widget_set_max_size(ui, widget, vec2_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ANCHOR_MIN) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value) || !se_ui_widget_get_layout(ui, widget, &layout)) {
			return false;
		}
		layout.anchor_min = vec2_value;
		layout.use_anchors = true;
		return se_ui_widget_set_anchor_rect(ui, widget, layout.anchor_min, layout.anchor_max);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ANCHOR_MAX) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value) || !se_ui_widget_get_layout(ui, widget, &layout)) {
			return false;
		}
		layout.anchor_max = vec2_value;
		layout.use_anchors = true;
		return se_ui_widget_set_anchor_rect(ui, widget, layout.anchor_min, layout.anchor_max);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ANCHORS_ENABLED) == 0) {
		if (!se_ui_widget_get_layout(ui, widget, &layout) || !se_editor_command_read_bool(command, layout.use_anchors, &bool_value)) {
			return false;
		}
		layout.use_anchors = bool_value;
		return se_ui_widget_set_layout(ui, widget, &layout);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_STYLE_PRESET) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		return se_ui_widget_use_style_preset(ui, widget, (se_ui_style_preset)i32_value);
	}
	if (se_editor_ui_style_resolve_state(&style, command->name, &style_state, &style_suffix) && style_state && style_suffix) {
		if (!se_ui_widget_get_style(ui, widget, &style)) {
			return false;
		}
		if (!se_editor_ui_style_apply_property(&style, command->name, &command->value)) {
			return false;
		}
		return se_ui_widget_set_style(ui, widget, &style);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_FOCUS) == 0) {
		return se_ui_focus_widget(ui, widget);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DETACH) == 0) {
		return se_ui_widget_detach(ui, widget);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_REMOVE) == 0) {
		return se_ui_widget_remove(ui, widget);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ATTACH_TO) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		return se_ui_widget_attach(ui, (se_ui_widget_handle)handle_value, widget);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_STACK_VERTICAL) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			f32_value = 0.01f;
		}
		return se_ui_widget_set_stack_vertical(ui, widget, f32_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_STACK_HORIZONTAL) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			f32_value = 0.01f;
		}
		return se_ui_widget_set_stack_horizontal(ui, widget, f32_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SCROLL_TO_ITEM) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		return se_ui_scrollbox_scroll_to_item(ui, widget, (se_ui_widget_handle)handle_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && (strcmp(command->name, SE_EDITOR_NAME_SET_SELECTED) == 0 || strcmp(command->name, SE_EDITOR_NAME_SCROLLBOX_SET_SELECTED) == 0)) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		return se_ui_scrollbox_set_selected(ui, widget, (se_ui_widget_handle)handle_value);
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SCROLLBOX_SELECTED) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		return se_ui_scrollbox_set_selected(ui, widget, (se_ui_widget_handle)handle_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SCROLLBOX_SINGLE_SELECT) == 0) {
		if (!se_editor_value_as_bool(&command->value, &bool_value)) {
			bool_value = true;
		}
		return se_ui_scrollbox_enable_single_select(ui, widget, bool_value);
	}
	return false;
}

static b8 se_editor_vfx_resolve_emitter(const se_editor_command* command, s_handle* out_emitter) {
	if (!command || !out_emitter) {
		return false;
	}
	if (command->item.owner_handle != S_HANDLE_NULL) {
		*out_emitter = command->item.owner_handle;
		return true;
	}
	return se_editor_value_as_handle(&command->value, out_emitter);
}

static b8 se_editor_apply_vfx_2d_command(se_editor* editor, const se_editor_command* command) {
	se_vfx_2d_handle vfx = (se_vfx_2d_handle)command->item.handle;
	s_handle emitter = S_HANDLE_NULL;
	s_handle handle_value = S_HANDLE_NULL;
	u32 u32_value = 0u;
	s_vec4 vec4_value = {0};
	f32 f32_value = 0.0f;
	b8 bool_value = false;
	se_window_handle window_handle = S_HANDLE_NULL;
	if (vfx == S_HANDLE_NULL) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_FOCUSED) == 0) {
		if (!se_editor_command_read_bool(command, editor->focused_vfx_2d == vfx, &bool_value)) {
			return false;
		}
		se_editor_set_focused_vfx_2d(editor, bool_value ? vfx : S_HANDLE_NULL);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_TICK) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			f32_value = 1.0f / 60.0f;
		}
		return se_vfx_2d_tick(vfx, f32_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_TICK_WINDOW) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		window_handle = (se_window_handle)handle_value;
		if (window_handle == S_HANDLE_NULL) {
			return false;
		}
		return se_vfx_2d_tick_window(vfx, window_handle);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		window_handle = (se_window_handle)handle_value;
		if (window_handle == S_HANDLE_NULL) {
			return false;
		}
		return se_vfx_2d_render(vfx, window_handle);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DRAW) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		window_handle = (se_window_handle)handle_value;
		if (window_handle == S_HANDLE_NULL) {
			return false;
		}
		return se_vfx_2d_draw(vfx, window_handle);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ADD_EMITTER) == 0) {
		se_vfx_emitter_2d_params params = SE_VFX_EMITTER_2D_PARAMS_DEFAULTS;
		if (se_editor_value_as_vec4(&command->value, &vec4_value)) {
			params.spawn_rate = vec4_value.x;
			params.burst_count = (u32)s_max((i32)vec4_value.y, 0);
			params.lifetime_min = vec4_value.z;
			params.lifetime_max = vec4_value.w;
		}
		return se_vfx_2d_add_emitter(vfx, &params) != S_HANDLE_NULL;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DESTROY) == 0) {
		se_vfx_2d_destroy(vfx);
		if (editor->focused_vfx_2d == vfx) {
			editor->focused_vfx_2d = S_HANDLE_NULL;
		}
		return true;
	}
	if (!se_editor_vfx_resolve_emitter(command, &emitter)) {
		return false;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_REMOVE) == 0) {
		return se_vfx_2d_remove_emitter(vfx, (se_vfx_emitter_2d_handle)emitter);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_START) == 0) {
		return se_vfx_2d_emitter_start(vfx, (se_vfx_emitter_2d_handle)emitter);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_STOP) == 0) {
		return se_vfx_2d_emitter_stop(vfx, (se_vfx_emitter_2d_handle)emitter);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_BURST) == 0) {
		if (!se_editor_value_as_u32(&command->aux_value, &u32_value)) {
			u32_value = 1u;
		}
		return se_vfx_2d_emitter_burst(vfx, (se_vfx_emitter_2d_handle)emitter, u32_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_SET_SPAWN) == 0) {
		if (!se_editor_value_as_vec4(&command->aux_value, &vec4_value)) {
			if (!se_editor_value_as_vec4(&command->value, &vec4_value)) {
				return false;
			}
		}
		return se_vfx_2d_emitter_set_spawn(
			vfx,
			(se_vfx_emitter_2d_handle)emitter,
			vec4_value.x,
			(u32)s_max((i32)vec4_value.y, 0),
			vec4_value.z,
			vec4_value.w);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_SET_BLEND_MODE) == 0) {
		i32 blend_mode = 0;
		if (!se_editor_value_as_i32(&command->aux_value, &blend_mode)) {
			if (!se_editor_value_as_i32(&command->value, &blend_mode)) {
				return false;
			}
		}
		return se_vfx_2d_emitter_set_blend_mode(vfx, (se_vfx_emitter_2d_handle)emitter, (se_vfx_blend_mode)blend_mode);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_SET_TEXTURE) == 0) {
		if (!se_editor_value_as_handle(&command->aux_value, &handle_value) && !se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		return se_vfx_2d_emitter_set_texture(vfx, (se_vfx_emitter_2d_handle)emitter, (se_texture_handle)handle_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_SET_SHADER) == 0) {
		const c8* vs_path = se_editor_value_as_text(&command->value);
		const c8* fs_path = se_editor_value_as_text(&command->aux_value);
		if (!vs_path || !fs_path) {
			return false;
		}
		return se_vfx_2d_emitter_set_shader(vfx, (se_vfx_emitter_2d_handle)emitter, vs_path, fs_path);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_CLEAR_TRACKS) == 0) {
		return se_vfx_2d_emitter_clear_tracks(vfx, (se_vfx_emitter_2d_handle)emitter);
	}
	return false;
}

static b8 se_editor_apply_vfx_3d_command(se_editor* editor, const se_editor_command* command) {
	se_vfx_3d_handle vfx = (se_vfx_3d_handle)command->item.handle;
	s_handle emitter = S_HANDLE_NULL;
	s_handle handle_value = S_HANDLE_NULL;
	u32 u32_value = 0u;
	s_vec4 vec4_value = {0};
	f32 f32_value = 0.0f;
	b8 bool_value = false;
	se_window_handle window_handle = S_HANDLE_NULL;
	se_camera_handle camera_handle = S_HANDLE_NULL;
	if (vfx == S_HANDLE_NULL) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_FOCUSED) == 0) {
		if (!se_editor_command_read_bool(command, editor->focused_vfx_3d == vfx, &bool_value)) {
			return false;
		}
		se_editor_set_focused_vfx_3d(editor, bool_value ? vfx : S_HANDLE_NULL);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_TICK) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			f32_value = 1.0f / 60.0f;
		}
		return se_vfx_3d_tick(vfx, f32_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_TICK_WINDOW) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		window_handle = (se_window_handle)handle_value;
		if (window_handle == S_HANDLE_NULL) {
			return false;
		}
		return se_vfx_3d_tick_window(vfx, window_handle);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		window_handle = (se_window_handle)handle_value;
		if (!se_editor_value_as_handle(&command->aux_value, &handle_value)) {
			handle_value = S_HANDLE_NULL;
		}
		camera_handle = (se_camera_handle)handle_value;
		if (window_handle == S_HANDLE_NULL || camera_handle == S_HANDLE_NULL) {
			return false;
		}
		return se_vfx_3d_render(vfx, window_handle, camera_handle);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DRAW) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			handle_value = editor->window;
		}
		window_handle = (se_window_handle)handle_value;
		if (window_handle == S_HANDLE_NULL) {
			return false;
		}
		return se_vfx_3d_draw(vfx, window_handle);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ADD_EMITTER) == 0) {
		se_vfx_emitter_3d_params params = SE_VFX_EMITTER_3D_PARAMS_DEFAULTS;
		if (se_editor_value_as_vec4(&command->value, &vec4_value)) {
			params.spawn_rate = vec4_value.x;
			params.burst_count = (u32)s_max((i32)vec4_value.y, 0);
			params.lifetime_min = vec4_value.z;
			params.lifetime_max = vec4_value.w;
		}
		return se_vfx_3d_add_emitter(vfx, &params) != S_HANDLE_NULL;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DESTROY) == 0) {
		se_vfx_3d_destroy(vfx);
		if (editor->focused_vfx_3d == vfx) {
			editor->focused_vfx_3d = S_HANDLE_NULL;
		}
		return true;
	}
	if (!se_editor_vfx_resolve_emitter(command, &emitter)) {
		return false;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_REMOVE) == 0) {
		return se_vfx_3d_remove_emitter(vfx, (se_vfx_emitter_3d_handle)emitter);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_START) == 0) {
		return se_vfx_3d_emitter_start(vfx, (se_vfx_emitter_3d_handle)emitter);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_STOP) == 0) {
		return se_vfx_3d_emitter_stop(vfx, (se_vfx_emitter_3d_handle)emitter);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_BURST) == 0) {
		if (!se_editor_value_as_u32(&command->aux_value, &u32_value)) {
			u32_value = 1u;
		}
		return se_vfx_3d_emitter_burst(vfx, (se_vfx_emitter_3d_handle)emitter, u32_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_SET_SPAWN) == 0) {
		if (!se_editor_value_as_vec4(&command->aux_value, &vec4_value)) {
			if (!se_editor_value_as_vec4(&command->value, &vec4_value)) {
				return false;
			}
		}
		return se_vfx_3d_emitter_set_spawn(
			vfx,
			(se_vfx_emitter_3d_handle)emitter,
			vec4_value.x,
			(u32)s_max((i32)vec4_value.y, 0),
			vec4_value.z,
			vec4_value.w);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_SET_BLEND_MODE) == 0) {
		i32 blend_mode = 0;
		if (!se_editor_value_as_i32(&command->aux_value, &blend_mode)) {
			if (!se_editor_value_as_i32(&command->value, &blend_mode)) {
				return false;
			}
		}
		return se_vfx_3d_emitter_set_blend_mode(vfx, (se_vfx_emitter_3d_handle)emitter, (se_vfx_blend_mode)blend_mode);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_SET_TEXTURE) == 0) {
		if (!se_editor_value_as_handle(&command->aux_value, &handle_value) && !se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		return se_vfx_3d_emitter_set_texture(vfx, (se_vfx_emitter_3d_handle)emitter, (se_texture_handle)handle_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_SET_MODEL) == 0) {
		if (!se_editor_value_as_handle(&command->aux_value, &handle_value) && !se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		return se_vfx_3d_emitter_set_model(vfx, (se_vfx_emitter_3d_handle)emitter, (se_model_handle)handle_value);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_SET_SHADER) == 0) {
		const c8* vs_path = se_editor_value_as_text(&command->value);
		const c8* fs_path = se_editor_value_as_text(&command->aux_value);
		if (!vs_path || !fs_path) {
			return false;
		}
		return se_vfx_3d_emitter_set_shader(vfx, (se_vfx_emitter_3d_handle)emitter, vs_path, fs_path);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_EMITTER_CLEAR_TRACKS) == 0) {
		return se_vfx_3d_emitter_clear_tracks(vfx, (se_vfx_emitter_3d_handle)emitter);
	}
	return false;
}

static b8 se_editor_apply_simulation_command(se_editor* editor, const se_editor_command* command) {
	se_simulation_handle simulation = (se_simulation_handle)command->item.handle;
	u32 tick_count = 1u;
	u64 entity_id = 0u;
	const c8* path = NULL;
	b8 bool_value = false;
	if (simulation == S_HANDLE_NULL) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_FOCUSED) == 0) {
		if (!se_editor_command_read_bool(command, editor->focused_simulation == simulation, &bool_value)) {
			return false;
		}
		se_editor_set_focused_simulation(editor, bool_value ? simulation : S_HANDLE_NULL);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_STEP) == 0) {
		if (!se_editor_value_as_u32(&command->value, &tick_count)) {
			tick_count = 1u;
		}
		return se_simulation_step(simulation, tick_count);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RESET) == 0) {
		se_simulation_reset(simulation);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SNAPSHOT_SAVE_FILE) == 0) {
		path = se_editor_value_as_text(&command->value);
		return path ? se_simulation_snapshot_save_file(simulation, path) : false;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SNAPSHOT_LOAD_FILE) == 0) {
		path = se_editor_value_as_text(&command->value);
		return path ? se_simulation_snapshot_load_file(simulation, path) : false;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_JSON_SAVE_FILE) == 0) {
		path = se_editor_value_as_text(&command->value);
		return path ? se_simulation_json_save_file(simulation, path) : false;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_JSON_LOAD_FILE) == 0) {
		path = se_editor_value_as_text(&command->value);
		return path ? se_simulation_json_load_file(simulation, path) : false;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ENTITY_CREATE) == 0) {
		return se_simulation_entity_create(simulation) != 0u;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ENTITY_DESTROY) == 0) {
		if (!se_editor_value_as_u64(&command->value, &entity_id) || entity_id == 0u) {
			return false;
		}
		return se_simulation_entity_destroy(simulation, entity_id);
	}
	return false;
}

static b8 se_editor_apply_model_command(se_editor* editor, const se_editor_command* command) {
	se_model_handle model = (se_model_handle)command->item.handle;
	s_vec3 vec3_value = {0};
	s_handle handle_value = S_HANDLE_NULL;
	(void)editor;
	if (model == S_HANDLE_NULL) {
		return false;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_TRANSLATE) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_model_translate(model, &vec3_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_ROTATE) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_model_rotate(model, &vec3_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SCALE) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_model_scale(model, &vec3_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DISCARD_CPU_DATA) == 0) {
		se_model* model_ptr = se_editor_model_ptr(editor, model);
		if (!model_ptr) {
			return false;
		}
		se_model_discard_cpu_data(model_ptr);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RENDER) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		se_model_render(model, (se_camera_handle)handle_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DESTROY) == 0) {
		se_model_destroy(model);
		return true;
	}
	return false;
}

static b8 se_editor_apply_shader_command(se_editor* editor, const se_editor_command* command) {
	se_shader_handle shader = (se_shader_handle)command->item.handle;
	se_shader* shader_ptr = se_editor_shader_ptr(editor, shader);
	const c8* uniform_name = NULL;
	i32 i32_value = 0;
	b8 bool_value = false;
	if (!shader || !shader_ptr) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_NEEDS_RELOAD) == 0) {
		if (!se_editor_command_read_bool(command, shader_ptr->needs_reload, &bool_value)) {
			return false;
		}
		shader_ptr->needs_reload = bool_value;
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_RELOAD_IF_CHANGED) == 0) {
		return se_shader_reload_if_changed(shader);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DESTROY) == 0) {
		se_shader_destroy(shader);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_USE) == 0) {
		b8 update_uniforms = true;
		b8 update_globals = true;
		if (se_editor_value_as_bool(&command->value, &update_uniforms)) {
			(void)0;
		}
		if (se_editor_value_as_bool(&command->aux_value, &update_globals)) {
			(void)0;
		}
		se_shader_use(shader, update_uniforms, update_globals);
		return true;
	}
	if (strncmp(command->name, SE_EDITOR_NAME_UNIFORM, 8) == 0) {
		uniform_name = command->name + 8;
		if (!uniform_name || uniform_name[0] == '\0') {
			return false;
		}
		switch (command->value.type) {
			case SE_EDITOR_VALUE_FLOAT:
				se_shader_set_float(shader, uniform_name, command->value.float_value);
				return true;
			case SE_EDITOR_VALUE_DOUBLE:
				se_shader_set_float(shader, uniform_name, (f32)command->value.double_value);
				return true;
			case SE_EDITOR_VALUE_INT:
				se_shader_set_int(shader, uniform_name, command->value.int_value);
				return true;
			case SE_EDITOR_VALUE_UINT:
				se_shader_set_int(shader, uniform_name, (i32)command->value.uint_value);
				return true;
			case SE_EDITOR_VALUE_BOOL:
				se_shader_set_int(shader, uniform_name, command->value.bool_value ? 1 : 0);
				return true;
			case SE_EDITOR_VALUE_VEC2:
				se_shader_set_vec2(shader, uniform_name, &command->value.vec2_value);
				return true;
			case SE_EDITOR_VALUE_VEC3:
				se_shader_set_vec3(shader, uniform_name, &command->value.vec3_value);
				return true;
			case SE_EDITOR_VALUE_VEC4:
				se_shader_set_vec4(shader, uniform_name, &command->value.vec4_value);
				return true;
			case SE_EDITOR_VALUE_MAT3:
				se_shader_set_mat3(shader, uniform_name, &command->value.mat3_value);
				return true;
			case SE_EDITOR_VALUE_MAT4:
				se_shader_set_mat4(shader, uniform_name, &command->value.mat4_value);
				return true;
			case SE_EDITOR_VALUE_HANDLE:
				se_shader_set_texture(shader, uniform_name, (u32)command->value.handle_value);
				return true;
			default:
				if (se_editor_value_as_i32(&command->value, &i32_value)) {
					se_shader_set_int(shader, uniform_name, i32_value);
					return true;
				}
				return false;
		}
	}
	return false;
}

static b8 se_editor_apply_texture_command(se_editor* editor, const se_editor_command* command) {
	se_texture_handle texture = (se_texture_handle)command->item.handle;
	se_texture* texture_ptr = se_editor_texture_ptr(editor, texture);
	(void)texture_ptr;
	if (texture == S_HANDLE_NULL || !texture_ptr) {
		return false;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DESTROY) == 0) {
		se_texture_destroy(texture);
		return true;
	}
	return false;
}

static b8 se_editor_apply_font_command(se_editor* editor, const se_editor_command* command) {
	se_font_handle font = (se_font_handle)command->item.handle;
	se_font* font_ptr = se_editor_font_ptr(editor, font);
	(void)font_ptr;
	if (font == S_HANDLE_NULL || !font_ptr) {
		return false;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DESTROY) == 0) {
		se_font_destroy(font);
		return true;
	}
	return false;
}

static b8 se_editor_apply_framebuffer_command(se_editor* editor, const se_editor_command* command) {
	se_framebuffer_handle framebuffer = (se_framebuffer_handle)command->item.handle;
	se_framebuffer* framebuffer_ptr = se_editor_framebuffer_ptr(editor, framebuffer);
	s_vec2 vec2_value = {0};
	b8 bool_value = false;
	if (!framebuffer || !framebuffer_ptr) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SIZE) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		se_framebuffer_set_size(framebuffer, &vec2_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_RATIO) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		framebuffer_ptr->ratio = vec2_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_AUTO_RESIZE) == 0) {
		if (!se_editor_command_read_bool(command, framebuffer_ptr->auto_resize, &bool_value)) {
			return false;
		}
		framebuffer_ptr->auto_resize = bool_value;
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_BIND) == 0) {
		se_framebuffer_bind(framebuffer);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UNBIND) == 0) {
		se_framebuffer_unbind(framebuffer);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CLEANUP) == 0) {
		se_framebuffer_cleanup(framebuffer);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DESTROY) == 0) {
		se_framebuffer_destroy(framebuffer);
		return true;
	}
	return false;
}

static b8 se_editor_apply_render_buffer_command(se_editor* editor, const se_editor_command* command) {
	se_render_buffer_handle render_buffer = (se_render_buffer_handle)command->item.handle;
	se_render_buffer* render_buffer_ptr = se_editor_render_buffer_ptr(editor, render_buffer);
	s_vec2 vec2_value = {0};
	s_handle handle_value = S_HANDLE_NULL;
	if (!render_buffer || !render_buffer_ptr) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SCALE) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		se_render_buffer_set_scale(render_buffer, &vec2_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_POSITION) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		se_render_buffer_set_position(render_buffer, &vec2_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SHADER) == 0) {
		if (!se_editor_value_as_handle(&command->value, &handle_value)) {
			return false;
		}
		if (handle_value == S_HANDLE_NULL) {
			se_render_buffer_unset_shader(render_buffer);
		} else {
			se_render_buffer_set_shader(render_buffer, (se_shader_handle)handle_value);
		}
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_BIND) == 0) {
		se_render_buffer_bind(render_buffer);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UNBIND) == 0) {
		se_render_buffer_unbind(render_buffer);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CLEANUP) == 0) {
		se_render_buffer_cleanup(render_buffer);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_DESTROY) == 0) {
		se_render_buffer_destroy(render_buffer);
		return true;
	}
	return false;
}

static se_audio_clip* se_editor_resolve_audio_clip(se_editor* editor, const se_editor_command* command) {
	se_audio_clip* clip = NULL;
	if (command) {
		clip = (se_audio_clip*)command->item.pointer;
		if (!clip && command->value.type == SE_EDITOR_VALUE_POINTER) {
			clip = (se_audio_clip*)command->value.pointer_value;
		}
	}
	if (!clip && editor && command) {
		if (command->item.index < s_array_get_size(&editor->clips)) {
			se_editor_audio_clip_ref* ref = s_array_get(&editor->clips, s_array_handle(&editor->clips, command->item.index));
			if (ref) {
				clip = ref->clip;
			}
		}
	}
	return clip;
}

static se_audio_stream* se_editor_resolve_audio_stream(se_editor* editor, const se_editor_command* command) {
	se_audio_stream* stream = NULL;
	if (command) {
		stream = (se_audio_stream*)command->item.pointer;
		if (!stream && command->value.type == SE_EDITOR_VALUE_POINTER) {
			stream = (se_audio_stream*)command->value.pointer_value;
		}
	}
	if (!stream && editor && command) {
		if (command->item.index < s_array_get_size(&editor->streams)) {
			se_editor_audio_stream_ref* ref = s_array_get(&editor->streams, s_array_handle(&editor->streams, command->item.index));
			if (ref) {
				stream = ref->stream;
			}
		}
	}
	return stream;
}

static se_audio_capture* se_editor_resolve_audio_capture(se_editor* editor, const se_editor_command* command) {
	se_audio_capture* capture = NULL;
	if (command) {
		capture = (se_audio_capture*)command->item.pointer;
		if (!capture && command->value.type == SE_EDITOR_VALUE_POINTER) {
			capture = (se_audio_capture*)command->value.pointer_value;
		}
	}
	if (!capture && editor && command) {
		if (command->item.index < s_array_get_size(&editor->captures)) {
			se_editor_audio_capture_ref* ref = s_array_get(&editor->captures, s_array_handle(&editor->captures, command->item.index));
			if (ref) {
				capture = ref->capture;
			}
		}
	}
	return capture;
}

static b8 se_editor_apply_audio_command(se_editor* editor, const se_editor_command* command) {
	f32 volume = 1.0f;
	if (!editor->audio) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_MASTER_VOLUME) == 0) {
		if (!se_editor_value_as_f32(&command->value, &volume)) {
			return false;
		}
		se_audio_bus_set_volume(editor->audio, SE_AUDIO_BUS_MASTER, volume);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_MUSIC_VOLUME) == 0) {
		if (!se_editor_value_as_f32(&command->value, &volume)) {
			return false;
		}
		se_audio_bus_set_volume(editor->audio, SE_AUDIO_BUS_MUSIC, volume);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SFX_VOLUME) == 0) {
		if (!se_editor_value_as_f32(&command->value, &volume)) {
			return false;
		}
		se_audio_bus_set_volume(editor->audio, SE_AUDIO_BUS_SFX, volume);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UPDATE) == 0) {
		se_audio_update(editor->audio);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_STOP_ALL) == 0) {
		se_audio_stop_all(editor->audio);
		return true;
	}
	return false;
}

static b8 se_editor_apply_audio_clip_command(se_editor* editor, const se_editor_command* command) {
	se_audio_clip* clip = se_editor_resolve_audio_clip(editor, command);
	se_audio_play_params params = {0};
	b8 bool_value = false;
	f32 f32_value = 0.0f;
	i32 i32_value = 0;
	s_vec4 vec4_value = s_vec4(0.0f, 0.0f, 0.0f, 0.0f);
	(void)editor;
	if (!clip) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_VOLUME) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		se_audio_clip_set_volume(clip, f32_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_PAN) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		se_audio_clip_set_pan(clip, f32_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_PLAY) == 0) {
		params.volume = 1.0f;
		params.pan = 0.0f;
		params.pitch = 1.0f;
		params.looping = false;
		params.bus = SE_AUDIO_BUS_SFX;
		if (se_editor_value_as_vec4(&command->value, &vec4_value)) {
			params.volume = vec4_value.x;
			params.pan = vec4_value.y;
			params.pitch = vec4_value.z;
			params.bus = (se_audio_bus)se_editor_clamp_i32((i32)vec4_value.w, 0, (i32)SE_AUDIO_BUS_COUNT - 1);
		} else if (se_editor_value_as_f32(&command->value, &f32_value)) {
			params.volume = f32_value;
		} else if (se_editor_value_as_i32(&command->value, &i32_value)) {
			params.bus = (se_audio_bus)se_editor_clamp_i32(i32_value, 0, (i32)SE_AUDIO_BUS_COUNT - 1);
		} else if (se_editor_value_as_bool(&command->value, &bool_value)) {
			params.looping = bool_value;
		}
		if (se_editor_value_as_vec4(&command->aux_value, &vec4_value)) {
			params.volume = vec4_value.x;
			params.pan = vec4_value.y;
			params.pitch = vec4_value.z;
			params.bus = (se_audio_bus)se_editor_clamp_i32((i32)vec4_value.w, 0, (i32)SE_AUDIO_BUS_COUNT - 1);
		} else if (se_editor_value_as_f32(&command->aux_value, &f32_value)) {
			params.pan = f32_value;
		} else if (se_editor_value_as_i32(&command->aux_value, &i32_value)) {
			params.bus = (se_audio_bus)se_editor_clamp_i32(i32_value, 0, (i32)SE_AUDIO_BUS_COUNT - 1);
		}
		if (se_editor_value_as_bool(&command->aux_value, &bool_value)) {
			params.looping = bool_value;
		}
		return se_audio_clip_play(clip, &params);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_STOP) == 0) {
		se_audio_clip_stop(clip);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UNTRACK) == 0) {
		return se_editor_untrack_audio_clip(editor, clip);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UNLOAD) == 0) {
		if (!editor->audio) {
			return false;
		}
		se_audio_clip_unload(editor->audio, clip);
		(void)se_editor_untrack_audio_clip(editor, clip);
		return true;
	}
	return false;
}

static b8 se_editor_apply_audio_stream_command(se_editor* editor, const se_editor_command* command) {
	se_audio_stream* stream = se_editor_resolve_audio_stream(editor, command);
	f32 f32_value = 0.0f;
	f64 f64_value = 0.0;
	b8 bool_value = false;
	(void)editor;
	if (!stream) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_VOLUME) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		se_audio_stream_set_volume(stream, f32_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_LOOPING) == 0) {
		if (!se_editor_command_read_bool(command, false, &bool_value)) {
			return false;
		}
		se_audio_stream_set_looping(stream, bool_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SEEK) == 0) {
		if (!se_editor_value_as_f64(&command->value, &f64_value)) {
			return false;
		}
		se_audio_stream_seek(stream, f64_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CLOSE) == 0) {
		se_audio_stream_close(stream);
		(void)se_editor_untrack_audio_stream(editor, stream);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UNTRACK) == 0) {
		return se_editor_untrack_audio_stream(editor, stream);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CLOSE_UNTRACK) == 0) {
		se_audio_stream_close(stream);
		(void)se_editor_untrack_audio_stream(editor, stream);
		return true;
	}
	return false;
}

static b8 se_editor_apply_audio_capture_command(se_editor* editor, const se_editor_command* command) {
	se_audio_capture* capture = se_editor_resolve_audio_capture(editor, command);
	if (!capture) {
		return false;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_STOP) == 0) {
		se_audio_capture_stop(capture);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_UNTRACK) == 0) {
		return se_editor_untrack_audio_capture(editor, capture);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_STOP_UNTRACK) == 0) {
		se_audio_capture_stop(capture);
		(void)se_editor_untrack_audio_capture(editor, capture);
		return true;
	}
	return false;
}

static b8 se_editor_apply_navigation_command(se_editor* editor, const se_editor_command* command) {
	s_vec2 vec2_value = {0};
	s_vec3 vec3_value = {0};
	s_vec4 vec4_value = {0};
	b8 bool_value = false;
	if (!editor->navigation) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_CELL_SIZE) == 0) {
		f32 size = 0.0f;
		if (!se_editor_value_as_f32(&command->value, &size) || size <= 0.0f) {
			return false;
		}
		editor->navigation->cell_size = size;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ORIGIN) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		editor->navigation->origin = vec2_value;
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_CLEAR) == 0) {
		if (!se_editor_value_as_bool(&command->value, &bool_value)) {
			bool_value = false;
		}
		se_navigation_grid_clear(editor->navigation, bool_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_CELL_BLOCKED) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		return se_navigation_grid_set_blocked(editor->navigation, (se_navigation_cell){ (i32)vec3_value.x, (i32)vec3_value.y }, vec3_value.z > 0.5f);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_DYNAMIC_CELL_BLOCKED) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_navigation_grid_set_dynamic_obstacle(editor->navigation, (se_navigation_cell){ (i32)vec3_value.x, (i32)vec3_value.y }, vec3_value.z > 0.5f);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_RECT_BLOCKED) == 0) {
		if (!se_editor_value_as_vec4(&command->value, &vec4_value)) {
			return false;
		}
		if (!se_editor_value_as_bool(&command->aux_value, &bool_value)) {
			bool_value = true;
		}
		se_navigation_grid_set_blocked_rect(
			editor->navigation,
			(se_navigation_cell){ (i32)vec4_value.x, (i32)vec4_value.y },
			(se_navigation_cell){ (i32)vec4_value.z, (i32)vec4_value.w },
			bool_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_CIRCLE_BLOCKED) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		if (!se_editor_value_as_bool(&command->aux_value, &bool_value)) {
			bool_value = true;
		}
		se_navigation_grid_set_blocked_circle(editor->navigation, (se_navigation_cell){ (i32)vec3_value.x, (i32)vec3_value.y }, (i32)vec3_value.z, bool_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_SET_DYNAMIC_RECT_BLOCKED) == 0) {
		if (!se_editor_value_as_vec4(&command->value, &vec4_value)) {
			return false;
		}
		if (!se_editor_value_as_bool(&command->aux_value, &bool_value)) {
			bool_value = true;
		}
		se_navigation_grid_set_dynamic_obstacle_rect(
			editor->navigation,
			(se_navigation_cell){ (i32)vec4_value.x, (i32)vec4_value.y },
			(se_navigation_cell){ (i32)vec4_value.z, (i32)vec4_value.w },
			bool_value);
		return true;
	}
	return false;
}

static b8 se_editor_apply_physics_2d_command(se_editor* editor, const se_editor_command* command) {
	s_vec2 vec2_value = {0};
	f32 f32_value = 0.0f;
	u32 u32_value = 0u;
	if (!editor->physics_2d) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_GRAVITY) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		se_physics_world_2d_set_gravity(editor->physics_2d, &vec2_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SOLVER_ITERATIONS) == 0) {
		if (!se_editor_value_as_u32(&command->value, &u32_value)) {
			return false;
		}
		editor->physics_2d->solver_iterations = u32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SHAPES_PER_BODY) == 0) {
		if (!se_editor_value_as_u32(&command->value, &u32_value)) {
			return false;
		}
		editor->physics_2d->shapes_per_body = u32_value;
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_STEP) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			f32_value = 1.0f / 60.0f;
		}
		se_physics_world_2d_step(editor->physics_2d, f32_value);
		return true;
	}
	return false;
}

static b8 se_editor_apply_physics_2d_body_command(se_editor* editor, const se_editor_command* command) {
	se_physics_body_2d* body = (se_physics_body_2d*)command->item.pointer;
	s_vec2 vec2_value = {0};
	f32 f32_value = 0.0f;
	i32 i32_value = 0;
	if (!editor || !body || !body->is_valid) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_TYPE) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		body->type = (se_physics_body_type)i32_value;
		se_editor_physics_update_mass_2d(body);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_POSITION) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		se_physics_body_2d_set_position(body, &vec2_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ROTATION) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		se_physics_body_2d_set_rotation(body, f32_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_VELOCITY) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		se_physics_body_2d_set_velocity(body, &vec2_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ANGULAR_VELOCITY) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->angular_velocity = f32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_FORCE) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		body->force = vec2_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_TORQUE) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->torque = f32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_MASS) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->mass = f32_value;
		se_editor_physics_update_mass_2d(body);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_RESTITUTION) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->restitution = f32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_FRICTION) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->friction = f32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_LINEAR_DAMPING) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->linear_damping = f32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ANGULAR_DAMPING) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->angular_damping = f32_value;
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_APPLY_FORCE) == 0) {
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		se_physics_body_2d_apply_force(body, &vec2_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_APPLY_IMPULSE) == 0) {
		s_vec2 point = s_vec2(0.0f, 0.0f);
		if (!se_editor_value_as_vec2(&command->value, &vec2_value)) {
			return false;
		}
		(void)se_editor_value_as_vec2(&command->aux_value, &point);
		se_physics_body_2d_apply_impulse(body, &vec2_value, &point);
		return true;
	}
	return false;
}

static b8 se_editor_apply_physics_3d_command(se_editor* editor, const se_editor_command* command) {
	s_vec3 vec3_value = {0};
	f32 f32_value = 0.0f;
	u32 u32_value = 0u;
	if (!editor->physics_3d) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_GRAVITY) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_physics_world_3d_set_gravity(editor->physics_3d, &vec3_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SOLVER_ITERATIONS) == 0) {
		if (!se_editor_value_as_u32(&command->value, &u32_value)) {
			return false;
		}
		editor->physics_3d->solver_iterations = u32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_SHAPES_PER_BODY) == 0) {
		if (!se_editor_value_as_u32(&command->value, &u32_value)) {
			return false;
		}
		editor->physics_3d->shapes_per_body = u32_value;
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_STEP) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			f32_value = 1.0f / 60.0f;
		}
		se_physics_world_3d_step(editor->physics_3d, f32_value);
		return true;
	}
	return false;
}

static b8 se_editor_apply_physics_3d_body_command(se_editor* editor, const se_editor_command* command) {
	se_physics_body_3d* body = (se_physics_body_3d*)command->item.pointer;
	s_vec3 vec3_value = {0};
	f32 f32_value = 0.0f;
	i32 i32_value = 0;
	if (!editor || !body || !body->is_valid) {
		return false;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_TYPE) == 0) {
		if (!se_editor_value_as_i32(&command->value, &i32_value)) {
			return false;
		}
		body->type = (se_physics_body_type)i32_value;
		se_editor_physics_update_mass_3d(body);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_POSITION) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_physics_body_3d_set_position(body, &vec3_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ROTATION) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_physics_body_3d_set_rotation(body, &vec3_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_VELOCITY) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_physics_body_3d_set_velocity(body, &vec3_value);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ANGULAR_VELOCITY) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		body->angular_velocity = vec3_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_FORCE) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		body->force = vec3_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_TORQUE) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		body->torque = vec3_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_MASS) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->mass = f32_value;
		se_editor_physics_update_mass_3d(body);
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_RESTITUTION) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->restitution = f32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_FRICTION) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->friction = f32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_LINEAR_DAMPING) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->linear_damping = f32_value;
		return true;
	}
	if (strcmp(command->name, SE_EDITOR_NAME_ANGULAR_DAMPING) == 0) {
		if (!se_editor_value_as_f32(&command->value, &f32_value)) {
			return false;
		}
		body->angular_damping = f32_value;
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_APPLY_FORCE) == 0) {
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		se_physics_body_3d_apply_force(body, &vec3_value);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, SE_EDITOR_NAME_APPLY_IMPULSE) == 0) {
		s_vec3 point = s_vec3(0.0f, 0.0f, 0.0f);
		if (!se_editor_value_as_vec3(&command->value, &vec3_value)) {
			return false;
		}
		(void)se_editor_value_as_vec3(&command->aux_value, &point);
		se_physics_body_3d_apply_impulse(body, &vec3_value, &point);
		return true;
	}
	return false;
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
		case SE_EDITOR_CATEGORY_BACKEND: return SE_EDITOR_NAME_BACKEND;
		case SE_EDITOR_CATEGORY_DEBUG: return SE_EDITOR_NAME_DEBUG;
		case SE_EDITOR_CATEGORY_WINDOW: return SE_EDITOR_NAME_WINDOW;
		case SE_EDITOR_CATEGORY_INPUT: return SE_EDITOR_NAME_INPUT;
		case SE_EDITOR_CATEGORY_CAMERA: return SE_EDITOR_NAME_CAMERA;
		case SE_EDITOR_CATEGORY_SCENE_2D: return SE_EDITOR_NAME_SCENE2D;
		case SE_EDITOR_CATEGORY_OBJECT_2D: return SE_EDITOR_NAME_OBJECT2D;
		case SE_EDITOR_CATEGORY_SCENE_3D: return SE_EDITOR_NAME_SCENE3D;
		case SE_EDITOR_CATEGORY_OBJECT_3D: return SE_EDITOR_NAME_OBJECT3D;
		case SE_EDITOR_CATEGORY_UI: return SE_EDITOR_NAME_UI;
		case SE_EDITOR_CATEGORY_UI_WIDGET: return SE_EDITOR_NAME_UI_WIDGET;
		case SE_EDITOR_CATEGORY_VFX_2D: return SE_EDITOR_NAME_VFX2D;
		case SE_EDITOR_CATEGORY_VFX_3D: return SE_EDITOR_NAME_VFX3D;
		case SE_EDITOR_CATEGORY_SIMULATION: return SE_EDITOR_NAME_SIMULATION;
		case SE_EDITOR_CATEGORY_MODEL: return SE_EDITOR_NAME_MODEL;
		case SE_EDITOR_CATEGORY_SHADER: return SE_EDITOR_NAME_SHADER;
		case SE_EDITOR_CATEGORY_TEXTURE: return SE_EDITOR_NAME_TEXTURE;
		case SE_EDITOR_CATEGORY_FRAMEBUFFER: return SE_EDITOR_NAME_FRAMEBUFFER;
		case SE_EDITOR_CATEGORY_RENDER_BUFFER: return SE_EDITOR_NAME_RENDER_BUFFER;
		case SE_EDITOR_CATEGORY_FONT: return SE_EDITOR_NAME_FONT;
		case SE_EDITOR_CATEGORY_AUDIO: return SE_EDITOR_NAME_AUDIO;
		case SE_EDITOR_CATEGORY_AUDIO_CLIP: return SE_EDITOR_NAME_AUDIO_CLIP;
		case SE_EDITOR_CATEGORY_AUDIO_STREAM: return SE_EDITOR_NAME_AUDIO_STREAM;
		case SE_EDITOR_CATEGORY_AUDIO_CAPTURE: return SE_EDITOR_NAME_AUDIO_CAPTURE;
		case SE_EDITOR_CATEGORY_NAVIGATION: return SE_EDITOR_NAME_NAVIGATION;
		case SE_EDITOR_CATEGORY_PHYSICS_2D: return SE_EDITOR_NAME_PHYSICS2D;
		case SE_EDITOR_CATEGORY_PHYSICS_2D_BODY: return SE_EDITOR_NAME_PHYSICS2D_BODY;
		case SE_EDITOR_CATEGORY_PHYSICS_3D: return SE_EDITOR_NAME_PHYSICS3D;
		case SE_EDITOR_CATEGORY_PHYSICS_3D_BODY: return SE_EDITOR_NAME_PHYSICS3D_BODY;
		case SE_EDITOR_CATEGORY_COUNT:
		default: return SE_EDITOR_NAME_UNKNOWN;
	}
}

se_editor_category se_editor_category_from_name(const c8* name) {
	c8 normalized_name[SE_MAX_NAME_LENGTH] = {0};
	typedef struct {
		const c8* key;
		se_editor_category category;
	} se_editor_category_alias;
	static const se_editor_category_alias aliases[] = {
		{ SE_EDITOR_NAME_PHYSICS2DWORLD, SE_EDITOR_CATEGORY_PHYSICS_2D },
		{ SE_EDITOR_NAME_PHYSICS3DWORLD, SE_EDITOR_CATEGORY_PHYSICS_3D },
		{ SE_EDITOR_NAME_PHYSICS2DBODY, SE_EDITOR_CATEGORY_PHYSICS_2D_BODY },
		{ SE_EDITOR_NAME_PHYSICS3DBODY, SE_EDITOR_CATEGORY_PHYSICS_3D_BODY },
		{ SE_EDITOR_NAME_UIWIDGET, SE_EDITOR_CATEGORY_UI_WIDGET },
		{ SE_EDITOR_NAME_WIDGET, SE_EDITOR_CATEGORY_UI_WIDGET },
		{ SE_EDITOR_NAME_SCENE, SE_EDITOR_CATEGORY_SCENE_2D },
		{ SE_EDITOR_NAME_SCENE2, SE_EDITOR_CATEGORY_SCENE_2D },
		{ SE_EDITOR_NAME_SCENE3, SE_EDITOR_CATEGORY_SCENE_3D },
		{ SE_EDITOR_NAME_OBJECT2, SE_EDITOR_CATEGORY_OBJECT_2D },
		{ SE_EDITOR_NAME_OBJECT3, SE_EDITOR_CATEGORY_OBJECT_3D }
	};
	if (!name || name[0] == '\0') {
		return SE_EDITOR_CATEGORY_COUNT;
	}
	se_editor_normalize_name(name, normalized_name, sizeof(normalized_name));
	if (normalized_name[0] == '\0') {
		return SE_EDITOR_CATEGORY_COUNT;
	}
	for (u32 i = 0; i < (u32)SE_EDITOR_CATEGORY_COUNT; ++i) {
		const se_editor_category category = (se_editor_category)i;
		const c8* category_name = se_editor_category_name(category);
		c8 normalized_category_name[SE_MAX_NAME_LENGTH] = {0};
		if (!category_name) {
			continue;
		}
		se_editor_normalize_name(category_name, normalized_category_name, sizeof(normalized_category_name));
		if (strcmp(normalized_name, normalized_category_name) == 0) {
			return category;
		}
	}
	for (u32 i = 0u; i < (u32)(sizeof(aliases) / sizeof(aliases[0])); ++i) {
		if (strcmp(normalized_name, aliases[i].key) == 0) {
			return aliases[i].category;
		}
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
	s_array_init(&editor->clips);
	s_array_init(&editor->streams);
	s_array_init(&editor->captures);
	s_array_init(&editor->items);
	s_array_init(&editor->properties);
	s_array_init(&editor->queue);

	editor->context = cfg.context ? cfg.context : se_current_context();
	editor->window = cfg.window;
	editor->input = cfg.input;
	editor->audio = cfg.audio;
	editor->navigation = cfg.navigation;
	editor->physics_2d = cfg.physics_2d;
	editor->physics_3d = cfg.physics_3d;
	editor->focused_simulation = cfg.focused_simulation;
	editor->focused_vfx_2d = cfg.focused_vfx_2d;
	editor->focused_vfx_3d = cfg.focused_vfx_3d;
	se_editor_refresh_defaults_internal(editor);

	se_set_last_error(SE_RESULT_OK);
	return editor;
}

void se_editor_destroy(se_editor* editor) {
	if (!editor) {
		return;
	}
	s_array_clear(&editor->queue);
	s_array_clear(&editor->properties);
	s_array_clear(&editor->items);
	s_array_clear(&editor->captures);
	s_array_clear(&editor->streams);
	s_array_clear(&editor->clips);
	free(editor);
}

void se_editor_reset(se_editor* editor) {
	if (!editor) {
		return;
	}
	se_editor_queue_clear(editor);
	se_editor_clear_runtime_arrays(editor);
	SE_EDITOR_ARRAY_POP_ALL(&editor->clips);
	SE_EDITOR_ARRAY_POP_ALL(&editor->streams);
	SE_EDITOR_ARRAY_POP_ALL(&editor->captures);
	editor->focused_simulation = S_HANDLE_NULL;
	editor->focused_vfx_2d = S_HANDLE_NULL;
	editor->focused_vfx_3d = S_HANDLE_NULL;
	se_editor_refresh_defaults_internal(editor);
}

void se_editor_refresh_defaults(se_editor* editor) {
	if (!editor) {
		return;
	}
	se_editor_refresh_defaults_internal(editor);
}

void se_editor_set_context(se_editor* editor, se_context* context) {
	if (!editor) {
		return;
	}
	editor->context = context;
	se_editor_refresh_defaults_internal(editor);
}

se_context* se_editor_get_context(const se_editor* editor) {
	return se_editor_context(editor);
}

void se_editor_set_window(se_editor* editor, se_window_handle window) {
	if (!editor) {
		return;
	}
	editor->window = window;
	se_editor_refresh_defaults_internal(editor);
}

se_window_handle se_editor_get_window(const se_editor* editor) {
	if (!editor) {
		return S_HANDLE_NULL;
	}
	return editor->window;
}

void se_editor_set_input(se_editor* editor, se_input_handle* input) {
	if (!editor) {
		return;
	}
	editor->input = input;
}

se_input_handle* se_editor_get_input(const se_editor* editor) {
	if (!editor) {
		return NULL;
	}
	return editor->input;
}

void se_editor_set_audio_engine(se_editor* editor, se_audio_engine* audio) {
	if (!editor) {
		return;
	}
	editor->audio = audio;
}

se_audio_engine* se_editor_get_audio_engine(const se_editor* editor) {
	if (!editor) {
		return NULL;
	}
	return editor->audio;
}

void se_editor_set_navigation(se_editor* editor, se_navigation_grid* navigation) {
	if (!editor) {
		return;
	}
	editor->navigation = navigation;
}

se_navigation_grid* se_editor_get_navigation(const se_editor* editor) {
	if (!editor) {
		return NULL;
	}
	return editor->navigation;
}

void se_editor_set_physics_2d(se_editor* editor, se_physics_world_2d* world) {
	if (!editor) {
		return;
	}
	editor->physics_2d = world;
}

se_physics_world_2d* se_editor_get_physics_2d(const se_editor* editor) {
	if (!editor) {
		return NULL;
	}
	return editor->physics_2d;
}

void se_editor_set_physics_3d(se_editor* editor, se_physics_world_3d* world) {
	if (!editor) {
		return;
	}
	editor->physics_3d = world;
}

se_physics_world_3d* se_editor_get_physics_3d(const se_editor* editor) {
	if (!editor) {
		return NULL;
	}
	return editor->physics_3d;
}

void se_editor_set_focused_simulation(se_editor* editor, se_simulation_handle simulation) {
	if (!editor) {
		return;
	}
	editor->focused_simulation = simulation;
	se_editor_refresh_defaults_internal(editor);
}

se_simulation_handle se_editor_get_focused_simulation(const se_editor* editor) {
	if (!editor) {
		return S_HANDLE_NULL;
	}
	return editor->focused_simulation;
}

void se_editor_set_focused_vfx_2d(se_editor* editor, se_vfx_2d_handle vfx) {
	if (!editor) {
		return;
	}
	editor->focused_vfx_2d = vfx;
	se_editor_refresh_defaults_internal(editor);
}

se_vfx_2d_handle se_editor_get_focused_vfx_2d(const se_editor* editor) {
	if (!editor) {
		return S_HANDLE_NULL;
	}
	return editor->focused_vfx_2d;
}

void se_editor_set_focused_vfx_3d(se_editor* editor, se_vfx_3d_handle vfx) {
	if (!editor) {
		return;
	}
	editor->focused_vfx_3d = vfx;
	se_editor_refresh_defaults_internal(editor);
}

se_vfx_3d_handle se_editor_get_focused_vfx_3d(const se_editor* editor) {
	if (!editor) {
		return S_HANDLE_NULL;
	}
	return editor->focused_vfx_3d;
}

b8 se_editor_track_audio_clip(se_editor* editor, se_audio_clip* clip, const c8* label) {
	if (!editor || !clip) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&editor->clips); ++i) {
		const s_handle item_handle = s_array_handle(&editor->clips, (u32)i);
		se_editor_audio_clip_ref* ref = s_array_get(&editor->clips, item_handle);
		if (ref && ref->clip == clip) {
			if (label) {
				se_editor_copy_text(ref->label, sizeof(ref->label), label);
			}
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	se_editor_audio_clip_ref ref = {0};
	ref.clip = clip;
	se_editor_copy_text(ref.label, sizeof(ref.label), label ? label : SE_EDITOR_NAME_AUDIO_CLIP);
	s_array_add(&editor->clips, ref);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_untrack_audio_clip(se_editor* editor, se_audio_clip* clip) {
	if (!editor || !clip) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&editor->clips); ++i) {
		const s_handle item_handle = s_array_handle(&editor->clips, (u32)i);
		se_editor_audio_clip_ref* ref = s_array_get(&editor->clips, item_handle);
		if (ref && ref->clip == clip) {
			s_array_remove(&editor->clips, item_handle);
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

b8 se_editor_track_audio_stream(se_editor* editor, se_audio_stream* stream, const c8* label) {
	if (!editor || !stream) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&editor->streams); ++i) {
		const s_handle item_handle = s_array_handle(&editor->streams, (u32)i);
		se_editor_audio_stream_ref* ref = s_array_get(&editor->streams, item_handle);
		if (ref && ref->stream == stream) {
			if (label) {
				se_editor_copy_text(ref->label, sizeof(ref->label), label);
			}
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	se_editor_audio_stream_ref ref = {0};
	ref.stream = stream;
	se_editor_copy_text(ref.label, sizeof(ref.label), label ? label : SE_EDITOR_NAME_AUDIO_STREAM);
	s_array_add(&editor->streams, ref);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_untrack_audio_stream(se_editor* editor, se_audio_stream* stream) {
	if (!editor || !stream) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&editor->streams); ++i) {
		const s_handle item_handle = s_array_handle(&editor->streams, (u32)i);
		se_editor_audio_stream_ref* ref = s_array_get(&editor->streams, item_handle);
		if (ref && ref->stream == stream) {
			s_array_remove(&editor->streams, item_handle);
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

b8 se_editor_track_audio_capture(se_editor* editor, se_audio_capture* capture, const c8* label) {
	if (!editor || !capture) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&editor->captures); ++i) {
		const s_handle item_handle = s_array_handle(&editor->captures, (u32)i);
		se_editor_audio_capture_ref* ref = s_array_get(&editor->captures, item_handle);
		if (ref && ref->capture == capture) {
			if (label) {
				se_editor_copy_text(ref->label, sizeof(ref->label), label);
			}
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	se_editor_audio_capture_ref ref = {0};
	ref.capture = capture;
	se_editor_copy_text(ref.label, sizeof(ref.label), label ? label : SE_EDITOR_NAME_AUDIO_CAPTURE);
	s_array_add(&editor->captures, ref);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_untrack_audio_capture(se_editor* editor, se_audio_capture* capture) {
	if (!editor || !capture) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&editor->captures); ++i) {
		const s_handle item_handle = s_array_handle(&editor->captures, (u32)i);
		se_editor_audio_capture_ref* ref = s_array_get(&editor->captures, item_handle);
		if (ref && ref->capture == capture) {
			s_array_remove(&editor->captures, item_handle);
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

b8 se_editor_collect_counts(se_editor* editor, se_editor_counts* out_counts) {
	se_context* context = se_editor_context(editor);
	if (!editor || !out_counts) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_editor_refresh_defaults_internal(editor);
	context = se_editor_context(editor);
	memset(out_counts, 0, sizeof(*out_counts));
	out_counts->inputs = editor->input ? 1u : 0u;
	out_counts->audio_clips = (u32)s_array_get_size(&editor->clips);
	out_counts->audio_streams = (u32)s_array_get_size(&editor->streams);
	out_counts->audio_captures = (u32)s_array_get_size(&editor->captures);
	out_counts->queued_commands = (u32)s_array_get_size(&editor->queue);
	if (editor->physics_2d) {
		out_counts->physics_2d_bodies = (u32)s_array_get_size(&editor->physics_2d->bodies);
	}
	if (editor->physics_3d) {
		out_counts->physics_3d_bodies = (u32)s_array_get_size(&editor->physics_3d->bodies);
	}
	if (context) {
		out_counts->windows = (u32)s_array_get_size(&context->windows);
		out_counts->cameras = (u32)s_array_get_size(&context->cameras);
		out_counts->scenes_2d = (u32)s_array_get_size(&context->scenes_2d);
		out_counts->objects_2d = (u32)s_array_get_size(&context->objects_2d);
		out_counts->scenes_3d = (u32)s_array_get_size(&context->scenes_3d);
		out_counts->objects_3d = (u32)s_array_get_size(&context->objects_3d);
		out_counts->uis = (u32)s_array_get_size(&context->ui_roots);
		out_counts->ui_widgets = (u32)s_array_get_size(&context->ui_elements);
		out_counts->vfx_2d = (u32)s_array_get_size(&context->vfx_2ds);
		out_counts->vfx_3d = (u32)s_array_get_size(&context->vfx_3ds);
		out_counts->simulations = (u32)s_array_get_size(&context->simulations);
		out_counts->models = (u32)s_array_get_size(&context->models);
		out_counts->shaders = (u32)s_array_get_size(&context->shaders);
		out_counts->textures = (u32)s_array_get_size(&context->textures);
		out_counts->framebuffers = (u32)s_array_get_size(&context->framebuffers);
		out_counts->render_buffers = (u32)s_array_get_size(&context->render_buffers);
		out_counts->fonts = (u32)s_array_get_size(&context->fonts);
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_collect_items(se_editor* editor, se_editor_category_mask category_mask, const se_editor_item** out_items, sz* out_count) {
	se_context* context = se_editor_context(editor);
	c8 label[SE_MAX_NAME_LENGTH] = {0};
	if (!editor || !out_items || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_editor_refresh_defaults_internal(editor);
	context = se_editor_context(editor);
	se_editor_clear_runtime_arrays(editor);
	if (category_mask == 0u) {
		category_mask = SE_EDITOR_CATEGORY_MASK_ALL;
	}

	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_BACKEND)) {
		se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_BACKEND, S_HANDLE_NULL, S_HANDLE_NULL, NULL, NULL, 0u, SE_EDITOR_NAME_BACKEND);
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_DEBUG)) {
		se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_DEBUG, S_HANDLE_NULL, S_HANDLE_NULL, NULL, NULL, 0u, SE_EDITOR_NAME_DEBUG);
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_INPUT) && editor->input) {
		se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_INPUT, S_HANDLE_NULL, S_HANDLE_NULL, editor->input, NULL, 0u, SE_EDITOR_NAME_INPUT);
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_AUDIO) && editor->audio) {
		se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_AUDIO, S_HANDLE_NULL, S_HANDLE_NULL, editor->audio, NULL, 0u, SE_EDITOR_NAME_AUDIO);
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_NAVIGATION) && editor->navigation) {
		se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_NAVIGATION, S_HANDLE_NULL, S_HANDLE_NULL, editor->navigation, NULL, 0u, SE_EDITOR_NAME_NAVIGATION);
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_PHYSICS_2D) && editor->physics_2d) {
		se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_PHYSICS_2D, S_HANDLE_NULL, S_HANDLE_NULL, editor->physics_2d, NULL, 0u, SE_EDITOR_NAME_PHYSICS2D);
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_PHYSICS_2D_BODY) && editor->physics_2d) {
		for (sz i = 0; i < s_array_get_size(&editor->physics_2d->bodies); ++i) {
			const s_handle body_handle = s_array_handle(&editor->physics_2d->bodies, (u32)i);
			se_physics_body_2d* body = s_array_get(&editor->physics_2d->bodies, body_handle);
			if (!body || !body->is_valid) {
				continue;
			}
			snprintf(label, sizeof(label), SE_EDITOR_NAME_PHYSICS2D_BODY_U, (u32)i);
			se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_PHYSICS_2D_BODY, S_HANDLE_NULL, S_HANDLE_NULL, body, editor->physics_2d, (u32)i, label);
		}
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_PHYSICS_3D) && editor->physics_3d) {
		se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_PHYSICS_3D, S_HANDLE_NULL, S_HANDLE_NULL, editor->physics_3d, NULL, 0u, SE_EDITOR_NAME_PHYSICS3D);
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_PHYSICS_3D_BODY) && editor->physics_3d) {
		for (sz i = 0; i < s_array_get_size(&editor->physics_3d->bodies); ++i) {
			const s_handle body_handle = s_array_handle(&editor->physics_3d->bodies, (u32)i);
			se_physics_body_3d* body = s_array_get(&editor->physics_3d->bodies, body_handle);
			if (!body || !body->is_valid) {
				continue;
			}
			snprintf(label, sizeof(label), SE_EDITOR_NAME_PHYSICS3D_BODY_U, (u32)i);
			se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_PHYSICS_3D_BODY, S_HANDLE_NULL, S_HANDLE_NULL, body, editor->physics_3d, (u32)i, label);
		}
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_AUDIO_CLIP)) {
		for (sz i = 0; i < s_array_get_size(&editor->clips); ++i) {
			const s_handle item_handle = s_array_handle(&editor->clips, (u32)i);
			se_editor_audio_clip_ref* ref = s_array_get(&editor->clips, item_handle);
			if (!ref || !ref->clip) {
				continue;
			}
			se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_AUDIO_CLIP, S_HANDLE_NULL, S_HANDLE_NULL, ref->clip, NULL, (u32)i, ref->label);
		}
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_AUDIO_STREAM)) {
		for (sz i = 0; i < s_array_get_size(&editor->streams); ++i) {
			const s_handle item_handle = s_array_handle(&editor->streams, (u32)i);
			se_editor_audio_stream_ref* ref = s_array_get(&editor->streams, item_handle);
			if (!ref || !ref->stream) {
				continue;
			}
			se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_AUDIO_STREAM, S_HANDLE_NULL, S_HANDLE_NULL, ref->stream, NULL, (u32)i, ref->label);
		}
	}
	if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_AUDIO_CAPTURE)) {
		for (sz i = 0; i < s_array_get_size(&editor->captures); ++i) {
			const s_handle item_handle = s_array_handle(&editor->captures, (u32)i);
			se_editor_audio_capture_ref* ref = s_array_get(&editor->captures, item_handle);
			if (!ref || !ref->capture) {
				continue;
			}
			se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_AUDIO_CAPTURE, S_HANDLE_NULL, S_HANDLE_NULL, ref->capture, NULL, (u32)i, ref->label);
		}
	}

	if (context) {
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_WINDOW)) {
			for (sz i = 0; i < s_array_get_size(&context->windows); ++i) {
				const se_window_handle handle = s_array_handle(&context->windows, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_WINDOW_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_WINDOW, handle, S_HANDLE_NULL, se_editor_window_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_CAMERA)) {
			for (sz i = 0; i < s_array_get_size(&context->cameras); ++i) {
				const se_camera_handle handle = s_array_handle(&context->cameras, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_CAMERA_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_CAMERA, handle, S_HANDLE_NULL, se_editor_camera_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_SCENE_2D)) {
			for (sz i = 0; i < s_array_get_size(&context->scenes_2d); ++i) {
				const se_scene_2d_handle handle = s_array_handle(&context->scenes_2d, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_SCENE2D_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_SCENE_2D, handle, S_HANDLE_NULL, se_editor_scene_2d_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_OBJECT_2D)) {
			for (sz i = 0; i < s_array_get_size(&context->objects_2d); ++i) {
				const se_object_2d_handle handle = s_array_handle(&context->objects_2d, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_OBJECT2D_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_OBJECT_2D, handle, S_HANDLE_NULL, se_editor_object_2d_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_SCENE_3D)) {
			for (sz i = 0; i < s_array_get_size(&context->scenes_3d); ++i) {
				const se_scene_3d_handle handle = s_array_handle(&context->scenes_3d, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_SCENE3D_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_SCENE_3D, handle, S_HANDLE_NULL, se_editor_scene_3d_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_OBJECT_3D)) {
			for (sz i = 0; i < s_array_get_size(&context->objects_3d); ++i) {
				const se_object_3d_handle handle = s_array_handle(&context->objects_3d, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_OBJECT3D_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_OBJECT_3D, handle, S_HANDLE_NULL, se_editor_object_3d_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_UI)) {
			for (sz i = 0; i < s_array_get_size(&context->ui_roots); ++i) {
				const se_ui_handle handle = s_array_handle(&context->ui_roots, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_UI_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_UI, handle, S_HANDLE_NULL, NULL, NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_UI_WIDGET)) {
			for (sz i = 0; i < s_array_get_size(&context->ui_elements); ++i) {
				const se_ui_widget_handle handle = s_array_handle(&context->ui_elements, (u32)i);
				const se_ui_handle owner = se_editor_find_widget_owner(editor, handle);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_UI_WIDGET_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_UI_WIDGET, handle, owner, NULL, NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_VFX_2D)) {
			for (sz i = 0; i < s_array_get_size(&context->vfx_2ds); ++i) {
				const se_vfx_2d_handle handle = s_array_handle(&context->vfx_2ds, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_VFX2D_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_VFX_2D, handle, S_HANDLE_NULL, NULL, NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_VFX_3D)) {
			for (sz i = 0; i < s_array_get_size(&context->vfx_3ds); ++i) {
				const se_vfx_3d_handle handle = s_array_handle(&context->vfx_3ds, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_VFX3D_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_VFX_3D, handle, S_HANDLE_NULL, NULL, NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_SIMULATION)) {
			for (sz i = 0; i < s_array_get_size(&context->simulations); ++i) {
				const se_simulation_handle handle = s_array_handle(&context->simulations, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_SIMULATION_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_SIMULATION, handle, S_HANDLE_NULL, NULL, NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_MODEL)) {
			for (sz i = 0; i < s_array_get_size(&context->models); ++i) {
				const se_model_handle handle = s_array_handle(&context->models, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_MODEL_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_MODEL, handle, S_HANDLE_NULL, se_editor_model_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_SHADER)) {
			for (sz i = 0; i < s_array_get_size(&context->shaders); ++i) {
				const se_shader_handle handle = s_array_handle(&context->shaders, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_SHADER_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_SHADER, handle, S_HANDLE_NULL, se_editor_shader_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_TEXTURE)) {
			for (sz i = 0; i < s_array_get_size(&context->textures); ++i) {
				const se_texture_handle handle = s_array_handle(&context->textures, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_TEXTURE_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_TEXTURE, handle, S_HANDLE_NULL, se_editor_texture_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_FRAMEBUFFER)) {
			for (sz i = 0; i < s_array_get_size(&context->framebuffers); ++i) {
				const se_framebuffer_handle handle = s_array_handle(&context->framebuffers, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_FRAMEBUFFER_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_FRAMEBUFFER, handle, S_HANDLE_NULL, se_editor_framebuffer_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_RENDER_BUFFER)) {
			for (sz i = 0; i < s_array_get_size(&context->render_buffers); ++i) {
				const se_render_buffer_handle handle = s_array_handle(&context->render_buffers, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_RENDER_BUFFER_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_RENDER_BUFFER, handle, S_HANDLE_NULL, se_editor_render_buffer_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
		if (se_editor_mask_has(category_mask, SE_EDITOR_CATEGORY_FONT)) {
			for (sz i = 0; i < s_array_get_size(&context->fonts); ++i) {
				const se_font_handle handle = s_array_handle(&context->fonts, (u32)i);
				snprintf(label, sizeof(label), SE_EDITOR_NAME_FONT_U, (u32)i);
				se_editor_push_item(&editor->items, SE_EDITOR_CATEGORY_FONT, handle, S_HANDLE_NULL, se_editor_font_ptr(editor, handle), NULL, (u32)i, label);
			}
		}
	}

	*out_items = s_array_get_data(&editor->items);
	*out_count = s_array_get_size(&editor->items);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_editor_collect_properties(se_editor* editor, const se_editor_item* item, const se_editor_property** out_properties, sz* out_count) {
	b8 ok = false;
	if (!editor || !item || !out_properties || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	SE_EDITOR_ARRAY_POP_ALL(&editor->properties);

	switch (item->category) {
		case SE_EDITOR_CATEGORY_BACKEND:
			ok = se_editor_collect_backend_properties(editor);
			break;
		case SE_EDITOR_CATEGORY_DEBUG:
			ok = se_editor_collect_debug_properties(editor);
			break;
		case SE_EDITOR_CATEGORY_WINDOW:
			ok = se_editor_collect_window_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_INPUT:
			ok = se_editor_collect_input_properties(editor);
			break;
		case SE_EDITOR_CATEGORY_CAMERA:
			ok = se_editor_collect_camera_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_SCENE_2D:
			ok = se_editor_collect_scene_2d_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_OBJECT_2D:
			ok = se_editor_collect_object_2d_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_SCENE_3D:
			ok = se_editor_collect_scene_3d_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_OBJECT_3D:
			ok = se_editor_collect_object_3d_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_UI:
			ok = se_editor_collect_ui_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_UI_WIDGET:
			ok = se_editor_collect_ui_widget_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_VFX_2D:
			ok = se_editor_collect_vfx_2d_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_VFX_3D:
			ok = se_editor_collect_vfx_3d_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_SIMULATION:
			ok = se_editor_collect_simulation_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_MODEL:
			ok = se_editor_collect_model_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_SHADER:
			ok = se_editor_collect_shader_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_TEXTURE:
			ok = se_editor_collect_texture_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_FRAMEBUFFER:
			ok = se_editor_collect_framebuffer_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_RENDER_BUFFER:
			ok = se_editor_collect_render_buffer_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_FONT:
			ok = se_editor_collect_font_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_AUDIO:
			ok = se_editor_collect_audio_properties(editor);
			break;
		case SE_EDITOR_CATEGORY_AUDIO_CLIP:
			ok = se_editor_collect_audio_clip_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_AUDIO_STREAM:
			ok = se_editor_collect_audio_stream_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_AUDIO_CAPTURE:
			ok = se_editor_collect_audio_capture_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_NAVIGATION:
			ok = se_editor_collect_navigation_properties(editor);
			break;
		case SE_EDITOR_CATEGORY_PHYSICS_2D:
			ok = se_editor_collect_physics_2d_properties(editor);
			break;
		case SE_EDITOR_CATEGORY_PHYSICS_2D_BODY:
			ok = se_editor_collect_physics_2d_body_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_PHYSICS_3D:
			ok = se_editor_collect_physics_3d_properties(editor);
			break;
		case SE_EDITOR_CATEGORY_PHYSICS_3D_BODY:
			ok = se_editor_collect_physics_3d_body_properties(editor, item);
			break;
		case SE_EDITOR_CATEGORY_COUNT:
		default:
			ok = false;
			break;
	}

	if (!ok) {
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

b8 se_editor_collect_diagnostics(se_editor* editor, se_editor_diagnostics* out_diagnostics) {
	se_editor_trace_summary trace_summary = {0};
	if (!editor || !out_diagnostics) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_editor_refresh_defaults_internal(editor);
	memset(out_diagnostics, 0, sizeof(*out_diagnostics));
	out_diagnostics->backend_info = se_get_backend_info();
	out_diagnostics->capabilities = se_capabilities_get();
	if (se_editor_trace_summary_collect(&trace_summary)) {
		out_diagnostics->trace_event_count = trace_summary.event_count;
		out_diagnostics->trace_stat_count_total = trace_summary.stat_count_total;
		out_diagnostics->trace_stat_count_last_frame = trace_summary.stat_count_last_frame;
		out_diagnostics->trace_thread_count_last_frame = trace_summary.thread_count_last_frame;
		out_diagnostics->trace_cpu_ms_last_frame = trace_summary.cpu_ms_last_frame;
		out_diagnostics->trace_gpu_ms_last_frame = trace_summary.gpu_ms_last_frame;
		out_diagnostics->has_trace_stats = true;
	}
	out_diagnostics->has_frame_timing = se_debug_get_last_frame_timing(&out_diagnostics->frame_timing);
	if (editor->window != S_HANDLE_NULL) {
		se_window_get_diagnostics(editor->window, &out_diagnostics->window_diagnostics);
		out_diagnostics->has_window_diagnostics = true;
	}
	if (editor->input) {
		se_input_get_diagnostics(editor->input, &out_diagnostics->input_diagnostics);
		out_diagnostics->has_input_diagnostics = true;
	}
	if (editor->focused_simulation != S_HANDLE_NULL) {
		se_simulation_get_diagnostics(editor->focused_simulation, &out_diagnostics->simulation_diagnostics);
		out_diagnostics->has_simulation_diagnostics = true;
	}
	if (editor->focused_vfx_2d != S_HANDLE_NULL) {
		out_diagnostics->has_vfx_2d_diagnostics = se_vfx_2d_get_diagnostics(editor->focused_vfx_2d, &out_diagnostics->vfx_2d_diagnostics);
	}
	if (editor->focused_vfx_3d != S_HANDLE_NULL) {
		out_diagnostics->has_vfx_3d_diagnostics = se_vfx_3d_get_diagnostics(editor->focused_vfx_3d, &out_diagnostics->vfx_3d_diagnostics);
	}
	if (s_array_get_size(&editor->clips) > 0) {
		se_editor_audio_clip_ref* ref = s_array_get(&editor->clips, s_array_handle(&editor->clips, 0u));
		if (ref && ref->clip) {
			out_diagnostics->has_audio_clip_bands = se_audio_clip_get_bands(ref->clip, &out_diagnostics->audio_clip_bands, 2048u);
		}
	}
	if (s_array_get_size(&editor->captures) > 0) {
		se_editor_audio_capture_ref* ref = s_array_get(&editor->captures, s_array_handle(&editor->captures, 0u));
		if (ref && ref->capture) {
			out_diagnostics->has_audio_capture_bands = se_audio_capture_get_bands(ref->capture, &out_diagnostics->audio_capture_bands);
		}
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
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

b8 se_editor_validate_item(se_editor* editor, const se_editor_item* item) {
	const se_editor_item* items = NULL;
	sz item_count = 0u;
	b8 should_retry_without_owner = false;
	const se_editor_category_mask mask = se_editor_category_to_mask(item ? item->category : SE_EDITOR_CATEGORY_COUNT);
	if (!editor || !item || (u32)item->category >= (u32)SE_EDITOR_CATEGORY_COUNT || mask == 0u) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_editor_collect_items(editor, mask, &items, &item_count)) {
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
	/* Some commands attach extra owner selectors (like emitter handles). Validate base item identity too. */
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
	sz item_count = 0;
	const se_editor_category_mask mask = se_editor_category_to_mask(category);
	if (!editor || !out_item || (u32)category >= (u32)SE_EDITOR_CATEGORY_COUNT || mask == 0u) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_editor_collect_items(editor, mask, &items, &item_count)) {
		return false;
	}
	for (sz i = 0; i < item_count; ++i) {
		if (!items) {
			break;
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
	sz item_count = 0;
	const se_editor_category_mask mask = se_editor_category_to_mask(category);
	if (!editor || !label || label[0] == '\0' || !out_item || (u32)category >= (u32)SE_EDITOR_CATEGORY_COUNT || mask == 0u) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_editor_collect_items(editor, mask, &items, &item_count)) {
		return false;
	}
	for (sz i = 0; i < item_count; ++i) {
		if (!items) {
			break;
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

b8 se_editor_apply_command(se_editor* editor, const se_editor_command* command) {
	b8 ok = false;
	if (!editor || !command || command->name[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_editor_validate_item(editor, &command->item)) {
		return false;
	}
	se_set_last_error(SE_RESULT_OK);

	switch (command->item.category) {
		case SE_EDITOR_CATEGORY_BACKEND:
			ok = se_editor_apply_backend_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_DEBUG:
			ok = se_editor_apply_debug_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_WINDOW:
			ok = se_editor_apply_window_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_INPUT:
			ok = se_editor_apply_input_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_CAMERA:
			ok = se_editor_apply_camera_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_SCENE_2D:
			ok = se_editor_apply_scene_2d_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_OBJECT_2D:
			ok = se_editor_apply_object_2d_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_SCENE_3D:
			ok = se_editor_apply_scene_3d_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_OBJECT_3D:
			ok = se_editor_apply_object_3d_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_UI:
			ok = se_editor_apply_ui_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_UI_WIDGET:
			ok = se_editor_apply_ui_widget_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_VFX_2D:
			ok = se_editor_apply_vfx_2d_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_VFX_3D:
			ok = se_editor_apply_vfx_3d_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_SIMULATION:
			ok = se_editor_apply_simulation_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_MODEL:
			ok = se_editor_apply_model_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_SHADER:
			ok = se_editor_apply_shader_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_TEXTURE:
			ok = se_editor_apply_texture_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_FRAMEBUFFER:
			ok = se_editor_apply_framebuffer_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_RENDER_BUFFER:
			ok = se_editor_apply_render_buffer_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_FONT:
			ok = se_editor_apply_font_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_AUDIO:
			ok = se_editor_apply_audio_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_AUDIO_CLIP:
			ok = se_editor_apply_audio_clip_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_AUDIO_STREAM:
			ok = se_editor_apply_audio_stream_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_AUDIO_CAPTURE:
			ok = se_editor_apply_audio_capture_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_NAVIGATION:
			ok = se_editor_apply_navigation_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_PHYSICS_2D:
			ok = se_editor_apply_physics_2d_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_PHYSICS_2D_BODY:
			ok = se_editor_apply_physics_2d_body_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_PHYSICS_3D:
			ok = se_editor_apply_physics_3d_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_PHYSICS_3D_BODY:
			ok = se_editor_apply_physics_3d_body_command(editor, command);
			break;
		case SE_EDITOR_CATEGORY_COUNT:
		default:
			ok = false;
			break;
	}

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
