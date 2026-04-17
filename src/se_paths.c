// Syphax-Engine - Ougi Washi

#include "se_defines.h"
#include "se_debug.h"
#include "window/se_window_backend_internal.h"

#include "syphax/s_files.h"

#include <stdio.h>
#include <string.h>

static char se_resource_root_override[SE_MAX_PATH_LENGTH] = {0};

static b8 se_path_has_prefix(const c8* path, const c8* prefix) {
	const sz prefix_len = prefix ? strlen(prefix) : 0u;
	return path && prefix && prefix_len > 0u && strncmp(path, prefix, prefix_len) == 0;
}

static const c8* se_path_skip_separators(const c8* path) {
	while (path && s_path_is_sep(path[0])) {
		path++;
	}
	return path;
}

static const c8* se_paths_get_resource_relative_path(const c8* path) {
	const c8* relative = path;
	const c8* root = se_paths_get_resource_root();
	if (!path || path[0] == '\0') {
		return NULL;
	}
	if (root && root[0] != '\0') {
		const sz root_len = strlen(root);
		if (root_len > 0u && strncmp(relative, root, root_len) == 0) {
			relative += root_len;
		}
	}
	relative = se_path_skip_separators(relative);
	if (se_path_has_prefix(relative, SE_RESOURCE_INTERNAL("")) ||
		se_path_has_prefix(relative, SE_RESOURCE_PUBLIC("")) ||
		se_path_has_prefix(relative, SE_RESOURCE_EXAMPLE(""))) {
		return relative;
	}
	return NULL;
}

static b8 se_paths_copy_path(c8* out_path, const sz out_path_size, const c8* path) {
	const sz path_len = strlen(path);
	if (path_len + 1 > out_path_size) {
		return false;
	}
	memcpy(out_path, path, path_len + 1);
	return true;
}

static b8 se_paths_create_parent_directory(const c8* path) {
	c8 parent_path[SE_MAX_PATH_LENGTH] = {0};
	if (!path || path[0] == '\0') {
		return false;
	}
	if (!s_path_parent(path, parent_path, sizeof(parent_path))) {
		return true;
	}
	if (parent_path[0] == '\0') {
		return true;
	}
	return s_directory_create(parent_path);
}

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

b8 se_paths_resolve_writable_path(char* out_path, sz out_path_size, const char* path) {
	const c8* resource_relative = NULL;
	if (!out_path || out_path_size == 0 || !path || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	if (se_window_backend_resolve_writable_path(path, out_path, out_path_size)) {
		if (!se_paths_create_parent_directory(out_path)) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		se_set_last_error(SE_RESULT_OK);
		return true;
	}

	resource_relative = se_paths_get_resource_relative_path(path);
	if (resource_relative != NULL) {
		if (!s_path_join(out_path, out_path_size, se_paths_get_resource_root(), resource_relative)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}
	} else if (!se_paths_copy_path(out_path, out_path_size, path)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	if (!se_paths_create_parent_directory(out_path)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}

	se_set_last_error(SE_RESULT_OK);
	return true;
}
