// Syphax-Engine - Ougi Washi

#include "se_scene.h"
#include "se_shader.h"
#include "se_debug.h"
#include "se_text.h"
#include "se_ui.h"
#include "se_window.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RTS_WINDOW_WIDTH 1600
#define RTS_WINDOW_HEIGHT 900
#define RTS_MAP_HALF_SIZE 28.0f
#define RTS_MAX_UNITS_PER_KIND 72
#define RTS_MAX_BUILDINGS_PER_KIND 20
#define RTS_MAX_RESOURCES 24
#define RTS_MAX_SELECTABLE_UNITS (RTS_MAX_UNITS_PER_KIND * 2)
#define RTS_MINIMAP_ALLY_CAP (RTS_MAX_SELECTABLE_UNITS + RTS_MAX_BUILDINGS_PER_KIND * 3)
#define RTS_MINIMAP_ENEMY_CAP (RTS_MAX_SELECTABLE_UNITS + RTS_MAX_BUILDINGS_PER_KIND * 3)
#define RTS_GROUND_TILE_GRID 14
#define RTS_GROUND_TILE_COUNT (RTS_GROUND_TILE_GRID * RTS_GROUND_TILE_GRID)
#define RTS_SELECTION_RADIUS 1.4f
#define RTS_COMMAND_RADIUS 1.6f
#define RTS_UI_TEXT_BUFFER 4096
#define RTS_MINIMAP_CENTER_X 0.74f
#define RTS_MINIMAP_CENTER_Y -0.70f
#define RTS_MINIMAP_HALF_SIZE 0.21f

#define RTS_CAMERA_DEFAULT_YAW (-PI)
#define RTS_CAMERA_DEFAULT_PITCH 1.0f
#define RTS_CAMERA_DEFAULT_DISTANCE 24.0f

#define RTS_LOG_PREFIX "[99_game] "

typedef enum {
	RTS_TEAM_ALLY = 0,
	RTS_TEAM_ENEMY = 1
} rts_team;

typedef enum {
	RTS_UNIT_ROLE_WORKER = 0,
	RTS_UNIT_ROLE_SOLDIER = 1
} rts_unit_role;

typedef enum {
	RTS_UNIT_KIND_ALLY_WORKER = 0,
	RTS_UNIT_KIND_ALLY_SOLDIER,
	RTS_UNIT_KIND_ENEMY_WORKER,
	RTS_UNIT_KIND_ENEMY_SOLDIER,
	RTS_UNIT_KIND_COUNT
} rts_unit_kind;

typedef enum {
	RTS_BUILDING_TYPE_HQ = 0,
	RTS_BUILDING_TYPE_BARRACKS,
	RTS_BUILDING_TYPE_TOWER
} rts_building_type;

typedef enum {
	RTS_BUILD_MODE_NONE = 0,
	RTS_BUILD_MODE_BARRACKS,
	RTS_BUILD_MODE_TOWER
} rts_build_mode;

typedef enum {
	RTS_BUILDING_KIND_ALLY_HQ = 0,
	RTS_BUILDING_KIND_ALLY_BARRACKS,
	RTS_BUILDING_KIND_ALLY_TOWER,
	RTS_BUILDING_KIND_ENEMY_HQ,
	RTS_BUILDING_KIND_ENEMY_BARRACKS,
	RTS_BUILDING_KIND_ENEMY_TOWER,
	RTS_BUILDING_KIND_COUNT
} rts_building_kind;

typedef enum {
	RTS_UNIT_STATE_IDLE = 0,
	RTS_UNIT_STATE_MOVE,
	RTS_UNIT_STATE_GATHER,
	RTS_UNIT_STATE_RETURN
} rts_unit_state;

typedef struct {
	b8 active;
	b8 selected;
	b8 attack_move;
	rts_team team;
	rts_unit_role role;
	rts_unit_state state;
	s_vec3 position;
	s_vec3 target_position;
	f32 facing_yaw;
	i32 target_resource_index;
	f32 health;
	f32 max_health;
	f32 speed;
	f32 attack_range;
	f32 attack_damage;
	f32 attack_cooldown;
	f32 attack_timer;
	f32 gather_rate;
	f32 gather_timer;
	f32 carry;
	f32 carry_capacity;
	f32 radius;
	se_instance_id instance_id;
	i32 serial;
} rts_unit;

typedef struct {
	b8 active;
	rts_team team;
	rts_building_type type;
	s_vec3 position;
	f32 health;
	f32 max_health;
	f32 radius;
	f32 spawn_timer;
	f32 attack_timer;
	se_instance_id instance_id;
	i32 serial;
} rts_building;

typedef struct {
	b8 active;
	s_vec3 position;
	f32 amount;
	f32 max_amount;
	f32 radius;
	se_instance_id instance_id;
	i32 serial;
} rts_resource_node;

typedef struct {
	b8 enabled;
	i32 phase;
	f32 phase_time;
	f32 total_time;
	f32 next_status_print;
	f32 stop_after_seconds;
	u32 phase_actions;
} rts_input_simulator;

typedef enum {
	RTS_AUTOTEST_SYSTEM_SIMULATOR = 0,
	RTS_AUTOTEST_SYSTEM_CAMERA,
	RTS_AUTOTEST_SYSTEM_PLAYER_INPUT,
	RTS_AUTOTEST_SYSTEM_ENEMY_AI,
	RTS_AUTOTEST_SYSTEM_UNITS,
	RTS_AUTOTEST_SYSTEM_NAVIGATION,
	RTS_AUTOTEST_SYSTEM_BUILDINGS,
	RTS_AUTOTEST_SYSTEM_SELECTION,
	RTS_AUTOTEST_SYSTEM_VISUALS,
	RTS_AUTOTEST_SYSTEM_BUILD_PREVIEW,
	RTS_AUTOTEST_SYSTEM_MINIMAP,
	RTS_AUTOTEST_SYSTEM_RENDER,
	RTS_AUTOTEST_SYSTEM_VALIDATE,
	RTS_AUTOTEST_SYSTEM_COUNT
} rts_autotest_system;

typedef struct {
	b8 enabled;
	b8 full_systems_required;
	b8 strict_fail_fast;
	f32 fixed_dt;
	i32 sim_substeps_per_frame;
	u64 checkpoints_mask;
	i32 checkpoints_passed;
	i32 system_call_count[RTS_AUTOTEST_SYSTEM_COUNT];

	i32 initial_ally_workers;
	i32 initial_ally_soldiers;
	i32 initial_enemy_workers;
	i32 initial_enemy_soldiers;
	f32 initial_ally_resources;
	f32 initial_enemy_resources;
	f32 initial_min_resource_amount;
	f32 initial_camera_x;
	f32 initial_camera_z;
	f32 initial_camera_yaw;
	f32 initial_camera_pitch;
	f32 initial_camera_distance;

	f32 min_enemy_hq_hp;
	f32 min_ally_hq_hp;
	f32 min_resource_amount;
	f32 max_ally_resources;

	b8 saw_worker_select;
	b8 saw_soldier_select;
	b8 saw_gather_order;
	b8 saw_move_order;
	b8 saw_attack_order;
	b8 saw_build_attempt;
	b8 saw_build_success;
	b8 saw_worker_train_attempt;
	b8 saw_worker_train_success;
	b8 saw_soldier_train_attempt;
	b8 saw_soldier_train_success;
	b8 saw_enemy_train_success;
	b8 saw_resource_harvest;
	b8 saw_resource_deposit;
	b8 saw_enemy_wave_command;
	b8 saw_tower_attack;
	b8 saw_unit_damage;
	b8 saw_building_damage;
	b8 saw_unit_death;
	b8 saw_building_death;
	b8 saw_camera_motion;
	b8 saw_minimap_update;
	b8 saw_build_preview_update;
	b8 saw_text_render;
	b8 saw_validation_pass;

	i32 failures;
	c8 failure_reason[256];
} rts_autotest_state;

typedef struct {
	se_context *ctx;
	se_window_handle window;
	se_scene_3d_handle scene;
	se_text_handle *text_handle;
	se_font_handle font;

	se_model_handle ground_model;
	se_model_handle ground_tile_models[2];
	se_model_handle unit_models[RTS_UNIT_KIND_COUNT];
	se_model_handle building_models[RTS_BUILDING_KIND_COUNT];
	se_model_handle resource_model;
	se_model_handle build_preview_models[2];
	se_model_handle selection_model;

	se_object_3d_handle ground_object;
	se_object_3d_handle ground_tile_objects[2];
	se_object_3d_handle unit_objects[RTS_UNIT_KIND_COUNT];
	se_object_3d_handle building_objects[RTS_BUILDING_KIND_COUNT];
	se_object_3d_handle resource_object;
	se_object_3d_handle build_preview_objects[2];
	se_object_3d_handle selection_object;
	se_instance_id build_preview_instance_ids[2];
	se_instance_id selection_instance_ids[RTS_MAX_SELECTABLE_UNITS];

	se_scene_2d_handle hud_scene;
	se_object_2d_handle hud_minimap_panel_object;
	se_object_2d_handle hud_minimap_frame_object;
	se_object_2d_handle hud_minimap_allies_object;
	se_object_2d_handle hud_minimap_enemies_object;
	se_object_2d_handle hud_minimap_resources_object;
	se_object_2d_handle hud_minimap_selected_object;
	se_instance_id hud_minimap_allies_ids[RTS_MINIMAP_ALLY_CAP];
	se_instance_id hud_minimap_enemies_ids[RTS_MINIMAP_ENEMY_CAP];
	se_instance_id hud_minimap_resources_ids[RTS_MAX_RESOURCES];
	se_instance_id hud_minimap_selected_ids[RTS_MAX_SELECTABLE_UNITS];

	rts_unit units[RTS_UNIT_KIND_COUNT][RTS_MAX_UNITS_PER_KIND];
	rts_building buildings[RTS_BUILDING_KIND_COUNT][RTS_MAX_BUILDINGS_PER_KIND];
	rts_resource_node resources[RTS_MAX_RESOURCES];

	f32 team_resources[2];
	f32 sim_time;
	u64 frame_index;
	i32 serial_counter;
	i32 selected_count;

	s_vec3 camera_target;
	f32 camera_yaw;
	f32 camera_pitch;
	f32 camera_distance;

	s_vec3 mouse_world;
	b8 mouse_world_valid;
	s_vec3 build_preview_world;
	b8 build_preview_valid;
	rts_build_mode build_mode;
	c8 build_block_reason[160];

	b8 show_debug_overlay;
	b8 show_minimap_ui;
	b8 perf_mode;
	b8 track_system_timing;
	b8 headless_mode;
	b8 validations_failed;
	c8 command_line[256];
	f32 command_line_timer;
	f32 next_system_timing_log_time;

	f32 enemy_strategy_timer;
	f32 enemy_wave_timer;
	f32 enemy_build_timer;
	i32 match_result; // 0 running, 1 ally win, -1 ally loss

	rts_input_simulator simulator;
	rts_autotest_state autotest;
} rts_game;

typedef struct {
	rts_unit *unit;
	rts_unit_kind kind;
	i32 index;
	f32 distance;
} rts_unit_target;

typedef struct {
	rts_building *building;
	rts_building_kind kind;
	i32 index;
	f32 distance;
} rts_building_target;

typedef struct {
	const rts_game *game;
} rts_pick_filter_state;

static void rts_autotest_mark_system(rts_game *game, const rts_autotest_system system);

static f32 rts_clampf(const f32 value, const f32 min_value, const f32 max_value) {
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

static f32 rts_randf(const f32 min_value, const f32 max_value) {
	const f32 t = (f32)rand() / (f32)RAND_MAX;
	return min_value + (max_value - min_value) * t;
}

static f32 rts_distance_xz(const s_vec3 *a, const s_vec3 *b) {
	const f32 dx = a->x - b->x;
	const f32 dz = a->z - b->z;
	return sqrtf(dx * dx + dz * dz);
}

static b8 rts_pick_filter_ally_unit_objects(const se_object_3d_handle object, void *user_data) {
	const rts_pick_filter_state *state = (const rts_pick_filter_state *)user_data;
	if (!state || !state->game) {
		return false;
	}
	return object == state->game->unit_objects[RTS_UNIT_KIND_ALLY_WORKER] ||
		   object == state->game->unit_objects[RTS_UNIT_KIND_ALLY_SOLDIER];
}

static rts_unit_kind rts_pick_unit_kind_from_object(const rts_game *game, const se_object_3d_handle object) {
	if (!game) {
		return (rts_unit_kind)-1;
	}
	if (object == game->unit_objects[RTS_UNIT_KIND_ALLY_WORKER]) {
		return RTS_UNIT_KIND_ALLY_WORKER;
	}
	if (object == game->unit_objects[RTS_UNIT_KIND_ALLY_SOLDIER]) {
		return RTS_UNIT_KIND_ALLY_SOLDIER;
	}
	return (rts_unit_kind)-1;
}

static void rts_log(const c8 *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf(RTS_LOG_PREFIX);
	vprintf(fmt, args);
	printf("\n");
	fflush(stdout);
	va_end(args);
}

static b8 rts_env_flag_enabled(const c8 *key) {
	if (!key) {
		return false;
	}
	const c8 *value = getenv(key);
	if (!value || value[0] == '\0') {
		return false;
	}
	if (strcmp(value, "0") == 0 ||
		strcmp(value, "false") == 0 ||
		strcmp(value, "off") == 0 ||
		strcmp(value, "no") == 0) {
		return false;
	}
	return true;
}

static b8 rts_terminal_allow_logs(void) {
	return
		rts_env_flag_enabled("SE_TERMINAL_ALLOW_LOGS") ||
		rts_env_flag_enabled("SE_TERMINAL_ALLOW_STDOUT_LOGS");
}

static void rts_set_command_line(rts_game *game, const c8 *fmt, ...) {
	if (!game || !fmt) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	vsnprintf(game->command_line, sizeof(game->command_line), fmt, args);
	va_end(args);
	game->command_line_timer = 3.0f;
	rts_log("%s", game->command_line);
}

static rts_team rts_unit_kind_team(const rts_unit_kind kind) {
	switch (kind) {
		case RTS_UNIT_KIND_ALLY_WORKER:
		case RTS_UNIT_KIND_ALLY_SOLDIER: return RTS_TEAM_ALLY;
		case RTS_UNIT_KIND_ENEMY_WORKER:
		case RTS_UNIT_KIND_ENEMY_SOLDIER: return RTS_TEAM_ENEMY;
		default: return RTS_TEAM_ALLY;
	}
}

static rts_unit_role rts_unit_kind_role(const rts_unit_kind kind) {
	switch (kind) {
		case RTS_UNIT_KIND_ALLY_WORKER:
		case RTS_UNIT_KIND_ENEMY_WORKER: return RTS_UNIT_ROLE_WORKER;
		case RTS_UNIT_KIND_ALLY_SOLDIER:
		case RTS_UNIT_KIND_ENEMY_SOLDIER: return RTS_UNIT_ROLE_SOLDIER;
		default: return RTS_UNIT_ROLE_WORKER;
	}
}

static rts_unit_kind rts_make_unit_kind(const rts_team team, const rts_unit_role role) {
	if (team == RTS_TEAM_ALLY) {
		return (role == RTS_UNIT_ROLE_WORKER) ? RTS_UNIT_KIND_ALLY_WORKER : RTS_UNIT_KIND_ALLY_SOLDIER;
	}
	return (role == RTS_UNIT_ROLE_WORKER) ? RTS_UNIT_KIND_ENEMY_WORKER : RTS_UNIT_KIND_ENEMY_SOLDIER;
}

static rts_team rts_building_kind_team(const rts_building_kind kind) {
	switch (kind) {
		case RTS_BUILDING_KIND_ALLY_HQ:
		case RTS_BUILDING_KIND_ALLY_BARRACKS:
		case RTS_BUILDING_KIND_ALLY_TOWER: return RTS_TEAM_ALLY;
		case RTS_BUILDING_KIND_ENEMY_HQ:
		case RTS_BUILDING_KIND_ENEMY_BARRACKS:
		case RTS_BUILDING_KIND_ENEMY_TOWER: return RTS_TEAM_ENEMY;
		default: return RTS_TEAM_ALLY;
	}
}

static rts_building_type rts_building_kind_type(const rts_building_kind kind) {
	switch (kind) {
		case RTS_BUILDING_KIND_ALLY_HQ:
		case RTS_BUILDING_KIND_ENEMY_HQ: return RTS_BUILDING_TYPE_HQ;
		case RTS_BUILDING_KIND_ALLY_BARRACKS:
		case RTS_BUILDING_KIND_ENEMY_BARRACKS: return RTS_BUILDING_TYPE_BARRACKS;
		case RTS_BUILDING_KIND_ALLY_TOWER:
		case RTS_BUILDING_KIND_ENEMY_TOWER: return RTS_BUILDING_TYPE_TOWER;
		default: return RTS_BUILDING_TYPE_HQ;
	}
}

static rts_building_kind rts_make_building_kind(const rts_team team, const rts_building_type type) {
	if (team == RTS_TEAM_ALLY) {
		if (type == RTS_BUILDING_TYPE_HQ) return RTS_BUILDING_KIND_ALLY_HQ;
		if (type == RTS_BUILDING_TYPE_BARRACKS) return RTS_BUILDING_KIND_ALLY_BARRACKS;
		return RTS_BUILDING_KIND_ALLY_TOWER;
	}
	if (type == RTS_BUILDING_TYPE_HQ) return RTS_BUILDING_KIND_ENEMY_HQ;
	if (type == RTS_BUILDING_TYPE_BARRACKS) return RTS_BUILDING_KIND_ENEMY_BARRACKS;
	return RTS_BUILDING_KIND_ENEMY_TOWER;
}

static s_vec3 rts_unit_scale(const rts_unit_role role, const b8 selected) {
	f32 base = (role == RTS_UNIT_ROLE_WORKER) ? 0.38f : 0.54f;
	if (selected) {
		base *= 1.24f;
	}
	return s_vec3(base, base, base);
}

static s_vec3 rts_building_scale(const rts_building_type type) {
	switch (type) {
		case RTS_BUILDING_TYPE_HQ: return s_vec3(2.5f, 1.6f, 2.5f);
		case RTS_BUILDING_TYPE_BARRACKS: return s_vec3(1.7f, 1.1f, 1.7f);
		case RTS_BUILDING_TYPE_TOWER: return s_vec3(0.85f, 1.95f, 0.85f);
		default: return s_vec3(1.0f, 1.0f, 1.0f);
	}
}

static f32 rts_building_radius(const rts_building_type type) {
	switch (type) {
		case RTS_BUILDING_TYPE_HQ: return 2.9f;
		case RTS_BUILDING_TYPE_BARRACKS: return 2.1f;
		case RTS_BUILDING_TYPE_TOWER: return 1.4f;
		default: return 1.0f;
	}
}

static rts_building_type rts_build_mode_to_type(const rts_build_mode mode) {
	if (mode == RTS_BUILD_MODE_BARRACKS) {
		return RTS_BUILDING_TYPE_BARRACKS;
	}
	if (mode == RTS_BUILD_MODE_TOWER) {
		return RTS_BUILDING_TYPE_TOWER;
	}
	return RTS_BUILDING_TYPE_HQ;
}

static const c8 *rts_building_type_name(const rts_building_type type) {
	switch (type) {
		case RTS_BUILDING_TYPE_HQ: return "HQ";
		case RTS_BUILDING_TYPE_BARRACKS: return "Barracks";
		case RTS_BUILDING_TYPE_TOWER: return "Tower";
		default: return "Building";
	}
}

static s_mat4 rts_hidden_transform(void) {
	s_mat4 transform = s_mat4_identity;
	const s_vec3 hidden_pos = s_vec3(0.0f, -250.0f, 0.0f);
	const s_vec3 hidden_scale = s_vec3(0.001f, 0.001f, 0.001f);
	s_mat4_set_translation(&transform, &hidden_pos);
	transform = s_mat4_scale(&transform, &hidden_scale);
	return transform;
}

static s_mat4 rts_make_transform(const s_vec3 *position, const s_vec3 *scale, const f32 yaw) {
	s_mat4 transform = s_mat4_identity;
	s_mat4_set_translation(&transform, position);
	transform = s_mat4_rotate_y(&transform, yaw);
	transform = s_mat4_scale(&transform, scale);
	return transform;
}

static void rts_clamp_world_position(s_vec3 *position) {
	if (!position) {
		return;
	}
	position->x = rts_clampf(position->x, -RTS_MAP_HALF_SIZE, RTS_MAP_HALF_SIZE);
	position->z = rts_clampf(position->z, -RTS_MAP_HALF_SIZE, RTS_MAP_HALF_SIZE);
}

static void rts_get_input_view_size(const rts_game *game, f32 *out_width, f32 *out_height) {
	u32 width_px = 0;
	u32 height_px = 0;
	if (game && game->window != S_HANDLE_NULL) {
		se_window_get_size(game->window, &width_px, &height_px);
		if (width_px <= 1 || height_px <= 1) {
			se_window_get_framebuffer_size(game->window, &width_px, &height_px);
		}
	}
	const f32 width = (f32)width_px;
	const f32 height = (f32)height_px;
	if (out_width) {
		*out_width = width;
	}
	if (out_height) {
		*out_height = height;
	}
}

static void rts_get_minimap_world_rect(se_box_2d *out_world_rect) {
	if (!out_world_rect) {
		return;
	}
	out_world_rect->min = s_vec2(-RTS_MAP_HALF_SIZE, -RTS_MAP_HALF_SIZE);
	out_world_rect->max = s_vec2(RTS_MAP_HALF_SIZE, RTS_MAP_HALF_SIZE);
}

static void rts_get_minimap_ui_rect_inner(se_box_2d *out_ui_rect) {
	if (!out_ui_rect) {
		return;
	}
	const f32 inner_half = RTS_MINIMAP_HALF_SIZE * 0.92f;
	out_ui_rect->min = s_vec2(RTS_MINIMAP_CENTER_X - inner_half, RTS_MINIMAP_CENTER_Y + inner_half);
	out_ui_rect->max = s_vec2(RTS_MINIMAP_CENTER_X + inner_half, RTS_MINIMAP_CENTER_Y - inner_half);
}

static void rts_get_minimap_ui_rect_bounds(se_box_2d *out_ui_rect) {
	if (!out_ui_rect) {
		return;
	}
	out_ui_rect->min = s_vec2(RTS_MINIMAP_CENTER_X - RTS_MINIMAP_HALF_SIZE, RTS_MINIMAP_CENTER_Y - RTS_MINIMAP_HALF_SIZE);
	out_ui_rect->max = s_vec2(RTS_MINIMAP_CENTER_X + RTS_MINIMAP_HALF_SIZE, RTS_MINIMAP_CENTER_Y + RTS_MINIMAP_HALF_SIZE);
}

static b8 rts_screen_to_world_ray(rts_game *game, const f32 mouse_x, const f32 mouse_y, s_vec3 *out_origin, s_vec3 *out_dir) {
	if (!game || game->window == S_HANDLE_NULL || !out_origin || !out_dir) {
		return false;
	}
	if (game->scene == S_HANDLE_NULL) {
		return false;
	}
	const se_camera_handle camera = se_scene_3d_get_camera(game->scene);
	if (camera == S_HANDLE_NULL) {
		return false;
	}

	f32 width = 0.0f;
	f32 height = 0.0f;
	rts_get_input_view_size(game, &width, &height);
	if (width <= 0.0f || height <= 0.0f) {
		return false;
	}

	return se_camera_screen_to_ray(camera, mouse_x, mouse_y, width, height, out_origin, out_dir);
}

static b8 rts_screen_to_ground(rts_game *game, const f32 mouse_x, const f32 mouse_y, s_vec3 *out_world) {
	if (!game || !out_world) {
		return false;
	}
	if (game->scene == S_HANDLE_NULL) {
		return false;
	}
	const se_camera_handle camera = se_scene_3d_get_camera(game->scene);
	if (camera == S_HANDLE_NULL) {
		return false;
	}

	f32 width = 0.0f;
	f32 height = 0.0f;
	rts_get_input_view_size(game, &width, &height);
	if (width <= 1.0f || height <= 1.0f) {
		return false;
	}

	const s_vec3 plane_point = s_vec3(0.0f, 0.0f, 0.0f);
	const s_vec3 plane_normal = s_vec3(0.0f, 1.0f, 0.0f);
	if (!se_camera_screen_to_plane(camera, mouse_x, mouse_y, width, height, &plane_point, &plane_normal, out_world)) {
		return false;
	}
	out_world->y = 0.0f;
	rts_clamp_world_position(out_world);
	return true;
}

static b8 rts_ray_hits_sphere(const s_vec3 *ray_origin, const s_vec3 *ray_dir, const s_vec3 *center, const f32 radius, f32 *out_t) {
	if (!ray_origin || !ray_dir || !center || radius <= 0.0f) {
		return false;
	}
	const s_vec3 oc = s_vec3_sub(ray_origin, center);
	const f32 b = s_vec3_dot(&oc, ray_dir);
	const f32 c = s_vec3_dot(&oc, &oc) - radius * radius;
	const f32 discriminant = b * b - c;
	if (discriminant < 0.0f) {
		return false;
	}

	const f32 sqrt_disc = sqrtf(discriminant);
	f32 t = -b - sqrt_disc;
	if (t < 0.0f) {
		t = -b + sqrt_disc;
	}
	if (t < 0.0f) {
		return false;
	}
	if (out_t) {
		*out_t = t;
	}
	return true;
}

static void rts_perf_find_bottleneck(const se_debug_frame_timing *timing, const c8 **out_name, f64 *out_ms) {
	if (!out_name || !out_ms) {
		return;
	}
	*out_name = "none";
	*out_ms = 0.0;
	if (!timing) {
		return;
	}
	const struct {
		const c8 *name;
		f64 value;
	} candidates[] = {
		{"scene3d", timing->scene3d_ms},
		{"text", timing->text_ms},
		{"present", timing->window_present_ms},
		{"scene2d", timing->scene2d_ms},
		{"ui", timing->ui_ms},
		{"input", timing->input_ms},
		{"window_update", timing->window_update_ms},
		{"navigation", timing->navigation_ms},
		{"other", timing->other_ms}
	};
	for (sz i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
		if (candidates[i].value > *out_ms) {
			*out_ms = candidates[i].value;
			*out_name = candidates[i].name;
		}
	}
}

static b8 rts_world_to_screen(rts_game *game, const s_vec3 *world, f32 *out_x, f32 *out_y) {
	if (!game || game->window == S_HANDLE_NULL || !world || !out_x || !out_y) {
		return false;
	}
	if (game->scene == S_HANDLE_NULL) {
		return false;
	}
	const se_camera_handle camera = se_scene_3d_get_camera(game->scene);
	if (camera == S_HANDLE_NULL) {
		return false;
	}

	s_vec2 screen = s_vec2(0.0f, 0.0f);
	if (!se_ui_world_to_window(camera, game->window, world, &screen)) {
		return false;
	}
	s_vec3 ndc = s_vec3(0.0f, 0.0f, 0.0f);
	if (!se_camera_world_to_ndc(camera, world, &ndc)) {
		return false;
	}
	const f32 ndc_x = ndc.x;
	const f32 ndc_y = ndc.y;
	const f32 ndc_z = ndc.z;
	if (ndc_z < -1.05f || ndc_z > 1.05f) {
		return false;
	}
	if (ndc_x < -1.4f || ndc_x > 1.4f || ndc_y < -1.4f || ndc_y > 1.4f) {
		return false;
	}
	*out_x = screen.x;
	*out_y = screen.y;
	return true;
}

static s_mat3 rts_make_ui_transform(const f32 x, const f32 y, const f32 sx, const f32 sy) {
	s_mat3 transform = s_mat3_identity;
	transform = s_mat3_scale(&transform, &s_vec2(sx, sy));
	s_mat3_set_translation(&transform, &s_vec2(x, y));
	return transform;
}

static void rts_apply_model_material(rts_game *game, const se_model_handle model, const s_vec3 *color, const f32 ambient_boost, const f32 specular_power, const f32 emissive) {
	(void)game;
	if (model == S_HANDLE_NULL || !color) {
		return;
	}

	const sz mesh_count = se_model_get_mesh_count(model);
	for (sz i = 0; i < mesh_count; ++i) {
		const se_shader_handle shader = se_model_get_mesh_shader(model, i);
		if (shader == S_HANDLE_NULL) {
			continue;
		}
		se_shader_set_vec3(shader, "u_team_color", color);
		se_shader_set_float(shader, "u_ambient_boost", ambient_boost);
		se_shader_set_float(shader, "u_specular_power", specular_power);
		se_shader_set_float(shader, "u_emissive_strength", emissive);
	}
}

static se_model_handle rts_load_colored_model(rts_game *game, const c8 *model_path, const s_vec3 *color, const f32 ambient_boost, const f32 specular_power, const f32 emissive) {
	se_model_handle model = se_model_load_obj_simple(
		model_path,
		SE_RESOURCE_EXAMPLE("99_game/rts_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("99_game/rts_fragment.glsl"));
	if (model == S_HANDLE_NULL) {
		rts_log("failed to load model: %s", model_path);
		return S_HANDLE_NULL;
	}
	rts_apply_model_material(game, model, color, ambient_boost, specular_power, emissive);
	return model;
}

static se_object_3d_handle rts_create_scene_object(rts_game *game, const se_model_handle model, const sz max_instances) {
	if (!game || model == S_HANDLE_NULL || game->scene == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	const s_mat4 transform = s_mat4_identity;
	se_object_3d_handle object = se_object_3d_create(model, &transform, max_instances);
	if (object != S_HANDLE_NULL) {
		se_scene_3d_add_object(game->scene, object);
	}
	return object;
}

static void rts_hide_minimap_instances(const se_object_2d_handle object, const se_instance_id *instance_ids, const i32 count) {
	if (object == S_HANDLE_NULL || !instance_ids || count <= 0) {
		return;
	}
	const s_mat3 hidden = rts_make_ui_transform(-5.0f, -5.0f, 0.0001f, 0.0001f);
	for (i32 i = 0; i < count; ++i) {
		if (instance_ids[i] >= 0) {
			se_object_2d_set_instance_transform(object, instance_ids[i], &hidden);
		}
	}
}

static b8 rts_world_to_minimap_ui(const s_vec3 *world, s_vec2 *out_ui) {
	if (!world || !out_ui) {
		return false;
	}
	se_box_2d world_rect = {0};
	se_box_2d ui_rect = {0};
	rts_get_minimap_world_rect(&world_rect);
	rts_get_minimap_ui_rect_inner(&ui_rect);
	const s_vec2 world_xz = s_vec2(
		rts_clampf(world->x, -RTS_MAP_HALF_SIZE, RTS_MAP_HALF_SIZE),
		rts_clampf(world->z, -RTS_MAP_HALF_SIZE, RTS_MAP_HALF_SIZE));
	return se_ui_minimap_world_to_ui(&world_rect, &ui_rect, &world_xz, out_ui);
}

static b8 rts_screen_to_ui(rts_game *game, const f32 mouse_x, const f32 mouse_y, s_vec2 *out_ui) {
	if (!game || game->window == S_HANDLE_NULL || !out_ui) {
		return false;
	}
	return se_window_pixel_to_normalized(game->window, mouse_x, mouse_y, out_ui);
}

static b8 rts_minimap_ui_to_world(const s_vec2 *ui, s_vec3 *out_world) {
	if (!ui || !out_world) {
		return false;
	}
	se_box_2d world_rect = {0};
	se_box_2d ui_rect = {0};
	se_box_2d ui_bounds = {0};
	rts_get_minimap_world_rect(&world_rect);
	rts_get_minimap_ui_rect_inner(&ui_rect);
	rts_get_minimap_ui_rect_bounds(&ui_bounds);
	if (!se_box_2d_is_inside(&ui_bounds, ui)) {
		return false;
	}
	s_vec2 world_xz = s_vec2(0.0f, 0.0f);
	if (!se_ui_minimap_ui_to_world(&world_rect, &ui_rect, ui, &world_xz)) {
		return false;
	}
	*out_world = s_vec3(
		rts_clampf(world_xz.x, -RTS_MAP_HALF_SIZE, RTS_MAP_HALF_SIZE),
		0.0f,
		rts_clampf(world_xz.y, -RTS_MAP_HALF_SIZE, RTS_MAP_HALF_SIZE));
	return true;
}

static void rts_set_minimap_instance(const se_object_2d_handle object, const se_instance_id instance_id, const f32 x, const f32 y, const f32 size) {
	if (object == S_HANDLE_NULL || instance_id < 0) {
		return;
	}
	const s_mat3 transform = rts_make_ui_transform(x, y, size, size);
	se_object_2d_set_instance_transform(object, instance_id, &transform);
}

static b8 rts_init_hud_scene(rts_game *game) {
	if (!game || game->headless_mode || game->window == S_HANDLE_NULL) {
		return true;
	}

	u32 hud_width = 0;
	u32 hud_height = 0;
	se_window_get_size(game->window, &hud_width, &hud_height);
	if (hud_width <= 1 || hud_height <= 1) {
		se_window_get_framebuffer_size(game->window, &hud_width, &hud_height);
	}
	game->hud_scene = se_scene_2d_create(&s_vec2((f32)hud_width, (f32)hud_height), 6);
	if (game->hud_scene == S_HANDLE_NULL) {
		rts_log("failed to create HUD scene");
		return false;
	}
	se_scene_2d_set_auto_resize(game->hud_scene, game->window, &s_vec2(1.0f, 1.0f));

	game->hud_minimap_panel_object = se_object_2d_create(SE_RESOURCE_EXAMPLE("99_game/ui_panel.glsl"), &s_mat3_identity, 0);
	game->hud_minimap_frame_object = se_object_2d_create(SE_RESOURCE_EXAMPLE("99_game/ui_frame.glsl"), &s_mat3_identity, 0);
	game->hud_minimap_allies_object = se_object_2d_create(SE_RESOURCE_EXAMPLE("ui/button.glsl"), &s_mat3_identity, RTS_MINIMAP_ALLY_CAP);
	game->hud_minimap_enemies_object = se_object_2d_create(SE_RESOURCE_EXAMPLE("ui/button.glsl"), &s_mat3_identity, RTS_MINIMAP_ENEMY_CAP);
	game->hud_minimap_resources_object = se_object_2d_create(SE_RESOURCE_EXAMPLE("ui/button.glsl"), &s_mat3_identity, RTS_MAX_RESOURCES);
	game->hud_minimap_selected_object = se_object_2d_create(SE_RESOURCE_EXAMPLE("ui/button.glsl"), &s_mat3_identity, RTS_MAX_SELECTABLE_UNITS);
	if (game->hud_minimap_panel_object == S_HANDLE_NULL ||
		game->hud_minimap_frame_object == S_HANDLE_NULL ||
		game->hud_minimap_allies_object == S_HANDLE_NULL ||
		game->hud_minimap_enemies_object == S_HANDLE_NULL ||
		game->hud_minimap_resources_object == S_HANDLE_NULL ||
		game->hud_minimap_selected_object == S_HANDLE_NULL) {
		rts_log("failed to create HUD objects");
		return false;
	}

	se_object_2d_set_position(game->hud_minimap_panel_object, &s_vec2(RTS_MINIMAP_CENTER_X, RTS_MINIMAP_CENTER_Y));
	se_object_2d_set_scale(game->hud_minimap_panel_object, &s_vec2(RTS_MINIMAP_HALF_SIZE, RTS_MINIMAP_HALF_SIZE));
	se_object_2d_set_position(game->hud_minimap_frame_object, &s_vec2(RTS_MINIMAP_CENTER_X, RTS_MINIMAP_CENTER_Y));
	se_object_2d_set_scale(game->hud_minimap_frame_object, &s_vec2(RTS_MINIMAP_HALF_SIZE * 1.06f, RTS_MINIMAP_HALF_SIZE * 1.06f));

	se_shader_handle allies_shader = se_object_2d_get_shader(game->hud_minimap_allies_object);
	se_shader_handle enemies_shader = se_object_2d_get_shader(game->hud_minimap_enemies_object);
	se_shader_handle resources_shader = se_object_2d_get_shader(game->hud_minimap_resources_object);
	se_shader_handle selected_shader = se_object_2d_get_shader(game->hud_minimap_selected_object);
	if (allies_shader != S_HANDLE_NULL) {
		se_shader_set_vec3(allies_shader, "u_color", &s_vec3(0.22f, 0.90f, 1.00f));
	}
	if (enemies_shader != S_HANDLE_NULL) {
		se_shader_set_vec3(enemies_shader, "u_color", &s_vec3(1.00f, 0.26f, 0.16f));
	}
	if (resources_shader != S_HANDLE_NULL) {
		se_shader_set_vec3(resources_shader, "u_color", &s_vec3(1.00f, 0.87f, 0.20f));
	}
	if (selected_shader != S_HANDLE_NULL) {
		se_shader_set_vec3(selected_shader, "u_color", &s_vec3(0.98f, 0.98f, 0.98f));
	}

	se_scene_2d_add_object(game->hud_scene, game->hud_minimap_panel_object);
	se_scene_2d_add_object(game->hud_scene, game->hud_minimap_frame_object);
	se_scene_2d_add_object(game->hud_scene, game->hud_minimap_resources_object);
	se_scene_2d_add_object(game->hud_scene, game->hud_minimap_allies_object);
	se_scene_2d_add_object(game->hud_scene, game->hud_minimap_enemies_object);
	se_scene_2d_add_object(game->hud_scene, game->hud_minimap_selected_object);

	const s_mat3 hidden = rts_make_ui_transform(-5.0f, -5.0f, 0.0001f, 0.0001f);
	for (i32 i = 0; i < RTS_MINIMAP_ALLY_CAP; ++i) {
		game->hud_minimap_allies_ids[i] = se_object_2d_add_instance(game->hud_minimap_allies_object, &hidden, &s_mat4_identity);
	}
	for (i32 i = 0; i < RTS_MINIMAP_ENEMY_CAP; ++i) {
		game->hud_minimap_enemies_ids[i] = se_object_2d_add_instance(game->hud_minimap_enemies_object, &hidden, &s_mat4_identity);
	}
	for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
		game->hud_minimap_resources_ids[i] = se_object_2d_add_instance(game->hud_minimap_resources_object, &hidden, &s_mat4_identity);
	}
	for (i32 i = 0; i < RTS_MAX_SELECTABLE_UNITS; ++i) {
		game->hud_minimap_selected_ids[i] = se_object_2d_add_instance(game->hud_minimap_selected_object, &hidden, &s_mat4_identity);
	}
	return true;
}

static void rts_update_minimap_ui(rts_game *game) {
	if (game && game->autotest.enabled) {
		game->autotest.saw_minimap_update = true;
		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_MINIMAP);
	}
	if (!game || game->headless_mode || game->hud_scene == S_HANDLE_NULL) {
		return;
	}

	rts_hide_minimap_instances(game->hud_minimap_allies_object, game->hud_minimap_allies_ids, RTS_MINIMAP_ALLY_CAP);
	rts_hide_minimap_instances(game->hud_minimap_enemies_object, game->hud_minimap_enemies_ids, RTS_MINIMAP_ENEMY_CAP);
	rts_hide_minimap_instances(game->hud_minimap_resources_object, game->hud_minimap_resources_ids, RTS_MAX_RESOURCES);
	rts_hide_minimap_instances(game->hud_minimap_selected_object, game->hud_minimap_selected_ids, RTS_MAX_SELECTABLE_UNITS);

	i32 allies_index = 0;
	i32 enemies_index = 0;
	i32 resources_index = 0;
	i32 selected_index = 0;

	for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
		const rts_resource_node *resource = &game->resources[i];
		if (!resource->active || resources_index >= RTS_MAX_RESOURCES) {
			continue;
		}
		s_vec2 ui = {0};
		if (!rts_world_to_minimap_ui(&resource->position, &ui)) {
			continue;
		}
		rts_set_minimap_instance(game->hud_minimap_resources_object, game->hud_minimap_resources_ids[resources_index++], ui.x, ui.y, 0.010f);
	}

	for (i32 kind = 0; kind < RTS_BUILDING_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
			const rts_building *building = &game->buildings[kind][i];
			if (!building->active) {
				continue;
			}
			s_vec2 ui = {0};
			if (!rts_world_to_minimap_ui(&building->position, &ui)) {
				continue;
			}
			if (building->team == RTS_TEAM_ALLY) {
				if (allies_index < RTS_MINIMAP_ALLY_CAP) {
					rts_set_minimap_instance(game->hud_minimap_allies_object, game->hud_minimap_allies_ids[allies_index++], ui.x, ui.y, 0.018f);
				}
			} else {
				if (enemies_index < RTS_MINIMAP_ENEMY_CAP) {
					rts_set_minimap_instance(game->hud_minimap_enemies_object, game->hud_minimap_enemies_ids[enemies_index++], ui.x, ui.y, 0.018f);
				}
			}
		}
	}

	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			const rts_unit *unit = &game->units[kind][i];
			if (!unit->active) {
				continue;
			}
			s_vec2 ui = {0};
			if (!rts_world_to_minimap_ui(&unit->position, &ui)) {
				continue;
			}
			if (unit->team == RTS_TEAM_ALLY) {
				if (allies_index < RTS_MINIMAP_ALLY_CAP) {
					rts_set_minimap_instance(game->hud_minimap_allies_object, game->hud_minimap_allies_ids[allies_index++], ui.x, ui.y, 0.0105f);
				}
				if (unit->selected && selected_index < RTS_MAX_SELECTABLE_UNITS) {
					rts_set_minimap_instance(game->hud_minimap_selected_object, game->hud_minimap_selected_ids[selected_index++], ui.x, ui.y, 0.0155f);
				}
			} else {
				if (enemies_index < RTS_MINIMAP_ENEMY_CAP) {
					rts_set_minimap_instance(game->hud_minimap_enemies_object, game->hud_minimap_enemies_ids[enemies_index++], ui.x, ui.y, 0.0105f);
				}
			}
		}
	}
}

static void rts_reset_slots(rts_game *game) {
	for (i32 i = 0; i < 2; ++i) {
		game->build_preview_instance_ids[i] = -1;
	}
	for (i32 i = 0; i < RTS_MAX_SELECTABLE_UNITS; ++i) {
		game->selection_instance_ids[i] = -1;
	}
	for (i32 i = 0; i < RTS_MINIMAP_ALLY_CAP; ++i) {
		game->hud_minimap_allies_ids[i] = -1;
	}
	for (i32 i = 0; i < RTS_MINIMAP_ENEMY_CAP; ++i) {
		game->hud_minimap_enemies_ids[i] = -1;
	}
	for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
		game->hud_minimap_resources_ids[i] = -1;
	}
	for (i32 i = 0; i < RTS_MAX_SELECTABLE_UNITS; ++i) {
		game->hud_minimap_selected_ids[i] = -1;
	}

	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			rts_unit *unit = &game->units[kind][i];
			memset(unit, 0, sizeof(*unit));
			unit->instance_id = -1;
			unit->target_resource_index = -1;
		}
	}
	for (i32 kind = 0; kind < RTS_BUILDING_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
			rts_building *building = &game->buildings[kind][i];
			memset(building, 0, sizeof(*building));
			building->instance_id = -1;
		}
	}
	for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
		rts_resource_node *resource = &game->resources[i];
		memset(resource, 0, sizeof(*resource));
		resource->instance_id = -1;
	}
}

static void rts_shutdown_runtime(rts_game *game) {
	if (!game) {
		return;
	}

	if (game->text_handle) {
		se_text_handle_destroy(game->text_handle);
		game->text_handle = NULL;
		game->font = S_HANDLE_NULL;
	}

	if (game->hud_scene != S_HANDLE_NULL) {
		se_scene_2d_destroy_full(game->hud_scene, true);
		game->hud_scene = S_HANDLE_NULL;
	}

	if (game->scene != S_HANDLE_NULL) {
		se_scene_3d_destroy_full(game->scene, true, true);
		game->scene = S_HANDLE_NULL;
	}

	game->window = S_HANDLE_NULL;

	if (game->ctx) {
		se_context_destroy(game->ctx);
		game->ctx = NULL;
	}
}

static b8 rts_init_rendering(rts_game *game) {
	if (!game || !game->ctx) {
		return false;
	}

	game->scene = se_scene_3d_create_for_window(game->window, 128);
	if (game->scene == S_HANDLE_NULL) {
		rts_log("failed to create 3D scene");
		return false;
	}

	se_scene_3d_set_culling(game->scene, true);

	const s_vec3 ground_color = s_vec3(0.18f, 0.25f, 0.19f);
	game->ground_model = rts_load_colored_model(game, SE_RESOURCE_PUBLIC("models/cube.obj"), &ground_color, 0.40f, 0.02f, 0.00f);
	if (game->ground_model == S_HANDLE_NULL) {
		return false;
	}
	game->ground_object = rts_create_scene_object(game, game->ground_model, 1);
	if (game->ground_object == S_HANDLE_NULL) {
		return false;
	}

	const s_vec3 ground_tile_colors[2] = {
		s_vec3(0.16f, 0.21f, 0.16f),
		s_vec3(0.12f, 0.17f, 0.13f)
	};
	for (i32 i = 0; i < 2; ++i) {
		game->ground_tile_models[i] = rts_load_colored_model(
			game,
			SE_RESOURCE_PUBLIC("models/cube.obj"),
			&ground_tile_colors[i],
			0.65f,
			0.02f,
			0.00f);
		if (game->ground_tile_models[i] == S_HANDLE_NULL) {
			return false;
		}
		game->ground_tile_objects[i] = rts_create_scene_object(game, game->ground_tile_models[i], RTS_GROUND_TILE_COUNT);
		if (game->ground_tile_objects[i] == S_HANDLE_NULL) {
			return false;
		}
	}

	const s_vec3 unit_colors[RTS_UNIT_KIND_COUNT] = {
		s_vec3(0.20f, 0.88f, 0.98f),
		s_vec3(0.15f, 0.47f, 1.00f),
		s_vec3(1.00f, 0.56f, 0.28f),
		s_vec3(1.00f, 0.19f, 0.12f)
	};
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		game->unit_models[kind] = rts_load_colored_model(
			game,
			SE_RESOURCE_PUBLIC("models/sphere.obj"),
			&unit_colors[kind],
			0.13f,
			0.35f,
			0.17f);
		if (game->unit_models[kind] == S_HANDLE_NULL) {
			return false;
		}
		game->unit_objects[kind] = rts_create_scene_object(game, game->unit_models[kind], RTS_MAX_UNITS_PER_KIND);
		if (game->unit_objects[kind] == S_HANDLE_NULL) {
			return false;
		}
	}

	const s_vec3 building_colors[RTS_BUILDING_KIND_COUNT] = {
		s_vec3(0.20f, 0.57f, 0.95f),
		s_vec3(0.16f, 0.78f, 0.90f),
		s_vec3(0.64f, 0.92f, 1.00f),
		s_vec3(0.95f, 0.26f, 0.20f),
		s_vec3(0.95f, 0.43f, 0.16f),
		s_vec3(1.00f, 0.65f, 0.34f)
	};
	for (i32 kind = 0; kind < RTS_BUILDING_KIND_COUNT; ++kind) {
		game->building_models[kind] = rts_load_colored_model(
			game,
			SE_RESOURCE_PUBLIC("models/cube.obj"),
			&building_colors[kind],
			0.20f,
			0.10f,
			0.06f);
		if (game->building_models[kind] == S_HANDLE_NULL) {
			return false;
		}
		game->building_objects[kind] = rts_create_scene_object(game, game->building_models[kind], RTS_MAX_BUILDINGS_PER_KIND);
		if (game->building_objects[kind] == S_HANDLE_NULL) {
			return false;
		}
	}

	const s_vec3 resource_color = s_vec3(0.96f, 0.87f, 0.26f);
	game->resource_model = rts_load_colored_model(game, SE_RESOURCE_PUBLIC("models/cube.obj"), &resource_color, 0.10f, 0.12f, 0.34f);
	if (game->resource_model == S_HANDLE_NULL) {
		return false;
	}
	game->resource_object = rts_create_scene_object(game, game->resource_model, RTS_MAX_RESOURCES);
	if (game->resource_object == S_HANDLE_NULL) {
		return false;
	}

	const s_vec3 preview_colors[2] = {
		s_vec3(0.16f, 0.94f, 0.48f),
		s_vec3(0.97f, 0.19f, 0.14f)
	};
	for (i32 i = 0; i < 2; ++i) {
		game->build_preview_models[i] = rts_load_colored_model(
			game,
			SE_RESOURCE_PUBLIC("models/cube.obj"),
			&preview_colors[i],
			0.20f,
			0.03f,
			0.08f);
		if (game->build_preview_models[i] == S_HANDLE_NULL) {
			return false;
		}
		game->build_preview_objects[i] = rts_create_scene_object(game, game->build_preview_models[i], 1);
		if (game->build_preview_objects[i] == S_HANDLE_NULL) {
			return false;
		}
		game->build_preview_instance_ids[i] = se_object_3d_add_instance(game->build_preview_objects[i], &s_mat4_identity, &s_mat4_identity);
		if (game->build_preview_instance_ids[i] == -1) {
			return false;
		}
		const s_mat4 hidden = rts_hidden_transform();
		se_object_3d_set_instance_transform(game->build_preview_objects[i], game->build_preview_instance_ids[i], &hidden);
	}

	const s_vec3 selection_color = s_vec3(1.00f, 0.97f, 0.42f);
	game->selection_model = rts_load_colored_model(game, SE_RESOURCE_PUBLIC("models/cube.obj"), &selection_color, 0.48f, 0.08f, 0.20f);
	if (game->selection_model == S_HANDLE_NULL) {
		return false;
	}
	game->selection_object = rts_create_scene_object(game, game->selection_model, RTS_MAX_SELECTABLE_UNITS);
	if (game->selection_object == S_HANDLE_NULL) {
		return false;
	}
	for (i32 i = 0; i < RTS_MAX_SELECTABLE_UNITS; ++i) {
		const s_mat4 hidden = rts_hidden_transform();
		game->selection_instance_ids[i] = se_object_3d_add_instance(game->selection_object, &hidden, &s_mat4_identity);
		if (game->selection_instance_ids[i] == -1) {
			return false;
		}
	}

	const s_vec3 ground_scale = s_vec3(RTS_MAP_HALF_SIZE, 0.02f, RTS_MAP_HALF_SIZE);
	const s_vec3 ground_pos = s_vec3(0.0f, 0.02f, 0.0f);
	const s_mat4 ground_transform = rts_make_transform(&ground_pos, &ground_scale, 0.0f);
	(void)se_object_3d_add_instance(game->ground_object, &ground_transform, &s_mat4_identity);

	const f32 tile_world_size = (RTS_MAP_HALF_SIZE * 2.0f) / (f32)RTS_GROUND_TILE_GRID;
	const s_vec3 tile_scale = s_vec3(tile_world_size * 0.5f, 0.01f, tile_world_size * 0.5f);
	for (i32 row = 0; row < RTS_GROUND_TILE_GRID; ++row) {
		for (i32 col = 0; col < RTS_GROUND_TILE_GRID; ++col) {
			const i32 color_index = (row + col) & 1;
			s_vec3 tile_pos = s_vec3(
				-RTS_MAP_HALF_SIZE + tile_world_size * ((f32)col + 0.5f),
				tile_scale.y,
				-RTS_MAP_HALF_SIZE + tile_world_size * ((f32)row + 0.5f));
			const s_mat4 tile_transform = rts_make_transform(&tile_pos, &tile_scale, 0.0f);
			(void)se_object_3d_add_instance(game->ground_tile_objects[color_index], &tile_transform, &s_mat4_identity);
		}
	}

	return true;
}

static rts_unit *rts_spawn_unit(rts_game *game, const rts_unit_kind kind, const s_vec3 *spawn_position) {
	if (!game || !spawn_position) {
		return NULL;
	}
	se_object_3d_handle object = game->unit_objects[kind];
	if (object == S_HANDLE_NULL && !game->headless_mode) {
		return NULL;
	}

	rts_unit *slot = NULL;
	for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
		rts_unit *candidate = &game->units[kind][i];
		if (!candidate->active && candidate->instance_id != -1) {
			slot = candidate;
			break;
		}
	}
	if (!slot) {
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			rts_unit *candidate = &game->units[kind][i];
			if (!candidate->active && candidate->instance_id == -1) {
				slot = candidate;
				break;
			}
		}
	}
	if (!slot) {
		rts_log("spawn blocked for unit kind %d (capacity)", kind);
		return NULL;
	}

	const rts_unit_role role = rts_unit_kind_role(kind);
	const rts_team team = rts_unit_kind_team(kind);
	s_vec3 position = *spawn_position;
	rts_clamp_world_position(&position);

	slot->active = true;
	slot->selected = false;
	slot->attack_move = false;
	slot->team = team;
	slot->role = role;
	slot->state = RTS_UNIT_STATE_IDLE;
	slot->position = position;
	slot->target_position = position;
	slot->facing_yaw = rts_randf(0.0f, PI * 2.0f);
	slot->target_resource_index = -1;
	slot->gather_timer = 0.0f;
	slot->carry = 0.0f;
	slot->attack_timer = 0.0f;

	if (role == RTS_UNIT_ROLE_WORKER) {
		slot->max_health = 56.0f;
		slot->speed = 6.1f;
		slot->attack_range = 1.2f;
		slot->attack_damage = 5.0f;
		slot->attack_cooldown = 1.1f;
		slot->gather_rate = 22.0f;
		slot->carry_capacity = 105.0f;
		slot->radius = 0.62f;
	} else {
		slot->max_health = 118.0f;
		slot->speed = 4.5f;
		slot->attack_range = 2.0f;
		slot->attack_damage = 13.0f;
		slot->attack_cooldown = 0.72f;
		slot->gather_rate = 0.0f;
		slot->carry_capacity = 0.0f;
		slot->radius = 0.78f;
	}
	slot->health = slot->max_health;
	slot->serial = ++game->serial_counter;

	const s_vec3 scale = rts_unit_scale(role, false);
	s_vec3 render_position = position;
	render_position.y = scale.y + 0.03f;
	const s_mat4 transform = rts_make_transform(&render_position, &scale, slot->facing_yaw);
	if (!game->headless_mode) {
		if (slot->instance_id == -1) {
			slot->instance_id = se_object_3d_add_instance(object, &transform, &s_mat4_identity);
		} else {
			se_object_3d_set_instance_transform(object, slot->instance_id, &transform);
			(void)se_object_3d_set_instance_active(object, slot->instance_id, true);
		}
		if (slot->instance_id == -1) {
			slot->active = false;
			return NULL;
		}
	}
	return slot;
}

static rts_building *rts_spawn_building(rts_game *game, const rts_building_kind kind, const s_vec3 *spawn_position) {
	if (!game || !spawn_position) {
		return NULL;
	}
	se_object_3d_handle object = game->building_objects[kind];
	if (object == S_HANDLE_NULL && !game->headless_mode) {
		return NULL;
	}

	rts_building *slot = NULL;
	for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
		rts_building *candidate = &game->buildings[kind][i];
		if (!candidate->active && candidate->instance_id != -1) {
			slot = candidate;
			break;
		}
	}
	if (!slot) {
		for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
			rts_building *candidate = &game->buildings[kind][i];
			if (!candidate->active && candidate->instance_id == -1) {
				slot = candidate;
				break;
			}
		}
	}
	if (!slot) {
		rts_log("spawn blocked for building kind %d (capacity)", kind);
		return NULL;
	}

	const rts_building_type type = rts_building_kind_type(kind);
	s_vec3 position = *spawn_position;
	rts_clamp_world_position(&position);

	slot->active = true;
	slot->team = rts_building_kind_team(kind);
	slot->type = type;
	slot->position = position;
	slot->radius = rts_building_radius(type);
	slot->spawn_timer = 0.0f;
	slot->attack_timer = 0.0f;
	slot->serial = ++game->serial_counter;

	switch (type) {
		case RTS_BUILDING_TYPE_HQ: slot->max_health = 1200.0f; break;
		case RTS_BUILDING_TYPE_BARRACKS: slot->max_health = 760.0f; break;
		case RTS_BUILDING_TYPE_TOWER: slot->max_health = 520.0f; break;
		default: slot->max_health = 300.0f; break;
	}
	slot->health = slot->max_health;

	const s_vec3 scale = rts_building_scale(type);
	s_vec3 render_position = position;
	render_position.y = scale.y;
	const s_mat4 transform = rts_make_transform(&render_position, &scale, 0.0f);
	if (!game->headless_mode) {
		if (slot->instance_id == -1) {
			slot->instance_id = se_object_3d_add_instance(object, &transform, &s_mat4_identity);
		} else {
			se_object_3d_set_instance_transform(object, slot->instance_id, &transform);
			(void)se_object_3d_set_instance_active(object, slot->instance_id, true);
		}
		if (slot->instance_id == -1) {
			slot->active = false;
			return NULL;
		}
	}
	return slot;
}

static rts_resource_node *rts_spawn_resource(rts_game *game, const s_vec3 *spawn_position, const f32 amount) {
	if (!game || !spawn_position) {
		return NULL;
	}

	rts_resource_node *slot = NULL;
	for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
		rts_resource_node *candidate = &game->resources[i];
		if (!candidate->active && candidate->instance_id != -1) {
			slot = candidate;
			break;
		}
	}
	if (!slot) {
		for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
			rts_resource_node *candidate = &game->resources[i];
			if (!candidate->active && candidate->instance_id == -1) {
				slot = candidate;
				break;
			}
		}
	}
	if (!slot) {
		rts_log("resource spawn blocked (capacity)");
		return NULL;
	}

	s_vec3 position = *spawn_position;
	rts_clamp_world_position(&position);
	slot->active = true;
	slot->position = position;
	slot->amount = amount;
	slot->max_amount = amount;
	slot->radius = 1.35f;
	slot->serial = ++game->serial_counter;

	const s_vec3 scale = s_vec3(0.78f, 1.30f, 0.78f);
	s_vec3 render_position = position;
	render_position.y = scale.y;
	const s_mat4 transform = rts_make_transform(&render_position, &scale, 0.0f);
	if (!game->headless_mode) {
		if (slot->instance_id == -1) {
			slot->instance_id = se_object_3d_add_instance(game->resource_object, &transform, &s_mat4_identity);
		} else {
			se_object_3d_set_instance_transform(game->resource_object, slot->instance_id, &transform);
			(void)se_object_3d_set_instance_active(game->resource_object, slot->instance_id, true);
		}
		if (slot->instance_id == -1) {
			slot->active = false;
			return NULL;
		}
	}
	return slot;
}

static i32 rts_count_units(const rts_game *game, const rts_team team, const rts_unit_role role_filter) {
	i32 count = 0;
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		if (rts_unit_kind_team((rts_unit_kind)kind) != team) {
			continue;
		}
		if (role_filter != (rts_unit_role)-1 && rts_unit_kind_role((rts_unit_kind)kind) != role_filter) {
			continue;
		}
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			if (game->units[kind][i].active) {
				count++;
			}
		}
	}
	return count;
}

static i32 rts_count_buildings(const rts_game *game, const rts_team team, const rts_building_type type) {
	if (!game) {
		return 0;
	}
	const rts_building_kind kind = rts_make_building_kind(team, type);
	i32 count = 0;
	for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
		if (game->buildings[kind][i].active) {
			count++;
		}
	}
	return count;
}

static rts_building *rts_find_primary_building(rts_game *game, const rts_team team, const rts_building_type type) {
	const rts_building_kind kind = rts_make_building_kind(team, type);
	for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
		rts_building *building = &game->buildings[kind][i];
		if (building->active) {
			return building;
		}
	}
	return NULL;
}

static i32 rts_find_nearest_resource_index(const rts_game *game, const s_vec3 *position) {
	f32 best_distance = 1e30f;
	i32 best_index = -1;
	for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
		const rts_resource_node *resource = &game->resources[i];
		if (!resource->active || resource->amount <= 0.0f) {
			continue;
		}
		const f32 distance = rts_distance_xz(position, &resource->position);
		if (distance < best_distance) {
			best_distance = distance;
			best_index = i;
		}
	}
	return best_index;
}

static const c8 *rts_autotest_system_name(const rts_autotest_system system) {
	switch (system) {
		case RTS_AUTOTEST_SYSTEM_SIMULATOR: return "simulator";
		case RTS_AUTOTEST_SYSTEM_CAMERA: return "camera";
		case RTS_AUTOTEST_SYSTEM_PLAYER_INPUT: return "player_input";
		case RTS_AUTOTEST_SYSTEM_ENEMY_AI: return "enemy_ai";
		case RTS_AUTOTEST_SYSTEM_UNITS: return "units";
		case RTS_AUTOTEST_SYSTEM_NAVIGATION: return "navigation";
		case RTS_AUTOTEST_SYSTEM_BUILDINGS: return "buildings";
		case RTS_AUTOTEST_SYSTEM_SELECTION: return "selection";
		case RTS_AUTOTEST_SYSTEM_VISUALS: return "visuals";
		case RTS_AUTOTEST_SYSTEM_BUILD_PREVIEW: return "build_preview";
		case RTS_AUTOTEST_SYSTEM_MINIMAP: return "minimap";
		case RTS_AUTOTEST_SYSTEM_RENDER: return "render";
		case RTS_AUTOTEST_SYSTEM_VALIDATE: return "validate";
		default: return "unknown";
	}
}

static void rts_autotest_mark_system(rts_game *game, const rts_autotest_system system) {
	if (!game || !game->autotest.enabled) {
		return;
	}
	if (system < 0 || system >= RTS_AUTOTEST_SYSTEM_COUNT) {
		return;
	}
	game->autotest.system_call_count[system]++;
}

static void rts_autotest_fail(rts_game *game, const c8 *fmt, ...) {
	if (!game || !game->autotest.enabled || !fmt) {
		return;
	}
	game->autotest.failures++;
	game->validations_failed = true;

	va_list args;
	va_start(args, fmt);
	vsnprintf(game->autotest.failure_reason, sizeof(game->autotest.failure_reason), fmt, args);
	va_end(args);

	rts_log("autotest failed: %s", game->autotest.failure_reason);
	if (game->autotest.strict_fail_fast) {
		if (game->window != S_HANDLE_NULL && !game->headless_mode) {
			se_window_set_should_close(game->window, true);
		}
		game->simulator.enabled = false;
	}
}

static void rts_autotest_capture_baseline(rts_game *game) {
	if (!game || !game->autotest.enabled) {
		return;
	}
	rts_autotest_state *autotest = &game->autotest;
	autotest->initial_ally_workers = rts_count_units(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_WORKER);
	autotest->initial_ally_soldiers = rts_count_units(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_SOLDIER);
	autotest->initial_enemy_workers = rts_count_units(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_WORKER);
	autotest->initial_enemy_soldiers = rts_count_units(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_SOLDIER);
	autotest->initial_ally_resources = game->team_resources[RTS_TEAM_ALLY];
	autotest->initial_enemy_resources = game->team_resources[RTS_TEAM_ENEMY];
	autotest->max_ally_resources = autotest->initial_ally_resources;
	autotest->initial_camera_x = game->camera_target.x;
	autotest->initial_camera_z = game->camera_target.z;
	autotest->initial_camera_yaw = game->camera_yaw;
	autotest->initial_camera_pitch = game->camera_pitch;
	autotest->initial_camera_distance = game->camera_distance;

	const rts_building *enemy_hq = rts_find_primary_building(game, RTS_TEAM_ENEMY, RTS_BUILDING_TYPE_HQ);
	const rts_building *ally_hq = rts_find_primary_building(game, RTS_TEAM_ALLY, RTS_BUILDING_TYPE_HQ);
	autotest->min_enemy_hq_hp = enemy_hq ? enemy_hq->health : 0.0f;
	autotest->min_ally_hq_hp = ally_hq ? ally_hq->health : 0.0f;

	autotest->min_resource_amount = 1e30f;
	for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
		const rts_resource_node *resource = &game->resources[i];
		if (resource->max_amount <= 0.0f) {
			continue;
		}
		autotest->min_resource_amount = s_min(autotest->min_resource_amount, resource->amount);
	}
	if (autotest->min_resource_amount > 9e29f) {
		autotest->min_resource_amount = 0.0f;
	}
	autotest->initial_min_resource_amount = autotest->min_resource_amount;
}

static void rts_autotest_sample_state(rts_game *game) {
	if (!game || !game->autotest.enabled) {
		return;
	}
	rts_autotest_state *autotest = &game->autotest;
	autotest->max_ally_resources = s_max(autotest->max_ally_resources, game->team_resources[RTS_TEAM_ALLY]);

	const rts_building *enemy_hq = rts_find_primary_building(game, RTS_TEAM_ENEMY, RTS_BUILDING_TYPE_HQ);
	const rts_building *ally_hq = rts_find_primary_building(game, RTS_TEAM_ALLY, RTS_BUILDING_TYPE_HQ);
	if (enemy_hq) {
		autotest->min_enemy_hq_hp = s_min(autotest->min_enemy_hq_hp, enemy_hq->health);
		if (enemy_hq->health < enemy_hq->max_health - 0.1f) {
			autotest->saw_building_damage = true;
		}
	}
	if (ally_hq) {
		autotest->min_ally_hq_hp = s_min(autotest->min_ally_hq_hp, ally_hq->health);
		if (ally_hq->health < ally_hq->max_health - 0.1f) {
			autotest->saw_building_damage = true;
		}
	}

	for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
		const rts_resource_node *resource = &game->resources[i];
		if (resource->max_amount <= 0.0f) {
			continue;
		}
		autotest->min_resource_amount = s_min(autotest->min_resource_amount, resource->amount);
	}

	const i32 ally_workers = rts_count_units(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_WORKER);
	const i32 ally_soldiers = rts_count_units(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_SOLDIER);
	const i32 enemy_workers = rts_count_units(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_WORKER);
	const i32 enemy_soldiers = rts_count_units(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_SOLDIER);
	if (ally_workers > autotest->initial_ally_workers) {
		autotest->saw_worker_train_success = true;
	}
	if (ally_soldiers > autotest->initial_ally_soldiers) {
		autotest->saw_soldier_train_success = true;
	}
	if (enemy_workers > autotest->initial_enemy_workers || enemy_soldiers > autotest->initial_enemy_soldiers) {
		autotest->saw_enemy_train_success = true;
	}

	const f32 camera_target_delta =
		fabsf(game->camera_target.x - autotest->initial_camera_x) +
		fabsf(game->camera_target.z - autotest->initial_camera_z);
	const f32 camera_orbit_delta =
		fabsf(game->camera_yaw - autotest->initial_camera_yaw) +
		fabsf(game->camera_pitch - autotest->initial_camera_pitch) +
		fabsf(game->camera_distance - autotest->initial_camera_distance);
	if (camera_target_delta > 0.35f || camera_orbit_delta > 0.30f) {
		autotest->saw_camera_motion = true;
	}
}

static void rts_autotest_checkpoint(rts_game *game, const u32 checkpoint_index, const f32 deadline, const b8 condition, const c8 *label) {
	if (!game || !game->autotest.enabled) {
		return;
	}
	if (checkpoint_index >= 63) {
		return;
	}
	rts_autotest_state *autotest = &game->autotest;
	const u64 mask = 1ull << checkpoint_index;
	if ((autotest->checkpoints_mask & mask) != 0ull) {
		return;
	}
	if (game->simulator.total_time < deadline) {
		return;
	}
	autotest->checkpoints_mask |= mask;
	if (!condition) {
		rts_autotest_fail(game, "checkpoint '%s' not met by %.2fs", label ? label : "unknown", deadline);
		return;
	}
	autotest->checkpoints_passed++;
	rts_log("autotest checkpoint ok: %s @%.2fs", label ? label : "unknown", game->simulator.total_time);
}

static void rts_autotest_check_progress(rts_game *game) {
	if (!game || !game->autotest.enabled) {
		return;
	}
	rts_autotest_sample_state(game);

	rts_autotest_checkpoint(game, 0, 0.90f, game->autotest.saw_worker_select, "worker select");
	rts_autotest_checkpoint(game, 1, 2.00f, game->autotest.saw_gather_order, "gather order");
	rts_autotest_checkpoint(game, 2, 3.60f, game->autotest.saw_build_attempt, "build attempt");
	rts_autotest_checkpoint(game, 3, 5.10f, game->autotest.saw_worker_train_attempt, "worker train attempt");
	rts_autotest_checkpoint(game, 4, 7.20f, game->autotest.saw_soldier_train_attempt, "soldier train attempt");
	rts_autotest_checkpoint(game, 5, 8.40f, game->autotest.saw_attack_order, "attack order");
	rts_autotest_checkpoint(game, 6, 10.5f, game->autotest.saw_build_success, "build success");
	rts_autotest_checkpoint(game, 7, 12.0f, game->autotest.saw_resource_harvest, "resource harvest");
	rts_autotest_checkpoint(game, 8, 14.0f, game->autotest.saw_resource_deposit, "resource deposit");
	rts_autotest_checkpoint(game, 9, 11.5f, game->autotest.saw_enemy_wave_command, "enemy wave command");
	rts_autotest_checkpoint(game, 10, 20.0f, game->autotest.saw_unit_damage || game->autotest.saw_building_damage, "combat damage");
	rts_autotest_checkpoint(game, 11, 16.0f, game->autotest.saw_validation_pass, "state validation pass");
	rts_autotest_checkpoint(
		game,
		12,
		16.0f,
		game->autotest.saw_worker_train_success && game->autotest.saw_soldier_train_success && game->autotest.saw_enemy_train_success,
		"training growth");

	if (game->autotest.full_systems_required) {
		rts_autotest_checkpoint(game, 13, 6.0f, game->autotest.saw_camera_motion, "camera movement");
		rts_autotest_checkpoint(game, 14, 4.0f, game->autotest.saw_minimap_update, "minimap updates");
		rts_autotest_checkpoint(game, 15, 4.0f, game->autotest.saw_build_preview_update, "build preview updates");
		rts_autotest_checkpoint(game, 16, 4.0f, game->autotest.saw_text_render, "overlay text render");
	}
}

static void rts_autotest_log_summary(const rts_game *game) {
	if (!game || !game->autotest.enabled) {
		return;
	}
	rts_log(
		"autotest summary: checkpoints=%d failures=%d harvest=%d deposit=%d build=%d worker_train=%d soldier_train=%d enemy_train=%d damage(unit=%d building=%d) deaths(unit=%d building=%d) camera=%d minimap=%d preview=%d text=%d",
		game->autotest.checkpoints_passed,
		game->autotest.failures,
		game->autotest.saw_resource_harvest ? 1 : 0,
		game->autotest.saw_resource_deposit ? 1 : 0,
		game->autotest.saw_build_success ? 1 : 0,
		game->autotest.saw_worker_train_success ? 1 : 0,
		game->autotest.saw_soldier_train_success ? 1 : 0,
		game->autotest.saw_enemy_train_success ? 1 : 0,
		game->autotest.saw_unit_damage ? 1 : 0,
		game->autotest.saw_building_damage ? 1 : 0,
		game->autotest.saw_unit_death ? 1 : 0,
		game->autotest.saw_building_death ? 1 : 0,
		game->autotest.saw_camera_motion ? 1 : 0,
		game->autotest.saw_minimap_update ? 1 : 0,
		game->autotest.saw_build_preview_update ? 1 : 0,
		game->autotest.saw_text_render ? 1 : 0);

	c8 system_buf[768] = {0};
	sz offset = 0;
	for (i32 i = 0; i < RTS_AUTOTEST_SYSTEM_COUNT; ++i) {
		const c8 *name = rts_autotest_system_name((rts_autotest_system)i);
		const i32 calls = game->autotest.system_call_count[i];
		const i32 wrote = snprintf(system_buf + offset, sizeof(system_buf) - offset, "%s%s=%d", (i == 0) ? "" : " ", name, calls);
		if (wrote <= 0 || (sz)wrote >= sizeof(system_buf) - offset) {
			break;
		}
		offset += (sz)wrote;
	}
	rts_log("autotest systems: %s", system_buf[0] ? system_buf : "n/a");
}

static void rts_autotest_finalize(rts_game *game) {
	if (!game || !game->autotest.enabled) {
		return;
	}
	if (game->autotest.failures > 0) {
		rts_autotest_log_summary(game);
		return;
	}
	const f32 stop_after = game->simulator.stop_after_seconds;
	const b8 expect_move = stop_after >= 8.0f;
	const b8 expect_attack = stop_after >= 8.2f;
	const b8 expect_training = stop_after >= 11.5f;
	const b8 expect_economy = stop_after >= 13.5f;
	const b8 expect_combat = stop_after >= 19.5f;
	const b8 expect_validation = stop_after >= 15.5f;

	rts_autotest_check_progress(game);
	for (i32 i = 0; i < RTS_AUTOTEST_SYSTEM_COUNT; ++i) {
		if (game->autotest.system_call_count[i] <= 0) {
			const b8 optional_system =
				(i == RTS_AUTOTEST_SYSTEM_MINIMAP || i == RTS_AUTOTEST_SYSTEM_RENDER) && !game->autotest.full_systems_required;
			if (!optional_system) {
				rts_autotest_fail(game, "system '%s' was never executed", rts_autotest_system_name((rts_autotest_system)i));
				break;
			}
		}
	}

	if (expect_move && !game->autotest.saw_move_order) {
		rts_autotest_fail(game, "move order path was not exercised");
	}
	if (expect_attack && !game->autotest.saw_attack_order) {
		rts_autotest_fail(game, "attack order path was not exercised");
	}
	if (expect_economy && game->autotest.min_resource_amount >= game->autotest.initial_min_resource_amount - 0.01f && !game->autotest.saw_resource_harvest) {
		rts_autotest_fail(game, "resources were not consumed from any node");
	}
	if (expect_training && (!game->autotest.saw_worker_train_success || !game->autotest.saw_soldier_train_success || !game->autotest.saw_enemy_train_success)) {
		rts_autotest_fail(game, "unit training coverage incomplete");
	}
	if (expect_combat && !game->autotest.saw_unit_damage && !game->autotest.saw_building_damage) {
		rts_autotest_fail(game, "no combat damage observed");
	}
	if (expect_validation && !game->autotest.saw_validation_pass) {
		rts_autotest_fail(game, "state validator never passed");
	}
	if (game->autotest.full_systems_required &&
		(!game->autotest.saw_camera_motion || !game->autotest.saw_minimap_update || !game->autotest.saw_build_preview_update || !game->autotest.saw_text_render)) {
		rts_autotest_fail(game, "window-system coverage incomplete (camera/minimap/preview/text)");
	}

	rts_autotest_log_summary(game);
}

static rts_unit_target rts_find_nearest_enemy_unit(rts_game *game, const rts_team team, const s_vec3 *from, const f32 max_distance) {
	rts_unit_target target = {0};
	target.distance = max_distance;
	target.index = -1;
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		if (rts_unit_kind_team((rts_unit_kind)kind) == team) {
			continue;
		}
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			rts_unit *unit = &game->units[kind][i];
			if (!unit->active) {
				continue;
			}
			const f32 distance = rts_distance_xz(from, &unit->position);
			if (distance < target.distance) {
				target.unit = unit;
				target.kind = (rts_unit_kind)kind;
				target.index = i;
				target.distance = distance;
			}
		}
	}
	return target;
}

static rts_building_target rts_find_nearest_enemy_building(rts_game *game, const rts_team team, const s_vec3 *from, const f32 max_distance) {
	rts_building_target target = {0};
	target.distance = max_distance;
	target.index = -1;
	for (i32 kind = 0; kind < RTS_BUILDING_KIND_COUNT; ++kind) {
		if (rts_building_kind_team((rts_building_kind)kind) == team) {
			continue;
		}
		for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
			rts_building *building = &game->buildings[kind][i];
			if (!building->active) {
				continue;
			}
			const f32 distance = rts_distance_xz(from, &building->position);
			if (distance < target.distance) {
				target.building = building;
				target.kind = (rts_building_kind)kind;
				target.index = i;
				target.distance = distance;
			}
		}
	}
	return target;
}

static b8 rts_move_towards(rts_unit *unit, const s_vec3 *target, const f32 dt) {
	if (!unit || !target) {
		return true;
	}
	s_vec3 delta = s_vec3_sub(target, &unit->position);
	delta.y = 0.0f;
	const f32 distance = s_vec3_length(&delta);
	if (distance < 0.05f) {
		unit->position.x = target->x;
		unit->position.z = target->z;
		return true;
	}
	s_vec3 direction = s_vec3_divs(&delta, distance);
	const f32 step = s_min(unit->speed * dt, distance);
	unit->position.x += direction.x * step;
	unit->position.z += direction.z * step;
	rts_clamp_world_position(&unit->position);
	unit->facing_yaw = atan2f(direction.x, direction.z);
	return distance <= 0.24f;
}

static void rts_hide_unit(rts_game *game, const rts_unit_kind kind, rts_unit *unit) {
	if (!game || !unit || unit->instance_id == -1) {
		return;
	}
	(void)se_object_3d_set_instance_active(game->unit_objects[kind], unit->instance_id, false);
}

static void rts_hide_building(rts_game *game, const rts_building_kind kind, rts_building *building) {
	if (!game || !building || building->instance_id == -1) {
		return;
	}
	(void)se_object_3d_set_instance_active(game->building_objects[kind], building->instance_id, false);
}

static void rts_hide_resource(rts_game *game, rts_resource_node *resource) {
	if (!game || !resource || resource->instance_id == -1) {
		return;
	}
	(void)se_object_3d_set_instance_active(game->resource_object, resource->instance_id, false);
}

static void rts_damage_unit(rts_game *game, const rts_unit_kind kind, const i32 index, const f32 damage) {
	rts_unit *unit = &game->units[kind][index];
	if (!unit->active) {
		return;
	}
	if (game && game->autotest.enabled && damage > 0.0f) {
		game->autotest.saw_unit_damage = true;
	}
	unit->health -= damage;
	if (unit->health > 0.0f) {
		return;
	}
	if (unit->selected) {
		unit->selected = false;
	}
	unit->active = false;
	unit->carry = 0.0f;
	unit->state = RTS_UNIT_STATE_IDLE;
	if (game && game->autotest.enabled) {
		game->autotest.saw_unit_death = true;
	}
	rts_hide_unit(game, kind, unit);
	rts_log("unit %d removed", unit->serial);
}

static void rts_damage_building(rts_game *game, const rts_building_kind kind, const i32 index, const f32 damage) {
	rts_building *building = &game->buildings[kind][index];
	if (!building->active) {
		return;
	}
	if (game && game->autotest.enabled && damage > 0.0f) {
		game->autotest.saw_building_damage = true;
	}
	building->health -= damage;
	if (building->health > 0.0f) {
		return;
	}
	building->active = false;
	if (game && game->autotest.enabled) {
		game->autotest.saw_building_death = true;
	}
	rts_hide_building(game, kind, building);
	rts_log("building %d destroyed", building->serial);
	if (building->type == RTS_BUILDING_TYPE_HQ) {
		rts_set_command_line(game, "%s HQ lost", building->team == RTS_TEAM_ALLY ? "Ally" : "Enemy");
	}
}

static void rts_clear_selection(rts_game *game) {
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		if (rts_unit_kind_team((rts_unit_kind)kind) != RTS_TEAM_ALLY) {
			continue;
		}
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			game->units[kind][i].selected = false;
		}
	}
	game->selected_count = 0;
}

static void rts_select_group(rts_game *game, const rts_unit_role role, const b8 append) {
	if (!append) {
		rts_clear_selection(game);
	}
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		if (rts_unit_kind_team((rts_unit_kind)kind) != RTS_TEAM_ALLY) {
			continue;
		}
		if (role != (rts_unit_role)-1 && rts_unit_kind_role((rts_unit_kind)kind) != role) {
			continue;
		}
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			rts_unit *unit = &game->units[kind][i];
			if (unit->active) {
				unit->selected = true;
			}
		}
	}
	if (game && game->autotest.enabled) {
		if (role == RTS_UNIT_ROLE_WORKER || role == (rts_unit_role)-1) {
			game->autotest.saw_worker_select = true;
		}
		if (role == RTS_UNIT_ROLE_SOLDIER || role == (rts_unit_role)-1) {
			game->autotest.saw_soldier_select = true;
		}
	}
}

static i32 rts_recount_selection(rts_game *game) {
	i32 count = 0;
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		if (rts_unit_kind_team((rts_unit_kind)kind) != RTS_TEAM_ALLY) {
			continue;
		}
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			if (game->units[kind][i].active && game->units[kind][i].selected) {
				count++;
			}
		}
	}
	game->selected_count = count;
	return count;
}

static b8 rts_has_selected_workers(const rts_game *game) {
	for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
		const rts_unit *unit = &game->units[RTS_UNIT_KIND_ALLY_WORKER][i];
		if (unit->active && unit->selected) {
			return true;
		}
	}
	return false;
}

static void rts_select_nearest_at_cursor(rts_game *game, const f32 mouse_x, const f32 mouse_y, const s_vec3 *point, const b8 append) {
	if (!append) {
		rts_clear_selection(game);
	}

	rts_unit *best_unit = NULL;
	if (game && game->scene != S_HANDLE_NULL) {
		f32 viewport_width = 0.0f;
		f32 viewport_height = 0.0f;
		rts_get_input_view_size(game, &viewport_width, &viewport_height);
		if (viewport_width > 1.0f && viewport_height > 1.0f) {
			const rts_pick_filter_state pick_state = { .game = game };
			se_object_3d_handle picked_object = S_HANDLE_NULL;
			if (se_scene_3d_pick_object_screen(
					game->scene,
					mouse_x,
					mouse_y,
					viewport_width,
					viewport_height,
					1.35f,
					rts_pick_filter_ally_unit_objects,
					(void*)&pick_state,
					&picked_object,
					NULL)) {
				const rts_unit_kind picked_kind = rts_pick_unit_kind_from_object(game, picked_object);
				if (picked_kind != (rts_unit_kind)-1) {
					f32 best_distance = 1e30f;
					for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
						rts_unit *unit = &game->units[picked_kind][i];
						if (!unit->active) {
							continue;
						}
						const f32 distance = point ? rts_distance_xz(point, &unit->position) : (f32)i;
						if (distance < best_distance) {
							best_distance = distance;
							best_unit = unit;
						}
					}
				}
			}
		}
	}

	s_vec3 ray_origin = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 ray_dir = s_vec3(0.0f, 0.0f, 0.0f);
	const b8 has_ray = best_unit ? false : rts_screen_to_world_ray(game, mouse_x, mouse_y, &ray_origin, &ray_dir);
	if (!best_unit && has_ray) {
		f32 best_t = 1e30f;
		for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
			if (rts_unit_kind_team((rts_unit_kind)kind) != RTS_TEAM_ALLY) {
				continue;
			}
			for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
				rts_unit *unit = &game->units[kind][i];
				if (!unit->active) {
					continue;
				}
				s_vec3 center = unit->position;
				center.y = rts_unit_scale(unit->role, false).y + 0.10f;
				const f32 radius = unit->radius * 1.25f;
				f32 t = 0.0f;
				if (rts_ray_hits_sphere(&ray_origin, &ray_dir, &center, radius, &t) && t < best_t) {
					best_t = t;
					best_unit = unit;
				}
			}
		}
	}

	if (!best_unit) {
		f32 best_px_distance = 46.0f;
		for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
			if (rts_unit_kind_team((rts_unit_kind)kind) != RTS_TEAM_ALLY) {
				continue;
			}
			for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
				rts_unit *unit = &game->units[kind][i];
				if (!unit->active) {
					continue;
				}
				s_vec3 world = unit->position;
				world.y = rts_unit_scale(unit->role, false).y + 0.20f;
				f32 sx = 0.0f;
				f32 sy = 0.0f;
				if (!rts_world_to_screen(game, &world, &sx, &sy)) {
					continue;
				}
				const f32 dx = sx - mouse_x;
				const f32 dy = sy - mouse_y;
				const f32 distance = sqrtf(dx * dx + dy * dy);
				if (distance < best_px_distance) {
					best_px_distance = distance;
					best_unit = unit;
				}
			}
		}
	}

	if (!best_unit && point) {
		f32 best_world_distance = RTS_SELECTION_RADIUS * 2.25f;
		for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
			if (rts_unit_kind_team((rts_unit_kind)kind) != RTS_TEAM_ALLY) {
				continue;
			}
			for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
				rts_unit *unit = &game->units[kind][i];
				if (!unit->active) {
					continue;
				}
				const f32 distance = rts_distance_xz(point, &unit->position);
				if (distance < best_world_distance) {
					best_world_distance = distance;
					best_unit = unit;
				}
			}
		}
	}

	if (best_unit) {
		best_unit->selected = true;
	}
	rts_recount_selection(game);
}

static void rts_issue_move_selected(rts_game *game, const s_vec3 *target, const b8 attack_move) {
	rts_unit *selected_units[RTS_MAX_UNITS_PER_KIND * 2] = {0};
	i32 selected_count = 0;
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		if (rts_unit_kind_team((rts_unit_kind)kind) != RTS_TEAM_ALLY) {
			continue;
		}
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			rts_unit *unit = &game->units[kind][i];
			if (!unit->active || !unit->selected) {
				continue;
			}
			if (selected_count < (i32)(sizeof(selected_units) / sizeof(selected_units[0]))) {
				selected_units[selected_count++] = unit;
			}
		}
	}

	if (selected_count <= 0) {
		return;
	}
	if (game && game->autotest.enabled) {
		game->autotest.saw_move_order = true;
		if (attack_move) {
			game->autotest.saw_attack_order = true;
		}
	}

	const i32 columns = (i32)ceilf(sqrtf((f32)selected_count));
	const i32 rows = (selected_count + columns - 1) / columns;
	const f32 spacing = 1.85f;
	const f32 center_col = ((f32)columns - 1.0f) * 0.5f;
	const f32 center_row = ((f32)rows - 1.0f) * 0.5f;
	const s_vec3 forward = s_vec3(sinf(game->camera_yaw), 0.0f, cosf(game->camera_yaw));
	const s_vec3 right = s_vec3(cosf(game->camera_yaw), 0.0f, -sinf(game->camera_yaw));

	for (i32 idx = 0; idx < selected_count; ++idx) {
		rts_unit *unit = selected_units[idx];
		if (!unit) {
			continue;
		}
		const i32 col = idx % columns;
		const i32 row = idx / columns;
		const f32 local_x = ((f32)col - center_col) * spacing;
		const f32 local_z = ((f32)row - center_row) * spacing;
		s_vec3 destination = *target;
		destination = s_vec3_add(&destination, &s_vec3_muls(&right, local_x));
		destination = s_vec3_add(&destination, &s_vec3_muls(&forward, local_z));
		rts_clamp_world_position(&destination);

		unit->state = RTS_UNIT_STATE_MOVE;
		unit->attack_move = attack_move;
		unit->target_resource_index = -1;
		unit->target_position = destination;
	}

	for (i32 kind = 0; kind < RTS_BUILDING_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
			const rts_building *building = &game->buildings[kind][i];
			if (!building->active) {
				continue;
			}
			const f32 avoid_radius = building->radius + 0.55f;
			for (i32 idx = 0; idx < selected_count; ++idx) {
				rts_unit *unit = selected_units[idx];
				if (!unit) {
					continue;
				}
				const f32 distance = rts_distance_xz(&unit->target_position, &building->position);
				if (distance < avoid_radius) {
					s_vec3 away = s_vec3_sub(&unit->target_position, &building->position);
					away.y = 0.0f;
					if (s_vec3_length(&away) < 0.001f) {
						away = s_vec3(1.0f, 0.0f, 0.0f);
					}
					away = s_vec3_normalize(&away);
					unit->target_position = s_vec3_add(&building->position, &s_vec3_muls(&away, avoid_radius + 0.2f));
					rts_clamp_world_position(&unit->target_position);
				}
			}
		}
	}
}

static void rts_issue_gather_selected(rts_game *game, const i32 resource_index) {
	if (resource_index < 0 || resource_index >= RTS_MAX_RESOURCES) {
		return;
	}
	b8 issued = false;
	for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
		rts_unit *worker = &game->units[RTS_UNIT_KIND_ALLY_WORKER][i];
		if (!worker->active || !worker->selected) {
			continue;
		}
		worker->state = RTS_UNIT_STATE_GATHER;
		worker->target_resource_index = resource_index;
		worker->attack_move = false;
		issued = true;
	}
	if (issued && game && game->autotest.enabled) {
		game->autotest.saw_gather_order = true;
	}
}

static b8 rts_is_build_position_valid_ex(
	const rts_game *game,
	const rts_team team,
	const rts_building_type type,
	const s_vec3 *position,
	c8 *out_reason,
	const sz out_reason_size) {
	if (out_reason && out_reason_size > 0) {
		out_reason[0] = '\0';
	}
	if (!game || !position) {
		if (out_reason && out_reason_size > 0) {
			snprintf(out_reason, out_reason_size, "Build validation failed");
		}
		return false;
	}
	s_vec3 place = *position;
	place.x = rts_clampf(place.x, -RTS_MAP_HALF_SIZE, RTS_MAP_HALF_SIZE);
	place.z = rts_clampf(place.z, -RTS_MAP_HALF_SIZE, RTS_MAP_HALF_SIZE);
	const f32 new_radius = rts_building_radius(type);
	const f32 edge_margin = new_radius + 0.10f;
	if (fabsf(place.x) > RTS_MAP_HALF_SIZE - edge_margin || fabsf(place.z) > RTS_MAP_HALF_SIZE - edge_margin) {
		if (out_reason && out_reason_size > 0) {
			snprintf(out_reason, out_reason_size, "Out of bounds: move inside map border");
		}
		return false;
	}

	for (i32 kind = 0; kind < RTS_BUILDING_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
			const rts_building *building = &game->buildings[kind][i];
			if (!building->active) {
				continue;
			}
			const f32 pad = (building->team == team) ? 0.08f : 0.18f;
			const f32 min_distance = building->radius + new_radius + pad;
			const f32 distance = rts_distance_xz(&building->position, &place);
			if (distance < min_distance) {
				if (out_reason && out_reason_size > 0) {
					snprintf(
						out_reason,
						out_reason_size,
						"Too close to %s %s (need %.1fm)",
						building->team == team ? "ally" : "enemy",
						rts_building_type_name(building->type),
						min_distance);
				}
				return false;
			}
		}
	}

	for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
		const rts_resource_node *resource = &game->resources[i];
		if (!resource->active) {
			continue;
		}
		const f32 min_distance = resource->radius + new_radius + 0.08f;
		if (rts_distance_xz(&resource->position, &place) < min_distance) {
			if (out_reason && out_reason_size > 0) {
				snprintf(out_reason, out_reason_size, "Too close to resource node");
			}
			return false;
		}
	}
	return true;
}

static b8 rts_try_train_unit(rts_game *game, const rts_team team, const rts_unit_role role, const b8 user_ordered) {
	if (game && game->autotest.enabled && team == RTS_TEAM_ALLY) {
		if (role == RTS_UNIT_ROLE_WORKER) {
			game->autotest.saw_worker_train_attempt = true;
		} else {
			game->autotest.saw_soldier_train_attempt = true;
		}
	}
	const f32 cost = (role == RTS_UNIT_ROLE_WORKER) ? 60.0f : 95.0f;
	if (game->team_resources[team] < cost) {
		if (user_ordered && team == RTS_TEAM_ALLY) {
			rts_set_command_line(game, "Not enough resources for %s", role == RTS_UNIT_ROLE_WORKER ? "worker" : "soldier");
		}
		return false;
	}

	const rts_building_type needed_type = (role == RTS_UNIT_ROLE_WORKER) ? RTS_BUILDING_TYPE_HQ : RTS_BUILDING_TYPE_BARRACKS;
	rts_building *source = NULL;
	const rts_building_kind source_kind = rts_make_building_kind(team, needed_type);
	for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
		rts_building *candidate = &game->buildings[source_kind][i];
		if (candidate->active && candidate->spawn_timer <= 0.0f) {
			source = candidate;
			break;
		}
	}
	if (!source) {
		if (user_ordered && team == RTS_TEAM_ALLY) {
			rts_set_command_line(game, "No ready %s", needed_type == RTS_BUILDING_TYPE_HQ ? "HQ" : "Barracks");
		}
		return false;
	}

	const f32 spawn_angle = rts_randf(0.0f, PI * 2.0f);
	const f32 spawn_distance = source->radius + 1.5f + rts_randf(0.2f, 1.6f);
	s_vec3 spawn_position = source->position;
	spawn_position.x += cosf(spawn_angle) * spawn_distance;
	spawn_position.z += sinf(spawn_angle) * spawn_distance;
	rts_clamp_world_position(&spawn_position);

	const rts_unit_kind unit_kind = rts_make_unit_kind(team, role);
	rts_unit *unit = rts_spawn_unit(game, unit_kind, &spawn_position);
	if (!unit) {
		if (user_ordered && team == RTS_TEAM_ALLY) {
			const i32 current = rts_count_units(game, team, role);
			if (current >= RTS_MAX_UNITS_PER_KIND) {
				rts_set_command_line(game, "%s cap reached", role == RTS_UNIT_ROLE_WORKER ? "Worker" : "Soldier");
			} else {
				rts_set_command_line(game, "Spawn blocked near %s", needed_type == RTS_BUILDING_TYPE_HQ ? "HQ" : "Barracks");
			}
		}
		return false;
	}

	game->team_resources[team] -= cost;
	source->spawn_timer = (role == RTS_UNIT_ROLE_WORKER) ? 1.2f : 2.5f;
	if (game && game->autotest.enabled) {
		if (team == RTS_TEAM_ALLY) {
			if (role == RTS_UNIT_ROLE_WORKER) {
				game->autotest.saw_worker_train_success = true;
			} else {
				game->autotest.saw_soldier_train_success = true;
			}
		} else {
			game->autotest.saw_enemy_train_success = true;
		}
	}
	if (user_ordered && team == RTS_TEAM_ALLY) {
		rts_set_command_line(game, "%s trained", role == RTS_UNIT_ROLE_WORKER ? "Worker" : "Soldier");
	}
	return true;
}

static b8 rts_try_build(rts_game *game, const rts_team team, const rts_building_type type, const s_vec3 *position, const b8 user_ordered) {
	if (game && game->autotest.enabled && team == RTS_TEAM_ALLY) {
		game->autotest.saw_build_attempt = true;
	}
	const f32 cost = (type == RTS_BUILDING_TYPE_BARRACKS) ? 185.0f : 145.0f;
	if (game->team_resources[team] < cost) {
		if (user_ordered && team == RTS_TEAM_ALLY) {
			rts_set_command_line(game, "Need more resources to build");
		}
		return false;
	}

	s_vec3 place = *position;
	rts_clamp_world_position(&place);
	c8 block_reason[160] = {0};
	if (!rts_is_build_position_valid_ex(game, team, type, &place, block_reason, sizeof(block_reason))) {
		if (user_ordered && team == RTS_TEAM_ALLY) {
			rts_set_command_line(game, "%s", block_reason[0] ? block_reason : "Build position blocked");
		}
		return false;
	}

	const rts_building_kind kind = rts_make_building_kind(team, type);
	if (!rts_spawn_building(game, kind, &place)) {
		if (user_ordered && team == RTS_TEAM_ALLY) {
			const i32 current = rts_count_buildings(game, team, type);
			if (current >= RTS_MAX_BUILDINGS_PER_KIND) {
				rts_set_command_line(game, "%s cap reached", rts_building_type_name(type));
			} else {
				rts_set_command_line(game, "Build failed: blocked spawn/slot");
			}
		}
		return false;
	}

	game->team_resources[team] -= cost;
	if (game && game->autotest.enabled && team == RTS_TEAM_ALLY) {
		game->autotest.saw_build_success = true;
	}
	if (user_ordered && team == RTS_TEAM_ALLY) {
		rts_set_command_line(game, "%s started", type == RTS_BUILDING_TYPE_BARRACKS ? "Barracks" : "Tower");
	}
	return true;
}

static void rts_command_team(rts_game *game, const rts_team team, const rts_unit_role role, const s_vec3 *target, const b8 attack_move) {
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		if (rts_unit_kind_team((rts_unit_kind)kind) != team) {
			continue;
		}
		if (rts_unit_kind_role((rts_unit_kind)kind) != role) {
			continue;
		}
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			rts_unit *unit = &game->units[kind][i];
			if (!unit->active) {
				continue;
			}
			unit->state = RTS_UNIT_STATE_MOVE;
			unit->target_position = *target;
			unit->attack_move = attack_move;
			unit->target_resource_index = -1;
		}
	}
}

static void rts_update_enemy_ai(rts_game *game, const f32 dt) {
	game->enemy_strategy_timer += dt;
	game->enemy_wave_timer += dt;
	game->enemy_build_timer += dt;

	const i32 enemy_workers = rts_count_units(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_WORKER);
	const i32 enemy_soldiers = rts_count_units(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_SOLDIER);

	if (game->enemy_strategy_timer >= 2.4f) {
		game->enemy_strategy_timer = 0.0f;
		if (enemy_workers < 5) {
			(void)rts_try_train_unit(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_WORKER, false);
		} else {
			(void)rts_try_train_unit(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_SOLDIER, false);
		}
	}

	if (enemy_workers >= 4 && enemy_soldiers >= 4 && game->enemy_wave_timer >= 6.5f) {
		game->enemy_wave_timer = 0.0f;
		rts_building *ally_hq = rts_find_primary_building(game, RTS_TEAM_ALLY, RTS_BUILDING_TYPE_HQ);
		if (ally_hq) {
			rts_command_team(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_SOLDIER, &ally_hq->position, true);
			if (game->autotest.enabled) {
				game->autotest.saw_enemy_wave_command = true;
			}
		}
	}

	if (game->enemy_build_timer >= 11.0f) {
		game->enemy_build_timer = 0.0f;
		const rts_building *enemy_hq = rts_find_primary_building(game, RTS_TEAM_ENEMY, RTS_BUILDING_TYPE_HQ);
		if (enemy_hq) {
			s_vec3 build_position = enemy_hq->position;
			build_position.x += rts_randf(-5.0f, 5.0f);
			build_position.z += rts_randf(-5.0f, 5.0f);
			(void)rts_try_build(game, RTS_TEAM_ENEMY, RTS_BUILDING_TYPE_TOWER, &build_position, false);
		}
	}

	if (!rts_find_primary_building(game, RTS_TEAM_ENEMY, RTS_BUILDING_TYPE_BARRACKS)) {
		rts_building *enemy_hq = rts_find_primary_building(game, RTS_TEAM_ENEMY, RTS_BUILDING_TYPE_HQ);
		if (enemy_hq) {
			s_vec3 build_position = enemy_hq->position;
			build_position.x += 4.8f;
			build_position.z -= 2.0f;
			(void)rts_try_build(game, RTS_TEAM_ENEMY, RTS_BUILDING_TYPE_BARRACKS, &build_position, false);
		}
	}
}

static void rts_update_unit(rts_game *game, const rts_unit_kind kind, const i32 index, const f32 dt) {
	rts_unit *unit = &game->units[kind][index];
	if (!unit->active) {
		return;
	}

	if (unit->attack_timer > 0.0f) {
		unit->attack_timer -= dt;
	}

	if (unit->role == RTS_UNIT_ROLE_WORKER && unit->state == RTS_UNIT_STATE_IDLE) {
		const i32 nearest_resource = rts_find_nearest_resource_index(game, &unit->position);
		if (nearest_resource >= 0) {
			unit->state = RTS_UNIT_STATE_GATHER;
			unit->target_resource_index = nearest_resource;
		}
	}

	if (unit->state == RTS_UNIT_STATE_GATHER) {
		if (unit->target_resource_index < 0 || unit->target_resource_index >= RTS_MAX_RESOURCES) {
			unit->state = RTS_UNIT_STATE_IDLE;
		} else {
			rts_resource_node *resource = &game->resources[unit->target_resource_index];
			if (!resource->active || resource->amount <= 0.0f) {
				unit->state = RTS_UNIT_STATE_IDLE;
				unit->target_resource_index = -1;
			} else {
				const f32 arrive_distance = resource->radius + unit->radius;
				if (rts_distance_xz(&unit->position, &resource->position) > arrive_distance) {
					(void)rts_move_towards(unit, &resource->position, dt);
				} else {
					unit->gather_timer += dt;
					if (unit->gather_timer >= 0.35f) {
						const f32 gather_amount = unit->gather_rate * unit->gather_timer;
						unit->gather_timer = 0.0f;
						const f32 available = s_max(resource->amount, 0.0f);
						const f32 capacity_left = s_max(unit->carry_capacity - unit->carry, 0.0f);
						const f32 collected = s_min(gather_amount, s_min(available, capacity_left));
						if (collected > 0.0f) {
							unit->carry += collected;
							resource->amount -= collected;
							if (game->autotest.enabled) {
								game->autotest.saw_resource_harvest = true;
							}
						}
						if (resource->amount <= 0.0f) {
							resource->active = false;
							rts_hide_resource(game, resource);
						}
						if (unit->carry >= unit->carry_capacity || !resource->active) {
							unit->state = RTS_UNIT_STATE_RETURN;
						}
					}
				}
			}
		}
	}

	if (unit->state == RTS_UNIT_STATE_RETURN) {
		rts_building *home_hq = rts_find_primary_building(game, unit->team, RTS_BUILDING_TYPE_HQ);
		if (!home_hq) {
			unit->state = RTS_UNIT_STATE_IDLE;
		} else {
			const f32 arrive_distance = home_hq->radius + unit->radius + 0.2f;
			if (rts_distance_xz(&unit->position, &home_hq->position) > arrive_distance) {
				(void)rts_move_towards(unit, &home_hq->position, dt);
			} else {
				game->team_resources[unit->team] += unit->carry;
				if (game->autotest.enabled && unit->carry > 0.0f) {
					game->autotest.saw_resource_deposit = true;
				}
				unit->carry = 0.0f;
				unit->state = RTS_UNIT_STATE_IDLE;
			}
		}
	}

	rts_unit_target enemy_unit = rts_find_nearest_enemy_unit(game, unit->team, &unit->position, (unit->role == RTS_UNIT_ROLE_SOLDIER) ? 10.0f : 4.6f);
	rts_building_target enemy_building = rts_find_nearest_enemy_building(game, unit->team, &unit->position, (unit->role == RTS_UNIT_ROLE_SOLDIER) ? 12.0f : 5.2f);

	const b8 allow_combat = (unit->role == RTS_UNIT_ROLE_SOLDIER) || (unit->team == RTS_TEAM_ENEMY) || unit->attack_move;
	if (allow_combat) {
		b8 attacked = false;
		if (enemy_unit.unit && enemy_unit.distance <= unit->attack_range) {
			if (unit->attack_timer <= 0.0f) {
				rts_damage_unit(game, enemy_unit.kind, enemy_unit.index, unit->attack_damage);
				unit->attack_timer = unit->attack_cooldown;
				attacked = true;
			}
		} else if (enemy_building.building) {
			const f32 attack_distance = enemy_building.building->radius + unit->radius + unit->attack_range;
			if (enemy_building.distance <= attack_distance && unit->attack_timer <= 0.0f) {
				rts_damage_building(game, enemy_building.kind, enemy_building.index, unit->attack_damage * 0.75f);
				unit->attack_timer = unit->attack_cooldown;
				attacked = true;
			}
		}
		if (!attacked && enemy_building.building && enemy_building.distance < enemy_building.building->radius + unit->radius + unit->attack_range + 0.8f) {
			if (unit->attack_timer <= 0.0f) {
				rts_damage_building(game, enemy_building.kind, enemy_building.index, unit->attack_damage * 0.40f);
				unit->attack_timer = unit->attack_cooldown * 1.15f;
				attacked = true; // allows chip damage when body-blocked at structure edge
			}
		}

		if (!attacked) {
			const b8 chase_unit = enemy_unit.unit && (!enemy_building.building || enemy_unit.distance < enemy_building.distance);
			if (chase_unit && enemy_unit.distance < 12.0f) {
				if (unit->state != RTS_UNIT_STATE_GATHER && unit->state != RTS_UNIT_STATE_RETURN) {
					(void)rts_move_towards(unit, &enemy_unit.unit->position, dt);
				}
			} else if (enemy_building.building && enemy_building.distance < 14.0f) {
				if (unit->state != RTS_UNIT_STATE_GATHER && unit->state != RTS_UNIT_STATE_RETURN) {
					(void)rts_move_towards(unit, &enemy_building.building->position, dt);
				}
			}
		}
	}

	if (unit->state == RTS_UNIT_STATE_MOVE) {
		if (rts_move_towards(unit, &unit->target_position, dt)) {
			unit->state = RTS_UNIT_STATE_IDLE;
			unit->attack_move = false;
		}
	}
}

static void rts_update_units(rts_game *game, const f32 dt) {
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			rts_update_unit(game, (rts_unit_kind)kind, i, dt);
		}
	}
}

static void rts_resolve_unit_navigation(rts_game *game, const f32 dt) {
	(void)dt;
	for (i32 kind_a = 0; kind_a < RTS_UNIT_KIND_COUNT; ++kind_a) {
		for (i32 idx_a = 0; idx_a < RTS_MAX_UNITS_PER_KIND; ++idx_a) {
			rts_unit *a = &game->units[kind_a][idx_a];
			if (!a->active) {
				continue;
			}

			for (i32 kind_b = kind_a; kind_b < RTS_UNIT_KIND_COUNT; ++kind_b) {
				const i32 begin_b = (kind_b == kind_a) ? (idx_a + 1) : 0;
				for (i32 idx_b = begin_b; idx_b < RTS_MAX_UNITS_PER_KIND; ++idx_b) {
					rts_unit *b = &game->units[kind_b][idx_b];
					if (!b->active) {
						continue;
					}
					s_vec3 delta = s_vec3_sub(&b->position, &a->position);
					delta.y = 0.0f;
					f32 distance = s_vec3_length(&delta);
					const f32 min_distance = a->radius + b->radius;
					if (distance >= min_distance) {
						continue;
					}
					if (distance < 0.001f) {
						delta = s_vec3(1.0f, 0.0f, 0.0f);
						distance = 1.0f;
					}
					const s_vec3 direction = s_vec3_divs(&delta, distance);
					const f32 push = (min_distance - distance) * 0.48f;
					a->position = s_vec3_sub(&a->position, &s_vec3_muls(&direction, push));
					b->position = s_vec3_add(&b->position, &s_vec3_muls(&direction, push));
					rts_clamp_world_position(&a->position);
					rts_clamp_world_position(&b->position);
				}
			}

			for (i32 kind = 0; kind < RTS_BUILDING_KIND_COUNT; ++kind) {
				for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
					const rts_building *building = &game->buildings[kind][i];
					if (!building->active) {
						continue;
					}
					s_vec3 away = s_vec3_sub(&a->position, &building->position);
					away.y = 0.0f;
					f32 distance = s_vec3_length(&away);
					const f32 min_distance = building->radius + a->radius + 0.05f;
					if (distance >= min_distance) {
						continue;
					}
					if (distance < 0.001f) {
						away = s_vec3(1.0f, 0.0f, 0.0f);
						distance = 1.0f;
					}
					away = s_vec3_divs(&away, distance);
					a->position = s_vec3_add(&building->position, &s_vec3_muls(&away, min_distance));
					rts_clamp_world_position(&a->position);
				}
			}
		}
	}
}

static void rts_update_buildings(rts_game *game, const f32 dt) {
	for (i32 kind = 0; kind < RTS_BUILDING_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
			rts_building *building = &game->buildings[kind][i];
			if (!building->active) {
				continue;
			}
			if (building->spawn_timer > 0.0f) {
				building->spawn_timer -= dt;
			}
			if (building->attack_timer > 0.0f) {
				building->attack_timer -= dt;
			}

			if (building->type == RTS_BUILDING_TYPE_TOWER && building->attack_timer <= 0.0f) {
				rts_unit_target enemy = rts_find_nearest_enemy_unit(game, building->team, &building->position, 9.5f);
				if (enemy.unit) {
					rts_damage_unit(game, enemy.kind, enemy.index, 18.0f);
					building->attack_timer = 0.95f;
					if (game->autotest.enabled) {
						game->autotest.saw_tower_attack = true;
					}
				}
			}
		}
	}
}

static void rts_update_visuals(rts_game *game) {
	i32 selection_marker_index = 0;
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			rts_unit *unit = &game->units[kind][i];
			if (unit->instance_id == -1) {
				continue;
			}
			if (!unit->active) {
				continue;
			}

			const s_vec3 scale = rts_unit_scale(unit->role, unit->selected);
			s_vec3 position = unit->position;
			position.y = scale.y + 0.03f + sinf(game->sim_time * 3.4f + (f32)unit->serial * 0.11f) * 0.03f;
			const s_mat4 transform = rts_make_transform(&position, &scale, unit->facing_yaw);
			se_object_3d_set_instance_transform(game->unit_objects[kind], unit->instance_id, &transform);

			if (unit->selected &&
				selection_marker_index < RTS_MAX_SELECTABLE_UNITS &&
				game->selection_object != S_HANDLE_NULL &&
				game->selection_instance_ids[selection_marker_index] != -1) {
				s_vec3 marker_scale = s_vec3(unit->radius + 0.44f, 0.04f, unit->radius + 0.44f);
				s_vec3 marker_pos = unit->position;
				marker_pos.y = marker_scale.y;
				const s_mat4 marker_transform = rts_make_transform(&marker_pos, &marker_scale, game->sim_time * 0.5f);
				const se_instance_id marker_id = game->selection_instance_ids[selection_marker_index];
				se_object_3d_set_instance_transform(game->selection_object, marker_id, &marker_transform);
				selection_marker_index++;
			}
		}
	}
	if (game->selection_object != S_HANDLE_NULL) {
		const s_mat4 hidden = rts_hidden_transform();
		for (i32 i = selection_marker_index; i < RTS_MAX_SELECTABLE_UNITS; ++i) {
			if (game->selection_instance_ids[i] != -1) {
				se_object_3d_set_instance_transform(game->selection_object, game->selection_instance_ids[i], &hidden);
			}
		}
	}

	for (i32 kind = 0; kind < RTS_BUILDING_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
			rts_building *building = &game->buildings[kind][i];
			if (building->instance_id == -1) {
				continue;
			}
			if (!building->active) {
				continue;
			}
			s_vec3 scale = rts_building_scale(building->type);
			if (building->spawn_timer > 0.0f) {
				const f32 pulse = 1.0f + sinf(game->sim_time * 10.0f + (f32)building->serial * 0.21f) * 0.03f;
				scale.x *= pulse;
				scale.z *= pulse;
			}
			s_vec3 position = building->position;
			position.y = scale.y;
			const s_mat4 transform = rts_make_transform(&position, &scale, 0.0f);
			se_object_3d_set_instance_transform(game->building_objects[kind], building->instance_id, &transform);
		}
	}

	for (i32 i = 0; i < RTS_MAX_RESOURCES; ++i) {
		rts_resource_node *resource = &game->resources[i];
		if (resource->instance_id == -1) {
			continue;
		}
		if (!resource->active || resource->amount <= 0.0f) {
			continue;
		}
		const f32 life_ratio = rts_clampf(resource->amount / s_max(resource->max_amount, 1.0f), 0.15f, 1.0f);
		s_vec3 scale = s_vec3(0.60f + life_ratio * 0.35f, 0.65f + life_ratio * 1.05f, 0.60f + life_ratio * 0.35f);
		s_vec3 position = resource->position;
		position.y = scale.y;
		const s_mat4 transform = rts_make_transform(&position, &scale, game->sim_time * 0.35f + (f32)resource->serial * 0.08f);
		se_object_3d_set_instance_transform(game->resource_object, resource->instance_id, &transform);
	}
}

static void rts_update_build_preview_visual(rts_game *game) {
	if (game && game->autotest.enabled) {
		game->autotest.saw_build_preview_update = true;
	}
	if (!game || game->headless_mode) {
		return;
	}
	const s_mat4 hidden = rts_hidden_transform();
	for (i32 i = 0; i < 2; ++i) {
		if (game->build_preview_objects[i] != S_HANDLE_NULL && game->build_preview_instance_ids[i] != -1) {
			se_object_3d_set_instance_transform(game->build_preview_objects[i], game->build_preview_instance_ids[i], &hidden);
		}
	}

	if (game->build_mode == RTS_BUILD_MODE_NONE || !game->mouse_world_valid) {
		game->build_preview_valid = false;
		game->build_block_reason[0] = '\0';
		return;
	}

	const rts_building_type type = rts_build_mode_to_type(game->build_mode);
	s_vec3 place = game->mouse_world;
	rts_clamp_world_position(&place);
	game->build_preview_world = place;
	game->build_preview_valid = rts_is_build_position_valid_ex(game, RTS_TEAM_ALLY, type, &place, game->build_block_reason, sizeof(game->build_block_reason));

	const i32 preview_idx = game->build_preview_valid ? 0 : 1;
	if (preview_idx < 0 || preview_idx > 1 || game->build_preview_objects[preview_idx] == S_HANDLE_NULL) {
		return;
	}
	s_vec3 scale = rts_building_scale(type);
	scale.x *= 0.98f;
	scale.z *= 0.98f;
	const f32 pulse = 1.0f + sinf(game->sim_time * 8.0f) * 0.02f;
	scale.x *= pulse;
	scale.z *= pulse;
	s_vec3 pos = place;
	pos.y = scale.y;
	const s_mat4 transform = rts_make_transform(&pos, &scale, 0.0f);
	const se_instance_id preview_instance = game->build_preview_instance_ids[preview_idx];
	if (preview_instance != -1) {
		se_object_3d_set_instance_transform(game->build_preview_objects[preview_idx], preview_instance, &transform);
	}
}

static void rts_update_camera(rts_game *game, const f32 dt) {
	if (game && game->headless_mode) {
		game->mouse_world_valid = true;
		game->mouse_world = game->camera_target;
		return;
	}
	if (!game || game->window == S_HANDLE_NULL) {
		return;
	}
	const se_camera_handle camera_handle = se_scene_3d_get_camera(game->scene);
	if (camera_handle == S_HANDLE_NULL) {
		return;
	}
	se_camera *camera = se_camera_get(camera_handle);
	if (!camera) {
		return;
	}

	const f32 mouse_x = se_window_get_mouse_position_x(game->window);
	const f32 mouse_y = se_window_get_mouse_position_y(game->window);
	f32 width = 0.0f;
	f32 height = 0.0f;
	rts_get_input_view_size(game, &width, &height);
	se_camera_set_aspect_from_window(camera_handle, game->window);
	se_camera_set_perspective(camera_handle, 52.0f, 0.1f, 220.0f);
	const b8 mouse_inside = mouse_x >= 0.0f && mouse_x <= width && mouse_y >= 0.0f && mouse_y <= height;

	const b8 shift = se_window_is_key_down(game->window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(game->window, SE_KEY_RIGHT_SHIFT);
	f32 movement_speed = shift ? 27.0f : 14.0f;
	movement_speed *= dt;
	s_vec3 camera_forward_xz = se_camera_get_forward_vector(camera_handle);
	camera_forward_xz.y = 0.0f;
	if (s_vec3_length(&camera_forward_xz) <= 0.0001f) {
		camera_forward_xz = s_vec3(-sinf(game->camera_yaw), 0.0f, -cosf(game->camera_yaw));
	}
	camera_forward_xz = s_vec3_normalize(&camera_forward_xz);
	const s_vec3 right_xz = s_vec3(-camera_forward_xz.z, 0.0f, camera_forward_xz.x);

	s_vec3 movement_delta = s_vec3(0.0f, 0.0f, 0.0f);

	if (se_window_is_key_down(game->window, SE_KEY_W) || se_window_is_key_down(game->window, SE_KEY_UP)) {
		movement_delta = s_vec3_add(&movement_delta, &s_vec3_muls(&camera_forward_xz, movement_speed));
	}
	if (se_window_is_key_down(game->window, SE_KEY_S) || se_window_is_key_down(game->window, SE_KEY_DOWN)) {
		movement_delta = s_vec3_sub(&movement_delta, &s_vec3_muls(&camera_forward_xz, movement_speed));
	}
	if (se_window_is_key_down(game->window, SE_KEY_D) || se_window_is_key_down(game->window, SE_KEY_RIGHT)) {
		movement_delta = s_vec3_add(&movement_delta, &s_vec3_muls(&right_xz, movement_speed));
	}
	if (se_window_is_key_down(game->window, SE_KEY_A) || se_window_is_key_down(game->window, SE_KEY_LEFT)) {
		movement_delta = s_vec3_sub(&movement_delta, &s_vec3_muls(&right_xz, movement_speed));
	}
	if (s_vec3_length(&movement_delta) > 0.0f) {
		se_camera_pan_world(camera_handle, &movement_delta);
	}

	const b8 rotate_left = se_window_is_key_down(game->window, SE_KEY_Q);
	const b8 rotate_right = se_window_is_key_down(game->window, SE_KEY_E);
	static b8 camera_pre_debug_printed = false;
	if (!camera_pre_debug_printed) {
		camera_pre_debug_printed = true;
		rts_log(
			"camera_pre yaw=%.2f pitch=%.2f dist=%.2f q=%d e=%d mouse=(%.1f %.1f)",
			game->camera_yaw,
			game->camera_pitch,
			game->camera_distance,
			rotate_left ? 1 : 0,
			rotate_right ? 1 : 0,
			mouse_x,
			mouse_y);
	}

	const f32 edge_margin = 18.0f;
	const b8 allow_edge_pan = (game->frame_index > 4) && (mouse_x > 0.5f || mouse_y > 0.5f);
	s_vec3 edge_delta = s_vec3(0.0f, 0.0f, 0.0f);
	if (allow_edge_pan && mouse_inside && !se_window_is_mouse_down(game->window, SE_MOUSE_MIDDLE) && !rotate_left && !rotate_right && game->build_mode == RTS_BUILD_MODE_NONE) {
		if (mouse_x < edge_margin) {
			edge_delta = s_vec3_sub(&edge_delta, &s_vec3_muls(&right_xz, movement_speed * 0.85f));
		}
		if (mouse_x > width - edge_margin) {
			edge_delta = s_vec3_add(&edge_delta, &s_vec3_muls(&right_xz, movement_speed * 0.85f));
		}
		if (mouse_y < edge_margin) {
			edge_delta = s_vec3_add(&edge_delta, &s_vec3_muls(&camera_forward_xz, movement_speed * 0.85f));
		}
		if (mouse_y > height - edge_margin) {
			edge_delta = s_vec3_sub(&edge_delta, &s_vec3_muls(&camera_forward_xz, movement_speed * 0.85f));
		}
	}
	if (s_vec3_length(&edge_delta) > 0.0f) {
		se_camera_pan_world(camera_handle, &edge_delta);
	}

	/* Q = rotate camera left (CCW around world up), E = rotate right (CW). */
	f32 yaw_delta = 0.0f;
	f32 pitch_delta = 0.0f;
	if (rotate_left) yaw_delta += dt * 0.95f;
	if (rotate_right) yaw_delta -= dt * 0.95f;
	if (se_window_is_key_down(game->window, SE_KEY_Z)) pitch_delta += dt * 0.45f;
	if (se_window_is_key_down(game->window, SE_KEY_X)) pitch_delta -= dt * 0.45f;
	if (yaw_delta != 0.0f || pitch_delta != 0.0f) {
		se_camera_orbit(camera_handle, &camera->target, yaw_delta, pitch_delta, 0.70f, 1.18f);
	}

	if (se_window_is_mouse_down(game->window, SE_MOUSE_MIDDLE) && game->build_mode == RTS_BUILD_MODE_NONE) {
		s_vec2 mouse_delta = {0};
		se_window_get_mouse_delta(game->window, &mouse_delta);
		const s_vec3 to_camera = s_vec3_sub(&camera->position, &camera->target);
		const f32 distance = s_vec3_length(&to_camera);
		const f32 drag_speed = rts_clampf(distance * 0.0018f, 0.010f, 0.082f);
		s_vec3 drag_delta = s_vec3(0.0f, 0.0f, 0.0f);
		drag_delta = s_vec3_sub(&drag_delta, &s_vec3_muls(&right_xz, mouse_delta.x * drag_speed));
		drag_delta = s_vec3_sub(&drag_delta, &s_vec3_muls(&camera_forward_xz, mouse_delta.y * drag_speed));
		se_camera_pan_world(camera_handle, &drag_delta);
	}

	s_vec2 scroll = {0};
	se_window_get_scroll_delta(game->window, &scroll);
	if (fabsf(scroll.y) > 0.0001f) {
		se_camera_dolly(camera_handle, &camera->target, -scroll.y * 1.8f, 12.0f, 54.0f);
	}

	if (se_window_is_key_pressed(game->window, SE_KEY_F)) {
		s_vec3 sum = s_vec3(0.0f, 0.0f, 0.0f);
		i32 count = 0;
		for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
			if (rts_unit_kind_team((rts_unit_kind)kind) != RTS_TEAM_ALLY) {
				continue;
			}
			for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
				const rts_unit *unit = &game->units[kind][i];
				if (!unit->active || !unit->selected) {
					continue;
				}
				sum = s_vec3_add(&sum, &unit->position);
				count++;
			}
		}
		if (count > 0) {
			const s_vec3 desired_target = s_vec3_divs(&sum, (f32)count);
			const s_vec3 to_target = s_vec3_sub(&desired_target, &camera->target);
			se_camera_pan_world(camera_handle, &to_target);
		}
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_H)) {
		const rts_building *ally_hq = rts_find_primary_building(game, RTS_TEAM_ALLY, RTS_BUILDING_TYPE_HQ);
		if (ally_hq) {
			const s_vec3 to_target = s_vec3_sub(&ally_hq->position, &camera->target);
			se_camera_pan_world(camera_handle, &to_target);
			se_camera_dolly(camera_handle, &camera->target, 0.0f, 18.0f, 60.0f);
			rts_set_command_line(game, "Camera centered on HQ");
		}
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_R)) {
		s_vec3 reset_target = camera->target;
		const rts_building *ally_hq = rts_find_primary_building(game, RTS_TEAM_ALLY, RTS_BUILDING_TYPE_HQ);
		if (ally_hq) {
			reset_target = ally_hq->position;
			reset_target.x += 1.8f;
			reset_target.z -= 1.6f;
		}
		camera->target = reset_target;
		const s_vec3 reset_offset = s_vec3(
			sinf(RTS_CAMERA_DEFAULT_YAW) * cosf(RTS_CAMERA_DEFAULT_PITCH) * RTS_CAMERA_DEFAULT_DISTANCE,
			sinf(RTS_CAMERA_DEFAULT_PITCH) * RTS_CAMERA_DEFAULT_DISTANCE,
			cosf(RTS_CAMERA_DEFAULT_YAW) * cosf(RTS_CAMERA_DEFAULT_PITCH) * RTS_CAMERA_DEFAULT_DISTANCE);
		camera->position = s_vec3_add(&camera->target, &reset_offset);
		rts_set_command_line(game, "Camera reset");
	}

	const s_vec3 min_bounds = s_vec3(-RTS_MAP_HALF_SIZE, 0.0f, -RTS_MAP_HALF_SIZE);
	const s_vec3 max_bounds = s_vec3(RTS_MAP_HALF_SIZE, 0.0f, RTS_MAP_HALF_SIZE);
	se_camera_clamp_target(camera_handle, &min_bounds, &max_bounds);
	camera->target.y = 0.0f;
	camera->up = s_vec3(0.0f, 1.0f, 0.0f);

	const s_vec3 cam_offset = s_vec3_sub(&camera->position, &camera->target);
	const f32 cam_distance = s_max(s_vec3_length(&cam_offset), 0.0001f);
	game->camera_target = camera->target;
	game->camera_distance = cam_distance;
	game->camera_yaw = atan2f(cam_offset.x, cam_offset.z);
	game->camera_pitch = asinf(rts_clampf(cam_offset.y / cam_distance, -1.0f, 1.0f));

	game->mouse_world_valid = rts_screen_to_ground(game, mouse_x, mouse_y, &game->mouse_world);
	if (!game->mouse_world_valid) {
		game->mouse_world = game->camera_target;
	}

	static b8 camera_debug_printed = false;
	if (!camera_debug_printed) {
		camera_debug_printed = true;
		s_vec3 center_world = s_vec3(0.0f, 0.0f, 0.0f);
		const b8 center_hit = rts_screen_to_ground(game, width * 0.5f, height * 0.5f, &center_world);
		s_vec3 view_dir = s_vec3_sub(&camera->target, &camera->position);
		view_dir = s_vec3_normalize(&view_dir);
		rts_log(
			"camera_init pos=(%.2f %.2f %.2f) target=(%.2f %.2f %.2f) yaw=%.2f pitch=%.2f dir=(%.2f %.2f %.2f) center_hit=%d center=(%.2f %.2f)",
			camera->position.x, camera->position.y, camera->position.z,
			camera->target.x, camera->target.y, camera->target.z,
			game->camera_yaw, game->camera_pitch,
			view_dir.x, view_dir.y, view_dir.z,
			center_hit ? 1 : 0,
			center_world.x, center_world.z);
	}
}

static void rts_handle_player_input(rts_game *game) {
	if (game && game->headless_mode) {
		return;
	}
	if (!game || game->window == S_HANDLE_NULL) {
		return;
	}

	if (se_window_is_key_pressed(game->window, SE_KEY_F1)) {
		game->show_debug_overlay = !game->show_debug_overlay;
		rts_set_command_line(game, "Debug details %s", game->show_debug_overlay ? "ON" : "OFF");
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_F2)) {
		game->show_minimap_ui = !game->show_minimap_ui;
		rts_set_command_line(game, "Minimap UI %s", game->show_minimap_ui ? "ON" : "OFF");
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_F3)) {
		game->track_system_timing = !game->track_system_timing;
		game->next_system_timing_log_time = game->sim_time + 0.25f;
		rts_set_command_line(game, "System timing tracker %s", game->track_system_timing ? "ON" : "OFF");
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_SPACE)) {
		game->simulator.enabled = !game->simulator.enabled;
		rts_set_command_line(game, "Simulation %s", game->simulator.enabled ? "ON" : "OFF");
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_ESCAPE) && game->build_mode != RTS_BUILD_MODE_NONE) {
		game->build_mode = RTS_BUILD_MODE_NONE;
		rts_set_command_line(game, "Build mode canceled");
	}

	const f32 mouse_x = se_window_get_mouse_position_x(game->window);
	const f32 mouse_y = se_window_get_mouse_position_y(game->window);
	s_vec2 mouse_ui = {0};
	const b8 mouse_ui_valid = rts_screen_to_ui(game, mouse_x, mouse_y, &mouse_ui);

	const b8 append_selection = se_window_is_key_down(game->window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(game->window, SE_KEY_RIGHT_SHIFT);
	if (se_window_is_key_pressed(game->window, SE_KEY_1)) {
		rts_select_group(game, RTS_UNIT_ROLE_WORKER, append_selection);
		rts_recount_selection(game);
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_2)) {
		rts_select_group(game, RTS_UNIT_ROLE_SOLDIER, append_selection);
		rts_recount_selection(game);
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_3)) {
		rts_select_group(game, (rts_unit_role)-1, append_selection);
		rts_recount_selection(game);
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_0)) {
		rts_clear_selection(game);
	}

	if (se_window_is_key_pressed(game->window, SE_KEY_U)) {
		(void)rts_try_train_unit(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_WORKER, true);
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_I)) {
		(void)rts_try_train_unit(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_SOLDIER, true);
	}

	if (se_window_is_key_pressed(game->window, SE_KEY_B)) {
		game->build_mode = (game->build_mode == RTS_BUILD_MODE_BARRACKS) ? RTS_BUILD_MODE_NONE : RTS_BUILD_MODE_BARRACKS;
		rts_set_command_line(game, game->build_mode == RTS_BUILD_MODE_BARRACKS ? "Build mode: Barracks" : "Build mode canceled");
	}
	if (se_window_is_key_pressed(game->window, SE_KEY_T)) {
		game->build_mode = (game->build_mode == RTS_BUILD_MODE_TOWER) ? RTS_BUILD_MODE_NONE : RTS_BUILD_MODE_TOWER;
		rts_set_command_line(game, game->build_mode == RTS_BUILD_MODE_TOWER ? "Build mode: Tower" : "Build mode canceled");
	}

	if (game->show_minimap_ui && game->build_mode == RTS_BUILD_MODE_NONE && mouse_ui_valid && se_window_is_mouse_pressed(game->window, SE_MOUSE_LEFT)) {
		s_vec3 minimap_world = s_vec3(0.0f, 0.0f, 0.0f);
		if (rts_minimap_ui_to_world(&mouse_ui, &minimap_world)) {
			game->camera_target = minimap_world;
			rts_set_command_line(game, "Camera moved by minimap");
			return;
		}
	}

	if (game->build_mode != RTS_BUILD_MODE_NONE) {
		/* Use current-frame mouse projection for placement, not previous-frame preview. */
		rts_update_build_preview_visual(game);

		if (se_window_is_mouse_pressed(game->window, SE_MOUSE_RIGHT)) {
			game->build_mode = RTS_BUILD_MODE_NONE;
			rts_set_command_line(game, "Build mode canceled");
		}
		if (se_window_is_mouse_pressed(game->window, SE_MOUSE_LEFT) && game->mouse_world_valid) {
			const rts_building_type type = rts_build_mode_to_type(game->build_mode);
			if (!game->build_preview_valid) {
				rts_set_command_line(game, "%s", game->build_block_reason[0] ? game->build_block_reason : "Build position blocked");
				return;
			}

			if (rts_try_build(game, RTS_TEAM_ALLY, type, &game->build_preview_world, true)) {
				game->build_mode = RTS_BUILD_MODE_NONE;
			}
		}
		return;
	}

	if (game->mouse_world_valid && se_window_is_key_pressed(game->window, SE_KEY_G)) {
		const i32 target_resource = rts_find_nearest_resource_index(game, &game->mouse_world);
		if (target_resource >= 0) {
			rts_issue_gather_selected(game, target_resource);
			rts_set_command_line(game, "Gather command");
		}
	}

	if (game->mouse_world_valid && se_window_is_key_pressed(game->window, SE_KEY_M)) {
		rts_issue_move_selected(game, &game->mouse_world, false);
		rts_set_command_line(game, "Move command");
	}
	if (game->mouse_world_valid && se_window_is_key_pressed(game->window, SE_KEY_V)) {
		rts_issue_move_selected(game, &game->mouse_world, true);
		rts_set_command_line(game, "Attack-move command");
	}

	if (se_window_is_mouse_pressed(game->window, SE_MOUSE_LEFT)) {
		const s_vec3 *fallback_point = game->mouse_world_valid ? &game->mouse_world : &game->camera_target;
		rts_select_nearest_at_cursor(game, mouse_x, mouse_y, fallback_point, append_selection);
	}

	if (game->mouse_world_valid && se_window_is_mouse_pressed(game->window, SE_MOUSE_RIGHT)) {
		if (game->selected_count > 0) {
			const rts_unit_target clicked_enemy_unit = rts_find_nearest_enemy_unit(game, RTS_TEAM_ALLY, &game->mouse_world, RTS_COMMAND_RADIUS);
			const rts_building_target clicked_enemy_building = rts_find_nearest_enemy_building(game, RTS_TEAM_ALLY, &game->mouse_world, RTS_COMMAND_RADIUS + 1.0f);
			if (clicked_enemy_unit.unit || clicked_enemy_building.building) {
				rts_issue_move_selected(game, &game->mouse_world, true);
				rts_set_command_line(game, "Attack target");
			} else if (rts_has_selected_workers(game)) {
				const i32 target_resource = rts_find_nearest_resource_index(game, &game->mouse_world);
				if (target_resource >= 0 && rts_distance_xz(&game->mouse_world, &game->resources[target_resource].position) < 2.8f) {
					rts_issue_gather_selected(game, target_resource);
					rts_set_command_line(game, "Worker gather");
				} else {
					rts_issue_move_selected(game, &game->mouse_world, false);
					rts_set_command_line(game, "Move order");
				}
			} else {
				rts_issue_move_selected(game, &game->mouse_world, false);
				rts_set_command_line(game, "Move order");
			}
		}
	}
}

static void rts_sim_clear_controls(rts_game *game) {
	if (!game || game->window == S_HANDLE_NULL) {
		return;
	}
	se_window_clear_input_state(game->window);
}

static void rts_sim_move_mouse_to_world(rts_game *game, const s_vec3 *world_position) {
	if (!game || game->window == S_HANDLE_NULL || !world_position) {
		return;
	}
	f32 screen_x = 0.0f;
	f32 screen_y = 0.0f;
	if (!rts_world_to_screen(game, world_position, &screen_x, &screen_y)) {
		return;
	}
	se_window_inject_mouse_position(game->window, screen_x, screen_y);
}

static b8 rts_sim_action_once(rts_game *game, const u32 action_bit) {
	if (!game || action_bit == 0) {
		return false;
	}
	if ((game->simulator.phase_actions & action_bit) != 0) {
		return false;
	}
	game->simulator.phase_actions |= action_bit;
	return true;
}

static void rts_simulator_advance(rts_game *game, const c8 *label) {
	game->simulator.phase++;
	game->simulator.phase_time = 0.0f;
	game->simulator.phase_actions = 0;
	rts_log("sim phase -> %d (%s)", game->simulator.phase, label ? label : "");
}

static void rts_run_input_simulator(rts_game *game, const f32 dt) {
	if (!game || !game->simulator.enabled) {
		return;
	}
	game->simulator.total_time += dt;
	game->simulator.phase_time += dt;

	rts_building *ally_hq = rts_find_primary_building(game, RTS_TEAM_ALLY, RTS_BUILDING_TYPE_HQ);
	rts_building *enemy_hq = rts_find_primary_building(game, RTS_TEAM_ENEMY, RTS_BUILDING_TYPE_HQ);

	const b8 deterministic_actions = game->headless_mode || game->autotest.enabled;
	if (deterministic_actions) {
		if (!game->headless_mode && game->window != S_HANDLE_NULL) {
			rts_sim_clear_controls(game);
		}
		switch (game->simulator.phase) {
			case 0: {
				if (game->simulator.phase_time > 0.12f && rts_sim_action_once(game, 1u << 0)) {
					rts_select_group(game, RTS_UNIT_ROLE_WORKER, false);
					rts_recount_selection(game);
				}
				if (game->simulator.phase_time > 0.55f) {
					rts_simulator_advance(game, "select workers");
				}
			} break;
			case 1: {
				if (ally_hq && game->simulator.phase_time > 0.16f && rts_sim_action_once(game, 1u << 0)) {
					const i32 resource_index = rts_find_nearest_resource_index(game, &ally_hq->position);
					if (resource_index >= 0) {
						rts_issue_gather_selected(game, resource_index);
					}
				}
				if (game->simulator.phase_time > 0.95f) {
					rts_simulator_advance(game, "gather");
				}
			} break;
			case 2: {
				if (ally_hq && game->simulator.phase_time > 0.18f && rts_sim_action_once(game, 1u << 0)) {
					s_vec3 build_pos = ally_hq->position;
					build_pos.x += 6.0f;
					build_pos.z += 3.2f;
					(void)rts_try_build(game, RTS_TEAM_ALLY, RTS_BUILDING_TYPE_BARRACKS, &build_pos, false);
				}
				if (game->simulator.phase_time > 0.9f) {
					rts_simulator_advance(game, "build barracks");
				}
			} break;
			case 3: {
				if ((game->simulator.phase_time > 0.14f && rts_sim_action_once(game, 1u << 0)) ||
					(game->simulator.phase_time > 0.64f && rts_sim_action_once(game, 1u << 1)) ||
					(game->simulator.phase_time > 1.16f && rts_sim_action_once(game, 1u << 2))) {
					(void)rts_try_train_unit(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_WORKER, false);
				}
				if (game->simulator.phase_time > 1.6f) {
					rts_simulator_advance(game, "train workers");
				}
			} break;
			case 4: {
				if (game->simulator.phase_time > 0.10f && rts_sim_action_once(game, 1u << 0)) {
					rts_select_group(game, RTS_UNIT_ROLE_SOLDIER, false);
					rts_recount_selection(game);
				}
				if ((game->simulator.phase_time > 0.35f && rts_sim_action_once(game, 1u << 1)) ||
					(game->simulator.phase_time > 0.82f && rts_sim_action_once(game, 1u << 2)) ||
					(game->simulator.phase_time > 1.30f && rts_sim_action_once(game, 1u << 3))) {
					(void)rts_try_train_unit(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_SOLDIER, false);
				}
				if (game->simulator.phase_time > 1.9f) {
					rts_simulator_advance(game, "train soldiers");
				}
			} break;
			case 5: {
				if (enemy_hq && game->simulator.phase_time > 0.25f && rts_sim_action_once(game, 1u << 0)) {
					rts_issue_move_selected(game, &enemy_hq->position, true);
				}
				if (game->simulator.phase_time > 1.1f) {
					rts_simulator_advance(game, "attack wave");
				}
			} break;
			case 6: {
				if (!game->headless_mode && game->window != S_HANDLE_NULL) {
					se_window_inject_key_state(game->window, SE_KEY_D, true);
					se_window_inject_key_state(game->window, SE_KEY_W, true);
					se_window_inject_scroll_delta(game->window, 0.0f, -0.4f);
				}
				if (game->simulator.phase_time > 0.22f && rts_sim_action_once(game, 1u << 0)) {
					(void)rts_try_train_unit(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_SOLDIER, false);
				}
				if (game->simulator.phase_time > 1.6f) {
					game->simulator.phase = 3;
					game->simulator.phase_time = 0.0f;
					rts_log("sim loop restart");
				}
			} break;
			default: {
				game->simulator.phase = 0;
				game->simulator.phase_time = 0.0f;
			} break;
		}
	} else {
		if (game->window == S_HANDLE_NULL) {
			return;
		}
		rts_sim_clear_controls(game);
		switch (game->simulator.phase) {
			case 0: {
				if (game->simulator.phase_time > 0.12f && rts_sim_action_once(game, 1u << 0)) {
					se_window_inject_key_state(game->window, SE_KEY_1, true);
				}
				if (game->simulator.phase_time > 0.55f) {
					rts_simulator_advance(game, "select workers");
				}
			} break;
			case 1: {
				const i32 resource_index = rts_find_nearest_resource_index(game, &game->camera_target);
				if (resource_index >= 0 && game->resources[resource_index].active) {
					rts_sim_move_mouse_to_world(game, &game->resources[resource_index].position);
					if (game->simulator.phase_time > 0.20f && rts_sim_action_once(game, 1u << 0)) {
						se_window_inject_mouse_button_state(game->window, SE_MOUSE_RIGHT, true);
					}
				}
				if (game->simulator.phase_time > 1.0f) {
					rts_simulator_advance(game, "gather");
				}
			} break;
			case 2: {
				if (ally_hq) {
					s_vec3 build_pos = ally_hq->position;
					build_pos.x += 6.0f;
					build_pos.z += 3.2f;
					rts_sim_move_mouse_to_world(game, &build_pos);
					if (game->simulator.phase_time > 0.18f && rts_sim_action_once(game, 1u << 0)) {
						se_window_inject_key_state(game->window, SE_KEY_B, true);
					}
					if (game->simulator.phase_time > 0.42f && rts_sim_action_once(game, 1u << 1)) {
						se_window_inject_mouse_button_state(game->window, SE_MOUSE_LEFT, true);
					}
				}
				if (game->simulator.phase_time > 0.9f) {
					rts_simulator_advance(game, "build barracks");
				}
			} break;
			case 3: {
				if ((game->simulator.phase_time > 0.14f && rts_sim_action_once(game, 1u << 0)) ||
					(game->simulator.phase_time > 0.64f && rts_sim_action_once(game, 1u << 1)) ||
					(game->simulator.phase_time > 1.16f && rts_sim_action_once(game, 1u << 2))) {
					se_window_inject_key_state(game->window, SE_KEY_U, true);
				}
				if (game->simulator.phase_time > 1.6f) {
					rts_simulator_advance(game, "train workers");
				}
			} break;
			case 4: {
				if (game->simulator.phase_time > 0.10f && rts_sim_action_once(game, 1u << 0)) {
					se_window_inject_key_state(game->window, SE_KEY_2, true);
				}
				if ((game->simulator.phase_time > 0.35f && rts_sim_action_once(game, 1u << 1)) ||
					(game->simulator.phase_time > 0.82f && rts_sim_action_once(game, 1u << 2)) ||
					(game->simulator.phase_time > 1.30f && rts_sim_action_once(game, 1u << 3))) {
					se_window_inject_key_state(game->window, SE_KEY_I, true);
				}
				if (game->simulator.phase_time > 1.9f) {
					rts_simulator_advance(game, "train soldiers");
				}
			} break;
			case 5: {
				if (enemy_hq) {
					rts_sim_move_mouse_to_world(game, &enemy_hq->position);
					if (game->simulator.phase_time > 0.25f && rts_sim_action_once(game, 1u << 0)) {
						se_window_inject_mouse_button_state(game->window, SE_MOUSE_RIGHT, true);
					}
				}
				if (game->simulator.phase_time > 1.1f) {
					rts_simulator_advance(game, "attack wave");
				}
			} break;
			case 6: {
				se_window_inject_key_state(game->window, SE_KEY_D, true);
				se_window_inject_key_state(game->window, SE_KEY_W, true);
				se_window_inject_scroll_delta(game->window, 0.0f, -0.4f);
				if (game->simulator.phase_time > 1.6f) {
					game->simulator.phase = 3;
					game->simulator.phase_time = 0.0f;
					rts_log("sim loop restart");
				}
			} break;
			default: {
				game->simulator.phase = 0;
				game->simulator.phase_time = 0.0f;
			} break;
		}
	}

	if (game->simulator.total_time >= game->simulator.next_status_print) {
		game->simulator.next_status_print += 1.0f;
		rts_log("sim t=%.1f ally_res=%.0f enemy_res=%.0f selected=%d", game->simulator.total_time, game->team_resources[RTS_TEAM_ALLY], game->team_resources[RTS_TEAM_ENEMY], game->selected_count);
	}

	if (!game->headless_mode && game->simulator.stop_after_seconds > 0.0f && game->simulator.total_time >= game->simulator.stop_after_seconds) {
		se_window_set_should_close(game->window, true);
	}
}

static void rts_validate_state(rts_game *game) {
	if (!game) {
		return;
	}
	rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_VALIDATE);
	b8 ok = true;

	if (game->team_resources[RTS_TEAM_ALLY] < -0.01f || game->team_resources[RTS_TEAM_ENEMY] < -0.01f) {
		rts_log("validation failed: resource below zero");
		ok = false;
	}

	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			const rts_unit *unit = &game->units[kind][i];
			if (!unit->active) {
				continue;
			}
			if (unit->health <= 0.0f) {
				rts_log("validation failed: active unit with hp<=0 (serial=%d)", unit->serial);
				ok = false;
			}
			if (fabsf(unit->position.x) > RTS_MAP_HALF_SIZE + 2.0f || fabsf(unit->position.z) > RTS_MAP_HALF_SIZE + 2.0f) {
				rts_log("validation failed: unit out of bounds (serial=%d)", unit->serial);
				ok = false;
			}
		}
	}

	i32 selection_count = 0;
	for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
		const rts_unit *worker = &game->units[RTS_UNIT_KIND_ALLY_WORKER][i];
		const rts_unit *soldier = &game->units[RTS_UNIT_KIND_ALLY_SOLDIER][i];
		if (worker->active && worker->selected) {
			selection_count++;
		}
		if (soldier->active && soldier->selected) {
			selection_count++;
		}
	}
	if (selection_count != game->selected_count) {
		rts_log("validation failed: selected count mismatch");
		game->selected_count = selection_count;
		ok = false;
	}

	if (!ok) {
		game->validations_failed = true;
	} else if (game->autotest.enabled) {
		game->autotest.saw_validation_pass = true;
	}
}

static b8 rts_team_defeated(rts_game *game, const rts_team team) {
	if (rts_find_primary_building(game, team, RTS_BUILDING_TYPE_HQ)) {
		return false;
	}
	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		if (rts_unit_kind_team((rts_unit_kind)kind) != team) {
			continue;
		}
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			if (game->units[kind][i].active) {
				return false;
			}
		}
	}
	return true;
}

static b8 rts_world_to_ndc(rts_game *game, const s_vec3 *world, s_vec2 *out_ndc) {
	if (!game || !world || !out_ndc || game->scene == S_HANDLE_NULL) {
		return false;
	}
	const se_camera_handle camera = se_scene_3d_get_camera(game->scene);
	if (camera == S_HANDLE_NULL) {
		return false;
	}
	return se_ui_world_to_ndc(camera, world, out_ndc);
}

static void rts_make_health_bar(const f32 ratio, c8 *out_bar, const i32 bar_len) {
	if (!out_bar || bar_len <= 0) {
		return;
	}
	const f32 clamped = rts_clampf(ratio, 0.0f, 1.0f);
	const i32 filled = (i32)roundf(clamped * (f32)bar_len);
	for (i32 i = 0; i < bar_len; ++i) {
		out_bar[i] = (i < filled) ? '#' : '-';
	}
	out_bar[bar_len] = '\0';
}

static void rts_render_health_labels(rts_game *game) {
	if (!game || !game->text_handle || game->font == S_HANDLE_NULL || game->headless_mode) {
		return;
	}
	c8 label[96] = {0};
	c8 bar[12] = {0};

	for (i32 kind = 0; kind < RTS_UNIT_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_UNITS_PER_KIND; ++i) {
			const rts_unit *unit = &game->units[kind][i];
			if (!unit->active) {
				continue;
			}
			if (!unit->selected && unit->health >= unit->max_health - 0.1f) {
				continue;
			}
			s_vec3 world = unit->position;
			world.y = rts_unit_scale(unit->role, false).y * 2.2f + 0.35f;
			s_vec2 ndc = {0};
			if (!rts_world_to_ndc(game, &world, &ndc)) {
				continue;
			}
			rts_make_health_bar(unit->health / unit->max_health, bar, 8);
			snprintf(label, sizeof(label), "%c[%s] %3.0f", unit->team == RTS_TEAM_ALLY ? 'A' : 'E', bar, unit->health);
			se_text_render(game->text_handle, game->font, label, &ndc, &s_vec2(0.62f, 0.62f), 0.045f);
		}
	}

	for (i32 kind = 0; kind < RTS_BUILDING_KIND_COUNT; ++kind) {
		for (i32 i = 0; i < RTS_MAX_BUILDINGS_PER_KIND; ++i) {
			const rts_building *building = &game->buildings[kind][i];
			if (!building->active) {
				continue;
			}
			if (building->type != RTS_BUILDING_TYPE_HQ && building->health >= building->max_health - 0.1f) {
				continue;
			}
			s_vec3 world = building->position;
			world.y = rts_building_scale(building->type).y * 2.0f + 0.6f;
			s_vec2 ndc = {0};
			if (!rts_world_to_ndc(game, &world, &ndc)) {
				continue;
			}
			rts_make_health_bar(building->health / building->max_health, bar, 8);
			snprintf(label, sizeof(label), "%c%s [%s]", building->team == RTS_TEAM_ALLY ? 'A' : 'E', building->type == RTS_BUILDING_TYPE_HQ ? "HQ" : "B", bar);
			se_text_render(game->text_handle, game->font, label, &ndc, &s_vec2(0.75f, 0.75f), 0.045f);
		}
	}
}

static void rts_render_overlay(rts_game *game) {
	if (game && game->headless_mode) {
		return;
	}
	if (!game || !game->text_handle || game->font == S_HANDLE_NULL) {
		return;
	}
	if (game->autotest.enabled) {
		game->autotest.saw_text_render = true;
	}

	const i32 ally_workers = rts_count_units(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_WORKER);
	const i32 ally_soldiers = rts_count_units(game, RTS_TEAM_ALLY, RTS_UNIT_ROLE_SOLDIER);
	const i32 enemy_workers = rts_count_units(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_WORKER);
	const i32 enemy_soldiers = rts_count_units(game, RTS_TEAM_ENEMY, RTS_UNIT_ROLE_SOLDIER);
	const f64 fps = se_window_get_fps(game->window);
	se_debug_frame_timing frame_timing = {0};
	(void)se_debug_get_last_frame_timing(&frame_timing);
	se_debug_system_stats debug_stats = {0};
	(void)se_debug_collect_stats(game->window, NULL, &debug_stats);
	c8 timing_lines[512] = {0};
	se_debug_dump_last_frame_timing_lines(timing_lines, sizeof(timing_lines));
	const c8 *build_mode = "NONE";
	if (game->build_mode == RTS_BUILD_MODE_BARRACKS) {
		build_mode = "BARRACKS";
	} else if (game->build_mode == RTS_BUILD_MODE_TOWER) {
		build_mode = "TOWER";
	}

	c8 text[RTS_UI_TEXT_BUFFER] = {0};
	snprintf(
		text,
		sizeof(text),
		"99_game :: RTS 3D Sandbox\n"
		"FPS: %6.2f   Time: %6.1f   Sim: %s phase %d\n"
		"Resources -> Ally: %6.0f  Enemy: %6.0f\n"
		"Ally Units: workers=%d soldiers=%d selected=%d\n"
		"Enemy Units: workers=%d soldiers=%d\n"
		"Frame(ms): total=%.2f upd=%.2f in=%.2f s2=%.2f s3=%.2f txt=%.2f ui=%.2f nav=%.2f prs=%.2f oth=%.2f\n"
		"Engine: win=%u cam=%u sc2=%u sc3=%u obj2=%u obj3=%u ui=%u traces=%u\n"
		"Camera: target(%6.2f %6.2f) yaw=%6.2f pitch=%5.2f dist=%5.2f\n"
		"Mouse Ground: %7.2f %7.2f valid=%d   Build:%s preview:%s reason:%s\n"
		"%s\n"
		"Msg: %s\n"
		"Controls: LMB select  RMB command  WASD+Arrows pan  MMB drag camera  wheel zoom\n"
		"Q/E rotate  Z/X pitch  F focus selected  H center HQ  R reset camera  F2 minimap toggle  click minimap to move camera\n"
		"1 workers 2 soldiers 3 all 0 clear  U worker  I soldier  B barracks  T tower  G gather  M move  V attack-move\n"
		"Build mode: LMB place  RMB/ESC cancel   Minimap: cyan ally / red enemy / gold resources\n",
		fps,
		game->sim_time,
		game->simulator.enabled ? "ON" : "OFF",
		game->simulator.phase,
		game->team_resources[RTS_TEAM_ALLY],
		game->team_resources[RTS_TEAM_ENEMY],
		ally_workers,
		ally_soldiers,
		game->selected_count,
		enemy_workers,
		enemy_soldiers,
		frame_timing.frame_ms,
		frame_timing.window_update_ms,
		frame_timing.input_ms,
		frame_timing.scene2d_ms,
		frame_timing.scene3d_ms,
		frame_timing.text_ms,
		frame_timing.ui_ms,
		frame_timing.navigation_ms,
		frame_timing.window_present_ms,
		frame_timing.other_ms,
		debug_stats.window_count,
		debug_stats.camera_count,
		debug_stats.scene2d_count,
		debug_stats.scene3d_count,
		debug_stats.object2d_count,
		debug_stats.object3d_count,
		debug_stats.ui_element_count,
		debug_stats.navigation_trace_count,
		game->camera_target.x,
		game->camera_target.z,
		game->camera_yaw,
		game->camera_pitch,
		game->camera_distance,
		game->mouse_world.x,
		game->mouse_world.z,
		game->mouse_world_valid ? 1 : 0,
		build_mode,
		game->build_mode == RTS_BUILD_MODE_NONE ? "-" : (game->build_preview_valid ? "VALID" : "BLOCKED"),
		game->build_mode == RTS_BUILD_MODE_NONE ? "-" : (game->build_preview_valid ? "-" : (game->build_block_reason[0] ? game->build_block_reason : "blocked")),
		timing_lines,
		game->command_line_timer > 0.0f ? game->command_line : "-");

	if (!game->show_debug_overlay) {
		snprintf(
			text,
			sizeof(text),
			"99_game :: Ally %.0f  Enemy %.0f  Selected %d  Build %s  Sim %s\n"
			"Frame %.2fms (s3=%.2f, prs=%.2f, txt=%.2f) traces=%u  F3 track %s\n"
			"SPACE simulator, F1 details, ESC quit\n"
			"%s",
			game->team_resources[RTS_TEAM_ALLY],
			game->team_resources[RTS_TEAM_ENEMY],
			game->selected_count,
			build_mode,
			game->simulator.enabled ? "ON" : "OFF",
			frame_timing.frame_ms,
			frame_timing.scene3d_ms,
			frame_timing.window_present_ms,
			frame_timing.text_ms,
			debug_stats.navigation_trace_count,
			game->track_system_timing ? "ON" : "OFF",
			game->command_line_timer > 0.0f ? game->command_line : "");
	}

	se_text_render(game->text_handle, game->font, text, &s_vec2(-0.98f, 0.94f), &s_vec2(1.0f, 1.0f), 0.053f);
}

static void rts_render_frame(rts_game *game) {
	rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_RENDER);
	if (!game || game->headless_mode) {
		return;
	}
	se_scene_3d_render_to_buffer(game->scene);
	se_scene_3d_render_to_screen(game->scene, game->window);
	rts_update_minimap_ui(game);
	if (game->show_minimap_ui && game->hud_scene != S_HANDLE_NULL) {
		const s_vec4 scene_bg = s_vec4(0.045f, 0.053f, 0.058f, 1.0f);
		/* Keep HUD framebuffer transparent so it overlays 3D instead of replacing it. */
		se_render_set_background_color(s_vec4(0.0f, 0.0f, 0.0f, 0.0f));
		se_scene_2d_render_to_buffer(game->hud_scene);
		se_render_set_background_color(scene_bg);
		se_scene_2d_render_to_screen(game->hud_scene, game->window);
	}
	rts_render_overlay(game);
	rts_render_health_labels(game);
	se_window_render_screen(game->window);
}

static void rts_seed_world(rts_game *game) {
	game->team_resources[RTS_TEAM_ALLY] = 320.0f;
	game->team_resources[RTS_TEAM_ENEMY] = 360.0f;
	game->build_block_reason[0] = '\0';

	const s_vec3 ally_hq_pos = s_vec3(-17.0f, 0.0f, 16.0f);
	const s_vec3 enemy_hq_pos = s_vec3(17.5f, 0.0f, -16.5f);
	const s_vec3 enemy_barracks_pos = s_vec3(13.0f, 0.0f, -12.5f);
	(void)rts_spawn_building(game, RTS_BUILDING_KIND_ALLY_HQ, &ally_hq_pos);
	(void)rts_spawn_building(game, RTS_BUILDING_KIND_ENEMY_HQ, &enemy_hq_pos);
	(void)rts_spawn_building(game, RTS_BUILDING_KIND_ENEMY_BARRACKS, &enemy_barracks_pos);

	const s_vec3 resource_positions[] = {
		s_vec3(-8.0f, 0.0f, 9.0f),
		s_vec3(-2.0f, 0.0f, 4.0f),
		s_vec3(1.5f, 0.0f, -0.5f),
		s_vec3(6.0f, 0.0f, -5.0f),
		s_vec3(11.5f, 0.0f, -10.0f),
		s_vec3(-12.0f, 0.0f, -3.0f),
		s_vec3(4.0f, 0.0f, 10.0f)
	};
	for (sz i = 0; i < sizeof(resource_positions) / sizeof(resource_positions[0]); ++i) {
		(void)rts_spawn_resource(game, &resource_positions[i], 520.0f + (f32)(i * 80));
	}

	for (i32 i = 0; i < 4; ++i) {
		s_vec3 pos = ally_hq_pos;
		pos.x += rts_randf(-2.3f, 2.3f);
		pos.z += rts_randf(-2.3f, 2.3f);
		(void)rts_spawn_unit(game, RTS_UNIT_KIND_ALLY_WORKER, &pos);
	}
	for (i32 i = 0; i < 2; ++i) {
		s_vec3 pos = ally_hq_pos;
		pos.x += rts_randf(-2.8f, 2.8f);
		pos.z += rts_randf(-2.8f, 2.8f);
		(void)rts_spawn_unit(game, RTS_UNIT_KIND_ALLY_SOLDIER, &pos);
	}
	for (i32 i = 0; i < 4; ++i) {
		s_vec3 pos = enemy_hq_pos;
		pos.x += rts_randf(-2.6f, 2.6f);
		pos.z += rts_randf(-2.6f, 2.6f);
		(void)rts_spawn_unit(game, RTS_UNIT_KIND_ENEMY_WORKER, &pos);
	}
	for (i32 i = 0; i < 4; ++i) {
		s_vec3 pos = enemy_hq_pos;
		pos.x += rts_randf(-3.0f, 3.0f);
		pos.z += rts_randf(-3.0f, 3.0f);
		(void)rts_spawn_unit(game, RTS_UNIT_KIND_ENEMY_SOLDIER, &pos);
	}

	game->camera_target = ally_hq_pos;
	game->camera_target.x += 1.8f;
	game->camera_target.z -= 1.6f;
	game->camera_yaw = RTS_CAMERA_DEFAULT_YAW;
	game->camera_pitch = RTS_CAMERA_DEFAULT_PITCH;
	game->camera_distance = RTS_CAMERA_DEFAULT_DISTANCE;
	if (!game->headless_mode && game->ctx && game->scene != S_HANDLE_NULL) {
		const se_camera_handle camera_handle = se_scene_3d_get_camera(game->scene);
		if (camera_handle != S_HANDLE_NULL) {
			se_camera *camera = se_camera_get(camera_handle);
			if (camera) {
				const s_vec3 camera_offset = s_vec3(
					sinf(game->camera_yaw) * cosf(game->camera_pitch) * game->camera_distance,
					sinf(game->camera_pitch) * game->camera_distance,
					cosf(game->camera_yaw) * cosf(game->camera_pitch) * game->camera_distance);
				camera->target = game->camera_target;
				camera->position = s_vec3_add(&camera->target, &camera_offset);
				camera->up = s_vec3(0.0f, 1.0f, 0.0f);
				se_camera_set_perspective(camera_handle, 52.0f, 0.1f, 220.0f);
				if (game->window != S_HANDLE_NULL) {
					se_camera_set_aspect_from_window(camera_handle, game->window);
				}
			}
		}
	}
	rts_log(
		"camera_seed yaw=%.2f pitch=%.2f dist=%.2f target=(%.2f %.2f)",
		game->camera_yaw,
		game->camera_pitch,
		game->camera_distance,
		game->camera_target.x,
		game->camera_target.z);

	rts_autotest_capture_baseline(game);
	rts_set_command_line(game, "RTS ready");
}

static int rts_run_headless_simulation(rts_game *game) {
	if (!game) {
		return 1;
	}
	game->headless_mode = true;
	if (!game->simulator.enabled) {
		game->simulator.enabled = true;
	}
	if (game->simulator.stop_after_seconds <= 0.0f) {
		game->simulator.stop_after_seconds = game->autotest.enabled ? 24.0f : 20.0f;
	}
	game->simulator.next_status_print = 1.0f;
	game->camera_target = s_vec3(0.0f, 0.0f, 0.0f);

	rts_reset_slots(game);
	rts_seed_world(game);
	if (game->autotest.enabled) {
		game->autotest.full_systems_required = false;
	}
	rts_log("running headless simulation for %.1fs", game->simulator.stop_after_seconds);

	const f32 dt = (game->autotest.enabled && game->autotest.fixed_dt > 0.0001f) ? game->autotest.fixed_dt : (1.0f / 60.0f);
	while (game->simulator.total_time < game->simulator.stop_after_seconds) {
		game->frame_index++;
		game->sim_time += dt;
		if (game->command_line_timer > 0.0f) {
			game->command_line_timer -= dt;
		}

		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_SIMULATOR);
		rts_run_input_simulator(game, dt);
		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_CAMERA);
		rts_update_camera(game, dt);
		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_PLAYER_INPUT);
		rts_handle_player_input(game);
		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_ENEMY_AI);
		rts_update_enemy_ai(game, dt);
		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_UNITS);
		rts_update_units(game, dt);
		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_NAVIGATION);
		rts_resolve_unit_navigation(game, dt);
		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_BUILDINGS);
		rts_update_buildings(game, dt);
		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_SELECTION);
		rts_recount_selection(game);
		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_VISUALS);
		rts_update_visuals(game);
		rts_autotest_mark_system(game, RTS_AUTOTEST_SYSTEM_BUILD_PREVIEW);
		rts_update_build_preview_visual(game);
		rts_update_minimap_ui(game);
		rts_render_frame(game);

		if ((game->frame_index % 30) == 0) {
			rts_validate_state(game);
			rts_autotest_check_progress(game);
		}
		if (game->autotest.enabled && game->validations_failed && game->autotest.strict_fail_fast) {
			break;
		}

		if (!game->autotest.enabled && (rts_team_defeated(game, RTS_TEAM_ENEMY) || rts_team_defeated(game, RTS_TEAM_ALLY))) {
			break;
		}
	}
	rts_autotest_finalize(game);

	const rts_building *ally_hq = rts_find_primary_building(game, RTS_TEAM_ALLY, RTS_BUILDING_TYPE_HQ);
	const rts_building *enemy_hq = rts_find_primary_building(game, RTS_TEAM_ENEMY, RTS_BUILDING_TYPE_HQ);
	rts_log(
		"hq_hp ally=%s%.1f enemy=%s%.1f",
		ally_hq ? "" : "NA/",
		ally_hq ? ally_hq->health : 0.0f,
		enemy_hq ? "" : "NA/",
		enemy_hq ? enemy_hq->health : 0.0f);

	rts_log("headless finished at t=%.2f, validations=%s", game->simulator.total_time, game->validations_failed ? "FAILED" : "OK");
	return game->validations_failed ? 2 : 0;
}

int main(int argc, char **argv) {
	rts_game game = {0};
	game.camera_target = s_vec3(0.0f, 0.0f, 0.0f);
	game.camera_yaw = RTS_CAMERA_DEFAULT_YAW;
	game.camera_pitch = RTS_CAMERA_DEFAULT_PITCH;
	game.camera_distance = 38.0f;
	game.show_debug_overlay = false;
	game.show_minimap_ui = true;
	game.track_system_timing = true;
	game.simulator.enabled = false;
	game.simulator.stop_after_seconds = 0.0f;
	game.simulator.next_status_print = 1.0f;
	game.autotest.enabled = false;
	game.autotest.strict_fail_fast = true;
	game.autotest.fixed_dt = 1.0f / 90.0f;
	game.autotest.sim_substeps_per_frame = 6;
	game.command_line[0] = '\0';
	game.next_system_timing_log_time = 1.0f;
	b8 headless_requested = false;
	b8 perf_check_enabled = false;
	b8 perf_assert_enabled = true;
	b8 perf_flag_explicit = false;
	b8 shader_hot_reload_enabled = false;
	f64 perf_fps_ema = 280.0;
	f64 perf_worst_ema = 10000.0;
	i32 perf_below_ema_frames = 0;
	const f64 perf_warmup_seconds = 2.0;
	f64 perf_next_log_time = perf_warmup_seconds;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--autotest") == 0) {
			game.simulator.enabled = true;
			game.autotest.enabled = true;
		}
		if (strcmp(argv[i], "--sim") == 0) {
			game.simulator.enabled = true;
		}
		if (strcmp(argv[i], "--headless") == 0) {
			headless_requested = true;
		}
		if (strcmp(argv[i], "--perf") == 0) {
			perf_flag_explicit = true;
			perf_check_enabled = true;
		}
		if (strcmp(argv[i], "--track-systems") == 0) {
			game.track_system_timing = true;
		}
		if (strcmp(argv[i], "--no-track-systems") == 0) {
			game.track_system_timing = false;
		}
		if (strcmp(argv[i], "--perf-no-assert") == 0) {
			perf_flag_explicit = true;
			perf_check_enabled = true;
			perf_assert_enabled = false;
		}
		if (strcmp(argv[i], "--hot-reload") == 0) {
			shader_hot_reload_enabled = true;
		}
		if (strncmp(argv[i], "--seconds=", 10) == 0) {
			game.simulator.enabled = true;
			game.simulator.stop_after_seconds = (f32)atof(argv[i] + 10);
		}
		if (strncmp(argv[i], "--autotest-steps=", 17) == 0) {
			game.autotest.sim_substeps_per_frame = s_max(1, atoi(argv[i] + 17));
		}
		if (strncmp(argv[i], "--autotest-dt=", 14) == 0) {
			const f32 parsed_dt = (f32)atof(argv[i] + 14);
			game.autotest.fixed_dt = rts_clampf(parsed_dt, 0.001f, 0.05f);
		}
	}
	const b8 terminal_render_active =
		rts_env_flag_enabled("SE_TERMINAL_RENDER") ||
		rts_env_flag_enabled("SE_TERMINAL_MIRROR") ||
		rts_env_flag_enabled("SE_WINDOW_TERMINAL");
	const b8 terminal_allow_logs = rts_terminal_allow_logs();
	if (terminal_render_active && !terminal_allow_logs) {
		// In terminal render mode, periodic stdout logs cause visible scene jitter.
		game.track_system_timing = false;
		game.simulator.next_status_print = 999999.0f;
	}
	if (game.autotest.enabled) {
		game.autotest.full_systems_required = !headless_requested;
		if (!perf_flag_explicit) {
			perf_check_enabled = false;
		}
	}
	if (game.autotest.enabled && game.simulator.stop_after_seconds <= 0.0f) {
		game.simulator.stop_after_seconds = 24.0f;
	}
	if (!game.autotest.enabled && game.simulator.enabled && game.simulator.stop_after_seconds <= 0.0f) {
		game.simulator.stop_after_seconds = 40.0f;
	}

	srand(42);

	if (headless_requested) {
		return rts_run_headless_simulation(&game);
	}

	game.ctx = se_context_create();
	if (!game.ctx) {
		fprintf(stderr, RTS_LOG_PREFIX "failed to create context\n");
		return 1;
	}

	game.window = se_window_create("Syphax-Engine - 99 RTS 3D", RTS_WINDOW_WIDTH, RTS_WINDOW_HEIGHT);
	if (game.window == S_HANDLE_NULL) {
		fprintf(stderr, RTS_LOG_PREFIX "failed to create window\n");
		if (game.simulator.enabled) {
			rts_log("falling back to headless simulation");
			rts_shutdown_runtime(&game);
			return rts_run_headless_simulation(&game);
		}
		rts_shutdown_runtime(&game);
		return 1;
	}
	if (!se_render_has_context()) {
		fprintf(stderr, RTS_LOG_PREFIX "no graphics context available\n");
		if (game.simulator.enabled) {
			rts_log("falling back to headless simulation");
			rts_shutdown_runtime(&game);
			return rts_run_headless_simulation(&game);
		}
		rts_shutdown_runtime(&game);
		return 1;
	}

	se_window_set_exit_key(game.window, SE_KEY_ESCAPE);
	se_window_set_vsync(game.window, false);
	se_window_set_target_fps(game.window, 240);
	se_render_set_background_color(s_vec4(0.045f, 0.053f, 0.058f, 1.0f));
	se_debug_set_overlay_enabled(false);
	if (perf_check_enabled) {
		rts_log("perf gate enabled (target=280, assert<260 after warmup)");
	}

	game.text_handle = se_text_handle_create(0);
	if (!game.text_handle) {
		fprintf(stderr, RTS_LOG_PREFIX "failed to create text handle\n");
		if (game.simulator.enabled) {
			rts_log("falling back to headless simulation");
			rts_shutdown_runtime(&game);
			return rts_run_headless_simulation(&game);
		}
		rts_shutdown_runtime(&game);
		return 1;
	}
	game.font = se_font_load(game.text_handle, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 26.0f);
	if (game.font == S_HANDLE_NULL) {
		fprintf(stderr, RTS_LOG_PREFIX "failed to load font\n");
		if (game.simulator.enabled) {
			rts_log("falling back to headless simulation");
			rts_shutdown_runtime(&game);
			return rts_run_headless_simulation(&game);
		}
		rts_shutdown_runtime(&game);
		return 1;
	}

	rts_reset_slots(&game);
	if (!rts_init_rendering(&game)) {
		fprintf(stderr, RTS_LOG_PREFIX "failed to initialize rendering assets\n");
		if (game.simulator.enabled) {
			rts_log("falling back to headless simulation");
			rts_shutdown_runtime(&game);
			return rts_run_headless_simulation(&game);
		}
		rts_shutdown_runtime(&game);
		return 1;
	}
	if (!rts_init_hud_scene(&game)) {
		fprintf(stderr, RTS_LOG_PREFIX "failed to initialize HUD\n");
		if (game.simulator.enabled) {
			rts_log("falling back to headless simulation");
			rts_shutdown_runtime(&game);
			return rts_run_headless_simulation(&game);
		}
		rts_shutdown_runtime(&game);
		return 1;
	}

	rts_seed_world(&game);
	rts_update_camera(&game, 0.016f);
	rts_update_visuals(&game);
	rts_update_build_preview_visual(&game);

	rts_log("started (simulator=%s, stop_after=%.1fs)", game.simulator.enabled ? "on" : "off", game.simulator.stop_after_seconds);
	f64 shader_reload_accumulator = 0.0;

	while (!se_window_should_close(game.window)) {
		se_window_tick(game.window);

		f32 frame_dt = (f32)se_window_get_delta_time(game.window);
		frame_dt = rts_clampf(frame_dt, 0.0005f, 0.05f);
		const i32 sim_steps = (game.autotest.enabled && game.simulator.enabled) ? s_max(1, game.autotest.sim_substeps_per_frame) : 1;
		const f32 sim_dt =
			(game.autotest.enabled && game.simulator.enabled && game.autotest.fixed_dt > 0.0001f)
				? game.autotest.fixed_dt
				: frame_dt;

		for (i32 step = 0; step < sim_steps && !se_window_should_close(game.window); ++step) {
			game.frame_index++;
			game.sim_time += sim_dt;
			if (game.command_line_timer > 0.0f) {
				game.command_line_timer -= sim_dt;
			}
			se_uniform_set_float(se_context_get_global_uniforms(), "u_time", game.sim_time);
			if (shader_hot_reload_enabled) {
				shader_reload_accumulator += (f64)sim_dt;
				if (shader_reload_accumulator >= 0.35) {
					se_context_reload_changed_shaders();
					shader_reload_accumulator = 0.0;
				}
			}

			rts_autotest_mark_system(&game, RTS_AUTOTEST_SYSTEM_SIMULATOR);
			rts_run_input_simulator(&game, sim_dt);
			rts_autotest_mark_system(&game, RTS_AUTOTEST_SYSTEM_CAMERA);
			rts_update_camera(&game, sim_dt);
			rts_autotest_mark_system(&game, RTS_AUTOTEST_SYSTEM_PLAYER_INPUT);
			rts_handle_player_input(&game);
			rts_autotest_mark_system(&game, RTS_AUTOTEST_SYSTEM_ENEMY_AI);
			rts_update_enemy_ai(&game, sim_dt);
			rts_autotest_mark_system(&game, RTS_AUTOTEST_SYSTEM_UNITS);
			rts_update_units(&game, sim_dt);
			rts_autotest_mark_system(&game, RTS_AUTOTEST_SYSTEM_NAVIGATION);
			rts_resolve_unit_navigation(&game, sim_dt);
			rts_autotest_mark_system(&game, RTS_AUTOTEST_SYSTEM_BUILDINGS);
			rts_update_buildings(&game, sim_dt);
			rts_autotest_mark_system(&game, RTS_AUTOTEST_SYSTEM_SELECTION);
			rts_recount_selection(&game);
			rts_autotest_mark_system(&game, RTS_AUTOTEST_SYSTEM_VISUALS);
			rts_update_visuals(&game);
			rts_autotest_mark_system(&game, RTS_AUTOTEST_SYSTEM_BUILD_PREVIEW);
			rts_update_build_preview_visual(&game);

			if ((game.frame_index % 30) == 0) {
				rts_validate_state(&game);
				rts_autotest_check_progress(&game);
			}

			if (game.match_result == 0) {
				if (rts_team_defeated(&game, RTS_TEAM_ENEMY)) {
					game.match_result = 1;
					rts_set_command_line(&game, "Victory");
					if (game.simulator.enabled && !game.autotest.enabled) {
						se_window_set_should_close(game.window, true);
					}
				} else if (rts_team_defeated(&game, RTS_TEAM_ALLY)) {
					game.match_result = -1;
					rts_set_command_line(&game, "Defeat");
					if (game.simulator.enabled && !game.autotest.enabled) {
						se_window_set_should_close(game.window, true);
					}
				}
			}

			if (game.autotest.enabled && game.validations_failed && game.autotest.strict_fail_fast) {
				se_window_set_should_close(game.window, true);
				break;
			}
		}

		rts_render_frame(&game);
		if (game.track_system_timing && game.sim_time >= game.next_system_timing_log_time) {
			game.next_system_timing_log_time = game.sim_time + 1.0f;
			c8 timing_lines[512] = {0};
			se_debug_dump_last_frame_timing_lines(timing_lines, sizeof(timing_lines));
			rts_log("systems ms:\n%s", timing_lines);
		}

		if (perf_check_enabled && game.sim_time >= perf_warmup_seconds) {
			const f64 fps = se_window_get_fps(game.window);
			perf_fps_ema = perf_fps_ema * 0.92 + fps * 0.08;
			if (perf_fps_ema < perf_worst_ema) {
				perf_worst_ema = perf_fps_ema;
			}

			se_debug_frame_timing timing = {0};
			(void)se_debug_get_last_frame_timing(&timing);
			const c8 *bottleneck_name = "none";
			f64 bottleneck_ms = 0.0;
			rts_perf_find_bottleneck(&timing, &bottleneck_name, &bottleneck_ms);

			if (perf_fps_ema < 260.0) {
				perf_below_ema_frames++;
				if (game.sim_time >= perf_next_log_time) {
					perf_next_log_time = game.sim_time + 0.5;
					c8 timing_text[320] = {0};
					se_debug_dump_last_frame_timing(timing_text, sizeof(timing_text));
					rts_log(
						"perf low: fps=%.2f ema=%.2f worst=%.2f bottleneck=%s %.3fms %s",
						fps,
						perf_fps_ema,
						perf_worst_ema,
						bottleneck_name,
						bottleneck_ms,
						timing_text);
				}
			} else {
				perf_below_ema_frames = 0;
			}

			if (perf_assert_enabled) {
				s_assertf(
					perf_below_ema_frames < 90,
					"99_game perf assertion failed: fps ema %.2f < 260 (target 280), bottleneck=%s %.3fms",
					perf_fps_ema,
					bottleneck_name,
					bottleneck_ms);
			}
		}
		if (game.autotest.enabled && game.validations_failed && game.autotest.strict_fail_fast) {
			break;
		}
	}

	rts_autotest_finalize(&game);
	if (perf_check_enabled) {
		rts_log("perf summary: worst ema fps=%.2f", perf_worst_ema);
	}
	rts_log("finished, validations=%s", game.validations_failed ? "FAILED" : "OK");

	rts_shutdown_runtime(&game);
	return game.validations_failed ? 2 : 0;
}
