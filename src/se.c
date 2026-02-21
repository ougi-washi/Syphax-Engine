// Syphax Engine - Ougi Washi

#include "se.h"

#include "se_camera.h"
#include "se_debug.h"
#include "se_framebuffer.h"
#include "se_model.h"
#include "se_physics.h"
#include "se_render_buffer.h"
#include "se_scene.h"
#include "se_shader.h"
#include "se_simulation.h"
#include "se_text.h"
#include "se_texture.h"
#include "se_ui.h"
#include "se_vfx.h"
#include "se_window.h"

#include <stdlib.h>
#include <string.h>

#define SE_CONTEXT_DEFAULT_WINDOWS 8
#define SE_CONTEXT_DEFAULT_FONTS 32
#define SE_CONTEXT_DEFAULT_MODELS 128
#define SE_CONTEXT_DEFAULT_CAMERAS 32
#define SE_CONTEXT_DEFAULT_FRAMEBUFFERS 64
#define SE_CONTEXT_DEFAULT_RENDER_BUFFERS 64
#define SE_CONTEXT_DEFAULT_TEXTURES 256
#define SE_CONTEXT_DEFAULT_SHADERS 128
#define SE_CONTEXT_DEFAULT_GLOBAL_UNIFORMS 64
#define SE_CONTEXT_DEFAULT_OBJECTS_2D 4096
#define SE_CONTEXT_DEFAULT_SCENES_2D 512
#define SE_CONTEXT_DEFAULT_OBJECTS_3D 4096
#define SE_CONTEXT_DEFAULT_SCENES_3D 256
#define SE_CONTEXT_DEFAULT_VFX_2DS 128
#define SE_CONTEXT_DEFAULT_VFX_3DS 128
#define SE_CONTEXT_DEFAULT_UI_ROOTS 64
#define SE_CONTEXT_DEFAULT_UI_ELEMENTS 512
#define SE_CONTEXT_DEFAULT_UI_TEXTS 2048

se_context *se_global_context = NULL;
SE_THREAD_LOCAL se_context *se_tls_context = NULL;
static se_context_destroy_report se_last_destroy_report = {0};

b8 se_context_get_last_destroy_report(se_context_destroy_report *out_report) {
	if (out_report == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_report = se_last_destroy_report;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static void se_context_destroy_ui_storage(se_context *context) {
	se_ui_destroy_all();
	s_array_clear(&context->ui_roots);
	s_array_clear(&context->ui_elements);
	s_array_clear(&context->ui_texts);
}

static void se_context_log_leaks(se_context *context) {
	if (!context) {
		return;
	}
	if (s_array_get_size(&context->windows) > 0) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_CORE,
			"se_context_log_leaks :: context teardown with %zu window(s) still alive",
			s_array_get_size(&context->windows));
	}
	if (s_array_get_size(&context->scenes_2d) > 0 || s_array_get_size(&context->scenes_3d) > 0) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_CORE,
			"se_context_log_leaks :: context teardown with scenes alive (2d=%zu, 3d=%zu)",
			s_array_get_size(&context->scenes_2d),
			s_array_get_size(&context->scenes_3d));
	}
	if (s_array_get_size(&context->objects_2d) > 0 || s_array_get_size(&context->objects_3d) > 0) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_CORE,
			"se_context_log_leaks :: context teardown with objects alive (2d=%zu, 3d=%zu)",
			s_array_get_size(&context->objects_2d),
			s_array_get_size(&context->objects_3d));
	}
	if (s_array_get_size(&context->textures) > 0 || s_array_get_size(&context->shaders) > 0) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_CORE,
			"se_context_log_leaks :: context teardown with GPU resources alive (textures=%zu, shaders=%zu)",
			s_array_get_size(&context->textures),
			s_array_get_size(&context->shaders));
	}
	if (s_array_get_size(&context->simulations) > 0) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_CORE,
			"se_context_log_leaks :: context teardown with simulations alive (%zu)",
			s_array_get_size(&context->simulations));
	}
	if (s_array_get_size(&context->physics_worlds_2d) > 0 || s_array_get_size(&context->physics_worlds_3d) > 0) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_CORE,
			"se_context_log_leaks :: context teardown with physics worlds alive (2d=%zu, 3d=%zu)",
			s_array_get_size(&context->physics_worlds_2d),
			s_array_get_size(&context->physics_worlds_3d));
	}
	if (s_array_get_size(&context->vfx_2ds) > 0 || s_array_get_size(&context->vfx_3ds) > 0) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_CORE,
			"se_context_log_leaks :: context teardown with vfx systems alive (2d=%zu, 3d=%zu)",
			s_array_get_size(&context->vfx_2ds),
			s_array_get_size(&context->vfx_3ds));
	}
}

se_context *se_context_create(void) {
	se_context *context = malloc(sizeof(se_context));
	if (context == NULL) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	memset(context, 0, sizeof(*context));

	s_array_init(&context->windows);
	s_array_init(&context->fonts);
	s_array_init(&context->models);
	s_array_init(&context->cameras);
	s_array_init(&context->framebuffers);
	s_array_init(&context->render_buffers);
	s_array_init(&context->textures);
	s_array_init(&context->shaders);
	s_array_init(&context->global_uniforms);
	s_array_init(&context->objects_2d);
	s_array_init(&context->scenes_2d);
	s_array_init(&context->objects_3d);
	s_array_init(&context->scenes_3d);
	s_array_init(&context->vfx_2ds);
	s_array_init(&context->vfx_3ds);
	s_array_init(&context->ui_roots);
	s_array_init(&context->ui_elements);
	s_array_init(&context->ui_texts);
	s_array_init(&context->simulations);
	s_array_init(&context->physics_worlds_2d);
	s_array_init(&context->physics_worlds_3d);
	context->ui_text_handle = NULL;
	context->default_text_handle = NULL;
	se_uniform_set_float((se_uniforms*)&context->global_uniforms, "u_time", 0.0f);

	if (se_global_context == NULL) {
		se_set_global_context(context);
	}
	if (se_tls_context == NULL) {
		se_set_tls_context(context);
	}

	se_set_last_error(SE_RESULT_OK);
	return context;
}

void se_context_destroy(se_context *context) {
	s_assertf(context, "se_context_destroy :: context is null");
	se_context *prev_ctx = se_push_tls_context(context);
	memset(&se_last_destroy_report, 0, sizeof(se_last_destroy_report));

	se_context_destroy_ui_storage(context);
	se_text_handle* ui_text_handle = context->ui_text_handle;
	se_text_handle* default_text_handle = context->default_text_handle;
	if (ui_text_handle) {
		se_text_handle_destroy(ui_text_handle);
	}
	if (default_text_handle && default_text_handle != ui_text_handle) {
		se_text_handle_destroy(default_text_handle);
	}
	context->ui_text_handle = NULL;
	context->default_text_handle = NULL;

	while (s_array_get_size(&context->vfx_2ds) > 0) {
		se_vfx_2d_handle vfx_handle = s_array_handle(&context->vfx_2ds, (u32)(s_array_get_size(&context->vfx_2ds) - 1));
		se_vfx_2d_destroy(vfx_handle);
		se_last_destroy_report.vfx_2ds++;
	}

	while (s_array_get_size(&context->vfx_3ds) > 0) {
		se_vfx_3d_handle vfx_handle = s_array_handle(&context->vfx_3ds, (u32)(s_array_get_size(&context->vfx_3ds) - 1));
		se_vfx_3d_destroy(vfx_handle);
		se_last_destroy_report.vfx_3ds++;
	}

	while (s_array_get_size(&context->scenes_2d) > 0) {
		se_scene_2d_handle scene_handle = s_array_handle(&context->scenes_2d, (u32)(s_array_get_size(&context->scenes_2d) - 1));
		se_scene_2d_destroy(scene_handle);
	}

	while (s_array_get_size(&context->scenes_3d) > 0) {
		se_scene_3d_handle scene_handle = s_array_handle(&context->scenes_3d, (u32)(s_array_get_size(&context->scenes_3d) - 1));
		se_scene_3d_destroy(scene_handle);
	}

	while (s_array_get_size(&context->objects_2d) > 0) {
		se_object_2d_handle object_handle = s_array_handle(&context->objects_2d, (u32)(s_array_get_size(&context->objects_2d) - 1));
		se_object_2d_destroy(object_handle);
	}

	while (s_array_get_size(&context->objects_3d) > 0) {
		se_object_3d_handle object_handle = s_array_handle(&context->objects_3d, (u32)(s_array_get_size(&context->objects_3d) - 1));
		se_object_3d_destroy(object_handle);
	}

	while (s_array_get_size(&context->models) > 0) {
		se_model_handle model_handle = s_array_handle(&context->models, (u32)(s_array_get_size(&context->models) - 1));
		se_model_destroy(model_handle);
		se_last_destroy_report.models++;
	}

	while (s_array_get_size(&context->cameras) > 0) {
		se_camera_handle camera_handle = s_array_handle(&context->cameras, (u32)(s_array_get_size(&context->cameras) - 1));
		se_camera_destroy(camera_handle);
		se_last_destroy_report.cameras++;
	}

	while (s_array_get_size(&context->framebuffers) > 0) {
		se_framebuffer_handle framebuffer_handle = s_array_handle(&context->framebuffers, (u32)(s_array_get_size(&context->framebuffers) - 1));
		se_framebuffer_destroy(framebuffer_handle);
		se_last_destroy_report.framebuffers++;
	}

	while (s_array_get_size(&context->render_buffers) > 0) {
		se_render_buffer_handle buffer_handle = s_array_handle(&context->render_buffers, (u32)(s_array_get_size(&context->render_buffers) - 1));
		se_render_buffer_destroy(buffer_handle);
		se_last_destroy_report.render_buffers++;
	}

	while (s_array_get_size(&context->shaders) > 0) {
		se_shader_handle shader_handle = s_array_handle(&context->shaders, (u32)(s_array_get_size(&context->shaders) - 1));
		se_shader_destroy(shader_handle);
		se_last_destroy_report.shaders++;
	}

	while (s_array_get_size(&context->textures) > 0) {
		se_texture_handle texture_handle = s_array_handle(&context->textures, (u32)(s_array_get_size(&context->textures) - 1));
		se_texture_destroy(texture_handle);
		se_last_destroy_report.textures++;
	}

	while (s_array_get_size(&context->fonts) > 0) {
		se_font_handle font_handle = s_array_handle(&context->fonts, (u32)(s_array_get_size(&context->fonts) - 1));
		se_font_destroy(font_handle);
		se_last_destroy_report.fonts++;
	}

	while (s_array_get_size(&context->simulations) > 0) {
		se_simulation_handle simulation_handle = s_array_handle(&context->simulations, (u32)(s_array_get_size(&context->simulations) - 1));
		se_simulation_destroy(simulation_handle);
	}

	while (s_array_get_size(&context->physics_worlds_2d) > 0) {
		se_physics_world_2d_handle world_handle = s_array_handle(&context->physics_worlds_2d, (u32)(s_array_get_size(&context->physics_worlds_2d) - 1));
		se_physics_world_2d_destroy(world_handle);
		se_last_destroy_report.physics_worlds_2d++;
	}

	while (s_array_get_size(&context->physics_worlds_3d) > 0) {
		se_physics_world_3d_handle world_handle = s_array_handle(&context->physics_worlds_3d, (u32)(s_array_get_size(&context->physics_worlds_3d) - 1));
		se_physics_world_3d_destroy(world_handle);
		se_last_destroy_report.physics_worlds_3d++;
	}

	while (s_array_get_size(&context->windows) > 0) {
		se_window_handle window_handle = s_array_handle(&context->windows, (u32)(s_array_get_size(&context->windows) - 1));
		const sz windows_before = s_array_get_size(&context->windows);
		se_window_destroy(window_handle);
		if (s_array_get_size(&context->windows) == windows_before) {
			s_array_remove(&context->windows, window_handle);
		}
		se_last_destroy_report.windows++;
	}

	se_context_log_leaks(context);
	s_array_clear(&context->global_uniforms);

	if (se_global_context == context) {
		se_global_context = NULL;
	}

	if (prev_ctx == context) {
		prev_ctx = NULL;
	}
	se_pop_tls_context(prev_ctx);

	free(context);
}
