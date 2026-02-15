// Syphax Engine - Ougi Washi

#ifndef SE_CONTEXT_H
#define SE_CONTEXT_H

#include "syphax/s_types.h"
#include "syphax/s_array.h"

#ifndef SE_THREAD_LOCAL
#if defined(__cplusplus)
#define SE_THREAD_LOCAL thread_local
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define SE_THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
#define SE_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define SE_THREAD_LOCAL __thread
#else
#define SE_THREAD_LOCAL
#endif
#endif

typedef struct se_text_handle se_text_handle;
typedef s_handle se_window_handle;
typedef s_handle se_font_handle;
typedef s_handle se_model_handle;
typedef s_handle se_camera_handle;
typedef s_handle se_framebuffer_handle;
typedef s_handle se_render_buffer_handle;
typedef s_handle se_texture_handle;
typedef s_handle se_shader_handle;
typedef s_handle se_object_2d_handle;
typedef s_handle se_scene_2d_handle;
typedef s_handle se_object_3d_handle;
typedef s_handle se_scene_3d_handle;
typedef s_handle se_ui_element_handle;
typedef s_handle se_ui_text_handle;
typedef s_handle se_simulation_handle;

struct se_simulation;

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
	s_array(struct se_simulation, simulations);
	se_text_handle *ui_text_handle;
} se_context;

typedef struct {
	u32 models;
	u32 cameras;
	u32 framebuffers;
	u32 render_buffers;
	u32 shaders;
	u32 textures;
	u32 fonts;
	u32 windows;
} se_context_destroy_report;

extern se_context *se_global_context;
extern SE_THREAD_LOCAL se_context *se_tls_context;

static inline void se_set_global_context(se_context *context) {
	se_global_context = context;
}

static inline se_context *se_get_global_context(void) {
	return se_global_context;
}

static inline void se_set_tls_context(se_context *context) {
	se_tls_context = context;
}

static inline se_context *se_get_tls_context(void) {
	return se_tls_context;
}

static inline se_context *se_current_context(void) {
	return se_tls_context != NULL ? se_tls_context : se_global_context;
}

static inline se_context *se_push_tls_context(se_context *context) {
	se_context *prev = se_tls_context;
	se_tls_context = context;
	return prev;
}

static inline void se_pop_tls_context(se_context *prev) {
	se_tls_context = prev;
}

extern se_context *se_context_create(void);
// Canonical teardown entry-point. It destroys every resource owned by `context`,
// including windows, then shuts down backend state when the last window is gone.
// Prefer this over destroying the final window manually.
extern void se_context_destroy(se_context *context);
extern b8 se_context_get_last_destroy_report(se_context_destroy_report *out_report);
#endif // SE_CONTEXT_H
