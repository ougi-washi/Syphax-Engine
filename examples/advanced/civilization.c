// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_scene.h"
#include "se_graphics.h"
#include "se_camera.h"
#include "se_audio.h"
#include "se_debug.h"

#define MAX_BUILDINGS 1024 * 1024
#define MAX_CHARACTERS 1024 * 1024

typedef struct landscape_t {
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

typedef union building_data_t {
    struct { building_type_t type;};
    struct { u16 resource_amount;};
} building_data_t;

typedef struct building_t {
    se_instance_id id;
    building_type_t type;
} building_t;

typedef struct character_t {
    se_instance_id id;
} character_t;

typedef struct world_t {
    se_scene_3d_handle scene;
    landscape_t landscape;
    se_object_3d_handle buildings;
    se_object_3d_handle characters;
} world_t;

typedef struct ui_t {
    se_ui_handle tooltip;
    se_ui_handle building_panel;
    se_ui_handle character_panel;
} ui_t;

void init_world(world_t *world, se_window_handle window) {
    world->scene = se_scene_3d_create_for_window(window, 128);
    world->landscape.object = se_object_3d_create(world->scene, &s_mat4_identity, 1);
    world->characters = se_object_3d_create(world->scene, &s_mat4_identity, MAX_CHARACTERS);
    world->buildings = se_object_3d_create(world->scene, &s_mat4_identity, MAX_BUILDINGS);
}

se_instance_id add_building(world_t *world, const s_vec3* position, const building_type_t type) {
    s_mat4 transform = s_mat4_identity;
    transform = s_mat4_translate(&transform, position);
    return se_object_3d_add_instance(world->buildings, &transform, &(s_mat4){0});
}

void render_world(world_t *world, se_window_handle window) {
    se_debug_trace_begin("render_world");

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

    se_window_destroy(window);
    se_context_destroy(ctx);
}

