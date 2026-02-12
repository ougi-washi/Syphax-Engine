// Syphax Engine - Ougi Washi

#include "se.h"

#include "se_camera.h"
#include "se_framebuffer.h"
#include "se_model.h"
#include "se_render_buffer.h"
#include "se_scene.h"
#include "se_shader.h"
#include "se_text.h"
#include "se_texture.h"
#include "se_ui.h"
#include "se_window.h"

#include "render/se_gl.h"

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
#define SE_CONTEXT_DEFAULT_UI_ELEMENTS 512
#define SE_CONTEXT_DEFAULT_UI_TEXTS 2048

static void se_context_destroy_fonts(se_context *context) {
	s_foreach(&context->fonts, i) {
		se_font *font = s_array_get(&context->fonts, i);
		if (font == NULL) {
			continue;
		}
		if (font->atlas_texture != 0) {
			glDeleteTextures(1, &font->atlas_texture);
			font->atlas_texture = 0;
		}
		s_array_clear(&font->packed_characters);
		s_array_clear(&font->aligned_quads);
		memset(font, 0, sizeof(*font));
	}
	s_array_clear(&context->fonts);
}

static void se_context_destroy_ui_storage(se_context *context) {
	s_foreach(&context->ui_elements, i) {
		se_ui_element *ui_element = s_array_get(&context->ui_elements, i);
		if (ui_element == NULL) {
			continue;
		}
		s_array_clear(&ui_element->children);
		ui_element->ui_handle = NULL;
		ui_element->parent = NULL;
		ui_element->scene_2d = NULL;
		ui_element->text = NULL;
		memset(ui_element, 0, sizeof(*ui_element));
	}
	s_array_clear(&context->ui_elements);

	s_foreach(&context->ui_texts, i) {
		se_ui_text *ui_text = s_array_get(&context->ui_texts, i);
		if (ui_text == NULL) {
			continue;
		}
		memset(ui_text, 0, sizeof(*ui_text));
	}
	s_array_clear(&context->ui_texts);
}

se_context *se_context_create(void) {
	se_context *context = malloc(sizeof(se_context));
	if (context == NULL) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	memset(context, 0, sizeof(*context));

	s_array_init(&context->windows, SE_CONTEXT_DEFAULT_WINDOWS);
	s_array_init(&context->fonts, SE_CONTEXT_DEFAULT_FONTS);
	s_array_init(&context->models, SE_CONTEXT_DEFAULT_MODELS);
	s_array_init(&context->cameras, SE_CONTEXT_DEFAULT_CAMERAS);
	s_array_init(&context->framebuffers, SE_CONTEXT_DEFAULT_FRAMEBUFFERS);
	s_array_init(&context->render_buffers, SE_CONTEXT_DEFAULT_RENDER_BUFFERS);
	s_array_init(&context->textures, SE_CONTEXT_DEFAULT_TEXTURES);
	s_array_init(&context->shaders, SE_CONTEXT_DEFAULT_SHADERS);
	s_array_init(&context->global_uniforms, SE_CONTEXT_DEFAULT_GLOBAL_UNIFORMS);
	s_array_init(&context->objects_2d, SE_CONTEXT_DEFAULT_OBJECTS_2D);
	s_array_init(&context->scenes_2d, SE_CONTEXT_DEFAULT_SCENES_2D);
	s_array_init(&context->objects_3d, SE_CONTEXT_DEFAULT_OBJECTS_3D);
	s_array_init(&context->scenes_3d, SE_CONTEXT_DEFAULT_SCENES_3D);
	s_array_init(&context->ui_elements, SE_CONTEXT_DEFAULT_UI_ELEMENTS);
	s_array_init(&context->ui_texts, SE_CONTEXT_DEFAULT_UI_TEXTS);

	se_set_last_error(SE_RESULT_OK);
	return context;
}

void se_context_destroy(se_context *context) {
	s_assertf(context, "se_context_destroy :: context is null");

	se_context_destroy_ui_storage(context);

	while (s_array_get_size(&context->scenes_2d) > 0) {
		se_scene_2d *curr_scene_2d = s_array_get(&context->scenes_2d, s_array_get_size(&context->scenes_2d) - 1);
		se_scene_2d_destroy(context, curr_scene_2d);
	}

	while (s_array_get_size(&context->scenes_3d) > 0) {
		se_scene_3d *curr_scene_3d = s_array_get(&context->scenes_3d, s_array_get_size(&context->scenes_3d) - 1);
		se_scene_3d_destroy(context, curr_scene_3d);
	}

	while (s_array_get_size(&context->objects_2d) > 0) {
		se_object_2d *curr_object_2d = s_array_get(&context->objects_2d, s_array_get_size(&context->objects_2d) - 1);
		se_object_2d_destroy(context, curr_object_2d);
	}

	while (s_array_get_size(&context->objects_3d) > 0) {
		se_object_3d *curr_object_3d = s_array_get(&context->objects_3d, s_array_get_size(&context->objects_3d) - 1);
		se_object_3d_destroy(context, curr_object_3d);
	}

	s_foreach(&context->models, i) {
		se_model *curr_model = s_array_get(&context->models, i);
		se_model_destroy(context, curr_model);
	}
	s_array_clear(&context->models);

	s_foreach(&context->cameras, i) {
		se_camera *curr_camera = s_array_get(&context->cameras, i);
		se_camera_destroy(context, curr_camera);
	}
	s_array_clear(&context->cameras);

	s_foreach(&context->framebuffers, i) {
		se_framebuffer *curr_framebuffer = s_array_get(&context->framebuffers, i);
		se_framebuffer_destroy(context, curr_framebuffer);
	}
	s_array_clear(&context->framebuffers);

	s_foreach(&context->render_buffers, i) {
		se_render_buffer *curr_buffer = s_array_get(&context->render_buffers, i);
		se_render_buffer_destroy(context, curr_buffer);
	}
	s_array_clear(&context->render_buffers);

	s_foreach(&context->shaders, i) {
		se_shader *curr_shader = s_array_get(&context->shaders, i);
		se_shader_destroy(context, curr_shader);
	}
	s_array_clear(&context->shaders);

	s_foreach(&context->textures, i) {
		se_texture *curr_texture = s_array_get(&context->textures, i);
		se_texture_destroy(context, curr_texture);
	}
	s_array_clear(&context->textures);

	se_context_destroy_fonts(context);

	s_foreach(&context->windows, i) {
		se_window *curr_window = s_array_get(&context->windows, i);
		if (curr_window == NULL) {
			continue;
		}
		se_window_destroy(curr_window);
	}
	s_array_clear(&context->windows);

	s_array_clear(&context->global_uniforms);

	free(context);
}
