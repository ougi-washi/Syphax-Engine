// Syphax-Engine - Ougi Washi

#ifndef SE_TEXTURE_H
#define SE_TEXTURE_H

#include "se.h"
#include "se_defines.h"

typedef struct se_texture {
	char path[SE_MAX_PATH_LENGTH];
	u32 id;
	i32 width;
	i32 height;
	i32 channels;
} se_texture;
typedef s_array(se_texture, se_textures);
typedef se_texture *se_texture_ptr;
typedef s_array(se_texture_ptr, se_textures_ptr);

typedef enum { SE_REPEAT, SE_CLAMP } se_texture_wrap;
extern se_texture *se_texture_load(se_context *cxt, const char *path, const se_texture_wrap wrap);
extern se_texture *se_texture_load_from_memory(se_context *cxt, const u8 *data, const sz size, const se_texture_wrap wrap);
extern void se_texture_destroy(se_context *cxt, se_texture *texture);

#endif // SE_TEXTURE_H
