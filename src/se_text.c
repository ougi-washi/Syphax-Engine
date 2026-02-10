// Syphax-Engine - Ougi Washi

#include "se_text.h"
#include "render/se_gl.h"
#include "syphax/s_files.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

se_text_handle* se_text_handle_create(se_render_handle* render_handle, const u32 fonts_count) {
	if (!render_handle) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	u32 resolved_fonts = fonts_count > 0 ? fonts_count : SE_TEXT_HANDLE_DEFAULT_FONTS;
	se_text_handle* text_handle = malloc(sizeof(se_text_handle));
	if (!text_handle) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	memset(text_handle, 0, sizeof(se_text_handle));
	text_handle->render_handle = render_handle;
	s_array_init(&text_handle->fonts, resolved_fonts);
	se_quad_2d_create(&text_handle->quad, SE_TEXT_CHAR_COUNT);
	se_quad_2d_add_instance_buffer(&text_handle->quad, text_handle->buffer, SE_TEXT_CHAR_COUNT);
	se_shader* shader = se_shader_load(render_handle, SE_RESOURCE_INTERNAL("shaders/text_vert.glsl"), SE_RESOURCE_INTERNAL("shaders/text_frag.glsl"));
	if (!shader) {
		se_quad_destroy(&text_handle->quad);
		s_array_clear(&text_handle->fonts);
		free(text_handle);
		return NULL;
	}
	text_handle->text_shader = shader;
	se_set_last_error(SE_RESULT_OK);
	return text_handle;
}

void se_text_handle_destroy(se_text_handle* text_handle) {
	s_assertf(text_handle, "se_text_handle_destroy :: text_handle is null");
	s_foreach(&text_handle->fonts, i) {
		se_font* curr_font = s_array_get(&text_handle->fonts, i);
		glDeleteTextures(1, &curr_font->atlas_texture);
	}
	s_array_clear(&text_handle->fonts);
	se_quad_destroy(&text_handle->quad);
	free(text_handle);
	text_handle = NULL;
}

se_font* se_font_load(se_text_handle* text_handle, const char* path, const f32 size) {
	if (!text_handle || !path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_render_handle* render_handle = text_handle->render_handle;
	if (!render_handle) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	
	// Check if font is already loaded
	s_foreach(&text_handle->fonts, i) {
		se_font* curr_font = s_array_get(&text_handle->fonts, i);
		if (strcmp(curr_font->path, path) == 0 && curr_font->size == size) {
			se_set_last_error(SE_RESULT_OK);
			return curr_font;
		}
	}

	se_font* new_font = s_array_increment(&text_handle->fonts);
	 
	strcpy(new_font->path, path);

	c8 new_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_paths_resolve_resource_path(new_path, SE_MAX_PATH_LENGTH, path)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	u8* font_file_data = NULL;
	if (!s_file_read_binary(new_path, &font_file_data, NULL)) {
		se_set_last_error(SE_RESULT_IO);
		return NULL;
	}

	s_assertf(font_file_data, "se_font_load :: file_data is null");
	
	const i32 font_count = stbtt_GetNumberOfFonts(font_file_data);
	if (font_count == 0) {
		free(font_file_data);
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return NULL;
	}
	
	new_font->atlas_width = 1024;
	new_font->atlas_height = 1024;
	u8* atlas_bitmap = malloc(new_font->atlas_width * new_font->atlas_height);

	// There are 95 ASCII characters from ASCII 32(Space) to ASCII 126(~)
	// ASCII 32(Space) to ASCII 126(~) are the commonly used characters in text 
	new_font->first_character = 32;
	new_font->characters_count = 95;
	new_font->size = size;

	s_array_init(&new_font->packed_characters, new_font->characters_count);
	s_array_init(&new_font->aligned_quads, new_font->characters_count);

	stbtt_pack_context ctx;
	stbtt_PackBegin(&ctx, atlas_bitmap, new_font->atlas_width, new_font->atlas_height, 0, 1, NULL);
	stbtt_PackFontRange(&ctx, font_file_data, 0, new_font->size, new_font->first_character, new_font->characters_count, new_font->packed_characters.data);
	
	for (i32 i = 0; i < new_font->characters_count; i++) {
		f32 unused_x, unused_y;
		stbtt_GetPackedQuad(new_font->packed_characters.data, 
				new_font->atlas_width, new_font->atlas_height, 
				i,
				&unused_x, &unused_y,
				s_array_get(&new_font->aligned_quads, i),
				0);
	}

	glGenTextures(1, &new_font->atlas_texture);
	glBindTexture(GL_TEXTURE_2D, new_font->atlas_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, new_font->atlas_width, new_font->atlas_height, 0, GL_RED, GL_UNSIGNED_BYTE, atlas_bitmap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);
	 
	free(font_file_data);
	free(atlas_bitmap);

	se_set_last_error(SE_RESULT_OK);
	return new_font;
}

void se_text_render(se_text_handle* text_handle, se_font* font, const c8* text, const s_vec2* position, const s_vec2* size, const f32 new_line_offset) {
	s_assertf(text_handle, "se_text_render :: text_handle is null");
	se_render_handle* render_handle = text_handle->render_handle;
	s_assertf(render_handle, "se_text_render :: render_handle is null");
	s_assertf(font, "se_text_render :: font is null");
	s_assertf(text, "se_text_render :: text is null");
	
	sz text_size = strlen(text);
	
	// Bind texture once
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, font->atlas_texture);
	se_shader_set_texture(text_handle->text_shader, "u_atlas_texture", font->atlas_texture);
	
	s_vec2 local_position = *position;
	const f32 pixel_scale = 1.f / 1024.f; // TODO: dynamic pixel scale by screen size
    s_vec2 text_scale = *size;
    text_scale.x *= pixel_scale;
    text_scale.y *= pixel_scale;
	i32 glyph_count = 0;
	for (i32 i = 0; i < text_size; i++) {
		c8 c = text[i];
		
		if (c == '\n') {
			local_position.y -= new_line_offset * size->y;
			local_position.x = position->x;
			continue;
		}
		
		if (c >= font->first_character && c <= font->first_character + font->characters_count) {
			stbtt_packedchar* packed_char = s_array_get(&font->packed_characters, c - font->first_character);
			stbtt_aligned_quad* aligned_quad = s_array_get(&font->aligned_quads, c - font->first_character);
			
			f32 glyph_width = (packed_char->x1 - packed_char->x0) * text_scale.x; 
			f32 glyph_height = (packed_char->y1 - packed_char->y0) * text_scale.x;
			
			// xoff/yoff are offsets to top-left corner, but shader centers quad
			// So we add half size to get offset to the center
			f32 glyph_x = packed_char->xoff * text_scale.x + glyph_width * 0.5f;
			f32 glyph_y = -packed_char->yoff * text_scale.y - glyph_height * 0.5f;

			text_handle->buffer[glyph_count].m[0][0] = glyph_x;
			text_handle->buffer[glyph_count].m[0][1] = glyph_y;
			text_handle->buffer[glyph_count].m[0][2] = glyph_width;
			text_handle->buffer[glyph_count].m[0][3] = glyph_height;
			text_handle->buffer[glyph_count].m[1][0] = aligned_quad->s0;
			text_handle->buffer[glyph_count].m[1][1] = aligned_quad->t0;
			text_handle->buffer[glyph_count].m[1][2] = aligned_quad->s1;
			text_handle->buffer[glyph_count].m[1][3] = aligned_quad->t1;
			text_handle->buffer[glyph_count].m[2][0] = local_position.x;
			text_handle->buffer[glyph_count].m[2][1] = local_position.y;

			// Advance cursor by xadvance (scaled)
			local_position.x += packed_char->xadvance * text_scale.x;
			glyph_count++;
		}
	}
	
	se_render_set_blending(true);
	se_shader_use(text_handle->render_handle, text_handle->text_shader, true, true);
	text_handle->quad.instance_buffers_dirty = true;
	se_quad_render(&text_handle->quad, glyph_count);
	se_render_set_blending(false);
}
