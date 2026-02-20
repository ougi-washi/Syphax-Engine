// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_scene.h"
#include "se_graphics.h"
#include "se_camera.h"
#include "se_audio.h"
#include "se_debug.h"

#define MAX_BUILDINGS 	1024 * 1024
#define MAX_UNITS 		1024 * 1024

typedef struct landscape_t {
    se_model_handle model;
    se_object_3d_handle object;
} landscape_t;

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

typedef struct building_t {
    se_instance_id id;
    building_type_t type;
} building_t;

typedef struct unit_t {
    se_instance_id id;
} unit_t;

static se_model_handle load_model(const c8 *landscape_path, const c8 *fs_path) {
    se_model_handle model = se_model_load_obj_simple(landscape_path, SE_RESOURCE_PUBLIC("shaders/scene_3d_vertex.glsl"), fs_path);
    if (model == S_HANDLE_NULL) {
        se_log("civilization :: failed to load model %s", landscape_path);
    }
    return model;
}

typedef struct world_t {
    se_scene_3d_handle scene;
    landscape_t landscape;
    se_object_3d_handle buildings;
    se_object_3d_handle units;
    se_model_handle building_model;
    se_model_handle unit_model;
} world_t;

static se_object_3d_handle create_object(world_t *world, const se_model_handle model, const sz max_instances) {
    if (!world || world->scene == S_HANDLE_NULL || model == S_HANDLE_NULL) {
        return S_HANDLE_NULL;
    }
    const s_mat4 transform = s_mat4_identity;
    se_object_3d_handle object = se_object_3d_create(model, &transform, max_instances);
    if (object != S_HANDLE_NULL) {
        se_scene_3d_add_object(world->scene, object);
    }
    return object;
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
    world->landscape.model = load_model(SE_RESOURCE_PUBLIC("models/cube.obj"), SE_RESOURCE_PUBLIC("shaders/scene_3d_fragment.glsl"));
    world->building_model = load_model(SE_RESOURCE_PUBLIC("models/cube.obj"), SE_RESOURCE_PUBLIC("shaders/scene_3d_fragment.glsl"));
    world->unit_model = load_model(SE_RESOURCE_PUBLIC("models/sphere.obj"), SE_RESOURCE_PUBLIC("shaders/scene_3d_fragment.glsl"));
    world->landscape.object = create_object(world, world->landscape.model, 1);
    world->buildings = create_object(world, world->building_model, MAX_BUILDINGS);
    world->units = create_object(world, world->unit_model, MAX_UNITS);

    //if (world->landscape.object != S_HANDLE_NULL) {
    //    s_mat4 transform = s_mat4_identity;
    //    s_mat4_set_scale(&transform, &s_vec3(36.0f, 0.2f, 36.0f));
    //    s_mat4_set_translation(&transform, &s_vec3(0.0f, -1.0f, 0.0f));
    //    add_instance_safe(world->landscape.object, &transform);
    //}

    if (world->scene != S_HANDLE_NULL) {
        se_camera_handle camera = se_scene_3d_get_camera(world->scene);
        if (camera != S_HANDLE_NULL) {
            const s_vec3 pivot = s_vec3(0.0f, 0.5f, 0.0f);
            se_camera_set_orbit_defaults(camera, window, &pivot, 18.0f);
        }
    }
}

void render_world(world_t *world, se_window_handle window) {
	se_debug_trace_begin("render_world");

	if (world->scene == S_HANDLE_NULL) {
		se_debug_trace_end("render_world");
		return;
	}

	se_camera_handle camera = se_scene_3d_get_camera(world->scene);
	if (camera != S_HANDLE_NULL) {
		se_camera_set_aspect_from_window(camera, window);
	}
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

    world_t world = {0};
    init_world(&world, window);

    se_window_loop(window, 
        se_render_clear();
	    se_render_set_background_color(s_vec4(0.0f, 0.0f, 0.0f, 0.0f));
        render_world(&world, window);
        se_debug_render_overlay(window, NULL);
    );

    if (world.scene != S_HANDLE_NULL) {
        se_scene_3d_destroy_full(world.scene, true, true);
    }

    se_context_destroy(ctx);
}
