// Syphax-Engine - Ougi Washi

#ifndef SE_TEXTURE_H
#define SE_TEXTURE_H

#include "se.h"
#include "se_defines.h"

typedef enum { SE_REPEAT, SE_CLAMP } se_texture_wrap;

typedef struct se_texture {
	char path[SE_MAX_PATH_LENGTH];
	u32 id;
	i32 width;
	i32 height;
	i32 depth;
	i32 channels;
	i32 cpu_channels;
	u32 target;
	se_texture_wrap wrap;
	u8 *cpu_pixels;
	sz cpu_pixels_size;
} se_texture;
typedef s_array(se_texture, se_textures);
typedef se_texture_handle se_texture_ptr;
typedef s_array(se_texture_handle, se_textures_ptr);

extern se_texture_handle se_texture_load(const char *path, const se_texture_wrap wrap);
extern se_texture_handle se_texture_load_from_memory(const u8 *data, const sz size, const se_texture_wrap wrap);
extern se_texture_handle se_texture_create_3d_rgba16f(const f32 *data, const u32 width, const u32 height, const u32 depth, const se_texture_wrap wrap);
extern se_texture_handle se_texture_find_by_id(const u32 texture_id);
extern b8 se_texture_sample_rgb(const se_texture_handle texture, const s_vec2 *uv, s_vec3 *out_color);
extern void se_texture_destroy(const se_texture_handle texture);

#endif // SE_TEXTURE_H
