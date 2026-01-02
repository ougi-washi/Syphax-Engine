// Syphax-Engine - Ougi Washi

#include "se_text.h"
#include "se_gl.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

const sz SE_TEXT_VBO_SIZE = 600000 * sizeof(se_vertex_3d); // Size enough for 600000 vertices (100000 quads)

se_text_handle* se_text_handle_create(se_render_handle* render_handle, const u32 fonts_count) {
    se_text_handle* text_handle = malloc(sizeof(se_text_handle));
    memset(text_handle, 0, sizeof(se_text_handle));
    text_handle->render_handle = render_handle;
    s_array_init(&text_handle->fonts, fonts_count);
    se_quad_2d_create(&text_handle->quad);
    return text_handle;
}

void se_text_handle_cleanup(se_text_handle* text_handle) {
    s_array_clear(&text_handle->fonts);
    se_quad_destroy(&text_handle->quad);
    free(text_handle);
    text_handle = NULL;
}

se_font* se_font_load(se_text_handle* text_handle, const char* path) {
    s_assertf(text_handle, "se_font_load :: text_handle is null");
    se_render_handle* render_handle = text_handle->render_handle;
    s_assertf(render_handle, "se_font_load :: render_handle is null");
    s_assertf(path, "se_font_load :: path is null");
    se_font* new_font = s_array_increment(&text_handle->fonts);
   
    sz font_file_size = 0;
    c8 new_path[SE_MAX_PATH_LENGTH] = "";
    strncpy(new_path, RESOURCES_DIR, SE_MAX_PATH_LENGTH - 1);
    strncat(new_path, path, SE_MAX_PATH_LENGTH - strlen(new_path) - 1);
    uc8* font_file_data = load_file_uc8(new_path, &font_file_size);

    s_assertf(font_file_data, "se_font_load :: file_data is null");
  
    const i32 font_count = stbtt_GetNumberOfFonts(font_file_data);
    if (font_count == 0) {
        printf("se_font_load :: No fonts found in file: %s\n", path);
        return NULL;
    }
    
    new_font->atlas_width = 1024;
    new_font->atlas_height = 1024;
    u8* atlas_bitmap = malloc(new_font->atlas_width * new_font->atlas_height);

    // There are 95 ASCII characters from ASCII 32(Space) to ASCII 126(~)
    // ASCII 32(Space) to ASCII 126(~) are the commonly used characters in text 
    new_font->first_character = 32;
    new_font->characters_count = 95;
    new_font->size = 64.0f;

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

    return new_font;
}

void se_init_text_render(se_text_handle* text_handle) {
    s_assertf(text_handle, "se_init_text_render :: text_handle is null");
    se_render_handle* render_handle = text_handle->render_handle;
    s_assertf(render_handle, "se_init_text_render :: render_handle is null");
    text_handle->text_shader = se_shader_load(render_handle, "shaders/text_vert.glsl", "shaders/text_frag.glsl");
}

void se_text_render(se_text_handle* text_handle, se_font* font, const c8* text, const se_vec2* position, const f32 size) {
    s_assertf(text_handle, "se_text_render :: text_handle is null");
    se_render_handle* render_handle = text_handle->render_handle;
    s_assertf(render_handle, "se_text_render :: render_handle is null");
    s_assertf(font, "se_text_render :: font is null");
    s_assertf(text, "se_text_render :: text is null");

    text_handle->text_vertex_index = 0; // TODO: Move to text_render_start

    // start of hack, this part should be called only once before rendering all text with the same font
    glBindTexture(GL_TEXTURE_2D, font->atlas_texture);
    glActiveTexture(GL_TEXTURE0);
    se_shader_set_texture(text_handle->text_shader, "u_atlas_texture", font->atlas_texture);
    // end of hack

    // TODO: Parse text, set it up, instance and push count + data to the GPU

    se_vec2 local_position = *position;

    i8 order[6] = { 0, 1, 2, 0, 2, 3 };
    f32 pixel_scale = 1.f / 1024.f; // TODO: dynamic for screen size-
   
    //s_array_init(&render_handle->text_vertices, strlen(text) * 6);
    
    for (i32 i = 0; i < strlen(text); i++) {
        c8 c = text[i];
        if (c >= font->first_character && c <= font->first_character + font->characters_count) {
            stbtt_packedchar* packed_char = s_array_get(&font->packed_characters, c - font->first_character);
            stbtt_aligned_quad* aligned_quad = s_array_get(&font->aligned_quads, c - font->first_character);

            se_vec2 glyph_size = se_vec2((packed_char->x1 - packed_char->x0) * pixel_scale * size,
                                        (packed_char->y1 - packed_char->y0) * pixel_scale * size);


            se_vec2 glyph_bounding_box_min = se_vec2(local_position.x + packed_char->xoff * pixel_scale * size,
                                                    local_position.y - (packed_char->yoff + packed_char->y1 - packed_char->y0) * pixel_scale * size);
        
            se_vec2 glyph_vertices[4] = {
                { glyph_bounding_box_min.x + glyph_size.x, glyph_bounding_box_min.y + glyph_size.y},
                { glyph_bounding_box_min.x, glyph_bounding_box_min.y + glyph_size.y},
                { glyph_bounding_box_min.x, glyph_bounding_box_min.y},
                { glyph_bounding_box_min.x + glyph_size.x, glyph_bounding_box_min.y}
            };
            se_vec2 glyph_texture_coords[4] = {
                { aligned_quad->s1, aligned_quad->t0 },
                { aligned_quad->s0, aligned_quad->t0 },
                { aligned_quad->s0, aligned_quad->t1 },
                { aligned_quad->s1, aligned_quad->t1 },
            };
            
            // TEMP
            s_array(se_vec2, temp_positions);
            s_array(se_vec2, temp_uvs);
            s_array_init(&temp_positions, 6);
            s_array_init(&temp_uvs, 6);

            for (i32 i = 0; i < 6; i++) {
                //se_vertex_3d* vertex = s_array_get(&render_handle->text_vertices, render_handle->text_vertex_index + i);
                //vertex->position = se_vec3(glyph_vertices[order[i]].x, glyph_vertices[order[i]].y, 0);
                se_vec2* vertex_position = s_array_increment(&temp_positions);
                *vertex_position = glyph_vertices[order[i]];
                se_vec2* vertex_uv = s_array_increment(&temp_uvs);
                *vertex_uv = glyph_texture_coords[order[i]];
            }

            printf("Character %c, position: %f, %f, uv: %f, %f\n", c, glyph_vertices[0].x, glyph_vertices[0].y, glyph_texture_coords[0].x, glyph_texture_coords[0].y);
            s_array_clear(&temp_positions);
            s_array_clear(&temp_uvs);
            
            //render_handle->text_vertex_index += 6;
            local_position.x += packed_char->xadvance * pixel_scale * size;
        }

        else if(c == '\n')
        {
            // advance y by fontSize, reset x-coordinate
            local_position.y -= font->size * pixel_scale * size;
            local_position.x = position->x;
        }
    }

    // start of hack, this part should be fully reworked 
    //sz size_of_vertices = s_array_get_size(&render_handle->text_vertices) * sizeof(se_vertex_3d);
    //u32 draw_calls_count = (size_of_vertices / SE_TEXT_VBO_SIZE) + 1;

    //se_mat4 view_projection_mat = mat4_ortho(0, 1024, 0, 768, 0, 1);

    //for (i32 i = 0; i < draw_calls_count; i++) {
    //    const se_vertex_3d* vertices_data = render_handle->text_vertices.data + i * SE_TEXT_VBO_SIZE;
    //    u32 vertices_count = 
    //        i == draw_calls_count - i ? 
    //            (size_of_vertices % SE_TEXT_VBO_SIZE) / sizeof(se_vertex_3d) :
    //            SE_TEXT_VBO_SIZE / sizeof(se_vertex_3d) * 6;
    //    // This does not make sense to be here
    //    se_shader_set_mat4(text_handle->text_shader, "u_view_projection_mat", &view_projection_mat);
    //    //glBindVertexArray(render_handle->text_vao);
    //    //glBindBuffer(GL_ARRAY_BUFFER, render_handle->text_vbo);
    //    // TODO: add this to se_gl.h
    //    //glBufferSubData(GL_ARRAY_BUFFER, 0, size_of_vertices, render_handle->text_vertices.data);
    //    //glDrawArrays(GL_TRIANGLES, 0, vertices_count);
    //}
    // end of hack
}

void se_font_cleanup(se_font* font) {
    glDeleteTextures(1, &font->atlas_texture);
}


