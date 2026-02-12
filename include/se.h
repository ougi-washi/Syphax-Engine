// Syphax Engine - Ougi Washi

#ifndef SE_CONTEXT_H
#define SE_CONTEXT_H

#include "syphax/s_types.h"
#include "syphax/s_array.h"

typedef struct se_context {
	s_array(struct se_window, windows);
	s_array(struct se_font, fonts);
	s_array(struct se_model, models);
	s_array(struct se_camera, cameras);
	s_array(struct se_framebuffer, framebuffers);
	s_array(struct se_render_buffer, render_buffers);
	s_array(struct se_texture, textures);
	s_array(struct se_shader, shaders);
	s_array(struct se_uniform, global_uniforms);
	s_array(struct se_object_2d, objects_2d);
	s_array(struct se_scene_2d, scenes_2d);
	s_array(struct se_object_3d, objects_3d);
	s_array(struct se_scene_3d, scenes_3d);
	s_array(struct se_ui_element, ui_elements);
	s_array(struct se_ui_text, ui_texts);
} se_context;

extern se_context *se_context_create(void);
extern void se_context_destroy(se_context *context);
#endif // SE_CONTEXT_H
