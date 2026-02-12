// Syphax-Engine - Ougi Washi

#ifndef SE_TEXT_H
#define SE_TEXT_H

#include "se_render.h"
#include "se_shader.h"
#include "stb_truetype.h"

#define SE_TEXT_CHAR_COUNT 1024 * 8
#define SE_TEXT_HANDLE_DEFAULT_FONTS 4

typedef struct se_font {
	c8 path[SE_MAX_PATH_LENGTH];
	u32 atlas_texture;
	f32 size;
	u16 atlas_width, atlas_height;
	u16 first_character, characters_count;
	s_array(stbtt_packedchar, packed_characters);
	s_array(stbtt_aligned_quad, aligned_quads);
} se_font;
typedef s_array(se_font, se_fonts);

typedef struct {
	se_quad quad;
	s_mat4 buffer[SE_TEXT_CHAR_COUNT];
	se_shader_handle text_shader;
	i32 text_vertex_index; // ?
	se_context* ctx;
} se_text_handle;

// Font && text functions
extern se_text_handle* se_text_handle_create(const u32 fonts_count);
extern void se_text_handle_destroy(se_text_handle* text_handle);
extern se_font_handle se_font_load(se_text_handle* text_handle, const char* path, const f32 size);
extern void se_text_render(se_text_handle* text_handle, const se_font_handle font, const c8* text, const s_vec2* position, const s_vec2* size, const f32 new_line_offset);

#endif // SE_TEXT_H
