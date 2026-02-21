// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_scene.h"
#include "se_graphics.h"
#include "se_camera.h"
#include "se_audio.h"
#include "se_debug.h"

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
    se_scene_3d_handle scene;

    se_object_3d_handle landscape;
    se_object_3d_handle buildings;
    se_object_3d_handle units;
	
	se_model_handle landscape_model;
    se_model_handle building_model;
    se_model_handle unit_model;
} world_t;

static se_object_3d_handle create_object(world_t *world, const se_model_handle model, const sz max_instances) {
    if (!world || world->scene == S_HANDLE_NULL || model == S_HANDLE_NULL) {
        return S_HANDLE_NULL;
    }
    const s_mat4 transform = s_mat4_identity;
    return se_object_3d_create(model, &transform, max_instances);
}

typedef struct ui_t {
    se_ui_handle tooltip;
    se_ui_handle building_panel;
    se_ui_handle unit_panel;
} ui_t;

void init_world(world_t *world, se_window_handle window) {
    world->scene = se_scene_3d_create_for_window(window, 128);
    if (world->scene == S_HANDLE_NULL) {
        se_log("civilization :: failed to create 3D scene");
        return;
    }

	world->landscape_model = se_model_load_obj_simple(	SE_RESOURCE_PUBLIC("models/plane.obj"), 
														SE_RESOURCE_EXAMPLE("civilization/landscape_vs.glsl"), 
														SE_RESOURCE_EXAMPLE("civilization/landscape_fs.glsl"));
	world->building_model = se_model_load_obj_simple(	SE_RESOURCE_PUBLIC("models/cube.obj"),
														SE_RESOURCE_EXAMPLE("civilization/building_vs.glsl"),
														SE_RESOURCE_EXAMPLE("civilization/building_fs.glsl"));
	world->unit_model = se_model_load_obj_simple(		SE_RESOURCE_PUBLIC("models/sphere.obj"),
														SE_RESOURCE_EXAMPLE("civilization/unit_vs.glsl"),
														SE_RESOURCE_EXAMPLE("civilization/unit_fs.glsl"));

	world->landscape = create_object(world, world->landscape_model, 0);
    world->buildings = create_object(world, world->building_model, MAX_BUILDINGS);
    world->units = create_object(world, world->unit_model, MAX_UNITS);

	se_scene_3d_handle scene = world->scene;

	se_scene_3d_add_object(scene, world->landscape);
	se_scene_3d_add_object(scene, world->buildings);
	se_scene_3d_add_object(scene, world->units);

	se_object_3d_set_scale(world->landscape, &s_vec3(LANDSCAPE_SIZE, LANDSCAPE_SIZE, LANDSCAPE_SIZE));

	se_camera_handle camera = se_scene_3d_get_camera(scene);
	se_camera_set_location(camera, &s_vec3(.0f, 5.0f, -1.f));
	se_camera_set_target(camera, &s_vec3(0.0f, 0.0f, 0.0f));
}

void render_world(world_t *world, se_window_handle window) {
	se_debug_trace_begin("render_world");

	se_scene_3d_draw(world->scene, window);

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
	    se_render_set_background_color(s_vec4(0.0f, 0.0f, 0.0f, 0.0f));
        render_world(&world, window);
		render_ui(window);
        se_debug_render_overlay(window, NULL);
    );

    if (world.scene != S_HANDLE_NULL) {
        se_scene_3d_destroy_full(world.scene, true, true);
    }

    se_context_destroy(ctx);
}
