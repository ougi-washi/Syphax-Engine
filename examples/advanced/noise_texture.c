// Syphax-Engine - Ougi Washi

#include "se_graphics.h"
#include "se_noise.h"
#include "se_scene.h"
#include "se_shader.h"
#include "se_texture.h"
#include "se_window.h"
#include "se_debug.h"

#include <stdio.h>

#define NOISE_TEXTURE_WINDOW_WIDTH 1024
#define NOISE_TEXTURE_WINDOW_HEIGHT 768
#define NOISE_TEXTURE_COUNT 4u

typedef struct {
	const c8* label;
	se_noise_2d params;
	s_vec2 position;
} se_noise_preview;

static i32 se_noise_expect_texture(const c8* label, const se_texture_handle texture) {
	if (texture != S_HANDLE_NULL) {
		return 0;
	}
	fprintf(stderr, "advanced/noise_texture :: failed to create %s (%s)\n", label, se_result_str(se_get_last_error()));
	return 1;
}

static i32 se_noise_expect_sample(const c8* label, const se_texture_handle texture, const s_vec2* uv) {
	s_vec3 color = s_vec3(0.0f, 0.0f, 0.0f);
	if (se_texture_sample_rgb(texture, uv, &color)) {
		printf("%s sample at (%.2f, %.2f): %.3f\n", label, uv->x, uv->y, color.x);
		return 0;
	}
	fprintf(stderr, "advanced/noise_texture :: failed to sample %s (%s)\n", label, se_result_str(se_get_last_error()));
	return 1;
}

static u32 se_noise_texture_id(se_context* context, const se_texture_handle texture) {
	se_texture* texture_ptr = NULL;
	if (!context || texture == S_HANDLE_NULL) {
		return 0u;
	}
	texture_ptr = s_array_get(&context->textures, texture);
	return texture_ptr ? texture_ptr->id : 0u;
}

static i32 se_noise_preview_bind_texture(
	se_context* context,
	const se_object_2d_handle object,
	const se_texture_handle texture) {
	const se_shader_handle shader = se_object_2d_get_shader(object);
	const u32 texture_id = se_noise_texture_id(context, texture);
	if (shader == S_HANDLE_NULL || texture_id == 0u) {
		fprintf(stderr, "advanced/noise_texture :: failed to bind preview texture\n");
		return 1;
	}
	se_shader_set_texture(shader, "u_texture", texture_id);
	return 0;
}

i32 main(void) {
	printf("advanced/noise_texture :: Advanced example (reference)\n");

	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create(
		"Syphax-Engine - Noise Texture",
		NOISE_TEXTURE_WINDOW_WIDTH,
		NOISE_TEXTURE_WINDOW_HEIGHT);
	if (window == S_HANDLE_NULL) {
		fprintf(stderr, "advanced/noise_texture :: failed to create window (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background(s_vec4(0.08f, 0.09f, 0.11f, 1.0f));

	const s_vec2 sample_a = s_vec2(0.25f, 0.25f);
	const s_vec2 sample_b = s_vec2(0.73f, 0.61f);
	const s_vec2 tile_scale = s_vec2(0.38f, 0.38f);
	const se_noise_preview previews[NOISE_TEXTURE_COUNT] = {
		{
			.label = "simplex",
			.params = { .type = SE_NOISE_SIMPLEX, .frequency = 3.0f, .offset = s_vec2(0.10f, 0.15f), .scale = s_vec2(1.0f, 1.0f), .seed = 11u },
			.position = s_vec2(-0.42f, 0.42f)
		},
		{
			.label = "perlin",
			.params = { .type = SE_NOISE_PERLIN, .frequency = 4.0f, .offset = s_vec2(0.35f, 0.20f), .scale = s_vec2(1.5f, 0.75f), .seed = 23u },
			.position = s_vec2(0.42f, 0.42f)
		},
		{
			.label = "vornoi",
			.params = { .type = SE_NOISE_VORNOI, .frequency = 6.0f, .offset = s_vec2(0.0f, 0.0f), .scale = s_vec2(1.0f, 1.0f), .seed = 37u },
			.position = s_vec2(-0.42f, -0.42f)
		},
		{
			.label = "worley",
			.params = { .type = SE_NOISE_WORLEY, .frequency = 6.0f, .offset = s_vec2(0.2f, 0.4f), .scale = s_vec2(1.0f, 1.0f), .seed = 41u },
			.position = s_vec2(0.42f, -0.42f)
		}
	};

	se_scene_2d_handle scene = se_scene_2d_create(
		&s_vec2((f32)NOISE_TEXTURE_WINDOW_WIDTH, (f32)NOISE_TEXTURE_WINDOW_HEIGHT),
		(u16)NOISE_TEXTURE_COUNT);
	if (scene == S_HANDLE_NULL) {
		fprintf(stderr, "advanced/noise_texture :: failed to create scene (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}
	se_scene_2d_set_fit_to_window(scene, window, &s_vec2(1.0f, 1.0f));

	se_texture_handle textures[NOISE_TEXTURE_COUNT] = {0};
	se_object_2d_handle tiles[NOISE_TEXTURE_COUNT] = {0};

	for (u32 i = 0u; i < NOISE_TEXTURE_COUNT; ++i) {
		textures[i] = se_noise_2d_create(context, &previews[i].params);
		if (se_noise_expect_texture(previews[i].label, textures[i]) != 0) {
			se_scene_2d_destroy_full(scene, true);
			se_context_destroy(context);
			return 1;
		}
		if (se_noise_expect_sample(previews[i].label, textures[i], &sample_a) != 0 ||
			se_noise_expect_sample(previews[i].label, textures[i], &sample_b) != 0) {
			se_scene_2d_destroy_full(scene, true);
			se_context_destroy(context);
			return 1;
		}

		tiles[i] = se_object_2d_create(SE_RESOURCE_INTERNAL("shaders/render_quad_frag.glsl"), &s_mat3_identity, 0);
		if (tiles[i] == S_HANDLE_NULL) {
			fprintf(stderr, "advanced/noise_texture :: failed to create %s tile (%s)\n", previews[i].label, se_result_str(se_get_last_error()));
			se_scene_2d_destroy_full(scene, true);
			se_context_destroy(context);
			return 1;
		}

		se_object_2d_set_position(tiles[i], &previews[i].position);
		se_object_2d_set_scale(tiles[i], &tile_scale);
		se_scene_2d_add_object(scene, tiles[i]);
		if (se_noise_preview_bind_texture(context, tiles[i], textures[i]) != 0) {
			se_scene_2d_destroy_full(scene, true);
			se_context_destroy(context);
			return 1;
		}
	}

	printf("advanced/noise_texture controls:\n");
	printf("  Esc: quit\n");

	se_scene_2d_render_to_buffer(scene);
	se_debug_set_overlay_enabled(true);
	
	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_render_clear();
		se_scene_2d_render_to_screen(scene, window);
		se_window_end_frame(window);
	}

	se_scene_2d_destroy_full(scene, true);
	for (u32 i = 0u; i < NOISE_TEXTURE_COUNT; ++i) {
		se_noise_2d_destroy(context, textures[i]);
	}
	se_context_destroy(context);
	return 0;
}
