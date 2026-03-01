// Syphax-Engine - Ougi Washi

#include "se_graphics.h"
#include "se_scene.h"
#include "se_sdf.h"
#include "se_window.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static b8 sdf_json_save(const se_sdf_scene_handle scene, const c8* path) {
	s_json* root = se_sdf_to_json(scene);
	if (!root) {
		return 0;
	}
	c8* text = s_json_stringify(root);
	s_json_free(root);
	if (!text) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return 0;
	}
	const b8 ok = s_file_write(path, text, strlen(text));
	free(text);
	if (!ok) {
		se_set_last_error(SE_RESULT_IO);
	}
	return ok;
}

static b8 sdf_json_load(const se_sdf_scene_handle scene, const c8* path) {
	return se_sdf_from_json_file(scene, path);
}

int main(void) {
	const c8* json_path = "sdf_snapshot.json";
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - SDF JSON", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("sdf_json :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.03f, 0.04f, 0.06f, 1.0f));

	se_sdf_scene_handle source_scene = se_sdf_scene_create(NULL);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	root_desc.operation_amount = 0.35f;
	se_sdf_node_handle root = se_sdf_node_create_group(source_scene, &root_desc);
	se_sdf_scene_set_root(source_scene, root);

	se_sdf_node_primitive_desc box_desc = {0};
	box_desc.transform = s_mat4_identity;
	box_desc.operation = SE_SDF_OP_UNION;
	box_desc.primitive.type = SE_SDF_PRIMITIVE_BOX;
	box_desc.primitive.box.size = s_vec3(1.35f, 1.35f, 1.35f);

	se_sdf_node_primitive_desc sphere_desc = {0};
	sphere_desc.transform = s_mat4_identity;
	sphere_desc.operation = SE_SDF_OP_UNION;
	sphere_desc.primitive.type = SE_SDF_PRIMITIVE_SPHERE;
	sphere_desc.primitive.sphere.radius = 1.00f;

	se_sdf_node_handle box = se_sdf_node_create_primitive(source_scene, &box_desc);
	se_sdf_node_handle sphere = se_sdf_node_create_primitive(source_scene, &sphere_desc);
	se_sdf_node_add_child(source_scene, root, box);
	se_sdf_node_add_child(source_scene, root, sphere);

	if (!sdf_json_save(source_scene, json_path)) {
		printf("sdf_json :: save failed (%s)\n", se_result_str(se_get_last_error()));
		se_sdf_scene_destroy(source_scene);
		se_context_destroy(context);
		return 1;
	}

	se_sdf_scene_handle loaded_scene = se_sdf_scene_create(NULL);
	if (!sdf_json_load(loaded_scene, json_path)) {
		printf("sdf_json :: load failed (%s)\n", se_result_str(se_get_last_error()));
		se_sdf_scene_destroy(loaded_scene);
		se_sdf_scene_destroy(source_scene);
		se_context_destroy(context);
		return 1;
	}
	se_sdf_scene_destroy(source_scene);

	u32 initial_fb_w = 0;
	u32 initial_fb_h = 0;
	se_window_get_framebuffer_size(window, &initial_fb_w, &initial_fb_h);
	const f32 initial_width = initial_fb_w > 0 ? (f32)initial_fb_w : 1280.0f;
	const f32 initial_height = initial_fb_h > 0 ? (f32)initial_fb_h : 720.0f;
	se_scene_2d_handle scene_2d = se_scene_2d_create(&s_vec2(initial_width, initial_height), 1);
	if (scene_2d == S_HANDLE_NULL) {
		printf("sdf_json :: scene_2d create failed (%s)\n", se_result_str(se_get_last_error()));
		se_sdf_scene_destroy(loaded_scene);
		se_context_destroy(context);
		return 1;
	}
	se_scene_2d_set_auto_resize(scene_2d, window, &s_vec2(1.0f, 1.0f));

	se_object_2d_handle sdf_object = se_sdf_scene_to_object_2d(loaded_scene);
	if (sdf_object == S_HANDLE_NULL) {
		printf("sdf_json :: se_sdf_scene_to_object_2d failed (%s)\n", se_result_str(se_get_last_error()));
		se_scene_2d_destroy(scene_2d);
		se_sdf_scene_destroy(loaded_scene);
		se_context_destroy(context);
		return 1;
	}
	se_scene_2d_add_object(scene_2d, sdf_object);

	printf("sdf_json :: saved and loaded %s\n", json_path);
	printf("sdf_json controls:\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_scene_2d_draw(scene_2d, window);
		se_window_end_frame(window);
	}

	se_scene_2d_destroy_full(scene_2d, 0);
	se_sdf_scene_destroy(loaded_scene);
	se_sdf_shutdown();
	se_context_destroy(context);
	return 0;
}
