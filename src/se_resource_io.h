// Syphax-Engine - Ougi Washi

#ifndef SE_RESOURCE_IO_H
#define SE_RESOURCE_IO_H

#include "se_defines.h"
#include <time.h>

extern b8 se_resource_read_text_file(const c8* path, c8** out_data, sz* out_size);
extern b8 se_resource_read_binary_file(const c8* path, u8** out_data, sz* out_size);
extern b8 se_resource_read_text_path(const c8* path, c8** out_data, sz* out_size);
extern b8 se_resource_read_binary_path(const c8* path, u8** out_data, sz* out_size);
extern b8 se_resource_file_mtime(const c8* path, time_t* out_mtime);

#endif // SE_RESOURCE_IO_H
