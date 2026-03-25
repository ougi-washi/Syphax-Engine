// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_scene.h"
#include "se_sdf.h"
#include "se_graphics.h"
#include "se_camera.h"
#include "se_audio.h"
#include "se_debug.h"
#include "se_texture.h"

#define MAX_BUILDINGS 	1024 * 1024
#define MAX_UNITS 		1024 * 1024
#define LANDSCAPE_SIZE	1024

typedef enum building_type_t {
    BUILDING_RESIDENCE_T0,
    BUILDING_RESIDENCE_T1,
    BUILDING_RESIDENCE_T2,
    BUILDING_RESIDENCE_T3,
    BUILDING_TEMPLE_T0,
    BUILDING_TEMPLE_T1,
    BUILDING_TEMPLE_T2,
    BUILDING_TEMPLE_T3,
    BUILDING_MARKET_T0,
    BUILDING_MARKET_T1,
    BUILDING_MARKET_T2,
    BUILDING_MARKET_T3,
} building_type_t;

typedef enum resource_t {
	RESOURCE_WOOD,
	RESOURCE_STONE,
	RESOURCE_COAL,
	RESOURCE_COPPER,
	RESOURCE_IRON,
	RESOURCE_GOLD,
	RESOURCE_SILVER,
	RESOURCE_DIAMOND,
} resource_t;

typedef struct building_data_t {
	building_type_t type;
} building_data_t;

typedef struct unit_t {
    se_instance_id id;
} unit_t;

typedef struct world_t {
    se_camera_handle camera;
    se_sdf_handle sdf_handle;
    se_sdf_renderer_handle sdf_renderer;
    se_input_handle* input;
} world_t;

typedef struct ui_t {
    se_ui_handle tooltip;
    se_ui_handle building_panel;
    se_ui_handle unit_panel;
} ui_t;

void move_camera_forward(void *user_data) {
    se_camera_handle camera = (se_camera_handle)user_data;
    const f32 speed = 1.0f;
    se_camera_add_location(camera, &s_vec3(0.0f, 0.0f, speed));
}

void move_camera_left(void *user_data) {
    se_camera_handle camera = (se_camera_handle)user_data;
    const f32 speed = 1.0f;
    se_camera_add_location(camera, &s_vec3(-speed, 0.0f, 0.0f));
}

void move_camera_backward(void *user_data) {
    se_camera_handle camera = (se_camera_handle)user_data;
    const f32 speed = 1.0f;
    se_camera_add_location(camera, &s_vec3(0.0f, 0.0f, -speed));
}

void move_camera_right(void *user_data) {
    se_camera_handle camera = (se_camera_handle)user_data;
    const f32 speed = 1.0f;
    se_camera_add_location(camera, &s_vec3(speed, 0.0f, 0.0f));
}

void bind_camera(se_camera_handle camera, se_input_handle *input) {
    se_input_bind(input, SE_KEY_W, SE_INPUT_DOWN, &move_camera_forward, &camera); 
    se_input_bind(input, SE_KEY_A, SE_INPUT_DOWN, &move_camera_left, &camera); 
    se_input_bind(input, SE_KEY_S, SE_INPUT_DOWN, &move_camera_backward, &camera);
    se_input_bind(input, SE_KEY_D, SE_INPUT_DOWN, &move_camera_right, &camera); 
}

void init_world(world_t *world, se_window_handle window) {
    
    world->camera = se_camera_create(); 
	se_camera_set_location(world->camera, &s_vec3(.0f, 5.0f, -1.f));
	se_camera_set_target(world->camera, &s_vec3(0.0f, 0.0f, 0.0f));
    world->input = se_input_create(window, 128);
    bind_camera(world->camera, world->input);

    world->sdf_handle = se_sdf_create(NULL);
	se_sdf_node_primitive_desc sphere_node_desc = {
		.transform = s_mat4_identity,
		.operation = SE_SDF_OP_UNION,
		.primitive = {
			.type = SE_SDF_PRIMITIVE_SPHERE,
			.sphere = {
				.radius = 1. 
			}
		}
	};
	se_sdf_node_handle sphere_node_handle = se_sdf_node_create_primitive(world->sdf_handle, &sphere_node_desc);
    se_sdf_set_root(world->sdf_handle, sphere_node_handle);
	
    se_sdf_renderer_set_sdf(world->sdf_renderer, world->sdf_handle);
	{
		se_sdf_raymarch_quality quality = SE_SDF_RAYMARCH_QUALITY_DEFAULTS;
		quality.max_steps = 56;
		quality.hit_epsilon = 0.0025f;
		quality.enable_shadows = 0;
		quality.shadow_steps = 0;
		quality.shadow_strength = 0.0f;
		se_sdf_renderer_set_quality(world->sdf_renderer, &quality);
	}
	(void)se_sdf_renderer_set_light_rig(
		world->sdf_renderer,
		&s_vec3(-0.55f, -0.85f, -0.20f),
		&s_vec3(1.0f, 0.98f, 0.94f),
		&s_vec3(0.09f, 0.12f, 0.16f),
		0.03f
	);
    se_sdf_prepare_desc prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
    prepare_desc.lod_count = 1u;
    prepare_desc.lod_resolutions[0] = 4u;
    se_sdf_prepare_async(world->sdf_handle, NULL);
}

void render_world(world_t *world, se_window_handle window) {
	se_debug_trace_begin("render_world");

    // set frame desc
	u32 width = 0u;
	u32 height = 0u;
	se_window_get_framebuffer_size(window, &width, &height);
	se_sdf_frame_desc frame_desc = SE_SDF_FRAME_DESC_DEFAULTS;
	frame_desc.camera = world->camera;
	frame_desc.resolution = s_vec2((f32)s_max(width, 1u), (f32)s_max(height, 1u));
	frame_desc.time_seconds = (f32)se_window_get_time(window);
    
    // render
    se_sdf_renderer_render(world->sdf_renderer, &frame_desc); 

	se_debug_trace_end("render_world");
}

void render_ui(se_window_handle window) {
    se_debug_trace_begin("render_ui");

    se_debug_trace_end("render_ui");
}

i32 main() {
	se_context *ctx = se_context_create();
    se_window_handle window = se_window_create("Syphax - Civilization", 1280, 720);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);

    world_t world = {0};
    init_world(&world, window);

    se_window_loop(window, 
        se_render_clear();
	    se_render_set_background(s_vec4(0.0f, 0.0f, 0.0f, 0.0f));
        render_world(&world, window);
		render_ui(window);
        se_debug_render_overlay(window, NULL);
    );

    se_context_destroy(ctx);
}
