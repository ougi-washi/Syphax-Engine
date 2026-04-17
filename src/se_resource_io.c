// Syphax-Engine - Ougi Washi

#include "se_resource_io.h"

#include "window/se_window_backend_internal.h"
#include "syphax/s_files.h"

static b8 se_resource_resolve_path(const c8* path, c8* out_path, const sz out_path_size) {
	if (!path || !out_path || out_path_size == 0) {
		return false;
	}
	return se_paths_resolve_resource_path(out_path, out_path_size, path);
}

b8 se_resource_read_text_file(const c8* path, c8** out_data, sz* out_size) {
	if (!path || !out_data) {
		return false;
	}
	if (s_file_read(path, out_data, out_size)) {
		return true;
	}
	return se_window_backend_read_asset_text(path, out_data, out_size);
}

b8 se_resource_read_binary_file(const c8* path, u8** out_data, sz* out_size) {
	if (!path || !out_data) {
		return false;
	}
	if (s_file_read_binary(path, out_data, out_size)) {
		return true;
	}
	return se_window_backend_read_asset_binary(path, out_data, out_size);
}

b8 se_resource_read_text_path(const c8* path, c8** out_data, sz* out_size) {
	c8 resolved_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_resource_resolve_path(path, resolved_path, sizeof(resolved_path))) {
		return false;
	}
	return se_resource_read_text_file(resolved_path, out_data, out_size);
}

b8 se_resource_read_binary_path(const c8* path, u8** out_data, sz* out_size) {
	c8 resolved_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_resource_resolve_path(path, resolved_path, sizeof(resolved_path))) {
		return false;
	}
	return se_resource_read_binary_file(resolved_path, out_data, out_size);
}

b8 se_resource_file_mtime(const c8* path, time_t* out_mtime) {
	if (!path || !out_mtime) {
		return false;
	}
	if (s_file_mtime(path, out_mtime)) {
		return true;
	}
	return se_window_backend_get_asset_mtime(path, out_mtime);
}
