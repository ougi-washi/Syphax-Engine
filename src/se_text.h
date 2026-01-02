// Syphax-Engine - Ougi Washi

#ifndef SE_TEXT_H
#define SE_TEXT_H

#include "se_render.h"
#include "stb_truetype.h"

typedef struct {
    GLuint atlas_texture;
    f32 size;
    u16 atlas_width, atlas_height;
    u16 first_character, characters_count;
    s_array(stbtt_packedchar, packed_characters);
    s_array(stbtt_aligned_quad, aligned_quads);
} se_font;
typedef s_array(se_font, se_fonts);

typedef struct {
    se_fonts fonts;
    se_quad quad;
    se_shader* text_shader;
    i32 text_vertex_index; // ?
    se_render_handle* render_handle;
} se_text_handle;

// Font && text functions
extern se_text_handle* se_text_handle_create(se_render_handle* render_handle, const u32 fonts_count);
extern void se_text_handle_cleanup(se_text_handle* text_handle);
extern se_font* se_font_load(se_text_handle* text_handle, const char* path);
extern void se_init_text_render(se_text_handle* text_handle);
extern void se_text_render(se_text_handle* text_handle, se_font* font, const c8* text, const se_vec2* position, const f32 size);
extern void se_font_cleanup(se_font* font);

#endif // SE_TEXT_H
