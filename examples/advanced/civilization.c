// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_scene.h"
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
    se_scene_3d_handle scene;

    se_object_3d_handle landscape;
    se_object_3d_handle buildings;
    se_object_3d_handle units;
	
	se_model_handle landscape_model;
    se_model_handle building_model;
    se_model_handle unit_model;

    se_input_handle* input;
} world_t;

se_object_3d_handle create_object(world_t *world, const se_model_handle model, const sz max_instances) {
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

se_object_3d_handle generate_landscape_sdf(const c8* heightmap_texture, const f32 scale, const f32 height) {
	if (!heightmap_texture || heightmap_texture[0] == '\0' || scale <= 0.0f || height <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	const se_model_handle model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/plane.obj"),
		SE_RESOURCE_EXAMPLE("civilization/landscape_vs.glsl"),
		SE_RESOURCE_EXAMPLE("civilization/landscape_fs.glsl"));
	if (model == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}

	const se_texture_handle heightmap = se_texture_load(heightmap_texture, SE_CLAMP);
	if (heightmap == S_HANDLE_NULL) {
		se_model_destroy(model);
		return S_HANDLE_NULL;
	}

	se_context* context = se_current_context();
	se_texture* heightmap_ptr = context ? s_array_get(&context->textures, heightmap) : NULL;
	if (!heightmap_ptr) {
		se_texture_destroy(heightmap);
		se_model_destroy(model);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}

	const sz mesh_count = se_model_get_mesh_count(model);
	for (sz i = 0; i < mesh_count; ++i) {
		const se_shader_handle shader = se_model_get_mesh_shader(model, i);
		if (shader == S_HANDLE_NULL) {
			continue;
		}
		se_shader_set_texture(shader, "u_heightmap", heightmap_ptr->id);
		se_shader_set_float(shader, "u_landscape_scale", scale);
		se_shader_set_float(shader, "u_landscape_height", height);
	}

	const s_mat4 transform = s_mat4_identity;
	const se_object_3d_handle object = se_object_3d_create(model, &transform, 0);
	if (object == S_HANDLE_NULL) {
		se_texture_destroy(heightmap);
		se_model_destroy(model);
		return S_HANDLE_NULL;
	}
	return object;
}

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
    
    world->input = se_input_create(window, 128);
    bind_camera(camera, world->input);
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
