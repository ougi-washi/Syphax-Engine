// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_camera.h"
#include "se_framebuffer.h"
#include "se_model.h"
#include "se_render_buffer.h"
#include "se_shader.h"
#include "se_texture.h"
#include "se_vfx.h"

#include <stdio.h>

static const u8 se_example_png_1x1_rgba[] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
	0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
	0x89, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x44, 0x41,
	0x54, 0x78, 0x9C, 0x63, 0xF8, 0xCF, 0xC0, 0xF0,
	0x1F, 0x00, 0x05, 0x00, 0x01, 0xFF, 0x89, 0x99,
	0x3D, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
	0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

static i32 se_expect_handle(const s_handle handle, const c8 *label) {
	if (handle != S_HANDLE_NULL) {
		return 0;
	}
	fprintf(stderr, "21_context_lifecycle :: failed to create %s\n", label);
	return 1;
}

static i32 se_expect_report(
	const se_context_destroy_report *expected,
	const se_context_destroy_report *actual) {
	if (expected->models != actual->models) return 1;
	if (expected->cameras != actual->cameras) return 1;
	if (expected->framebuffers != actual->framebuffers) return 1;
	if (expected->render_buffers != actual->render_buffers) return 1;
	if (expected->shaders != actual->shaders) return 1;
	if (expected->textures != actual->textures) return 1;
	if (expected->fonts != actual->fonts) return 1;
	if (expected->windows != actual->windows) return 1;
	if (expected->vfx_2ds != actual->vfx_2ds) return 1;
	if (expected->vfx_3ds != actual->vfx_3ds) return 1;
	return 0;
}

i32 main(void) {
	// Create the primary context that owns this lifecycle run.
	se_context *context = se_context_create();
	if (!context) {
		return 1;
	}

	// Create multiple windows in one context to validate normal shared ownership.
	se_window_handle window_a = se_window_create("Syphax-Engine - 21_context_lifecycle A", 320, 180);
	if (se_expect_handle(window_a, "window_a") != 0) {
		se_context_destroy(context);
		return 1;
	}

	se_window_handle window_b = se_window_create("Syphax-Engine - 21_context_lifecycle B", 320, 180);
	if (se_expect_handle(window_b, "window_b") != 0) {
		se_context_destroy(context);
		return 1;
	}

	// Ensure cross-context window creation is blocked while windows are alive elsewhere.
	se_context *other_context = se_context_create();
	if (!other_context) {
		se_context_destroy(context);
		return 1;
	}
	se_set_tls_context(other_context);
	se_window_handle should_fail_window = se_window_create("Syphax-Engine - 21_context_lifecycle blocked", 160, 90);
	if (should_fail_window != S_HANDLE_NULL) {
		fprintf(stderr, "21_context_lifecycle :: expected cross-context window creation to fail\n");
		se_context_destroy(other_context);
		se_set_tls_context(context);
		se_context_destroy(context);
		return 1;
	}
	se_context_destroy(other_context);
	se_set_tls_context(context);

	// Populate the context with representative resources for teardown accounting.
	se_camera_handle camera_a = se_camera_create();
	se_camera_handle camera_b = se_camera_create();
	if (se_expect_handle(camera_a, "camera_a") != 0 || se_expect_handle(camera_b, "camera_b") != 0) {
		se_context_destroy(context);
		return 1;
	}

	const s_vec2 framebuffer_size = s_vec2(64.0f, 64.0f);
	se_framebuffer_handle framebuffer_a = se_framebuffer_create(&framebuffer_size);
	se_framebuffer_handle framebuffer_b = se_framebuffer_create(&framebuffer_size);
	if (se_expect_handle(framebuffer_a, "framebuffer_a") != 0 || se_expect_handle(framebuffer_b, "framebuffer_b") != 0) {
		se_context_destroy(context);
		return 1;
	}

	se_render_buffer_handle render_buffer_a = se_render_buffer_create(64, 64, SE_RESOURCE_INTERNAL("shaders/render_quad_frag.glsl"));
	se_render_buffer_handle render_buffer_b = se_render_buffer_create(64, 64, SE_RESOURCE_INTERNAL("shaders/render_quad_frag.glsl"));
	if (se_expect_handle(render_buffer_a, "render_buffer_a") != 0 || se_expect_handle(render_buffer_b, "render_buffer_b") != 0) {
		se_context_destroy(context);
		return 1;
	}

	se_shader_handle shader_a = se_shader_load(SE_RESOURCE_INTERNAL("shaders/render_quad_vert.glsl"), SE_RESOURCE_INTERNAL("shaders/render_quad_frag.glsl"));
	se_shader_handle shader_b = se_shader_load(SE_RESOURCE_INTERNAL("shaders/render_quad_vert.glsl"), SE_RESOURCE_INTERNAL("shaders/render_quad_frag.glsl"));
	if (se_expect_handle(shader_a, "shader_a") != 0 || se_expect_handle(shader_b, "shader_b") != 0) {
		se_context_destroy(context);
		return 1;
	}

	se_texture_handle texture_a = se_texture_load_from_memory(se_example_png_1x1_rgba, sizeof(se_example_png_1x1_rgba), SE_REPEAT);
	se_texture_handle texture_b = se_texture_load_from_memory(se_example_png_1x1_rgba, sizeof(se_example_png_1x1_rgba), SE_CLAMP);
	if (se_expect_handle(texture_a, "texture_a") != 0 || se_expect_handle(texture_b, "texture_b") != 0) {
		se_context_destroy(context);
		return 1;
	}

	se_model_handle model_a = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_PUBLIC("shaders/scene_3d_vertex.glsl"),
		SE_RESOURCE_PUBLIC("shaders/scene_3d_fragment.glsl"));
	se_model_handle model_b = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/sphere.obj"),
		SE_RESOURCE_PUBLIC("shaders/scene_3d_vertex.glsl"),
		SE_RESOURCE_PUBLIC("shaders/scene_3d_fragment.glsl"));
	if (se_expect_handle(model_a, "model_a") != 0 || se_expect_handle(model_b, "model_b") != 0) {
		se_context_destroy(context);
		return 1;
	}

	se_vfx_2d_handle vfx_2d = se_vfx_2d_create(NULL);
	se_vfx_3d_handle vfx_3d = se_vfx_3d_create(NULL);
	if (se_expect_handle(vfx_2d, "vfx_2d") != 0 || se_expect_handle(vfx_3d, "vfx_3d") != 0) {
		se_context_destroy(context);
		return 1;
	}

	// Snapshot counts so the destroy report can be validated after teardown.
	se_context_destroy_report expected = {0};
	expected.models = (u32)s_array_get_size(&context->models);
	expected.cameras = (u32)s_array_get_size(&context->cameras);
	expected.framebuffers = (u32)s_array_get_size(&context->framebuffers);
	expected.render_buffers = (u32)s_array_get_size(&context->render_buffers);
	expected.shaders = (u32)s_array_get_size(&context->shaders);
	expected.textures = (u32)s_array_get_size(&context->textures);
	expected.fonts = (u32)s_array_get_size(&context->fonts);
	expected.windows = (u32)s_array_get_size(&context->windows);
	expected.vfx_2ds = (u32)s_array_get_size(&context->vfx_2ds);
	expected.vfx_3ds = (u32)s_array_get_size(&context->vfx_3ds);

	// Destroy the context and compare reported cleanup totals with the snapshot.
	se_context_destroy(context);

	se_context_destroy_report actual = {0};
	if (!se_context_get_last_destroy_report(&actual)) {
		fprintf(stderr, "21_context_lifecycle :: failed to fetch destroy report\n");
		return 1;
	}
	if (se_expect_report(&expected, &actual) != 0) {
		fprintf(
			stderr,
			"21_context_lifecycle :: destroy report mismatch expected(m=%u c=%u f=%u rb=%u s=%u t=%u fn=%u w=%u v2=%u v3=%u) actual(m=%u c=%u f=%u rb=%u s=%u t=%u fn=%u w=%u v2=%u v3=%u)\n",
			expected.models,
			expected.cameras,
			expected.framebuffers,
			expected.render_buffers,
			expected.shaders,
			expected.textures,
			expected.fonts,
			expected.windows,
			expected.vfx_2ds,
			expected.vfx_3ds,
			actual.models,
			actual.cameras,
			actual.framebuffers,
			actual.render_buffers,
			actual.shaders,
			actual.textures,
			actual.fonts,
			actual.windows,
			actual.vfx_2ds,
			actual.vfx_3ds);
		return 1;
	}

	// Recreate context/window once more to verify the lifecycle resets correctly.
	se_context *post_context = se_context_create();
	if (!post_context) {
		return 1;
	}
	se_set_tls_context(post_context);
	se_window_handle post_window = se_window_create("Syphax-Engine - 21_context_lifecycle post", 320, 180);
	if (se_expect_handle(post_window, "post_window") != 0) {
		se_context_destroy(post_context);
		return 1;
	}
	se_context_destroy(post_context);

	printf("21_context_lifecycle :: destroy report and single-context lifecycle passed\n");
	return 0;
}
