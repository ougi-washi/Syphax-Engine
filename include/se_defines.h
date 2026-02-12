// Syphax-Engine - Ougi Washi

#ifndef SE_DEFINES_H
#define SE_DEFINES_H

#include "syphax/s_types.h"

#define SE_MAX_NAME_LENGTH 64
#define SE_MAX_PATH_LENGTH 256
#ifndef RESOURCES_DIR
#	define RESOURCES_DIR "resources/"
#endif

#define SE_RESOURCE_INTERNAL(path) "internal/" path
#define SE_RESOURCE_PUBLIC(path) "public/" path
#define SE_RESOURCE_EXAMPLE(path) "examples/" path

typedef enum {
	SE_RESULT_OK = 0,
	SE_RESULT_INVALID_ARGUMENT,
	SE_RESULT_OUT_OF_MEMORY,
	SE_RESULT_NOT_FOUND,
	SE_RESULT_IO,
	SE_RESULT_BACKEND_FAILURE,
	SE_RESULT_UNSUPPORTED,
	SE_RESULT_CAPACITY_EXCEEDED
} se_result;

extern const char* se_result_str(se_result result);
extern se_result se_get_last_error(void);
extern void se_set_last_error(se_result result);

extern void se_paths_set_resource_root(const char* path);
extern const char* se_paths_get_resource_root(void);
extern b8 se_paths_resolve_resource_path(char* out_path, sz out_path_size, const char* path);

#endif // SE_DEFINES_H
