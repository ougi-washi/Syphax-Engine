// Syphax-Engine - Ougi Washi

#include "se_gltf.h"
#include "render/se_gl.h"

#include "syphax/s_files.h"
#include "syphax/s_json.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SE_GLTF_GLTF_MAGIC 0x46546C67u
#define SE_GLTF_CHUNK_JSON 0x4E4F534Au
#define SE_GLTF_CHUNK_BIN  0x004E4942u

typedef struct {
	char *json_text;
	sz json_size;
	u8 *bin_data;
	sz bin_size;
} se_gltf_glb_data;

static void se_gltf_set_default_load_params(se_gltf_load_params *out, const se_gltf_load_params *params) {
	if (params) {
		*out = *params;
		return;
	}
	out->load_buffers = true;
	out->load_images = true;
	out->decode_data_uris = true;
}

static void se_gltf_set_default_write_params(se_gltf_write_params *out, const se_gltf_write_params *params) {
	if (params) {
		*out = *params;
		return;
	}
	out->embed_buffers = false;
	out->embed_images = false;
	out->write_glb = false;
}

static s_json *se_gltf_json_clone(const s_json *node) {
	if (node == NULL) return NULL;
	s_json *clone = s_json_new(node->type, node->name);
	if (clone == NULL) return NULL;
	switch (node->type) {
		case S_JSON_STRING:
			clone->as.string = s_json_strdup(node->as.string ? node->as.string : "");
			if (clone->as.string == NULL) {
				s_json_free(clone);
				return NULL;
			}
			break;
		case S_JSON_BOOL:
			clone->as.boolean = node->as.boolean;
			break;
		case S_JSON_NUMBER:
			clone->as.number = node->as.number;
			break;
		case S_JSON_ARRAY:
		case S_JSON_OBJECT: {
			for (sz i = 0; i < node->as.children.count; i++) {
				s_json *child = node->as.children.items[i];
				s_json *child_clone = se_gltf_json_clone(child);
				if (child_clone == NULL || !s_json_add(clone, child_clone)) {
					s_json_free(child_clone);
					s_json_free(clone);
					return NULL;
				}
			}
			break;
		}
		default:
			break;
	}
	return clone;
}

static void se_gltf_parse_extras_extensions(const s_json *obj, s_json **out_extras, s_json **out_extensions) {
	if (out_extras) {
		s_json *extras = s_json_get(obj, "extras");
		*out_extras = extras ? se_gltf_json_clone(extras) : NULL;
	}
	if (out_extensions) {
		s_json *extensions = s_json_get(obj, "extensions");
		*out_extensions = extensions ? se_gltf_json_clone(extensions) : NULL;
	}
}

static b8 se_gltf_json_get_string_dup(const s_json *obj, const char *key, char **out) {
	if (out == NULL) return false;
	s_json *val = s_json_get(obj, key);
	if (val == NULL || val->type != S_JSON_STRING) return false;
	*out = s_files_strdup(val->as.string ? val->as.string : "");
	return *out != NULL;
}

static b8 se_gltf_json_get_i32(const s_json *obj, const char *key, i32 *out) {
	s_json *val = s_json_get(obj, key);
	if (val == NULL || val->type != S_JSON_NUMBER) return false;
	if (out) *out = (i32)val->as.number;
	return true;
}

static b8 se_gltf_json_get_u32(const s_json *obj, const char *key, u32 *out) {
	s_json *val = s_json_get(obj, key);
	if (val == NULL || val->type != S_JSON_NUMBER) return false;
	if (out) *out = (u32)val->as.number;
	return true;
}

static b8 se_gltf_json_get_f32(const s_json *obj, const char *key, f32 *out) {
	s_json *val = s_json_get(obj, key);
	if (val == NULL || val->type != S_JSON_NUMBER) return false;
	if (out) *out = (f32)val->as.number;
	return true;
}

static b8 se_gltf_json_get_bool(const s_json *obj, const char *key, b8 *out) {
	s_json *val = s_json_get(obj, key);
	if (val == NULL || val->type != S_JSON_BOOL) return false;
	if (out) *out = val->as.boolean;
	return true;
}

static b8 se_gltf_json_read_float_array(const s_json *arr, f32 *out, sz max, sz *out_count) {
	if (arr == NULL || arr->type != S_JSON_ARRAY) return false;
	sz count = arr->as.children.count;
	if (count > max) count = max;
	for (sz i = 0; i < count; i++) {
		s_json *item = arr->as.children.items[i];
		if (item == NULL || item->type != S_JSON_NUMBER) return false;
		out[i] = (f32)item->as.number;
	}
	if (out_count) *out_count = count;
	return true;
}

static b8 se_gltf_json_read_i32_array(const s_json *arr, se_gltf_i32_array *out) {
	if (arr == NULL || arr->type != S_JSON_ARRAY) return false;
	s_array_init(out, arr->as.children.count);
	for (sz i = 0; i < arr->as.children.count; i++) {
		s_json *item = arr->as.children.items[i];
		if (item == NULL || item->type != S_JSON_NUMBER) return false;
		i32 *dst = s_array_increment(out);
		*dst = (i32)item->as.number;
	}
	return true;
}

static b8 se_gltf_json_read_string_array(const s_json *arr, se_gltf_strings *out) {
	if (arr == NULL || arr->type != S_JSON_ARRAY) return false;
	s_array_init(out, arr->as.children.count);
	for (sz i = 0; i < arr->as.children.count; i++) {
		s_json *item = arr->as.children.items[i];
		if (item == NULL || item->type != S_JSON_STRING) return false;
		char **dst = s_array_increment(out);
		*dst = s_files_strdup(item->as.string ? item->as.string : "");
		if (*dst == NULL) return false;
	}
	return true;
}

static const char *se_gltf_accessor_type_to_string(se_gltf_accessor_type type) {
	switch (type) {
		case SE_GLTF_ACCESSOR_SCALAR: return "SCALAR";
		case SE_GLTF_ACCESSOR_VEC2: return "VEC2";
		case SE_GLTF_ACCESSOR_VEC3: return "VEC3";
		case SE_GLTF_ACCESSOR_VEC4: return "VEC4";
		case SE_GLTF_ACCESSOR_MAT2: return "MAT2";
		case SE_GLTF_ACCESSOR_MAT3: return "MAT3";
		case SE_GLTF_ACCESSOR_MAT4: return "MAT4";
		default: return "SCALAR";
	}
}

static sz se_gltf_accessor_component_count(se_gltf_accessor_type type) {
	switch (type) {
		case SE_GLTF_ACCESSOR_SCALAR: return 1;
		case SE_GLTF_ACCESSOR_VEC2: return 2;
		case SE_GLTF_ACCESSOR_VEC3: return 3;
		case SE_GLTF_ACCESSOR_VEC4: return 4;
		case SE_GLTF_ACCESSOR_MAT2: return 4;
		case SE_GLTF_ACCESSOR_MAT3: return 9;
		case SE_GLTF_ACCESSOR_MAT4: return 16;
		default: return 1;
	}
}

static b8 se_gltf_accessor_type_from_string(const char *str, se_gltf_accessor_type *out) {
	if (str == NULL || out == NULL) return false;
	if (strcmp(str, "SCALAR") == 0) *out = SE_GLTF_ACCESSOR_SCALAR;
	else if (strcmp(str, "VEC2") == 0) *out = SE_GLTF_ACCESSOR_VEC2;
	else if (strcmp(str, "VEC3") == 0) *out = SE_GLTF_ACCESSOR_VEC3;
	else if (strcmp(str, "VEC4") == 0) *out = SE_GLTF_ACCESSOR_VEC4;
	else if (strcmp(str, "MAT2") == 0) *out = SE_GLTF_ACCESSOR_MAT2;
	else if (strcmp(str, "MAT3") == 0) *out = SE_GLTF_ACCESSOR_MAT3;
	else if (strcmp(str, "MAT4") == 0) *out = SE_GLTF_ACCESSOR_MAT4;
	else return false;
	return true;
}

static b8 se_gltf_decode_base64(const char *in, u8 **out_data, sz *out_size) {
	if (in == NULL || out_data == NULL || out_size == NULL) return false;
	sz len = strlen(in);
	while (len > 0 && isspace((unsigned char)in[len - 1])) len--;
	if (len == 0) return false;
	sz padding = 0;
	if (len >= 1 && in[len - 1] == '=') padding++;
	if (len >= 2 && in[len - 2] == '=') padding++;
	sz out_len = (len / 4) * 3;
	if (padding > 0) out_len -= padding;
	u8 *out = (u8 *)malloc(out_len ? out_len : 1);
	if (out == NULL) return false;
	sz out_pos = 0;
	for (sz i = 0; i < len; i += 4) {
		u32 val = 0;
		u8 quad[4] = {0, 0, 0, 0};
		for (u8 j = 0; j < 4; j++) {
			char c = in[i + j];
			u8 v = 0;
			if (c >= 'A' && c <= 'Z') v = (u8)(c - 'A');
			else if (c >= 'a' && c <= 'z') v = (u8)(c - 'a' + 26);
			else if (c >= '0' && c <= '9') v = (u8)(c - '0' + 52);
			else if (c == '+') v = 62;
			else if (c == '/') v = 63;
			else if (c == '=') v = 0;
			quad[j] = v;
		}
		val = (quad[0] << 18) | (quad[1] << 12) | (quad[2] << 6) | quad[3];
		if (out_pos < out_len) out[out_pos++] = (u8)((val >> 16) & 0xFF);
		if (out_pos < out_len) out[out_pos++] = (u8)((val >> 8) & 0xFF);
		if (out_pos < out_len) out[out_pos++] = (u8)(val & 0xFF);
	}
	*out_data = out;
	*out_size = out_len;
	return true;
}

static char *se_gltf_encode_base64(const u8 *data, sz size, sz *out_len) {
	static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (data == NULL || size == 0) {
		char *out = (char *)malloc(1);
		if (out) out[0] = '\0';
		if (out_len) *out_len = 0;
		return out;
	}
	sz encoded_len = ((size + 2) / 3) * 4;
	char *out = (char *)malloc(encoded_len + 1);
	if (out == NULL) return NULL;
	sz j = 0;
	for (sz i = 0; i < size; i += 3) {
		u32 octet_a = i < size ? data[i] : 0;
		u32 octet_b = (i + 1) < size ? data[i + 1] : 0;
		u32 octet_c = (i + 2) < size ? data[i + 2] : 0;
		u32 triple = (octet_a << 16) | (octet_b << 8) | octet_c;
		out[j++] = table[(triple >> 18) & 0x3F];
		out[j++] = table[(triple >> 12) & 0x3F];
		out[j++] = (i + 1) < size ? table[(triple >> 6) & 0x3F] : '=';
		out[j++] = (i + 2) < size ? table[triple & 0x3F] : '=';
	}
	out[j] = '\0';
	if (out_len) *out_len = j;
	return out;
}

static b8 se_gltf_parse_data_uri(const char *uri, char **out_mime, u8 **out_data, sz *out_size) {
	if (uri == NULL || strncmp(uri, "data:", 5) != 0) return false;
	const char *comma = strchr(uri, ',');
	if (comma == NULL) return false;
	const char *meta = uri + 5;
	const char *data = comma + 1;
	b8 is_base64 = strstr(meta, ";base64") != NULL;
	if (!is_base64) return false;
	if (out_mime) {
		const char *semi = strchr(meta, ';');
		if (semi && semi > meta) {
			sz len = (sz)(semi - meta);
			char *mime = (char *)malloc(len + 1);
			if (mime == NULL) return false;
			memcpy(mime, meta, len);
			mime[len] = '\0';
			*out_mime = mime;
		} else {
			*out_mime = NULL;
		}
	}
	return se_gltf_decode_base64(data, out_data, out_size);
}

static b8 se_gltf_read_glb(const char *path, se_gltf_glb_data *out) {
	memset(out, 0, sizeof(*out));
	u8 *file_data = NULL;
	sz file_size = 0;
	if (!s_file_read_binary(path, &file_data, &file_size)) return false;
	if (file_size < 12) {
		free(file_data);
		return false;
	}
	u32 magic = *(u32 *)&file_data[0];
	u32 version = *(u32 *)&file_data[4];
	u32 length = *(u32 *)&file_data[8];
	if (magic != SE_GLTF_GLTF_MAGIC || version != 2 || length > file_size) {
		free(file_data);
		return false;
	}
	sz offset = 12;
	while (offset + 8 <= file_size) {
		u32 chunk_length = *(u32 *)&file_data[offset];
		u32 chunk_type = *(u32 *)&file_data[offset + 4];
		offset += 8;
		if (offset + chunk_length > file_size) break;
		if (chunk_type == SE_GLTF_CHUNK_JSON) {
			out->json_text = (char *)malloc(chunk_length + 1);
			if (out->json_text == NULL) {
				free(file_data);
				return false;
			}
			memcpy(out->json_text, file_data + offset, chunk_length);
			out->json_text[chunk_length] = '\0';
			out->json_size = chunk_length;
		} else if (chunk_type == SE_GLTF_CHUNK_BIN) {
			out->bin_data = (u8 *)malloc(chunk_length);
			if (out->bin_data == NULL) {
				free(file_data);
				return false;
			}
			memcpy(out->bin_data, file_data + offset, chunk_length);
			out->bin_size = chunk_length;
		}
		offset += chunk_length;
	}
	free(file_data);
	return out->json_text != NULL;
}

static void se_gltf_glb_data_free(se_gltf_glb_data *data) {
	if (data == NULL) return;
	free(data->json_text);
	free(data->bin_data);
	memset(data, 0, sizeof(*data));
}

static void se_gltf_strings_free(se_gltf_strings *arr) {
	s_foreach(arr, i) {
		char **str = s_array_get(arr, i);
		free(*str);
	}
	s_array_clear(arr);
}

static void se_gltf_attribute_set_free(se_gltf_attribute_set *set) {
	s_foreach(&set->attributes, i) {
		se_gltf_attribute *attr = s_array_get(&set->attributes, i);
		free(attr->name);
	}
	s_array_clear(&set->attributes);
}

static void se_gltf_primitive_free(se_gltf_primitive *prim) {
	se_gltf_attribute_set_free(&prim->attributes);
	s_foreach(&prim->targets, i) {
		se_gltf_attribute_set *target = s_array_get(&prim->targets, i);
		se_gltf_attribute_set_free(target);
	}
	s_array_clear(&prim->targets);
	s_json_free(prim->extras);
	s_json_free(prim->extensions);
}

static void se_gltf_mesh_free(se_gltf_mesh *mesh) {
	s_foreach(&mesh->primitives, i) {
		se_gltf_primitive *prim = s_array_get(&mesh->primitives, i);
		se_gltf_primitive_free(prim);
	}
	s_array_clear(&mesh->primitives);
	s_array_clear(&mesh->weights);
	free(mesh->name);
	s_json_free(mesh->extras);
	s_json_free(mesh->extensions);
}

static b8 se_gltf_parse_attribute_set(const s_json *obj, se_gltf_attribute_set *out) {
	if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
	s_array_init(&out->attributes, obj->as.children.count);
	for (sz i = 0; i < obj->as.children.count; i++) {
		s_json *child = obj->as.children.items[i];
		if (child == NULL || child->type != S_JSON_NUMBER || child->name == NULL) return false;
		se_gltf_attribute *attr = s_array_increment(&out->attributes);
		attr->name = s_files_strdup(child->name);
		attr->accessor = (i32)child->as.number;
		if (attr->name == NULL) return false;
	}
	return true;
}

static b8 se_gltf_parse_asset_info(const s_json *root, se_gltf_asset_info *asset) {
	s_json *asset_obj = s_json_get(root, "asset");
	if (asset_obj == NULL || asset_obj->type != S_JSON_OBJECT) {
		printf("se_gltf_load :: missing asset object\n");
		return false;
	}
	if (!se_gltf_json_get_string_dup(asset_obj, "version", &asset->version)) {
		printf("se_gltf_load :: asset.version missing\n");
		return false;
	}
	se_gltf_json_get_string_dup(asset_obj, "generator", &asset->generator);
	se_gltf_json_get_string_dup(asset_obj, "minVersion", &asset->min_version);
	se_gltf_json_get_string_dup(asset_obj, "copyright", &asset->copyright);
	se_gltf_parse_extras_extensions(asset_obj, &asset->extras, &asset->extensions);
	return true;
}

static b8 se_gltf_parse_buffers(const s_json *root, se_gltf_asset *asset, const se_gltf_load_params *params, const u8 *glb_bin, sz glb_bin_size) {
	s_json *buffers = s_json_get(root, "buffers");
	if (buffers == NULL) return true;
	if (buffers->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->buffers, buffers->as.children.count);
	for (sz i = 0; i < buffers->as.children.count; i++) {
		s_json *obj = buffers->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_buffer *buf = s_array_increment(&asset->buffers);
		memset(buf, 0, sizeof(*buf));
		se_gltf_json_get_string_dup(obj, "uri", &buf->uri);
		se_gltf_json_get_string_dup(obj, "name", &buf->name);
		if (!se_gltf_json_get_u32(obj, "byteLength", &buf->byte_length)) return false;
		se_gltf_parse_extras_extensions(obj, &buf->extras, &buf->extensions);
		if (!params->load_buffers) continue;
		if (buf->uri == NULL && glb_bin != NULL) {
			buf->data = (u8 *)malloc(glb_bin_size);
			if (buf->data == NULL) return false;
			memcpy(buf->data, glb_bin, glb_bin_size);
			buf->data_size = glb_bin_size;
			buf->owns_data = true;
			continue;
		}
		if (buf->uri != NULL) {
			u8 *data = NULL;
			sz size = 0;
			char *mime = NULL;
			if (params->decode_data_uris && se_gltf_parse_data_uri(buf->uri, &mime, &data, &size)) {
				free(mime);
				buf->data = data;
				buf->data_size = size;
				buf->owns_data = true;
			} else {
				char full_path[SE_MAX_PATH_LENGTH] = {0};
				if (asset->base_dir && asset->base_dir[0]) {
					if (!s_path_join(full_path, SE_MAX_PATH_LENGTH, asset->base_dir, buf->uri)) return false;
				} else {
					if (!s_path_join(full_path, SE_MAX_PATH_LENGTH, "", buf->uri)) return false;
				}
				if (!s_file_read_binary(full_path, &data, &size)) return false;
				buf->data = data;
				buf->data_size = size;
				buf->owns_data = true;
			}
		}
	}
	return true;
}

static b8 se_gltf_parse_buffer_views(const s_json *root, se_gltf_asset *asset) {
	s_json *views = s_json_get(root, "bufferViews");
	if (views == NULL) return true;
	if (views->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->buffer_views, views->as.children.count);
	for (sz i = 0; i < views->as.children.count; i++) {
		s_json *obj = views->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_buffer_view *view = s_array_increment(&asset->buffer_views);
		memset(view, 0, sizeof(*view));
		if (!se_gltf_json_get_i32(obj, "buffer", &view->buffer)) return false;
		se_gltf_json_get_u32(obj, "byteOffset", &view->byte_offset);
		if (!se_gltf_json_get_u32(obj, "byteLength", &view->byte_length)) return false;
		view->has_byte_stride = se_gltf_json_get_u32(obj, "byteStride", &view->byte_stride);
		view->has_target = se_gltf_json_get_i32(obj, "target", &view->target);
		se_gltf_json_get_string_dup(obj, "name", &view->name);
		se_gltf_parse_extras_extensions(obj, &view->extras, &view->extensions);
	}
	return true;
}

static b8 se_gltf_parse_accessor_sparse(const s_json *obj, se_gltf_accessor_sparse *sparse) {
	if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
	if (!se_gltf_json_get_u32(obj, "count", &sparse->count)) return false;
	s_json *indices = s_json_get(obj, "indices");
	if (indices == NULL || indices->type != S_JSON_OBJECT) return false;
	if (!se_gltf_json_get_i32(indices, "bufferView", &sparse->indices.buffer_view)) return false;
	sparse->indices.has_byte_offset = se_gltf_json_get_u32(indices, "byteOffset", &sparse->indices.byte_offset);
	if (!se_gltf_json_get_i32(indices, "componentType", &sparse->indices.component_type)) return false;
	se_gltf_parse_extras_extensions(indices, &sparse->indices.extras, &sparse->indices.extensions);
	s_json *values = s_json_get(obj, "values");
	if (values == NULL || values->type != S_JSON_OBJECT) return false;
	if (!se_gltf_json_get_i32(values, "bufferView", &sparse->values.buffer_view)) return false;
	sparse->values.has_byte_offset = se_gltf_json_get_u32(values, "byteOffset", &sparse->values.byte_offset);
	se_gltf_parse_extras_extensions(values, &sparse->values.extras, &sparse->values.extensions);
	se_gltf_parse_extras_extensions(obj, &sparse->extras, &sparse->extensions);
	return true;
}

static b8 se_gltf_parse_accessors(const s_json *root, se_gltf_asset *asset) {
	s_json *accessors = s_json_get(root, "accessors");
	if (accessors == NULL) return true;
	if (accessors->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->accessors, accessors->as.children.count);
	for (sz i = 0; i < accessors->as.children.count; i++) {
		s_json *obj = accessors->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_accessor *acc = s_array_increment(&asset->accessors);
		memset(acc, 0, sizeof(*acc));
		acc->has_buffer_view = se_gltf_json_get_i32(obj, "bufferView", &acc->buffer_view);
		acc->has_byte_offset = se_gltf_json_get_u32(obj, "byteOffset", &acc->byte_offset);
		if (!se_gltf_json_get_u32(obj, "count", &acc->count)) return false;
		if (!se_gltf_json_get_i32(obj, "componentType", &acc->component_type)) return false;
		acc->has_normalized = se_gltf_json_get_bool(obj, "normalized", &acc->normalized);
		s_json *type_node = s_json_get(obj, "type");
		if (type_node == NULL || type_node->type != S_JSON_STRING) return false;
		if (!se_gltf_accessor_type_from_string(type_node->as.string, &acc->type)) return false;
		s_json *min_node = s_json_get(obj, "min");
		if (min_node) {
			acc->has_min = se_gltf_json_read_float_array(min_node, acc->min_values, 16, NULL);
		}
		s_json *max_node = s_json_get(obj, "max");
		if (max_node) {
			acc->has_max = se_gltf_json_read_float_array(max_node, acc->max_values, 16, NULL);
		}
		s_json *sparse_node = s_json_get(obj, "sparse");
		if (sparse_node) {
			acc->has_sparse = se_gltf_parse_accessor_sparse(sparse_node, &acc->sparse);
			if (!acc->has_sparse) return false;
		}
		se_gltf_json_get_string_dup(obj, "name", &acc->name);
		se_gltf_parse_extras_extensions(obj, &acc->extras, &acc->extensions);
	}
	return true;
}

static b8 se_gltf_parse_images(const s_json *root, se_gltf_asset *asset, const se_gltf_load_params *params) {
	s_json *images = s_json_get(root, "images");
	if (images == NULL) return true;
	if (images->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->images, images->as.children.count);
	for (sz i = 0; i < images->as.children.count; i++) {
		s_json *obj = images->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_image *img = s_array_increment(&asset->images);
		memset(img, 0, sizeof(*img));
		se_gltf_json_get_string_dup(obj, "uri", &img->uri);
		se_gltf_json_get_string_dup(obj, "mimeType", &img->mime_type);
		img->has_buffer_view = se_gltf_json_get_i32(obj, "bufferView", &img->buffer_view);
		se_gltf_json_get_string_dup(obj, "name", &img->name);
		se_gltf_parse_extras_extensions(obj, &img->extras, &img->extensions);
		if (!params->load_images) continue;
		if (img->uri != NULL) {
			u8 *data = NULL;
			sz size = 0;
			char *mime = NULL;
			if (params->decode_data_uris && se_gltf_parse_data_uri(img->uri, &mime, &data, &size)) {
				if (img->mime_type == NULL && mime) img->mime_type = mime; else free(mime);
				img->data = data;
				img->data_size = size;
				img->owns_data = true;
			} else {
				char full_path[SE_MAX_PATH_LENGTH] = {0};
				if (asset->base_dir && asset->base_dir[0]) {
					if (!s_path_join(full_path, SE_MAX_PATH_LENGTH, asset->base_dir, img->uri)) return false;
				} else {
					if (!s_path_join(full_path, SE_MAX_PATH_LENGTH, "", img->uri)) return false;
				}
				if (!s_file_read_binary(full_path, &data, &size)) return false;
				img->data = data;
				img->data_size = size;
				img->owns_data = true;
			}
		} else if (img->has_buffer_view) {
			if (img->buffer_view < 0 || (sz)img->buffer_view >= asset->buffer_views.size) return false;
			se_gltf_buffer_view *view = s_array_get(&asset->buffer_views, img->buffer_view);
			if (view->buffer < 0 || (sz)view->buffer >= asset->buffers.size) return false;
			se_gltf_buffer *buf = s_array_get(&asset->buffers, view->buffer);
			if (buf->data == NULL) continue;
			if (view->byte_offset + view->byte_length > buf->data_size) return false;
			img->data = (u8 *)malloc(view->byte_length);
			if (img->data == NULL) return false;
			memcpy(img->data, buf->data + view->byte_offset, view->byte_length);
			img->data_size = view->byte_length;
			img->owns_data = true;
		}
	}
	return true;
}

static b8 se_gltf_parse_samplers(const s_json *root, se_gltf_asset *asset) {
	s_json *samplers = s_json_get(root, "samplers");
	if (samplers == NULL) return true;
	if (samplers->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->samplers, samplers->as.children.count);
	for (sz i = 0; i < samplers->as.children.count; i++) {
		s_json *obj = samplers->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_sampler *sampler = s_array_increment(&asset->samplers);
		memset(sampler, 0, sizeof(*sampler));
		sampler->has_mag_filter = se_gltf_json_get_i32(obj, "magFilter", &sampler->mag_filter);
		sampler->has_min_filter = se_gltf_json_get_i32(obj, "minFilter", &sampler->min_filter);
		sampler->has_wrap_s = se_gltf_json_get_i32(obj, "wrapS", &sampler->wrap_s);
		sampler->has_wrap_t = se_gltf_json_get_i32(obj, "wrapT", &sampler->wrap_t);
		se_gltf_json_get_string_dup(obj, "name", &sampler->name);
		se_gltf_parse_extras_extensions(obj, &sampler->extras, &sampler->extensions);
	}
	return true;
}

static b8 se_gltf_parse_textures(const s_json *root, se_gltf_asset *asset) {
	s_json *textures = s_json_get(root, "textures");
	if (textures == NULL) return true;
	if (textures->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->textures, textures->as.children.count);
	for (sz i = 0; i < textures->as.children.count; i++) {
		s_json *obj = textures->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_texture *tex = s_array_increment(&asset->textures);
		memset(tex, 0, sizeof(*tex));
		tex->has_sampler = se_gltf_json_get_i32(obj, "sampler", &tex->sampler);
		tex->has_source = se_gltf_json_get_i32(obj, "source", &tex->source);
		se_gltf_json_get_string_dup(obj, "name", &tex->name);
		se_gltf_parse_extras_extensions(obj, &tex->extras, &tex->extensions);
	}
	return true;
}

static b8 se_gltf_parse_texture_info(const s_json *obj, se_gltf_texture_info *info) {
	if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
	if (!se_gltf_json_get_i32(obj, "index", &info->index)) return false;
	info->has_tex_coord = se_gltf_json_get_i32(obj, "texCoord", &info->tex_coord);
	se_gltf_parse_extras_extensions(obj, &info->extras, &info->extensions);
	return true;
}

static b8 se_gltf_parse_materials(const s_json *root, se_gltf_asset *asset) {
	s_json *materials = s_json_get(root, "materials");
	if (materials == NULL) return true;
	if (materials->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->materials, materials->as.children.count);
	for (sz i = 0; i < materials->as.children.count; i++) {
		s_json *obj = materials->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_material *mat = s_array_increment(&asset->materials);
		memset(mat, 0, sizeof(*mat));
		se_gltf_json_get_string_dup(obj, "name", &mat->name);
		s_json *pbr = s_json_get(obj, "pbrMetallicRoughness");
		if (pbr) {
			mat->has_pbr_metallic_roughness = true;
			se_gltf_pbr_metallic_roughness *dst = &mat->pbr_metallic_roughness;
			memset(dst, 0, sizeof(*dst));
			s_json *base_color_factor = s_json_get(pbr, "baseColorFactor");
			if (base_color_factor) dst->has_base_color_factor = se_gltf_json_read_float_array(base_color_factor, dst->base_color_factor, 4, NULL);
			dst->has_metallic_factor = se_gltf_json_get_f32(pbr, "metallicFactor", &dst->metallic_factor);
			dst->has_roughness_factor = se_gltf_json_get_f32(pbr, "roughnessFactor", &dst->roughness_factor);
			s_json *bct = s_json_get(pbr, "baseColorTexture");
			if (bct) {
				dst->has_base_color_texture = se_gltf_parse_texture_info(bct, &dst->base_color_texture);
			}
			s_json *mrt = s_json_get(pbr, "metallicRoughnessTexture");
			if (mrt) {
				dst->has_metallic_roughness_texture = se_gltf_parse_texture_info(mrt, &dst->metallic_roughness_texture);
			}
			se_gltf_parse_extras_extensions(pbr, &dst->extras, &dst->extensions);
		}
		s_json *normal_tex = s_json_get(obj, "normalTexture");
		if (normal_tex) {
			mat->has_normal_texture = true;
			mat->normal_texture.has_scale = se_gltf_json_get_f32(normal_tex, "scale", &mat->normal_texture.scale);
			if (!se_gltf_parse_texture_info(normal_tex, &mat->normal_texture.info)) return false;
		}
		s_json *occlusion_tex = s_json_get(obj, "occlusionTexture");
		if (occlusion_tex) {
			mat->has_occlusion_texture = true;
			mat->occlusion_texture.has_strength = se_gltf_json_get_f32(occlusion_tex, "strength", &mat->occlusion_texture.strength);
			if (!se_gltf_parse_texture_info(occlusion_tex, &mat->occlusion_texture.info)) return false;
		}
		s_json *emissive_tex = s_json_get(obj, "emissiveTexture");
		if (emissive_tex) {
			mat->has_emissive_texture = se_gltf_parse_texture_info(emissive_tex, &mat->emissive_texture);
			if (!mat->has_emissive_texture) return false;
		}
		s_json *emissive_factor = s_json_get(obj, "emissiveFactor");
		if (emissive_factor) mat->has_emissive_factor = se_gltf_json_read_float_array(emissive_factor, mat->emissive_factor, 3, NULL);
		mat->has_alpha_mode = se_gltf_json_get_string_dup(obj, "alphaMode", &mat->alpha_mode);
		mat->has_alpha_cutoff = se_gltf_json_get_f32(obj, "alphaCutoff", &mat->alpha_cutoff);
		mat->has_double_sided = se_gltf_json_get_bool(obj, "doubleSided", &mat->double_sided);
		se_gltf_parse_extras_extensions(obj, &mat->extras, &mat->extensions);
	}
	return true;
}

static b8 se_gltf_parse_meshes(const s_json *root, se_gltf_asset *asset) {
	s_json *meshes = s_json_get(root, "meshes");
	if (meshes == NULL) return true;
	if (meshes->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->meshes, meshes->as.children.count);
	for (sz i = 0; i < meshes->as.children.count; i++) {
		s_json *obj = meshes->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_mesh *mesh = s_array_increment(&asset->meshes);
		memset(mesh, 0, sizeof(*mesh));
		se_gltf_json_get_string_dup(obj, "name", &mesh->name);
		s_json *prims = s_json_get(obj, "primitives");
		if (prims == NULL || prims->type != S_JSON_ARRAY) return false;
		s_array_init(&mesh->primitives, prims->as.children.count);
		for (sz p = 0; p < prims->as.children.count; p++) {
			s_json *prim_obj = prims->as.children.items[p];
			if (prim_obj == NULL || prim_obj->type != S_JSON_OBJECT) return false;
			se_gltf_primitive *prim = s_array_increment(&mesh->primitives);
			memset(prim, 0, sizeof(*prim));
			s_json *attrs = s_json_get(prim_obj, "attributes");
			if (!se_gltf_parse_attribute_set(attrs, &prim->attributes)) return false;
			prim->has_indices = se_gltf_json_get_i32(prim_obj, "indices", &prim->indices);
			prim->has_material = se_gltf_json_get_i32(prim_obj, "material", &prim->material);
			prim->has_mode = se_gltf_json_get_i32(prim_obj, "mode", &prim->mode);
			s_json *targets = s_json_get(prim_obj, "targets");
			if (targets && targets->type == S_JSON_ARRAY) {
				s_array_init(&prim->targets, targets->as.children.count);
				for (sz t = 0; t < targets->as.children.count; t++) {
					s_json *target_obj = targets->as.children.items[t];
					se_gltf_attribute_set *target = s_array_increment(&prim->targets);
					if (!se_gltf_parse_attribute_set(target_obj, target)) return false;
				}
			}
			se_gltf_parse_extras_extensions(prim_obj, &prim->extras, &prim->extensions);
		}
		s_json *weights = s_json_get(obj, "weights");
		if (weights && weights->type == S_JSON_ARRAY) {
			mesh->has_weights = true;
			s_array_init(&mesh->weights, weights->as.children.count);
			for (sz w = 0; w < weights->as.children.count; w++) {
				s_json *item = weights->as.children.items[w];
				if (item == NULL || item->type != S_JSON_NUMBER) return false;
				f32 *dst = s_array_increment(&mesh->weights);
				*dst = (f32)item->as.number;
			}
		}
		se_gltf_parse_extras_extensions(obj, &mesh->extras, &mesh->extensions);
	}
	return true;
}

static b8 se_gltf_parse_nodes(const s_json *root, se_gltf_asset *asset) {
	s_json *nodes = s_json_get(root, "nodes");
	if (nodes == NULL) return true;
	if (nodes->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->nodes, nodes->as.children.count);
	for (sz i = 0; i < nodes->as.children.count; i++) {
		s_json *obj = nodes->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_node *node = s_array_increment(&asset->nodes);
		memset(node, 0, sizeof(*node));
		se_gltf_json_get_string_dup(obj, "name", &node->name);
		s_json *children = s_json_get(obj, "children");
		if (children && children->type == S_JSON_ARRAY) {
			if (!se_gltf_json_read_i32_array(children, &node->children)) return false;
		}
		node->has_mesh = se_gltf_json_get_i32(obj, "mesh", &node->mesh);
		node->has_skin = se_gltf_json_get_i32(obj, "skin", &node->skin);
		node->has_camera = se_gltf_json_get_i32(obj, "camera", &node->camera);
		s_json *matrix = s_json_get(obj, "matrix");
		if (matrix) node->has_matrix = se_gltf_json_read_float_array(matrix, node->matrix, 16, NULL);
		s_json *translation = s_json_get(obj, "translation");
		if (translation) node->has_translation = se_gltf_json_read_float_array(translation, node->translation, 3, NULL);
		s_json *rotation = s_json_get(obj, "rotation");
		if (rotation) node->has_rotation = se_gltf_json_read_float_array(rotation, node->rotation, 4, NULL);
		s_json *scale = s_json_get(obj, "scale");
		if (scale) node->has_scale = se_gltf_json_read_float_array(scale, node->scale, 3, NULL);
		s_json *weights = s_json_get(obj, "weights");
		if (weights && weights->type == S_JSON_ARRAY) {
			node->has_weights = true;
			s_array_init(&node->weights, weights->as.children.count);
			for (sz w = 0; w < weights->as.children.count; w++) {
				s_json *item = weights->as.children.items[w];
				if (item == NULL || item->type != S_JSON_NUMBER) return false;
				f32 *dst = s_array_increment(&node->weights);
				*dst = (f32)item->as.number;
			}
		}
		se_gltf_parse_extras_extensions(obj, &node->extras, &node->extensions);
	}
	return true;
}

static b8 se_gltf_parse_scenes(const s_json *root, se_gltf_asset *asset) {
	s_json *scenes = s_json_get(root, "scenes");
	if (scenes == NULL) return true;
	if (scenes->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->scenes, scenes->as.children.count);
	for (sz i = 0; i < scenes->as.children.count; i++) {
		s_json *obj = scenes->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_scene *scene = s_array_increment(&asset->scenes);
		memset(scene, 0, sizeof(*scene));
		se_gltf_json_get_string_dup(obj, "name", &scene->name);
		s_json *nodes = s_json_get(obj, "nodes");
		if (nodes && nodes->type == S_JSON_ARRAY) {
			if (!se_gltf_json_read_i32_array(nodes, &scene->nodes)) return false;
		}
		se_gltf_parse_extras_extensions(obj, &scene->extras, &scene->extensions);
	}
	return true;
}

static b8 se_gltf_parse_skins(const s_json *root, se_gltf_asset *asset) {
	s_json *skins = s_json_get(root, "skins");
	if (skins == NULL) return true;
	if (skins->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->skins, skins->as.children.count);
	for (sz i = 0; i < skins->as.children.count; i++) {
		s_json *obj = skins->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_skin *skin = s_array_increment(&asset->skins);
		memset(skin, 0, sizeof(*skin));
		se_gltf_json_get_string_dup(obj, "name", &skin->name);
		skin->has_inverse_bind_matrices = se_gltf_json_get_i32(obj, "inverseBindMatrices", &skin->inverse_bind_matrices);
		skin->has_skeleton = se_gltf_json_get_i32(obj, "skeleton", &skin->skeleton);
		s_json *joints = s_json_get(obj, "joints");
		if (joints == NULL || joints->type != S_JSON_ARRAY) return false;
		if (!se_gltf_json_read_i32_array(joints, &skin->joints)) return false;
		se_gltf_parse_extras_extensions(obj, &skin->extras, &skin->extensions);
	}
	return true;
}

static b8 se_gltf_parse_animations(const s_json *root, se_gltf_asset *asset) {
	s_json *animations = s_json_get(root, "animations");
	if (animations == NULL) return true;
	if (animations->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->animations, animations->as.children.count);
	for (sz i = 0; i < animations->as.children.count; i++) {
		s_json *obj = animations->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_animation *anim = s_array_increment(&asset->animations);
		memset(anim, 0, sizeof(*anim));
		se_gltf_json_get_string_dup(obj, "name", &anim->name);
		s_json *samplers = s_json_get(obj, "samplers");
		if (samplers == NULL || samplers->type != S_JSON_ARRAY) return false;
		s_array_init(&anim->samplers, samplers->as.children.count);
		for (sz s = 0; s < samplers->as.children.count; s++) {
			s_json *sampler_obj = samplers->as.children.items[s];
			if (sampler_obj == NULL || sampler_obj->type != S_JSON_OBJECT) return false;
			se_gltf_animation_sampler *sampler = s_array_increment(&anim->samplers);
			memset(sampler, 0, sizeof(*sampler));
			if (!se_gltf_json_get_i32(sampler_obj, "input", &sampler->input)) return false;
			if (!se_gltf_json_get_i32(sampler_obj, "output", &sampler->output)) return false;
			sampler->has_interpolation = se_gltf_json_get_string_dup(sampler_obj, "interpolation", &sampler->interpolation);
			se_gltf_parse_extras_extensions(sampler_obj, &sampler->extras, &sampler->extensions);
		}
		s_json *channels = s_json_get(obj, "channels");
		if (channels == NULL || channels->type != S_JSON_ARRAY) return false;
		s_array_init(&anim->channels, channels->as.children.count);
		for (sz c = 0; c < channels->as.children.count; c++) {
			s_json *channel_obj = channels->as.children.items[c];
			if (channel_obj == NULL || channel_obj->type != S_JSON_OBJECT) return false;
			se_gltf_animation_channel *channel = s_array_increment(&anim->channels);
			memset(channel, 0, sizeof(*channel));
			if (!se_gltf_json_get_i32(channel_obj, "sampler", &channel->sampler)) return false;
			s_json *target = s_json_get(channel_obj, "target");
			if (target == NULL || target->type != S_JSON_OBJECT) return false;
			channel->target.has_node = se_gltf_json_get_i32(target, "node", &channel->target.node);
			if (!se_gltf_json_get_string_dup(target, "path", &channel->target.path)) return false;
			se_gltf_parse_extras_extensions(target, &channel->target.extras, &channel->target.extensions);
			se_gltf_parse_extras_extensions(channel_obj, &channel->extras, &channel->extensions);
		}
		se_gltf_parse_extras_extensions(obj, &anim->extras, &anim->extensions);
	}
	return true;
}

static b8 se_gltf_parse_cameras(const s_json *root, se_gltf_asset *asset) {
	s_json *cameras = s_json_get(root, "cameras");
	if (cameras == NULL) return true;
	if (cameras->type != S_JSON_ARRAY) return false;
	s_array_init(&asset->cameras, cameras->as.children.count);
	for (sz i = 0; i < cameras->as.children.count; i++) {
		s_json *obj = cameras->as.children.items[i];
		if (obj == NULL || obj->type != S_JSON_OBJECT) return false;
		se_gltf_camera *cam = s_array_increment(&asset->cameras);
		memset(cam, 0, sizeof(*cam));
		se_gltf_json_get_string_dup(obj, "name", &cam->name);
		cam->has_type = se_gltf_json_get_string_dup(obj, "type", &cam->type);
		s_json *persp = s_json_get(obj, "perspective");
		if (persp && persp->type == S_JSON_OBJECT) {
			cam->has_perspective = true;
			se_gltf_camera_perspective *p = &cam->perspective;
			if (!se_gltf_json_get_f32(persp, "yfov", &p->yfov)) return false;
			if (!se_gltf_json_get_f32(persp, "znear", &p->znear)) return false;
			p->has_zfar = se_gltf_json_get_f32(persp, "zfar", &p->zfar);
			p->has_aspect_ratio = se_gltf_json_get_f32(persp, "aspectRatio", &p->aspect_ratio);
		}
		s_json *ortho = s_json_get(obj, "orthographic");
		if (ortho && ortho->type == S_JSON_OBJECT) {
			cam->has_orthographic = true;
			se_gltf_camera_orthographic *o = &cam->orthographic;
			if (!se_gltf_json_get_f32(ortho, "xmag", &o->xmag)) return false;
			if (!se_gltf_json_get_f32(ortho, "ymag", &o->ymag)) return false;
			if (!se_gltf_json_get_f32(ortho, "znear", &o->znear)) return false;
			if (!se_gltf_json_get_f32(ortho, "zfar", &o->zfar)) return false;
		}
		se_gltf_parse_extras_extensions(obj, &cam->extras, &cam->extensions);
	}
	return true;
}

static b8 se_gltf_parse_top_level(const s_json *root, se_gltf_asset *asset) {
	se_gltf_parse_extras_extensions(root, &asset->extras, &asset->extensions);
	s_json *extensions_used = s_json_get(root, "extensionsUsed");
	if (extensions_used && !se_gltf_json_read_string_array(extensions_used, &asset->extensions_used)) return false;
	s_json *extensions_required = s_json_get(root, "extensionsRequired");
	if (extensions_required && !se_gltf_json_read_string_array(extensions_required, &asset->extensions_required)) return false;
	asset->has_default_scene = se_gltf_json_get_i32(root, "scene", &asset->default_scene);
	return true;
}

static void se_gltf_asset_init(se_gltf_asset *asset) {
	memset(asset, 0, sizeof(*asset));
	asset->default_scene = -1;
}

static void se_gltf_asset_free(se_gltf_asset *asset) {
	if (asset == NULL) return;
	free(asset->asset.version);
	free(asset->asset.generator);
	free(asset->asset.min_version);
	free(asset->asset.copyright);
	s_json_free(asset->asset.extras);
	s_json_free(asset->asset.extensions);
	se_gltf_strings_free(&asset->extensions_used);
	se_gltf_strings_free(&asset->extensions_required);
	s_foreach(&asset->buffers, i) {
		se_gltf_buffer *buf = s_array_get(&asset->buffers, i);
		free(buf->uri);
		free(buf->name);
		if (buf->owns_data) free(buf->data);
		s_json_free(buf->extras);
		s_json_free(buf->extensions);
	}
	s_array_clear(&asset->buffers);
	s_foreach(&asset->buffer_views, i) {
		se_gltf_buffer_view *view = s_array_get(&asset->buffer_views, i);
		free(view->name);
		s_json_free(view->extras);
		s_json_free(view->extensions);
	}
	s_array_clear(&asset->buffer_views);
	s_foreach(&asset->accessors, i) {
		se_gltf_accessor *acc = s_array_get(&asset->accessors, i);
		free(acc->name);
		s_json_free(acc->extras);
		s_json_free(acc->extensions);
		s_json_free(acc->sparse.extras);
		s_json_free(acc->sparse.extensions);
		s_json_free(acc->sparse.indices.extras);
		s_json_free(acc->sparse.indices.extensions);
		s_json_free(acc->sparse.values.extras);
		s_json_free(acc->sparse.values.extensions);
	}
	s_array_clear(&asset->accessors);
	s_foreach(&asset->images, i) {
		se_gltf_image *img = s_array_get(&asset->images, i);
		free(img->uri);
		free(img->mime_type);
		free(img->name);
		if (img->owns_data) free(img->data);
		s_json_free(img->extras);
		s_json_free(img->extensions);
	}
	s_array_clear(&asset->images);
	s_foreach(&asset->samplers, i) {
		se_gltf_sampler *sampler = s_array_get(&asset->samplers, i);
		free(sampler->name);
		s_json_free(sampler->extras);
		s_json_free(sampler->extensions);
	}
	s_array_clear(&asset->samplers);
	s_foreach(&asset->textures, i) {
		se_gltf_texture *tex = s_array_get(&asset->textures, i);
		free(tex->name);
		s_json_free(tex->extras);
		s_json_free(tex->extensions);
	}
	s_array_clear(&asset->textures);
	s_foreach(&asset->materials, i) {
		se_gltf_material *mat = s_array_get(&asset->materials, i);
		free(mat->name);
		free(mat->alpha_mode);
		s_json_free(mat->extras);
		s_json_free(mat->extensions);
		s_json_free(mat->pbr_metallic_roughness.extras);
		s_json_free(mat->pbr_metallic_roughness.extensions);
		if (mat->has_normal_texture) {
			s_json_free(mat->normal_texture.info.extras);
			s_json_free(mat->normal_texture.info.extensions);
		}
		if (mat->has_occlusion_texture) {
			s_json_free(mat->occlusion_texture.info.extras);
			s_json_free(mat->occlusion_texture.info.extensions);
		}
		if (mat->has_emissive_texture) {
			s_json_free(mat->emissive_texture.extras);
			s_json_free(mat->emissive_texture.extensions);
		}
		if (mat->has_pbr_metallic_roughness) {
			if (mat->pbr_metallic_roughness.has_base_color_texture) {
				s_json_free(mat->pbr_metallic_roughness.base_color_texture.extras);
				s_json_free(mat->pbr_metallic_roughness.base_color_texture.extensions);
			}
			if (mat->pbr_metallic_roughness.has_metallic_roughness_texture) {
				s_json_free(mat->pbr_metallic_roughness.metallic_roughness_texture.extras);
				s_json_free(mat->pbr_metallic_roughness.metallic_roughness_texture.extensions);
			}
		}
	}
	s_array_clear(&asset->materials);
	s_foreach(&asset->meshes, i) {
		se_gltf_mesh *mesh = s_array_get(&asset->meshes, i);
		se_gltf_mesh_free(mesh);
	}
	s_array_clear(&asset->meshes);
	s_foreach(&asset->nodes, i) {
		se_gltf_node *node = s_array_get(&asset->nodes, i);
		free(node->name);
		s_array_clear(&node->children);
		s_array_clear(&node->weights);
		s_json_free(node->extras);
		s_json_free(node->extensions);
	}
	s_array_clear(&asset->nodes);
	s_foreach(&asset->scenes, i) {
		se_gltf_scene *scene = s_array_get(&asset->scenes, i);
		free(scene->name);
		s_array_clear(&scene->nodes);
		s_json_free(scene->extras);
		s_json_free(scene->extensions);
	}
	s_array_clear(&asset->scenes);
	s_foreach(&asset->skins, i) {
		se_gltf_skin *skin = s_array_get(&asset->skins, i);
		free(skin->name);
		s_array_clear(&skin->joints);
		s_json_free(skin->extras);
		s_json_free(skin->extensions);
	}
	s_array_clear(&asset->skins);
	s_foreach(&asset->animations, i) {
		se_gltf_animation *anim = s_array_get(&asset->animations, i);
		free(anim->name);
		s_foreach(&anim->samplers, s) {
			se_gltf_animation_sampler *sampler = s_array_get(&anim->samplers, s);
			free(sampler->interpolation);
			s_json_free(sampler->extras);
			s_json_free(sampler->extensions);
		}
		s_foreach(&anim->channels, c) {
			se_gltf_animation_channel *channel = s_array_get(&anim->channels, c);
			free(channel->target.path);
			s_json_free(channel->target.extras);
			s_json_free(channel->target.extensions);
			s_json_free(channel->extras);
			s_json_free(channel->extensions);
		}
		s_array_clear(&anim->samplers);
		s_array_clear(&anim->channels);
		s_json_free(anim->extras);
		s_json_free(anim->extensions);
	}
	s_array_clear(&asset->animations);
	s_foreach(&asset->cameras, i) {
		se_gltf_camera *cam = s_array_get(&asset->cameras, i);
		free(cam->name);
		free(cam->type);
		s_json_free(cam->extras);
		s_json_free(cam->extensions);
	}
	s_array_clear(&asset->cameras);
	free(asset->source_path);
	free(asset->base_dir);
	s_json_free(asset->extras);
	s_json_free(asset->extensions);
}

se_gltf_asset *se_gltf_load(const char *path, const se_gltf_load_params *params) {
	if (path == NULL || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_gltf_load_params cfg;
	se_gltf_set_default_load_params(&cfg, params);
	se_gltf_asset *asset = (se_gltf_asset *)malloc(sizeof(se_gltf_asset));
	if (asset == NULL) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	se_gltf_asset_init(asset);
	asset->source_path = s_files_strdup(path);
	{
		char base[SE_MAX_PATH_LENGTH] = {0};
		if (s_path_parent(path, base, SE_MAX_PATH_LENGTH)) {
			asset->base_dir = s_files_strdup(base);
		}
	}
	const char *ext = s_path_extension(path);
	char *json_text = NULL;
	s_json *root = NULL;
	se_gltf_glb_data glb = {0};
	const u8 *bin_data = NULL;
	sz bin_size = 0;
	if (ext && strcmp(ext, ".glb") == 0) {
		if (!se_gltf_read_glb(path, &glb)) {
			se_gltf_free(asset);
			se_set_last_error(SE_RESULT_IO);
			return NULL;
		}
		json_text = glb.json_text;
		bin_data = glb.bin_data;
		bin_size = glb.bin_size;
	} else {
		if (!s_file_read(path, &json_text, NULL)) {
			se_gltf_free(asset);
			se_set_last_error(SE_RESULT_IO);
			return NULL;
		}
	}
	root = s_json_parse(json_text);
	if (root == NULL) {
		free(json_text);
		se_gltf_glb_data_free(&glb);
		se_gltf_free(asset);
		se_set_last_error(SE_RESULT_IO);
		return NULL;
	}
	if (!se_gltf_parse_asset_info(root, &asset->asset) ||
		!se_gltf_parse_top_level(root, asset) ||
		!se_gltf_parse_buffers(root, asset, &cfg, bin_data, bin_size) ||
		!se_gltf_parse_buffer_views(root, asset) ||
		!se_gltf_parse_accessors(root, asset) ||
		!se_gltf_parse_images(root, asset, &cfg) ||
		!se_gltf_parse_samplers(root, asset) ||
		!se_gltf_parse_textures(root, asset) ||
		!se_gltf_parse_materials(root, asset) ||
		!se_gltf_parse_meshes(root, asset) ||
		!se_gltf_parse_nodes(root, asset) ||
		!se_gltf_parse_scenes(root, asset) ||
		!se_gltf_parse_skins(root, asset) ||
		!se_gltf_parse_animations(root, asset) ||
		!se_gltf_parse_cameras(root, asset)) {
		s_json_free(root);
		free(json_text);
		se_gltf_glb_data_free(&glb);
		se_gltf_free(asset);
		se_set_last_error(SE_RESULT_IO);
		return NULL;
	}
	s_json_free(root);
	free(json_text);
	se_gltf_glb_data_free(&glb);
	se_set_last_error(SE_RESULT_OK);
	return asset;
}

void se_gltf_free(se_gltf_asset *asset) {
	if (asset == NULL) return;
	se_gltf_asset_free(asset);
	free(asset);
}

static s_json *se_gltf_json_add_object(s_json *parent, const char *name) {
	s_json *obj = s_json_object_empty(name);
	if (obj == NULL || !s_json_add(parent, obj)) {
		s_json_free(obj);
		return NULL;
	}
	return obj;
}

static b8 se_gltf_json_add_string(s_json *parent, const char *name, const char *value) {
	s_json *node = s_json_str(name, value ? value : "");
	if (node == NULL) return false;
	if (!s_json_add(parent, node)) {
		s_json_free(node);
		return false;
	}
	return true;
}

static b8 se_gltf_json_add_number(s_json *parent, const char *name, f64 value) {
	s_json *node = s_json_num(name, value);
	if (node == NULL) return false;
	if (!s_json_add(parent, node)) {
		s_json_free(node);
		return false;
	}
	return true;
}

static b8 se_gltf_json_add_bool(s_json *parent, const char *name, b8 value) {
	s_json *node = s_json_bool(name, value);
	if (node == NULL) return false;
	if (!s_json_add(parent, node)) {
		s_json_free(node);
		return false;
	}
	return true;
}

static s_json *se_gltf_json_add_array(s_json *parent, const char *name) {
	s_json *arr = s_json_array_empty(name);
	if (arr == NULL || !s_json_add(parent, arr)) {
		s_json_free(arr);
		return NULL;
	}
	return arr;
}

static b8 se_gltf_json_add_float_array(s_json *parent, const char *name, const f32 *values, sz count) {
	s_json *arr = s_json_array_empty(name);
	if (arr == NULL) return false;
	for (sz i = 0; i < count; i++) {
		s_json *num = s_json_num(NULL, values[i]);
		if (num == NULL || !s_json_add(arr, num)) {
			s_json_free(num);
			s_json_free(arr);
			return false;
		}
	}
	if (!s_json_add(parent, arr)) {
		s_json_free(arr);
		return false;
	}
	return true;
}

static b8 se_gltf_json_add_i32_array(s_json *parent, const char *name, const se_gltf_i32_array *arr) {
	s_json *node = s_json_array_empty(name);
	if (node == NULL) return false;
	for (sz i = 0; i < arr->size; i++) {
		s_json *num = s_json_num(NULL, arr->data[i]);
		if (num == NULL || !s_json_add(node, num)) {
			s_json_free(num);
			s_json_free(node);
			return false;
		}
	}
	if (!s_json_add(parent, node)) {
		s_json_free(node);
		return false;
	}
	return true;
}

static b8 se_gltf_json_add_string_array(s_json *parent, const char *name, const se_gltf_strings *arr) {
	s_json *node = s_json_array_empty(name);
	if (node == NULL) return false;
	for (sz i = 0; i < arr->size; i++) {
		s_json *str = s_json_str(NULL, arr->data[i]);
		if (str == NULL || !s_json_add(node, str)) {
			s_json_free(str);
			s_json_free(node);
			return false;
		}
	}
	if (!s_json_add(parent, node)) {
		s_json_free(node);
		return false;
	}
	return true;
}

static b8 se_gltf_json_add_extras_extensions(s_json *parent, const s_json *extras, const s_json *extensions) {
	if (extras) {
		s_json *clone = se_gltf_json_clone(extras);
		if (clone == NULL || !s_json_add(parent, clone)) {
			s_json_free(clone);
			return false;
		}
	}
	if (extensions) {
		s_json *clone = se_gltf_json_clone(extensions);
		if (clone == NULL || !s_json_add(parent, clone)) {
			s_json_free(clone);
			return false;
		}
	}
	return true;
}

static b8 se_gltf_write_buffers_json(const se_gltf_asset *asset, s_json *root, const se_gltf_write_params *params, const char *base_name, char ***out_generated_uris, sz *out_generated_uri_count) {
	if (asset->buffers.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "buffers");
	if (arr == NULL) return false;
	*out_generated_uri_count = asset->buffers.size;
	*out_generated_uris = (char **)calloc(asset->buffers.size, sizeof(char *));
	for (sz i = 0; i < asset->buffers.size; i++) {
		se_gltf_buffer *buf = s_array_get(&asset->buffers, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		u32 byte_length = buf->byte_length ? buf->byte_length : (u32)buf->data_size;
		if (!se_gltf_json_add_number(obj, "byteLength", byte_length)) return false;
		char *uri = NULL;
		if (!params->write_glb) {
			if (params->embed_buffers && buf->data) {
				sz encoded_len = 0;
				char *encoded = se_gltf_encode_base64(buf->data, buf->data_size, &encoded_len);
				if (encoded == NULL) return false;
				const char *mime = "application/octet-stream";
				sz uri_len = strlen("data:") + strlen(mime) + strlen(";base64,") + encoded_len + 1;
				uri = (char *)malloc(uri_len);
				if (uri == NULL) { free(encoded); return false; }
				snprintf(uri, uri_len, "data:%s;base64,%s", mime, encoded);
				free(encoded);
			} else if (buf->uri) {
				uri = s_files_strdup(buf->uri);
			} else {
				char name_buf[SE_MAX_PATH_LENGTH] = {0};
				if (asset->buffers.size == 1) {
					snprintf(name_buf, sizeof(name_buf), "%s.bin", base_name);
				} else {
					snprintf(name_buf, sizeof(name_buf), "%s_buffer%zu.bin", base_name, i);
				}
				uri = s_files_strdup(name_buf);
			}
			if (uri == NULL) return false;
			(*out_generated_uris)[i] = uri;
			if (!se_gltf_json_add_string(obj, "uri", uri)) return false;
		}
		if (buf->name) if (!se_gltf_json_add_string(obj, "name", buf->name)) return false;
		if (!se_gltf_json_add_extras_extensions(obj, buf->extras, buf->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_glb_buffers_json(const se_gltf_asset *asset, s_json *root, sz bin_size) {
	s_json *arr = se_gltf_json_add_array(root, "buffers");
	if (arr == NULL) return false;
	s_json *obj = s_json_object_empty(NULL);
	if (obj == NULL) return false;
	if (!se_gltf_json_add_number(obj, "byteLength", (u32)bin_size)) return false;
	if (asset->buffers.size > 0) {
		se_gltf_buffer *buf = s_array_get(&asset->buffers, 0);
		if (buf->name) if (!se_gltf_json_add_string(obj, "name", buf->name)) return false;
		if (!se_gltf_json_add_extras_extensions(obj, buf->extras, buf->extensions)) return false;
	}
	if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	return true;
}

static b8 se_gltf_write_assets_json(const se_gltf_asset *asset, s_json *root) {
	s_json *asset_obj = s_json_object_empty("asset");
	if (asset_obj == NULL) return false;
	if (!se_gltf_json_add_string(asset_obj, "version", asset->asset.version ? asset->asset.version : "2.0")) return false;
	if (asset->asset.generator) if (!se_gltf_json_add_string(asset_obj, "generator", asset->asset.generator)) return false;
	if (asset->asset.min_version) if (!se_gltf_json_add_string(asset_obj, "minVersion", asset->asset.min_version)) return false;
	if (asset->asset.copyright) if (!se_gltf_json_add_string(asset_obj, "copyright", asset->asset.copyright)) return false;
	if (!se_gltf_json_add_extras_extensions(asset_obj, asset->asset.extras, asset->asset.extensions)) return false;
	if (!s_json_add(root, asset_obj)) { s_json_free(asset_obj); return false; }
	return true;
}

static b8 se_gltf_write_buffer_views_json(const se_gltf_asset *asset, s_json *root, const u32 *buffer_offsets, b8 remap_buffers) {
	if (asset->buffer_views.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "bufferViews");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->buffer_views.size; i++) {
		se_gltf_buffer_view *view = s_array_get(&asset->buffer_views, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		i32 buffer_index = view->buffer;
		u32 byte_offset = view->byte_offset;
		if (remap_buffers && buffer_offsets) {
			byte_offset += buffer_offsets[buffer_index];
			buffer_index = 0;
		}
		if (!se_gltf_json_add_number(obj, "buffer", buffer_index)) return false;
		if (byte_offset != 0) if (!se_gltf_json_add_number(obj, "byteOffset", byte_offset)) return false;
		if (!se_gltf_json_add_number(obj, "byteLength", view->byte_length)) return false;
		if (view->has_byte_stride) if (!se_gltf_json_add_number(obj, "byteStride", view->byte_stride)) return false;
		if (view->has_target) if (!se_gltf_json_add_number(obj, "target", view->target)) return false;
		if (view->name) if (!se_gltf_json_add_string(obj, "name", view->name)) return false;
		if (!se_gltf_json_add_extras_extensions(obj, view->extras, view->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_accessors_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->accessors.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "accessors");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->accessors.size; i++) {
		se_gltf_accessor *acc = s_array_get(&asset->accessors, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		if (acc->has_buffer_view) if (!se_gltf_json_add_number(obj, "bufferView", acc->buffer_view)) return false;
		if (acc->has_byte_offset && acc->byte_offset != 0) if (!se_gltf_json_add_number(obj, "byteOffset", acc->byte_offset)) return false;
		if (!se_gltf_json_add_number(obj, "count", acc->count)) return false;
		if (!se_gltf_json_add_number(obj, "componentType", acc->component_type)) return false;
		if (acc->has_normalized) if (!se_gltf_json_add_bool(obj, "normalized", acc->normalized)) return false;
		if (!se_gltf_json_add_string(obj, "type", se_gltf_accessor_type_to_string(acc->type))) return false;
		sz component_count = se_gltf_accessor_component_count(acc->type);
		if (acc->has_min) if (!se_gltf_json_add_float_array(obj, "min", acc->min_values, component_count)) return false;
		if (acc->has_max) if (!se_gltf_json_add_float_array(obj, "max", acc->max_values, component_count)) return false;
		if (acc->has_sparse) {
			s_json *sparse = se_gltf_json_add_object(obj, "sparse");
			if (sparse == NULL) return false;
			if (!se_gltf_json_add_number(sparse, "count", acc->sparse.count)) return false;
			s_json *indices = se_gltf_json_add_object(sparse, "indices");
			if (indices == NULL) return false;
			if (!se_gltf_json_add_number(indices, "bufferView", acc->sparse.indices.buffer_view)) return false;
			if (acc->sparse.indices.has_byte_offset && acc->sparse.indices.byte_offset != 0) if (!se_gltf_json_add_number(indices, "byteOffset", acc->sparse.indices.byte_offset)) return false;
			if (!se_gltf_json_add_number(indices, "componentType", acc->sparse.indices.component_type)) return false;
			if (!se_gltf_json_add_extras_extensions(indices, acc->sparse.indices.extras, acc->sparse.indices.extensions)) return false;
			s_json *values = se_gltf_json_add_object(sparse, "values");
			if (values == NULL) return false;
			if (!se_gltf_json_add_number(values, "bufferView", acc->sparse.values.buffer_view)) return false;
			if (acc->sparse.values.has_byte_offset && acc->sparse.values.byte_offset != 0) if (!se_gltf_json_add_number(values, "byteOffset", acc->sparse.values.byte_offset)) return false;
			if (!se_gltf_json_add_extras_extensions(values, acc->sparse.values.extras, acc->sparse.values.extensions)) return false;
			if (!se_gltf_json_add_extras_extensions(sparse, acc->sparse.extras, acc->sparse.extensions)) return false;
		}
		if (acc->name) if (!se_gltf_json_add_string(obj, "name", acc->name)) return false;
		if (!se_gltf_json_add_extras_extensions(obj, acc->extras, acc->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static void se_gltf_mesh_finalize(se_mesh *mesh, se_vertex_3d *vertices, u32 *indices, u32 vertex_count, u32 index_count) {
	mesh->vertices = malloc(vertex_count * sizeof(se_vertex_3d));
	mesh->indices = malloc(index_count * sizeof(u32));
	memcpy(mesh->vertices, vertices, vertex_count * sizeof(se_vertex_3d));
	memcpy(mesh->indices, indices, index_count * sizeof(u32));
	mesh->vertex_count = vertex_count;
	mesh->index_count = index_count;
	mesh->matrix = s_mat4_identity;
	mesh->shader = NULL;
	mesh->texture_id = 0;
	glGenVertexArrays(1, &mesh->vao);
	glGenBuffers(1, &mesh->vbo);
	glGenBuffers(1, &mesh->ebo);
	glBindVertexArray(mesh->vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(se_vertex_3d), mesh->vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(u32), mesh->indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)offsetof(se_vertex_3d, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)offsetof(se_vertex_3d, uv));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
}

static b8 se_gltf_write_images_json(const se_gltf_asset *asset, s_json *root, const se_gltf_write_params *params, const char *base_name, char **out_generated_uris) {
	if (asset->images.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "images");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->images.size; i++) {
		se_gltf_image *img = s_array_get(&asset->images, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		char *uri = NULL;
		b8 keep_uri = false;
		if (img->uri && !params->embed_images) {
			uri = s_files_strdup(img->uri);
		} else if (params->embed_images && img->data) {
			sz encoded_len = 0;
			char *encoded = se_gltf_encode_base64(img->data, img->data_size, &encoded_len);
			if (encoded == NULL) return false;
			const char *mime = img->mime_type ? img->mime_type : "application/octet-stream";
			sz uri_len = strlen("data:") + strlen(mime) + strlen(";base64,") + encoded_len + 1;
			uri = (char *)malloc(uri_len);
			if (uri == NULL) { free(encoded); return false; }
			snprintf(uri, uri_len, "data:%s;base64,%s", mime, encoded);
			free(encoded);
		} else if (img->data && !params->embed_images) {
			char name_buf[SE_MAX_PATH_LENGTH] = {0};
			const char *ext = ".bin";
			if (img->mime_type) {
				if (strcmp(img->mime_type, "image/png") == 0) ext = ".png";
				else if (strcmp(img->mime_type, "image/jpeg") == 0) ext = ".jpg";
			}
			snprintf(name_buf, sizeof(name_buf), "%s_image%zu%s", base_name, i, ext);
			uri = s_files_strdup(name_buf);
			out_generated_uris[i] = uri;
			keep_uri = true;
		}
		if (uri) {
			if (!se_gltf_json_add_string(obj, "uri", uri)) return false;
			if (!keep_uri) free(uri);
		}
		if (img->has_buffer_view) if (!se_gltf_json_add_number(obj, "bufferView", img->buffer_view)) return false;
		if (img->mime_type) if (!se_gltf_json_add_string(obj, "mimeType", img->mime_type)) return false;
		if (img->name) if (!se_gltf_json_add_string(obj, "name", img->name)) return false;
		if (!se_gltf_json_add_extras_extensions(obj, img->extras, img->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_samplers_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->samplers.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "samplers");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->samplers.size; i++) {
		se_gltf_sampler *sampler = s_array_get(&asset->samplers, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		if (sampler->has_mag_filter) if (!se_gltf_json_add_number(obj, "magFilter", sampler->mag_filter)) return false;
		if (sampler->has_min_filter) if (!se_gltf_json_add_number(obj, "minFilter", sampler->min_filter)) return false;
		if (sampler->has_wrap_s) if (!se_gltf_json_add_number(obj, "wrapS", sampler->wrap_s)) return false;
		if (sampler->has_wrap_t) if (!se_gltf_json_add_number(obj, "wrapT", sampler->wrap_t)) return false;
		if (sampler->name) if (!se_gltf_json_add_string(obj, "name", sampler->name)) return false;
		if (!se_gltf_json_add_extras_extensions(obj, sampler->extras, sampler->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_textures_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->textures.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "textures");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->textures.size; i++) {
		se_gltf_texture *tex = s_array_get(&asset->textures, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		if (tex->has_sampler) if (!se_gltf_json_add_number(obj, "sampler", tex->sampler)) return false;
		if (tex->has_source) if (!se_gltf_json_add_number(obj, "source", tex->source)) return false;
		if (tex->name) if (!se_gltf_json_add_string(obj, "name", tex->name)) return false;
		if (!se_gltf_json_add_extras_extensions(obj, tex->extras, tex->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_texture_info_json(s_json *parent, const char *name, const se_gltf_texture_info *info) {
	s_json *obj = s_json_object_empty(name);
	if (obj == NULL) return false;
	if (!se_gltf_json_add_number(obj, "index", info->index)) return false;
	if (info->has_tex_coord) if (!se_gltf_json_add_number(obj, "texCoord", info->tex_coord)) return false;
	if (!se_gltf_json_add_extras_extensions(obj, info->extras, info->extensions)) return false;
	if (!s_json_add(parent, obj)) { s_json_free(obj); return false; }
	return true;
}

static b8 se_gltf_write_materials_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->materials.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "materials");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->materials.size; i++) {
		se_gltf_material *mat = s_array_get(&asset->materials, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		if (mat->name) if (!se_gltf_json_add_string(obj, "name", mat->name)) return false;
		if (mat->has_pbr_metallic_roughness) {
			s_json *pbr = se_gltf_json_add_object(obj, "pbrMetallicRoughness");
			if (pbr == NULL) return false;
			if (mat->pbr_metallic_roughness.has_base_color_factor) if (!se_gltf_json_add_float_array(pbr, "baseColorFactor", mat->pbr_metallic_roughness.base_color_factor, 4)) return false;
			if (mat->pbr_metallic_roughness.has_metallic_factor) if (!se_gltf_json_add_number(pbr, "metallicFactor", mat->pbr_metallic_roughness.metallic_factor)) return false;
			if (mat->pbr_metallic_roughness.has_roughness_factor) if (!se_gltf_json_add_number(pbr, "roughnessFactor", mat->pbr_metallic_roughness.roughness_factor)) return false;
			if (mat->pbr_metallic_roughness.has_base_color_texture) if (!se_gltf_write_texture_info_json(pbr, "baseColorTexture", &mat->pbr_metallic_roughness.base_color_texture)) return false;
			if (mat->pbr_metallic_roughness.has_metallic_roughness_texture) if (!se_gltf_write_texture_info_json(pbr, "metallicRoughnessTexture", &mat->pbr_metallic_roughness.metallic_roughness_texture)) return false;
			if (!se_gltf_json_add_extras_extensions(pbr, mat->pbr_metallic_roughness.extras, mat->pbr_metallic_roughness.extensions)) return false;
		}
		if (mat->has_normal_texture) {
			s_json *normal = s_json_object_empty("normalTexture");
			if (normal == NULL) return false;
			if (!se_gltf_json_add_number(normal, "index", mat->normal_texture.info.index)) return false;
			if (mat->normal_texture.info.has_tex_coord) if (!se_gltf_json_add_number(normal, "texCoord", mat->normal_texture.info.tex_coord)) return false;
			if (mat->normal_texture.has_scale) if (!se_gltf_json_add_number(normal, "scale", mat->normal_texture.scale)) return false;
			if (!se_gltf_json_add_extras_extensions(normal, mat->normal_texture.info.extras, mat->normal_texture.info.extensions)) return false;
			if (!s_json_add(obj, normal)) { s_json_free(normal); return false; }
		}
		if (mat->has_occlusion_texture) {
			s_json *occ = s_json_object_empty("occlusionTexture");
			if (occ == NULL) return false;
			if (!se_gltf_json_add_number(occ, "index", mat->occlusion_texture.info.index)) return false;
			if (mat->occlusion_texture.info.has_tex_coord) if (!se_gltf_json_add_number(occ, "texCoord", mat->occlusion_texture.info.tex_coord)) return false;
			if (mat->occlusion_texture.has_strength) if (!se_gltf_json_add_number(occ, "strength", mat->occlusion_texture.strength)) return false;
			if (!se_gltf_json_add_extras_extensions(occ, mat->occlusion_texture.info.extras, mat->occlusion_texture.info.extensions)) return false;
			if (!s_json_add(obj, occ)) { s_json_free(occ); return false; }
		}
		if (mat->has_emissive_texture) if (!se_gltf_write_texture_info_json(obj, "emissiveTexture", &mat->emissive_texture)) return false;
		if (mat->has_emissive_factor) if (!se_gltf_json_add_float_array(obj, "emissiveFactor", mat->emissive_factor, 3)) return false;
		if (mat->has_alpha_mode) if (!se_gltf_json_add_string(obj, "alphaMode", mat->alpha_mode)) return false;
		if (mat->has_alpha_cutoff) if (!se_gltf_json_add_number(obj, "alphaCutoff", mat->alpha_cutoff)) return false;
		if (mat->has_double_sided) if (!se_gltf_json_add_bool(obj, "doubleSided", mat->double_sided)) return false;
		if (!se_gltf_json_add_extras_extensions(obj, mat->extras, mat->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_meshes_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->meshes.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "meshes");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->meshes.size; i++) {
		se_gltf_mesh *mesh = s_array_get(&asset->meshes, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		if (mesh->name) if (!se_gltf_json_add_string(obj, "name", mesh->name)) return false;
		s_json *prims = se_gltf_json_add_array(obj, "primitives");
		if (prims == NULL) return false;
		for (sz p = 0; p < mesh->primitives.size; p++) {
			se_gltf_primitive *prim = s_array_get(&mesh->primitives, p);
			s_json *prim_obj = s_json_object_empty(NULL);
			if (prim_obj == NULL) return false;
			s_json *attrs = s_json_object_empty("attributes");
			if (attrs == NULL) return false;
			for (sz a = 0; a < prim->attributes.attributes.size; a++) {
				se_gltf_attribute *attr = s_array_get(&prim->attributes.attributes, a);
				s_json *num = s_json_num(attr->name, attr->accessor);
				if (num == NULL || !s_json_add(attrs, num)) { s_json_free(num); return false; }
			}
			if (!s_json_add(prim_obj, attrs)) { s_json_free(attrs); return false; }
			if (prim->has_indices) if (!se_gltf_json_add_number(prim_obj, "indices", prim->indices)) return false;
			if (prim->has_material) if (!se_gltf_json_add_number(prim_obj, "material", prim->material)) return false;
			if (prim->has_mode) if (!se_gltf_json_add_number(prim_obj, "mode", prim->mode)) return false;
			if (prim->targets.size > 0) {
				s_json *targets = se_gltf_json_add_array(prim_obj, "targets");
				if (targets == NULL) return false;
				for (sz t = 0; t < prim->targets.size; t++) {
					se_gltf_attribute_set *target = s_array_get(&prim->targets, t);
					s_json *target_obj = s_json_object_empty(NULL);
					if (target_obj == NULL) return false;
					for (sz a = 0; a < target->attributes.size; a++) {
						se_gltf_attribute *attr = s_array_get(&target->attributes, a);
						s_json *num = s_json_num(attr->name, attr->accessor);
						if (num == NULL || !s_json_add(target_obj, num)) { s_json_free(num); return false; }
					}
					if (!s_json_add(targets, target_obj)) { s_json_free(target_obj); return false; }
				}
			}
			if (!se_gltf_json_add_extras_extensions(prim_obj, prim->extras, prim->extensions)) return false;
			if (!s_json_add(prims, prim_obj)) { s_json_free(prim_obj); return false; }
		}
		if (mesh->has_weights && mesh->weights.size > 0) {
			s_json *weights = s_json_array_empty("weights");
			if (weights == NULL) return false;
			for (sz w = 0; w < mesh->weights.size; w++) {
				s_json *num = s_json_num(NULL, mesh->weights.data[w]);
				if (num == NULL || !s_json_add(weights, num)) { s_json_free(num); return false; }
			}
			if (!s_json_add(obj, weights)) { s_json_free(weights); return false; }
		}
		if (!se_gltf_json_add_extras_extensions(obj, mesh->extras, mesh->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_nodes_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->nodes.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "nodes");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->nodes.size; i++) {
		se_gltf_node *node = s_array_get(&asset->nodes, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		if (node->name) if (!se_gltf_json_add_string(obj, "name", node->name)) return false;
		if (node->children.size > 0) if (!se_gltf_json_add_i32_array(obj, "children", &node->children)) return false;
		if (node->has_mesh) if (!se_gltf_json_add_number(obj, "mesh", node->mesh)) return false;
		if (node->has_skin) if (!se_gltf_json_add_number(obj, "skin", node->skin)) return false;
		if (node->has_camera) if (!se_gltf_json_add_number(obj, "camera", node->camera)) return false;
		if (node->has_matrix) if (!se_gltf_json_add_float_array(obj, "matrix", node->matrix, 16)) return false;
		if (node->has_translation) if (!se_gltf_json_add_float_array(obj, "translation", node->translation, 3)) return false;
		if (node->has_rotation) if (!se_gltf_json_add_float_array(obj, "rotation", node->rotation, 4)) return false;
		if (node->has_scale) if (!se_gltf_json_add_float_array(obj, "scale", node->scale, 3)) return false;
		if (node->has_weights && node->weights.size > 0) {
			s_json *weights = s_json_array_empty("weights");
			if (weights == NULL) return false;
			for (sz w = 0; w < node->weights.size; w++) {
				s_json *num = s_json_num(NULL, node->weights.data[w]);
				if (num == NULL || !s_json_add(weights, num)) { s_json_free(num); return false; }
			}
			if (!s_json_add(obj, weights)) { s_json_free(weights); return false; }
		}
		if (!se_gltf_json_add_extras_extensions(obj, node->extras, node->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_scenes_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->scenes.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "scenes");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->scenes.size; i++) {
		se_gltf_scene *scene = s_array_get(&asset->scenes, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		if (scene->name) if (!se_gltf_json_add_string(obj, "name", scene->name)) return false;
		if (scene->nodes.size > 0) if (!se_gltf_json_add_i32_array(obj, "nodes", &scene->nodes)) return false;
		if (!se_gltf_json_add_extras_extensions(obj, scene->extras, scene->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	if (asset->has_default_scene) if (!se_gltf_json_add_number(root, "scene", asset->default_scene)) return false;
	return true;
}

static b8 se_gltf_write_skins_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->skins.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "skins");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->skins.size; i++) {
		se_gltf_skin *skin = s_array_get(&asset->skins, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		if (skin->name) if (!se_gltf_json_add_string(obj, "name", skin->name)) return false;
		if (skin->has_inverse_bind_matrices) if (!se_gltf_json_add_number(obj, "inverseBindMatrices", skin->inverse_bind_matrices)) return false;
		if (skin->has_skeleton) if (!se_gltf_json_add_number(obj, "skeleton", skin->skeleton)) return false;
		if (!se_gltf_json_add_i32_array(obj, "joints", &skin->joints)) return false;
		if (!se_gltf_json_add_extras_extensions(obj, skin->extras, skin->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_animations_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->animations.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "animations");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->animations.size; i++) {
		se_gltf_animation *anim = s_array_get(&asset->animations, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		if (anim->name) if (!se_gltf_json_add_string(obj, "name", anim->name)) return false;
		s_json *samplers = se_gltf_json_add_array(obj, "samplers");
		if (samplers == NULL) return false;
		for (sz s = 0; s < anim->samplers.size; s++) {
			se_gltf_animation_sampler *sampler = s_array_get(&anim->samplers, s);
			s_json *sampler_obj = s_json_object_empty(NULL);
			if (sampler_obj == NULL) return false;
			if (!se_gltf_json_add_number(sampler_obj, "input", sampler->input)) return false;
			if (!se_gltf_json_add_number(sampler_obj, "output", sampler->output)) return false;
			if (sampler->has_interpolation) if (!se_gltf_json_add_string(sampler_obj, "interpolation", sampler->interpolation)) return false;
			if (!se_gltf_json_add_extras_extensions(sampler_obj, sampler->extras, sampler->extensions)) return false;
			if (!s_json_add(samplers, sampler_obj)) { s_json_free(sampler_obj); return false; }
		}
		s_json *channels = se_gltf_json_add_array(obj, "channels");
		if (channels == NULL) return false;
		for (sz c = 0; c < anim->channels.size; c++) {
			se_gltf_animation_channel *channel = s_array_get(&anim->channels, c);
			s_json *channel_obj = s_json_object_empty(NULL);
			if (channel_obj == NULL) return false;
			if (!se_gltf_json_add_number(channel_obj, "sampler", channel->sampler)) return false;
			s_json *target = se_gltf_json_add_object(channel_obj, "target");
			if (target == NULL) return false;
			if (channel->target.has_node) if (!se_gltf_json_add_number(target, "node", channel->target.node)) return false;
			if (!se_gltf_json_add_string(target, "path", channel->target.path)) return false;
			if (!se_gltf_json_add_extras_extensions(target, channel->target.extras, channel->target.extensions)) return false;
			if (!se_gltf_json_add_extras_extensions(channel_obj, channel->extras, channel->extensions)) return false;
			if (!s_json_add(channels, channel_obj)) { s_json_free(channel_obj); return false; }
		}
		if (!se_gltf_json_add_extras_extensions(obj, anim->extras, anim->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_cameras_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->cameras.size == 0) return true;
	s_json *arr = se_gltf_json_add_array(root, "cameras");
	if (arr == NULL) return false;
	for (sz i = 0; i < asset->cameras.size; i++) {
		se_gltf_camera *cam = s_array_get(&asset->cameras, i);
		s_json *obj = s_json_object_empty(NULL);
		if (obj == NULL) return false;
		if (cam->name) if (!se_gltf_json_add_string(obj, "name", cam->name)) return false;
		if (cam->has_type) if (!se_gltf_json_add_string(obj, "type", cam->type)) return false;
		if (cam->has_perspective) {
			s_json *p = se_gltf_json_add_object(obj, "perspective");
			if (p == NULL) return false;
			if (!se_gltf_json_add_number(p, "yfov", cam->perspective.yfov)) return false;
			if (!se_gltf_json_add_number(p, "znear", cam->perspective.znear)) return false;
			if (cam->perspective.has_zfar) if (!se_gltf_json_add_number(p, "zfar", cam->perspective.zfar)) return false;
			if (cam->perspective.has_aspect_ratio) if (!se_gltf_json_add_number(p, "aspectRatio", cam->perspective.aspect_ratio)) return false;
		}
		if (cam->has_orthographic) {
			s_json *o = se_gltf_json_add_object(obj, "orthographic");
			if (o == NULL) return false;
			if (!se_gltf_json_add_number(o, "xmag", cam->orthographic.xmag)) return false;
			if (!se_gltf_json_add_number(o, "ymag", cam->orthographic.ymag)) return false;
			if (!se_gltf_json_add_number(o, "znear", cam->orthographic.znear)) return false;
			if (!se_gltf_json_add_number(o, "zfar", cam->orthographic.zfar)) return false;
		}
		if (!se_gltf_json_add_extras_extensions(obj, cam->extras, cam->extensions)) return false;
		if (!s_json_add(arr, obj)) { s_json_free(obj); return false; }
	}
	return true;
}

static b8 se_gltf_write_top_level_json(const se_gltf_asset *asset, s_json *root) {
	if (asset->extensions_used.size > 0) if (!se_gltf_json_add_string_array(root, "extensionsUsed", &asset->extensions_used)) return false;
	if (asset->extensions_required.size > 0) if (!se_gltf_json_add_string_array(root, "extensionsRequired", &asset->extensions_required)) return false;
	if (!se_gltf_json_add_extras_extensions(root, asset->extras, asset->extensions)) return false;
	return true;
}

static b8 se_gltf_write_glb(const se_gltf_asset *asset, const char *path, const s_json *json_root, const u32 *buffer_offsets, const u8 *bin_data, sz bin_size) {
	char *json_text = s_json_stringify(json_root);
	if (json_text == NULL) return false;
	sz json_size = strlen(json_text);
	sz json_padded = (json_size + 3) & ~3u;
	sz bin_padded = (bin_size + 3) & ~3u;
	sz total_length = 12 + 8 + json_padded + (bin_size > 0 ? (8 + bin_padded) : 0);
	u8 *out = (u8 *)malloc(total_length);
	if (out == NULL) { free(json_text); return false; }
	*(u32 *)&out[0] = SE_GLTF_GLTF_MAGIC;
	*(u32 *)&out[4] = 2;
	*(u32 *)&out[8] = (u32)total_length;
	*(u32 *)&out[12] = (u32)json_padded;
	*(u32 *)&out[16] = SE_GLTF_CHUNK_JSON;
	memcpy(out + 20, json_text, json_size);
	for (sz i = json_size; i < json_padded; i++) out[20 + i] = ' ';
	sz offset = 20 + json_padded;
	if (bin_size > 0) {
		*(u32 *)&out[offset] = (u32)bin_padded;
		*(u32 *)&out[offset + 4] = SE_GLTF_CHUNK_BIN;
		memcpy(out + offset + 8, bin_data, bin_size);
		for (sz i = bin_size; i < bin_padded; i++) out[offset + 8 + i] = 0;
	}
	free(json_text);
	const b8 ok = s_file_write_binary(path, out, total_length);
	free(out);
	return ok;
}

static b8 se_gltf_collect_buffer_data(const se_gltf_asset *asset, u8 **out_data, sz *out_size, u32 **out_offsets) {
	sz buffer_count = asset->buffers.size;
	if (buffer_count == 0) { *out_data = NULL; *out_size = 0; return true; }
	u32 *offsets = (u32 *)malloc(sizeof(u32) * buffer_count);
	if (offsets == NULL) return false;
	sz total = 0;
	for (sz i = 0; i < buffer_count; i++) {
		se_gltf_buffer *buf = s_array_get(&asset->buffers, i);
		offsets[i] = (u32)total;
		sz size = buf->data_size;
		total += (size + 3) & ~3u;
	}
	u8 *data = (u8 *)malloc(total ? total : 1);
	if (data == NULL) { free(offsets); return false; }
	memset(data, 0, total);
	for (sz i = 0; i < buffer_count; i++) {
		se_gltf_buffer *buf = s_array_get(&asset->buffers, i);
		if (buf->data == NULL) {
			free(offsets);
			free(data);
			return false;
		}
		memcpy(data + offsets[i], buf->data, buf->data_size);
	}
	*out_data = data;
	*out_size = total;
	*out_offsets = offsets;
	return true;
}

static b8 se_gltf_write_external_buffers(const se_gltf_asset *asset, const char *dir, char **uris) {
	for (sz i = 0; i < asset->buffers.size; i++) {
		se_gltf_buffer *buf = s_array_get(&asset->buffers, i);
		if (uris[i] == NULL) continue;
		if (buf->data == NULL) return false;
		char full_path[SE_MAX_PATH_LENGTH] = {0};
		if (dir && dir[0]) {
			if (!s_path_join(full_path, SE_MAX_PATH_LENGTH, dir, uris[i])) return false;
		} else {
			if (!s_path_join(full_path, SE_MAX_PATH_LENGTH, "", uris[i])) return false;
		}
		if (!s_file_write_binary(full_path, buf->data, buf->data_size)) return false;
	}
	return true;
}

static b8 se_gltf_write_external_images(const se_gltf_asset *asset, const char *dir, char **uris) {
	for (sz i = 0; i < asset->images.size; i++) {
		se_gltf_image *img = s_array_get(&asset->images, i);
		if (uris[i] == NULL) continue;
		if (img->data == NULL) return false;
		char full_path[SE_MAX_PATH_LENGTH] = {0};
		if (dir && dir[0]) {
			if (!s_path_join(full_path, SE_MAX_PATH_LENGTH, dir, uris[i])) return false;
		} else {
			if (!s_path_join(full_path, SE_MAX_PATH_LENGTH, "", uris[i])) return false;
		}
		if (!s_file_write_binary(full_path, img->data, img->data_size)) return false;
	}
	return true;
}

b8 se_gltf_write(const se_gltf_asset *asset, const char *path, const se_gltf_write_params *params) {
	if (asset == NULL || path == NULL || path[0] == '\0') return false;
	se_gltf_write_params cfg;
	se_gltf_set_default_write_params(&cfg, params);
	char base_name[SE_MAX_PATH_LENGTH] = {0};
	const char *filename = s_path_filename(path);
	if (filename == NULL) return false;
	strncpy(base_name, filename, sizeof(base_name) - 1);
	char *dot = strrchr(base_name, '.');
	if (dot) *dot = '\0';
	char output_dir[SE_MAX_PATH_LENGTH] = {0};
	if (s_path_parent(path, output_dir, SE_MAX_PATH_LENGTH) == false) output_dir[0] = '\0';
	s_json *root = s_json_object_empty(NULL);
	if (root == NULL) return false;
	if (!se_gltf_write_assets_json(asset, root)) { s_json_free(root); return false; }
	if (!se_gltf_write_top_level_json(asset, root)) { s_json_free(root); return false; }
	char **buffer_uris = NULL;
	sz buffer_uri_count = 0;
	char **image_uris = NULL;
	if (!cfg.write_glb) {
		if (!se_gltf_write_buffers_json(asset, root, &cfg, base_name, &buffer_uris, &buffer_uri_count)) { s_json_free(root); return false; }
	}
	if (asset->images.size > 0) {
		image_uris = (char **)calloc(asset->images.size, sizeof(char *));
	}
	if (!se_gltf_write_buffer_views_json(asset, root, NULL, false) ||
		!se_gltf_write_accessors_json(asset, root) ||
		!se_gltf_write_images_json(asset, root, &cfg, base_name, image_uris) ||
		!se_gltf_write_samplers_json(asset, root) ||
		!se_gltf_write_textures_json(asset, root) ||
		!se_gltf_write_materials_json(asset, root) ||
		!se_gltf_write_meshes_json(asset, root) ||
		!se_gltf_write_nodes_json(asset, root) ||
		!se_gltf_write_scenes_json(asset, root) ||
		!se_gltf_write_skins_json(asset, root) ||
		!se_gltf_write_animations_json(asset, root) ||
		!se_gltf_write_cameras_json(asset, root)) {
		s_json_free(root);
		free(buffer_uris);
		free(image_uris);
		return false;
	}
	b8 ok = false;
	if (cfg.write_glb) {
		u8 *bin_data = NULL;
		sz bin_size = 0;
		u32 *offsets = NULL;
		if (!se_gltf_collect_buffer_data(asset, &bin_data, &bin_size, &offsets)) {
			s_json_free(root);
			free(buffer_uris);
			free(image_uris);
			return false;
		}
		s_json *glb_root = s_json_object_empty(NULL);
		if (glb_root == NULL) {
			free(bin_data);
			free(offsets);
			s_json_free(root);
			free(buffer_uris);
			free(image_uris);
			return false;
		}
		if (!se_gltf_write_assets_json(asset, glb_root) ||
			!se_gltf_write_top_level_json(asset, glb_root) ||
			!se_gltf_write_glb_buffers_json(asset, glb_root, bin_size) ||
			!se_gltf_write_buffer_views_json(asset, glb_root, offsets, true) ||
			!se_gltf_write_accessors_json(asset, glb_root) ||
			!se_gltf_write_images_json(asset, glb_root, &cfg, base_name, image_uris) ||
			!se_gltf_write_samplers_json(asset, glb_root) ||
			!se_gltf_write_textures_json(asset, glb_root) ||
			!se_gltf_write_materials_json(asset, glb_root) ||
			!se_gltf_write_meshes_json(asset, glb_root) ||
			!se_gltf_write_nodes_json(asset, glb_root) ||
			!se_gltf_write_scenes_json(asset, glb_root) ||
			!se_gltf_write_skins_json(asset, glb_root) ||
			!se_gltf_write_animations_json(asset, glb_root) ||
			!se_gltf_write_cameras_json(asset, glb_root)) {
			s_json_free(glb_root);
			free(bin_data);
			free(offsets);
			s_json_free(root);
			free(buffer_uris);
			free(image_uris);
			return false;
		}
		ok = se_gltf_write_glb(asset, path, glb_root, offsets, bin_data, bin_size);
		s_json_free(glb_root);
		free(bin_data);
		free(offsets);
	} else {
		char *json_text = s_json_stringify(root);
		if (json_text == NULL) {
			s_json_free(root);
			free(buffer_uris);
			free(image_uris);
			return false;
		}
		ok = s_file_write(path, json_text, strlen(json_text));
		free(json_text);
		if (ok) {
			if (!cfg.embed_buffers && buffer_uris) {
				ok = se_gltf_write_external_buffers(asset, output_dir, buffer_uris);
			}
			if (ok && !cfg.embed_images && image_uris) {
				ok = se_gltf_write_external_images(asset, output_dir, image_uris);
			}
		}
	}
	s_json_free(root);
	if (buffer_uris) {
		for (sz i = 0; i < buffer_uri_count; i++) free(buffer_uris[i]);
		free(buffer_uris);
	}
	if (image_uris) {
		for (sz i = 0; i < asset->images.size; i++) free(image_uris[i]);
		free(image_uris);
	}
	return ok;
}

se_model *se_gltf_to_model(se_render_handle *render_handle, const se_gltf_asset *asset, const i32 mesh_index) {
	if (render_handle == NULL || asset == NULL) { se_set_last_error(SE_RESULT_INVALID_ARGUMENT); return NULL; }
	if (mesh_index < 0 || (sz)mesh_index >= asset->meshes.size) { se_set_last_error(SE_RESULT_INVALID_ARGUMENT); return NULL; }
	if (s_array_get_capacity(&render_handle->models) == s_array_get_size(&render_handle->models)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}
	se_gltf_mesh *mesh = s_array_get(&asset->meshes, mesh_index);
	se_model *model = s_array_increment(&render_handle->models);
	s_array_init(&model->meshes, mesh->primitives.size);
	for (sz i = 0; i < mesh->primitives.size; i++) {
		se_gltf_primitive *prim = s_array_get(&mesh->primitives, i);
		if (prim->has_mode && prim->mode != 4) {
			se_set_last_error(SE_RESULT_UNSUPPORTED);
			return NULL;
		}
		se_mesh *out_mesh = s_array_increment(&model->meshes);
		memset(out_mesh, 0, sizeof(*out_mesh));
		se_gltf_attribute *pos_attr = NULL;
		se_gltf_attribute *norm_attr = NULL;
		se_gltf_attribute *uv_attr = NULL;
		for (sz a = 0; a < prim->attributes.attributes.size; a++) {
			se_gltf_attribute *attr = s_array_get(&prim->attributes.attributes, a);
			if (strcmp(attr->name, "POSITION") == 0) pos_attr = attr;
			else if (strcmp(attr->name, "NORMAL") == 0) norm_attr = attr;
			else if (strcmp(attr->name, "TEXCOORD_0") == 0) uv_attr = attr;
		}
		if (pos_attr == NULL) { se_set_last_error(SE_RESULT_IO); return NULL; }
		if (pos_attr->accessor < 0 || (sz)pos_attr->accessor >= asset->accessors.size) { se_set_last_error(SE_RESULT_IO); return NULL; }
		se_gltf_accessor *pos_acc = s_array_get(&asset->accessors, pos_attr->accessor);
		if (pos_acc->component_type != 5126 || pos_acc->type != SE_GLTF_ACCESSOR_VEC3) { se_set_last_error(SE_RESULT_UNSUPPORTED); return NULL; }
		if (!pos_acc->has_buffer_view) { se_set_last_error(SE_RESULT_IO); return NULL; }
		se_gltf_buffer_view *pos_view = s_array_get(&asset->buffer_views, pos_acc->buffer_view);
		se_gltf_buffer *pos_buf = s_array_get(&asset->buffers, pos_view->buffer);
		if (pos_buf->data == NULL) { se_set_last_error(SE_RESULT_IO); return NULL; }
		u32 stride = pos_view->has_byte_stride ? pos_view->byte_stride : sizeof(f32) * 3;
		u32 offset = pos_view->byte_offset + pos_acc->byte_offset;
		u32 vertex_count = pos_acc->count;
		se_vertex_3d *tmp_vertices = (se_vertex_3d *)malloc(sizeof(se_vertex_3d) * vertex_count);
		if (tmp_vertices == NULL) { se_set_last_error(SE_RESULT_OUT_OF_MEMORY); return NULL; }
		for (u32 v = 0; v < vertex_count; v++) {
			const u8 *ptr = pos_buf->data + offset + (v * stride);
			const f32 *fptr = (const f32 *)ptr;
			tmp_vertices[v].position = s_vec3(fptr[0], fptr[1], fptr[2]);
			tmp_vertices[v].normal = s_vec3(0.0f, 0.0f, 1.0f);
			tmp_vertices[v].uv = s_vec2(0.0f, 0.0f);
		}
		if (norm_attr) {
			se_gltf_accessor *nacc = s_array_get(&asset->accessors, norm_attr->accessor);
			se_gltf_buffer_view *nview = s_array_get(&asset->buffer_views, nacc->buffer_view);
			se_gltf_buffer *nbuf = s_array_get(&asset->buffers, nview->buffer);
			u32 nstride = nview->has_byte_stride ? nview->byte_stride : sizeof(f32) * 3;
			u32 noffset = nview->byte_offset + nacc->byte_offset;
			for (u32 v = 0; v < vertex_count; v++) {
				const u8 *ptr = nbuf->data + noffset + (v * nstride);
				const f32 *fptr = (const f32 *)ptr;
				tmp_vertices[v].normal = s_vec3(fptr[0], fptr[1], fptr[2]);
			}
		}
		if (uv_attr) {
			se_gltf_accessor *uacc = s_array_get(&asset->accessors, uv_attr->accessor);
			se_gltf_buffer_view *uview = s_array_get(&asset->buffer_views, uacc->buffer_view);
			se_gltf_buffer *ubuf = s_array_get(&asset->buffers, uview->buffer);
			u32 ustride = uview->has_byte_stride ? uview->byte_stride : sizeof(f32) * 2;
			u32 uoffset = uview->byte_offset + uacc->byte_offset;
			for (u32 v = 0; v < vertex_count; v++) {
				const u8 *ptr = ubuf->data + uoffset + (v * ustride);
				const f32 *fptr = (const f32 *)ptr;
				tmp_vertices[v].uv = s_vec2(fptr[0], fptr[1]);
			}
		}
		if (prim->has_indices) {
			se_gltf_accessor *iacc = s_array_get(&asset->accessors, prim->indices);
			se_gltf_buffer_view *iview = s_array_get(&asset->buffer_views, iacc->buffer_view);
			se_gltf_buffer *ibuf = s_array_get(&asset->buffers, iview->buffer);
			u32 stride_idx = iview->has_byte_stride ? iview->byte_stride : 0;
			u32 ioffset = iview->byte_offset + iacc->byte_offset;
			u32 index_count = iacc->count;
			u32 *tmp_indices = (u32 *)malloc(sizeof(u32) * index_count);
			if (tmp_indices == NULL) { free(tmp_vertices); se_set_last_error(SE_RESULT_OUT_OF_MEMORY); return NULL; }
			u32 component_size = (iacc->component_type == 5125) ? 4 : (iacc->component_type == 5123) ? 2 : 1;
			for (u32 idx = 0; idx < index_count; idx++) {
				const u8 *ptr = ibuf->data + ioffset + (stride_idx ? idx * stride_idx : idx * component_size);
				if (iacc->component_type == 5125) tmp_indices[idx] = *(const u32 *)ptr;
				else if (iacc->component_type == 5123) tmp_indices[idx] = *(const u16 *)ptr;
				else if (iacc->component_type == 5121) tmp_indices[idx] = *(const u8 *)ptr;
				else tmp_indices[idx] = 0;
			}
			se_gltf_mesh_finalize(out_mesh, tmp_vertices, tmp_indices, vertex_count, index_count);
			free(tmp_vertices);
			free(tmp_indices);
		} else {
			u32 index_count = vertex_count;
			u32 *tmp_indices = (u32 *)malloc(sizeof(u32) * index_count);
			if (tmp_indices == NULL) { free(tmp_vertices); se_set_last_error(SE_RESULT_OUT_OF_MEMORY); return NULL; }
			for (u32 idx = 0; idx < index_count; idx++) tmp_indices[idx] = idx;
			se_gltf_mesh_finalize(out_mesh, tmp_vertices, tmp_indices, vertex_count, index_count);
			free(tmp_vertices);
			free(tmp_indices);
		}
	}
	model->is_valid = true;
	se_set_last_error(SE_RESULT_OK);
	return model;
}

se_texture *se_gltf_image_load(se_render_handle *render_handle, const se_gltf_asset *asset, const i32 image_index, const se_texture_wrap wrap) {
	if (render_handle == NULL || asset == NULL) { se_set_last_error(SE_RESULT_INVALID_ARGUMENT); return NULL; }
	if (image_index < 0 || (sz)image_index >= asset->images.size) { se_set_last_error(SE_RESULT_INVALID_ARGUMENT); return NULL; }
	se_gltf_image *image = s_array_get(&asset->images, image_index);
	if (image->data && image->data_size > 0) {
		return se_texture_load_from_memory(render_handle, image->data, image->data_size, wrap);
	}
	if (image->uri && image->uri[0] != '\0') {
		return se_texture_load(render_handle, image->uri, wrap);
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return NULL;
}

se_texture *se_gltf_texture_load(se_render_handle *render_handle, const se_gltf_asset *asset, const i32 texture_index, const se_texture_wrap wrap) {
	if (render_handle == NULL || asset == NULL) { se_set_last_error(SE_RESULT_INVALID_ARGUMENT); return NULL; }
	if (texture_index < 0 || (sz)texture_index >= asset->textures.size) { se_set_last_error(SE_RESULT_INVALID_ARGUMENT); return NULL; }
	se_gltf_texture *texture = s_array_get(&asset->textures, texture_index);
	if (!texture->has_source) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	return se_gltf_image_load(render_handle, asset, texture->source, wrap);
}
