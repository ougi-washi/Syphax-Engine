// Syphax-Engine - Ougi Washi

#include "se_text.h"
#include "se_debug.h"
#include "render/se_gl.h"
#include "syphax/s_files.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

se_text_handle* se_text_handle_create(const u32 fonts_count) {
	se_context *ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	u32 resolved_fonts = fonts_count > 0 ? fonts_count : SE_TEXT_HANDLE_DEFAULT_FONTS;
	if (s_array_get_capacity(&ctx->fonts) == 0) {
		s_array_init(&ctx->fonts);
	}
	if (s_array_get_capacity(&ctx->fonts) < resolved_fonts) {
		s_array_reserve(&ctx->fonts, resolved_fonts);
	}
	se_text_handle* text_handle = malloc(sizeof(se_text_handle));
	if (!text_handle) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	memset(text_handle, 0, sizeof(se_text_handle));
	text_handle->ctx = ctx;
	se_quad_2d_create(&text_handle->quad, SE_TEXT_CHAR_COUNT);
	se_quad_2d_add_instance_buffer(&text_handle->quad, text_handle->buffer, SE_TEXT_CHAR_COUNT);
	se_shader_handle shader = se_shader_load(SE_RESOURCE_INTERNAL("shaders/text_vert.glsl"), SE_RESOURCE_INTERNAL("shaders/text_frag.glsl"));
	if (shader == S_HANDLE_NULL) {
		se_quad_destroy(&text_handle->quad);
		free(text_handle);
		return NULL;
	}
	text_handle->text_shader = shader;
	se_set_last_error(SE_RESULT_OK);
	return text_handle;
}

void se_text_handle_destroy(se_text_handle* text_handle) {
	s_assertf(text_handle, "se_text_handle_destroy :: text_handle is null");
	se_quad_destroy(&text_handle->quad);
	free(text_handle);
	text_handle = NULL;
}

se_font_handle se_font_load(se_text_handle* text_handle, const char* path, const f32 size) {
	if (!text_handle || !path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_context* ctx = text_handle->ctx;
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	
	// Check if font is already loaded
	for (sz i = 0; i < s_array_get_size(&ctx->fonts); ++i) {
		se_font* curr_font = s_array_get(&ctx->fonts, s_array_handle(&ctx->fonts, (u32)i));
		if (strcmp(curr_font->path, path) == 0 && curr_font->size == size) {
			se_set_last_error(SE_RESULT_OK);
			return s_array_handle(&ctx->fonts, (u32)i);
		}
	}

	se_font_handle font_handle = s_array_increment(&ctx->fonts);
	se_font* new_font = s_array_get(&ctx->fonts, font_handle);
	memset(new_font, 0, sizeof(*new_font));
	 
	strcpy(new_font->path, path);

	c8 new_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_paths_resolve_resource_path(new_path, SE_MAX_PATH_LENGTH, path)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		s_array_remove(&ctx->fonts, font_handle);
		return S_HANDLE_NULL;
	}
	u8* font_file_data = NULL;
	if (!s_file_read_binary(new_path, &font_file_data, NULL)) {
		se_set_last_error(SE_RESULT_IO);
		s_array_remove(&ctx->fonts, font_handle);
		return S_HANDLE_NULL;
	}

	s_assertf(font_file_data, "se_font_load :: file_data is null");
	
	const i32 font_count = stbtt_GetNumberOfFonts(font_file_data);
	if (font_count == 0) {
		free(font_file_data);
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		s_array_remove(&ctx->fonts, font_handle);
		return S_HANDLE_NULL;
	}
	
	new_font->atlas_width = 1024;
	new_font->atlas_height = 1024;
	u8* atlas_bitmap = malloc(new_font->atlas_width * new_font->atlas_height);

	// There are 95 ASCII characters from ASCII 32(Space) to ASCII 126(~)
	// ASCII 32(Space) to ASCII 126(~) are the commonly used characters in text 
	new_font->first_character = 32;
	new_font->characters_count = 95;
	new_font->size = size;

	s_array_init(&new_font->packed_characters);
	s_array_init(&new_font->aligned_quads);
	for (i32 i = 0; i < new_font->characters_count; ++i) {
		s_array_increment(&new_font->packed_characters);
		s_array_increment(&new_font->aligned_quads);
	}

	stbtt_pack_context pack_ctx;
	stbtt_PackBegin(&pack_ctx, atlas_bitmap, new_font->atlas_width, new_font->atlas_height, 0, 1, NULL);
	stbtt_PackFontRange(&pack_ctx, font_file_data, 0, new_font->size, new_font->first_character, new_font->characters_count, s_array_get_data(&new_font->packed_characters));
	
	for (i32 i = 0; i < new_font->characters_count; i++) {
		f32 unused_x, unused_y;
		stbtt_GetPackedQuad(s_array_get_data(&new_font->packed_characters), 
				new_font->atlas_width, new_font->atlas_height, 
				i,
				&unused_x, &unused_y,
				s_array_get(&new_font->aligned_quads, s_array_handle(&new_font->aligned_quads, (u32)i)),
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
	return font_handle;
}

void se_text_render(se_text_handle* text_handle, const se_font_handle font, const c8* text, const s_vec2* position, const s_vec2* size, const f32 new_line_offset) {
	se_debug_trace_begin("text_render");
	s_assertf(text_handle, "se_text_render :: text_handle is null");
	se_context* ctx = text_handle->ctx;
	s_assertf(ctx, "se_text_render :: ctx is null");
	se_font* font_ptr = s_array_get(&ctx->fonts, font);
	s_assertf(font_ptr, "se_text_render :: font is null");
	s_assertf(text, "se_text_render :: text is null");
	
	sz text_size = strlen(text);
	
	// Bind texture once
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, font_ptr->atlas_texture);
	se_shader_set_texture(text_handle->text_shader, "u_atlas_texture", font_ptr->atlas_texture);
	
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
		
		if (c >= font_ptr->first_character && c <= font_ptr->first_character + font_ptr->characters_count) {
			u32 char_index = (u32)(c - font_ptr->first_character);
			stbtt_packedchar* packed_char = s_array_get(&font_ptr->packed_characters, s_array_handle(&font_ptr->packed_characters, char_index));
			stbtt_aligned_quad* aligned_quad = s_array_get(&font_ptr->aligned_quads, s_array_handle(&font_ptr->aligned_quads, char_index));
			
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
	se_shader_use(text_handle->text_shader, true, true);
	text_handle->quad.instance_buffers_dirty = true;
	se_quad_render(&text_handle->quad, glyph_count);
	se_render_set_blending(false);
	se_debug_trace_end("text_render");
}
