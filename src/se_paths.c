// Syphax-Engine - Ougi Washi

#include "se_defines.h"
#include "se_debug.h"

#include "syphax/s_files.h"

#include <stdio.h>
#include <string.h>

static char se_resource_root_override[SE_MAX_PATH_LENGTH] = {0};

static b8 se_path_is_absolute(const char* path) {
	if (!path || path[0] == '\0') {
		return false;
	}

	if (s_path_is_sep(path[0])) {
		return true;
	}

#ifdef _WIN32
	if (path[0] && path[1] == ':' && s_path_is_sep(path[2])) {
		return true;
	}
#endif

	return false;
}

void se_paths_set_resource_root(const char* path) {
	if (!path || path[0] == '\0') {
		se_resource_root_override[0] = '\0';
		se_set_last_error(SE_RESULT_OK);
		return;
	}

	const sz path_len = strlen(path);
	if (path_len >= SE_MAX_PATH_LENGTH) {
		se_log("se_paths_set_resource_root :: path too long");
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}

	memcpy(se_resource_root_override, path, path_len + 1);
	se_set_last_error(SE_RESULT_OK);
}

const char* se_paths_get_resource_root(void) {
	if (se_resource_root_override[0] != '\0') {
		return se_resource_root_override;
	}
	return RESOURCES_DIR;
}

b8 se_paths_resolve_resource_path(char* out_path, sz out_path_size, const char* path) {
	if (!out_path || out_path_size == 0 || !path || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	if (se_path_is_absolute(path)) {
		const sz path_len = strlen(path);
		if (path_len + 1 > out_path_size) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}
		memcpy(out_path, path, path_len + 1);
		se_set_last_error(SE_RESULT_OK);
		return true;
	}

	if (!s_path_join(out_path, out_path_size, se_paths_get_resource_root(), path)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_set_last_error(SE_RESULT_OK);
	return true;
}
