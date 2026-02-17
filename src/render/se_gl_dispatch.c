// Syphax-Engine - Ougi Washi

#define SE_GL_NO_DISPATCH_WRAPPERS
#include "se_gl.h"

#include "render/se_render_queue.h"
#include <stddef.h>

static b8 se_gl_dispatch_direct(void) {
	return !se_render_queue_is_running() || se_render_queue_is_render_thread();
}

static b8 se_gl_dispatch_record_async(const se_render_queue_sync_fn fn, const void* payload, const u32 payload_bytes) {
	return se_render_queue_record_async(fn, payload, payload_bytes);
}

static b8 se_gl_dispatch_record_async_blob(const se_render_queue_sync_fn fn,
	const void* payload,
	const u32 payload_bytes,
	const u32 pointer_patch_offset,
	const void* blob,
	const u32 blob_bytes) {
	return se_render_queue_record_async_blob(fn, payload, payload_bytes, pointer_patch_offset, blob, blob_bytes);
}

static b8 se_gl_dispatch_compute_blob_bytes_i32(const i32 count, const u32 unit_size, u32* out_bytes) {
	if (!out_bytes || count <= 0) {
		return false;
	}
	const u64 total = (u64)(u32)count * (u64)unit_size;
	if (total == 0u || total > 0xFFFFFFFFu) {
		return false;
	}
	*out_bytes = (u32)total;
	return true;
}

static b8 se_gl_dispatch_compute_blob_bytes_size(const GLsizeiptr size, u32* out_bytes) {
	if (!out_bytes || size <= 0) {
		return false;
	}
	const u64 total = (u64)size;
	if (total == 0u || total > 0xFFFFFFFFu) {
		return false;
	}
	*out_bytes = (u32)total;
	return true;
}

typedef struct {
	GLsizei n;
	const GLuint* ids;
} se_gl_ids_in_payload;

typedef struct {
	GLsizei n;
	GLuint* ids;
} se_gl_ids_out_payload;

typedef struct {
	GLenum target;
	GLuint id;
} se_gl_bind_payload;

typedef struct {
	GLenum target;
	GLintptr offset;
	GLsizeiptr size;
	const void* data;
} se_gl_buffer_sub_data_payload;

typedef struct {
	GLenum target;
	GLsizeiptr size;
	const void* data;
	GLenum usage;
} se_gl_buffer_data_payload;

typedef struct {
	GLuint value;
} se_gl_u32_payload;

typedef struct {
	GLenum type;
} se_gl_create_shader_payload;

typedef struct {
	GLuint shader;
	GLsizei count;
	const GLchar** string;
	const GLint* length;
} se_gl_shader_source_payload;

typedef struct {
	GLuint shader;
	GLenum pname;
	GLint* out_params;
} se_gl_shader_iv_payload;

typedef struct {
	GLuint shader;
	GLsizei buf_size;
	GLsizei* out_length;
	GLchar* out_log;
} se_gl_shader_log_payload;

typedef struct {
	GLuint program;
	GLenum pname;
	GLint* out_params;
} se_gl_program_iv_payload;

typedef struct {
	GLuint program;
	GLsizei buf_size;
	GLsizei* out_length;
	GLchar* out_log;
} se_gl_program_log_payload;

typedef struct {
	GLuint program;
	const GLchar* name;
} se_gl_uniform_loc_payload;

typedef struct {
	GLint location;
	GLint value;
} se_gl_uniform1i_payload;

typedef struct {
	GLint location;
	GLfloat value;
} se_gl_uniform1f_payload;

typedef struct {
	GLint location;
	GLsizei count;
	const GLfloat* value;
} se_gl_uniform_fv_payload;

typedef struct {
	GLint location;
	GLsizei count;
	GLboolean transpose;
	const GLfloat* value;
} se_gl_uniform_matrix_payload;

typedef struct {
	GLenum target;
	GLenum attachment;
	GLenum renderbuffer_target;
	GLuint renderbuffer;
} se_gl_framebuffer_renderbuffer_payload;

typedef struct {
	GLenum target;
	GLenum attachment;
	GLenum texture_target;
	GLuint texture;
	GLint level;
} se_gl_framebuffer_texture2d_payload;

typedef struct {
	GLuint index;
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLsizei stride;
	const void* pointer;
} se_gl_vertex_attrib_pointer_payload;

typedef struct {
	GLuint index;
} se_gl_vertex_attrib_index_payload;

typedef struct {
	GLuint index;
	GLuint divisor;
} se_gl_vertex_attrib_divisor_payload;

typedef struct {
	GLenum mode;
	GLint first;
	GLsizei count;
	GLsizei instance_count;
} se_gl_draw_arrays_instanced_payload;

typedef struct {
	GLenum mode;
	GLsizei count;
	GLenum type;
	const void* indices;
	GLsizei instance_count;
} se_gl_draw_elements_instanced_payload;

typedef struct {
	GLenum mode;
	GLsizei count;
	GLenum type;
	const void* indices;
} se_gl_draw_elements_payload;

typedef struct {
	GLenum target;
	GLenum access;
} se_gl_map_buffer_payload;

typedef struct {
	GLenum target;
} se_gl_unmap_buffer_payload;

typedef struct {
	GLenum target;
	GLenum internal_format;
	GLsizei width;
	GLsizei height;
} se_gl_renderbuffer_storage_payload;

typedef struct {
	GLenum target;
} se_gl_framebuffer_status_payload;

typedef struct {
	GLint src_x0;
	GLint src_y0;
	GLint src_x1;
	GLint src_y1;
	GLint dst_x0;
	GLint dst_y0;
	GLint dst_x1;
	GLint dst_y1;
	GLbitfield mask;
	GLenum filter;
} se_gl_blit_framebuffer_payload;

typedef struct {
	GLenum cap;
} se_gl_enable_payload;

typedef struct {
	GLenum sfactor;
	GLenum dfactor;
} se_gl_blend_func_payload;

typedef struct {
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;
} se_gl_clear_color_payload;

typedef struct {
	GLboolean r;
	GLboolean g;
	GLboolean b;
	GLboolean a;
} se_gl_color_mask_payload;

typedef struct {
	GLbitfield mask;
} se_gl_clear_payload;

typedef struct {
	GLenum target;
	GLint level;
	GLint internal_format;
	GLsizei width;
	GLsizei height;
	GLint border;
	GLenum format;
	GLenum type;
	const void* pixels;
} se_gl_tex_image_2d_payload;

typedef struct {
	GLenum target;
	GLenum pname;
	GLint param;
} se_gl_tex_param_i_payload;

typedef struct {
	GLint x;
	GLint y;
	GLsizei width;
	GLsizei height;
} se_gl_viewport_payload;

typedef struct {
	GLenum pname;
	GLint* out_value;
} se_gl_get_int_payload;

typedef struct {
	GLenum pname;
	GLboolean* out_value;
} se_gl_get_bool_payload;

typedef struct {
	GLenum pname;
	GLfloat* out_value;
} se_gl_get_float_payload;

typedef struct {
	GLenum pname;
	GLint param;
} se_gl_pixel_store_i_payload;

typedef struct {
	GLint x;
	GLint y;
	GLsizei width;
	GLsizei height;
	GLenum format;
	GLenum type;
	void* pixels;
} se_gl_read_pixels_payload;

typedef struct {
	GLenum name;
} se_gl_get_string_payload;

static void se_gl_exec_delete_buffers(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_ids_in_payload* args = (const se_gl_ids_in_payload*)payload;
	glDeleteBuffers(args->n, args->ids);
}

void se_gl_dispatchDeleteBuffers(GLsizei n, const GLuint *buffers) {
	if (se_gl_dispatch_direct()) {
		glDeleteBuffers(n, buffers);
		return;
	}
	const se_gl_ids_in_payload payload = {n, buffers};
	u32 ids_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(n, (u32)sizeof(GLuint), &ids_bytes) && buffers != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_delete_buffers,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_ids_in_payload, ids),
			buffers,
			ids_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_delete_buffers, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_gen_buffers(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_ids_out_payload* args = (const se_gl_ids_out_payload*)payload;
	glGenBuffers(args->n, args->ids);
}

void se_gl_dispatchGenBuffers(GLsizei n, GLuint *buffers) {
	if (se_gl_dispatch_direct()) {
		glGenBuffers(n, buffers);
		return;
	}
	const se_gl_ids_out_payload payload = {n, buffers};
	(void)se_render_queue_call_sync_sized(se_gl_exec_gen_buffers, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_bind_buffer(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_bind_payload* args = (const se_gl_bind_payload*)payload;
	glBindBuffer(args->target, args->id);
}

void se_gl_dispatchBindBuffer(GLenum target, GLuint buffer) {
	if (se_gl_dispatch_direct()) {
		glBindBuffer(target, buffer);
		return;
	}
	const se_gl_bind_payload payload = {target, buffer};
	if (se_gl_dispatch_record_async(se_gl_exec_bind_buffer, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_bind_buffer, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_buffer_sub_data(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_buffer_sub_data_payload* args = (const se_gl_buffer_sub_data_payload*)payload;
	glBufferSubData(args->target, args->offset, args->size, args->data);
}

void se_gl_dispatchBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
	if (se_gl_dispatch_direct()) {
		glBufferSubData(target, offset, size, data);
		return;
	}
	const se_gl_buffer_sub_data_payload payload = {target, offset, size, data};
	u32 blob_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_size(size, &blob_bytes) && data != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_buffer_sub_data,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_buffer_sub_data_payload, data),
			data,
			blob_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_buffer_sub_data, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_buffer_data(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_buffer_data_payload* args = (const se_gl_buffer_data_payload*)payload;
	glBufferData(args->target, args->size, args->data, args->usage);
}

void se_gl_dispatchBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
	if (se_gl_dispatch_direct()) {
		glBufferData(target, size, data, usage);
		return;
	}
	const se_gl_buffer_data_payload payload = {target, size, data, usage};
	u32 blob_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_size(size, &blob_bytes) && data != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_buffer_data,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_buffer_data_payload, data),
			data,
			blob_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_buffer_data, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_use_program(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_u32_payload* args = (const se_gl_u32_payload*)payload;
	glUseProgram(args->value);
}

void se_gl_dispatchUseProgram(GLuint program) {
	if (se_gl_dispatch_direct()) {
		glUseProgram(program);
		return;
	}
	const se_gl_u32_payload payload = {program};
	if (se_gl_dispatch_record_async(se_gl_exec_use_program, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_use_program, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_create_shader(const void* payload, void* out_result) {
	const se_gl_create_shader_payload* args = (const se_gl_create_shader_payload*)payload;
	if (out_result) {
		*(GLuint*)out_result = glCreateShader(args->type);
	}
}

GLuint se_gl_dispatchCreateShader(GLenum type) {
	if (se_gl_dispatch_direct()) {
		return glCreateShader(type);
	}
	const se_gl_create_shader_payload payload = {type};
	GLuint out_shader = 0;
	if (!se_render_queue_call_sync_sized(se_gl_exec_create_shader, &payload, &out_shader, (u32)sizeof(payload))) {
		return 0;
	}
	return out_shader;
}

static void se_gl_exec_shader_source(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_shader_source_payload* args = (const se_gl_shader_source_payload*)payload;
	glShaderSource(args->shader, args->count, args->string, args->length);
}

void se_gl_dispatchShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length) {
	if (se_gl_dispatch_direct()) {
		glShaderSource(shader, count, string, length);
		return;
	}
	const se_gl_shader_source_payload payload = {shader, count, string, length};
	(void)se_render_queue_call_sync_sized(se_gl_exec_shader_source, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_compile_shader(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_u32_payload* args = (const se_gl_u32_payload*)payload;
	glCompileShader(args->value);
}

void se_gl_dispatchCompileShader(GLuint shader) {
	if (se_gl_dispatch_direct()) {
		glCompileShader(shader);
		return;
	}
	const se_gl_u32_payload payload = {shader};
	if (se_gl_dispatch_record_async(se_gl_exec_compile_shader, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_compile_shader, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_create_program(const void* payload, void* out_result) {
	(void)payload;
	if (out_result) {
		*(GLuint*)out_result = glCreateProgram();
	}
}

GLuint se_gl_dispatchCreateProgram(void) {
	if (se_gl_dispatch_direct()) {
		return glCreateProgram();
	}
	GLuint out_program = 0;
	if (!se_render_queue_call_sync_sized(se_gl_exec_create_program, NULL, &out_program, 0u)) {
		return 0;
	}
	return out_program;
}

static void se_gl_exec_link_program(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_u32_payload* args = (const se_gl_u32_payload*)payload;
	glLinkProgram(args->value);
}

void se_gl_dispatchLinkProgram(GLuint program) {
	if (se_gl_dispatch_direct()) {
		glLinkProgram(program);
		return;
	}
	const se_gl_u32_payload payload = {program};
	if (se_gl_dispatch_record_async(se_gl_exec_link_program, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_link_program, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_attach_shader(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_bind_payload* args = (const se_gl_bind_payload*)payload;
	glAttachShader(args->target, args->id);
}

void se_gl_dispatchAttachShader(GLuint program, GLuint shader) {
	if (se_gl_dispatch_direct()) {
		glAttachShader(program, shader);
		return;
	}
	const se_gl_bind_payload payload = {program, shader};
	if (se_gl_dispatch_record_async(se_gl_exec_attach_shader, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_attach_shader, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_delete_program(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_u32_payload* args = (const se_gl_u32_payload*)payload;
	glDeleteProgram(args->value);
}

void se_gl_dispatchDeleteProgram(GLuint program) {
	if (se_gl_dispatch_direct()) {
		glDeleteProgram(program);
		return;
	}
	const se_gl_u32_payload payload = {program};
	if (se_gl_dispatch_record_async(se_gl_exec_delete_program, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_delete_program, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_delete_shader(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_u32_payload* args = (const se_gl_u32_payload*)payload;
	glDeleteShader(args->value);
}

void se_gl_dispatchDeleteShader(GLuint shader) {
	if (se_gl_dispatch_direct()) {
		glDeleteShader(shader);
		return;
	}
	const se_gl_u32_payload payload = {shader};
	if (se_gl_dispatch_record_async(se_gl_exec_delete_shader, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_delete_shader, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_gen_renderbuffers(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_ids_out_payload* args = (const se_gl_ids_out_payload*)payload;
	glGenRenderbuffers(args->n, args->ids);
}

void se_gl_dispatchGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
	if (se_gl_dispatch_direct()) {
		glGenRenderbuffers(n, renderbuffers);
		return;
	}
	const se_gl_ids_out_payload payload = {n, renderbuffers};
	(void)se_render_queue_call_sync_sized(se_gl_exec_gen_renderbuffers, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_bind_framebuffer(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_bind_payload* args = (const se_gl_bind_payload*)payload;
	glBindFramebuffer(args->target, args->id);
}

void se_gl_dispatchBindFramebuffer(GLenum target, GLuint framebuffer) {
	if (se_gl_dispatch_direct()) {
		glBindFramebuffer(target, framebuffer);
		return;
	}
	const se_gl_bind_payload payload = {target, framebuffer};
	if (se_gl_dispatch_record_async(se_gl_exec_bind_framebuffer, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_bind_framebuffer, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_framebuffer_renderbuffer(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_framebuffer_renderbuffer_payload* args = (const se_gl_framebuffer_renderbuffer_payload*)payload;
	glFramebufferRenderbuffer(args->target, args->attachment, args->renderbuffer_target, args->renderbuffer);
}

void se_gl_dispatchFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
	if (se_gl_dispatch_direct()) {
		glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
		return;
	}
	const se_gl_framebuffer_renderbuffer_payload payload = {target, attachment, renderbuffertarget, renderbuffer};
	if (se_gl_dispatch_record_async(se_gl_exec_framebuffer_renderbuffer, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_framebuffer_renderbuffer, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_bind_vertex_array(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_u32_payload* args = (const se_gl_u32_payload*)payload;
	glBindVertexArray(args->value);
}

void se_gl_dispatchBindVertexArray(GLuint array) {
	if (se_gl_dispatch_direct()) {
		glBindVertexArray(array);
		return;
	}
	const se_gl_u32_payload payload = {array};
	if (se_gl_dispatch_record_async(se_gl_exec_bind_vertex_array, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_bind_vertex_array, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_gen_vertex_arrays(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_ids_out_payload* args = (const se_gl_ids_out_payload*)payload;
	glGenVertexArrays(args->n, args->ids);
}

void se_gl_dispatchGenVertexArrays(GLsizei n, GLuint *arrays) {
	if (se_gl_dispatch_direct()) {
		glGenVertexArrays(n, arrays);
		return;
	}
	const se_gl_ids_out_payload payload = {n, arrays};
	(void)se_render_queue_call_sync_sized(se_gl_exec_gen_vertex_arrays, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_delete_vertex_arrays(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_ids_in_payload* args = (const se_gl_ids_in_payload*)payload;
	glDeleteVertexArrays(args->n, args->ids);
}

void se_gl_dispatchDeleteVertexArrays(GLsizei n, const GLuint *arrays) {
	if (se_gl_dispatch_direct()) {
		glDeleteVertexArrays(n, arrays);
		return;
	}
	const se_gl_ids_in_payload payload = {n, arrays};
	u32 ids_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(n, (u32)sizeof(GLuint), &ids_bytes) && arrays != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_delete_vertex_arrays,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_ids_in_payload, ids),
			arrays,
			ids_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_delete_vertex_arrays, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_vertex_attrib_pointer(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_vertex_attrib_pointer_payload* args = (const se_gl_vertex_attrib_pointer_payload*)payload;
	glVertexAttribPointer(args->index, args->size, args->type, args->normalized, args->stride, args->pointer);
}

void se_gl_dispatchVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
	if (se_gl_dispatch_direct()) {
		glVertexAttribPointer(index, size, type, normalized, stride, pointer);
		return;
	}
	const se_gl_vertex_attrib_pointer_payload payload = {index, size, type, normalized, stride, pointer};
	if (se_gl_dispatch_record_async(se_gl_exec_vertex_attrib_pointer, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_vertex_attrib_pointer, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_enable_vertex_attrib_array(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_vertex_attrib_index_payload* args = (const se_gl_vertex_attrib_index_payload*)payload;
	glEnableVertexAttribArray(args->index);
}

void se_gl_dispatchEnableVertexAttribArray(GLuint index) {
	if (se_gl_dispatch_direct()) {
		glEnableVertexAttribArray(index);
		return;
	}
	const se_gl_vertex_attrib_index_payload payload = {index};
	if (se_gl_dispatch_record_async(se_gl_exec_enable_vertex_attrib_array, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_enable_vertex_attrib_array, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_vertex_attrib_divisor(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_vertex_attrib_divisor_payload* args = (const se_gl_vertex_attrib_divisor_payload*)payload;
	glVertexAttribDivisor(args->index, args->divisor);
}

void se_gl_dispatchVertexAttribDivisor(GLuint index, GLuint divisor) {
	if (se_gl_dispatch_direct()) {
		glVertexAttribDivisor(index, divisor);
		return;
	}
	const se_gl_vertex_attrib_divisor_payload payload = {index, divisor};
	if (se_gl_dispatch_record_async(se_gl_exec_vertex_attrib_divisor, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_vertex_attrib_divisor, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_draw_arrays_instanced(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_draw_arrays_instanced_payload* args = (const se_gl_draw_arrays_instanced_payload*)payload;
	glDrawArraysInstanced(args->mode, args->first, args->count, args->instance_count);
}

void se_gl_dispatchDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
	if (se_gl_dispatch_direct()) {
		glDrawArraysInstanced(mode, first, count, instancecount);
		return;
	}
	const se_gl_draw_arrays_instanced_payload payload = {mode, first, count, instancecount};
	if (se_gl_dispatch_record_async(se_gl_exec_draw_arrays_instanced, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_draw_arrays_instanced, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_gen_framebuffers(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_ids_out_payload* args = (const se_gl_ids_out_payload*)payload;
	glGenFramebuffers(args->n, args->ids);
}

void se_gl_dispatchGenFramebuffers(GLsizei n, GLuint *framebuffers) {
	if (se_gl_dispatch_direct()) {
		glGenFramebuffers(n, framebuffers);
		return;
	}
	const se_gl_ids_out_payload payload = {n, framebuffers};
	(void)se_render_queue_call_sync_sized(se_gl_exec_gen_framebuffers, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_framebuffer_texture2d(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_framebuffer_texture2d_payload* args = (const se_gl_framebuffer_texture2d_payload*)payload;
	glFramebufferTexture2D(args->target, args->attachment, args->texture_target, args->texture, args->level);
}

void se_gl_dispatchFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
	if (se_gl_dispatch_direct()) {
		glFramebufferTexture2D(target, attachment, textarget, texture, level);
		return;
	}
	const se_gl_framebuffer_texture2d_payload payload = {target, attachment, textarget, texture, level};
	if (se_gl_dispatch_record_async(se_gl_exec_framebuffer_texture2d, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_framebuffer_texture2d, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_get_shader_iv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_shader_iv_payload* args = (const se_gl_shader_iv_payload*)payload;
	glGetShaderiv(args->shader, args->pname, args->out_params);
}

void se_gl_dispatchGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
	if (se_gl_dispatch_direct()) {
		glGetShaderiv(shader, pname, params);
		return;
	}
	const se_gl_shader_iv_payload payload = {shader, pname, params};
	(void)se_render_queue_call_sync_sized(se_gl_exec_get_shader_iv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_get_shader_info_log(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_shader_log_payload* args = (const se_gl_shader_log_payload*)payload;
	glGetShaderInfoLog(args->shader, args->buf_size, args->out_length, args->out_log);
}

void se_gl_dispatchGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
	if (se_gl_dispatch_direct()) {
		glGetShaderInfoLog(shader, bufSize, length, infoLog);
		return;
	}
	const se_gl_shader_log_payload payload = {shader, bufSize, length, infoLog};
	(void)se_render_queue_call_sync_sized(se_gl_exec_get_shader_info_log, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_get_program_iv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_program_iv_payload* args = (const se_gl_program_iv_payload*)payload;
	glGetProgramiv(args->program, args->pname, args->out_params);
}

void se_gl_dispatchGetProgramiv(GLuint program, GLenum pname, GLint *params) {
	if (se_gl_dispatch_direct()) {
		glGetProgramiv(program, pname, params);
		return;
	}
	const se_gl_program_iv_payload payload = {program, pname, params};
	(void)se_render_queue_call_sync_sized(se_gl_exec_get_program_iv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_get_program_info_log(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_program_log_payload* args = (const se_gl_program_log_payload*)payload;
	glGetProgramInfoLog(args->program, args->buf_size, args->out_length, args->out_log);
}

void se_gl_dispatchGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
	if (se_gl_dispatch_direct()) {
		glGetProgramInfoLog(program, bufSize, length, infoLog);
		return;
	}
	const se_gl_program_log_payload payload = {program, bufSize, length, infoLog};
	(void)se_render_queue_call_sync_sized(se_gl_exec_get_program_info_log, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_draw_elements_instanced(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_draw_elements_instanced_payload* args = (const se_gl_draw_elements_instanced_payload*)payload;
	glDrawElementsInstanced(args->mode, args->count, args->type, args->indices, args->instance_count);
}

void se_gl_dispatchDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount) {
	if (se_gl_dispatch_direct()) {
		glDrawElementsInstanced(mode, count, type, indices, primcount);
		return;
	}
	const se_gl_draw_elements_instanced_payload payload = {mode, count, type, indices, primcount};
	if (se_gl_dispatch_record_async(se_gl_exec_draw_elements_instanced, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_draw_elements_instanced, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_map_buffer(const void* payload, void* out_result) {
	const se_gl_map_buffer_payload* args = (const se_gl_map_buffer_payload*)payload;
	if (out_result) {
		*(void**)out_result = glMapBuffer(args->target, args->access);
	}
}

void *se_gl_dispatchMapBuffer(GLenum target, GLenum access) {
	if (se_gl_dispatch_direct()) {
		return glMapBuffer(target, access);
	}
	const se_gl_map_buffer_payload payload = {target, access};
	void* out_ptr = NULL;
	if (!se_render_queue_call_sync_sized(se_gl_exec_map_buffer, &payload, &out_ptr, (u32)sizeof(payload))) {
		return NULL;
	}
	return out_ptr;
}

static void se_gl_exec_unmap_buffer(const void* payload, void* out_result) {
	const se_gl_unmap_buffer_payload* args = (const se_gl_unmap_buffer_payload*)payload;
	if (out_result) {
		*(GLboolean*)out_result = glUnmapBuffer(args->target);
	}
}

GLboolean se_gl_dispatchUnmapBuffer(GLenum target) {
	if (se_gl_dispatch_direct()) {
		return glUnmapBuffer(target);
	}
	const se_gl_unmap_buffer_payload payload = {target};
	GLboolean out_value = GL_FALSE;
	if (!se_render_queue_call_sync_sized(se_gl_exec_unmap_buffer, &payload, &out_value, (u32)sizeof(payload))) {
		return GL_FALSE;
	}
	return out_value;
}

static void se_gl_exec_get_uniform_location(const void* payload, void* out_result) {
	const se_gl_uniform_loc_payload* args = (const se_gl_uniform_loc_payload*)payload;
	if (out_result) {
		*(GLint*)out_result = glGetUniformLocation(args->program, args->name);
	}
}

GLint se_gl_dispatchGetUniformLocation(GLuint program, const GLchar *name) {
	if (se_gl_dispatch_direct()) {
		return glGetUniformLocation(program, name);
	}
	const se_gl_uniform_loc_payload payload = {program, name};
	GLint out_location = -1;
	if (!se_render_queue_call_sync_sized(se_gl_exec_get_uniform_location, &payload, &out_location, (u32)sizeof(payload))) {
		return -1;
	}
	return out_location;
}

static void se_gl_exec_uniform1i(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_uniform1i_payload* args = (const se_gl_uniform1i_payload*)payload;
	glUniform1i(args->location, args->value);
}

void se_gl_dispatchUniform1i(GLint location, GLint v0) {
	if (se_gl_dispatch_direct()) {
		glUniform1i(location, v0);
		return;
	}
	const se_gl_uniform1i_payload payload = {location, v0};
	if (se_gl_dispatch_record_async(se_gl_exec_uniform1i, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_uniform1i, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_uniform1f(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_uniform1f_payload* args = (const se_gl_uniform1f_payload*)payload;
	glUniform1f(args->location, args->value);
}

void se_gl_dispatchUniform1f(GLint location, GLfloat v0) {
	if (se_gl_dispatch_direct()) {
		glUniform1f(location, v0);
		return;
	}
	const se_gl_uniform1f_payload payload = {location, v0};
	if (se_gl_dispatch_record_async(se_gl_exec_uniform1f, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_uniform1f, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_uniform1fv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_uniform_fv_payload* args = (const se_gl_uniform_fv_payload*)payload;
	glUniform1fv(args->location, args->count, args->value);
}

void se_gl_dispatchUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
	if (se_gl_dispatch_direct()) {
		glUniform1fv(location, count, value);
		return;
	}
	const se_gl_uniform_fv_payload payload = {location, count, value};
	u32 blob_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(count, (u32)sizeof(GLfloat), &blob_bytes) && value != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_uniform1fv,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_uniform_fv_payload, value),
			value,
			blob_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_uniform1fv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_uniform2fv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_uniform_fv_payload* args = (const se_gl_uniform_fv_payload*)payload;
	glUniform2fv(args->location, args->count, args->value);
}

void se_gl_dispatchUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
	if (se_gl_dispatch_direct()) {
		glUniform2fv(location, count, value);
		return;
	}
	const se_gl_uniform_fv_payload payload = {location, count, value};
	u32 blob_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(count, 2u * (u32)sizeof(GLfloat), &blob_bytes) && value != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_uniform2fv,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_uniform_fv_payload, value),
			value,
			blob_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_uniform2fv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_uniform3fv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_uniform_fv_payload* args = (const se_gl_uniform_fv_payload*)payload;
	glUniform3fv(args->location, args->count, args->value);
}

void se_gl_dispatchUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
	if (se_gl_dispatch_direct()) {
		glUniform3fv(location, count, value);
		return;
	}
	const se_gl_uniform_fv_payload payload = {location, count, value};
	u32 blob_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(count, 3u * (u32)sizeof(GLfloat), &blob_bytes) && value != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_uniform3fv,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_uniform_fv_payload, value),
			value,
			blob_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_uniform3fv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_uniform4fv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_uniform_fv_payload* args = (const se_gl_uniform_fv_payload*)payload;
	glUniform4fv(args->location, args->count, args->value);
}

void se_gl_dispatchUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
	if (se_gl_dispatch_direct()) {
		glUniform4fv(location, count, value);
		return;
	}
	const se_gl_uniform_fv_payload payload = {location, count, value};
	u32 blob_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(count, 4u * (u32)sizeof(GLfloat), &blob_bytes) && value != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_uniform4fv,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_uniform_fv_payload, value),
			value,
			blob_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_uniform4fv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_uniform_matrix3fv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_uniform_matrix_payload* args = (const se_gl_uniform_matrix_payload*)payload;
	glUniformMatrix3fv(args->location, args->count, args->transpose, args->value);
}

void se_gl_dispatchUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	if (se_gl_dispatch_direct()) {
		glUniformMatrix3fv(location, count, transpose, value);
		return;
	}
	const se_gl_uniform_matrix_payload payload = {location, count, transpose, value};
	u32 blob_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(count, 9u * (u32)sizeof(GLfloat), &blob_bytes) && value != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_uniform_matrix3fv,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_uniform_matrix_payload, value),
			value,
			blob_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_uniform_matrix3fv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_uniform_matrix4fv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_uniform_matrix_payload* args = (const se_gl_uniform_matrix_payload*)payload;
	glUniformMatrix4fv(args->location, args->count, args->transpose, args->value);
}

void se_gl_dispatchUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	if (se_gl_dispatch_direct()) {
		glUniformMatrix4fv(location, count, transpose, value);
		return;
	}
	const se_gl_uniform_matrix_payload payload = {location, count, transpose, value};
	u32 blob_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(count, 16u * (u32)sizeof(GLfloat), &blob_bytes) && value != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_uniform_matrix4fv,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_uniform_matrix_payload, value),
			value,
			blob_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_uniform_matrix4fv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_bind_renderbuffer(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_bind_payload* args = (const se_gl_bind_payload*)payload;
	glBindRenderbuffer(args->target, args->id);
}

void se_gl_dispatchBindRenderbuffer(GLenum target, GLuint renderbuffer) {
	if (se_gl_dispatch_direct()) {
		glBindRenderbuffer(target, renderbuffer);
		return;
	}
	const se_gl_bind_payload payload = {target, renderbuffer};
	if (se_gl_dispatch_record_async(se_gl_exec_bind_renderbuffer, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_bind_renderbuffer, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_delete_renderbuffers(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_ids_in_payload* args = (const se_gl_ids_in_payload*)payload;
	glDeleteRenderbuffers(args->n, args->ids);
}

void se_gl_dispatchDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
	if (se_gl_dispatch_direct()) {
		glDeleteRenderbuffers(n, renderbuffers);
		return;
	}
	const se_gl_ids_in_payload payload = {n, renderbuffers};
	u32 ids_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(n, (u32)sizeof(GLuint), &ids_bytes) && renderbuffers != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_delete_renderbuffers,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_ids_in_payload, ids),
			renderbuffers,
			ids_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_delete_renderbuffers, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_delete_framebuffers(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_ids_in_payload* args = (const se_gl_ids_in_payload*)payload;
	glDeleteFramebuffers(args->n, args->ids);
}

void se_gl_dispatchDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
	if (se_gl_dispatch_direct()) {
		glDeleteFramebuffers(n, framebuffers);
		return;
	}
	const se_gl_ids_in_payload payload = {n, framebuffers};
	u32 ids_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(n, (u32)sizeof(GLuint), &ids_bytes) && framebuffers != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_delete_framebuffers,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_ids_in_payload, ids),
			framebuffers,
			ids_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_delete_framebuffers, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_renderbuffer_storage(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_renderbuffer_storage_payload* args = (const se_gl_renderbuffer_storage_payload*)payload;
	glRenderbufferStorage(args->target, args->internal_format, args->width, args->height);
}

void se_gl_dispatchRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
	if (se_gl_dispatch_direct()) {
		glRenderbufferStorage(target, internalformat, width, height);
		return;
	}
	const se_gl_renderbuffer_storage_payload payload = {target, internalformat, width, height};
	if (se_gl_dispatch_record_async(se_gl_exec_renderbuffer_storage, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_renderbuffer_storage, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_check_framebuffer_status(const void* payload, void* out_result) {
	const se_gl_framebuffer_status_payload* args = (const se_gl_framebuffer_status_payload*)payload;
	if (out_result) {
		*(GLenum*)out_result = glCheckFramebufferStatus(args->target);
	}
}

GLenum se_gl_dispatchCheckFramebufferStatus(GLenum target) {
	if (se_gl_dispatch_direct()) {
		return glCheckFramebufferStatus(target);
	}
	const se_gl_framebuffer_status_payload payload = {target};
	GLenum out_status = 0;
	if (!se_render_queue_call_sync_sized(se_gl_exec_check_framebuffer_status, &payload, &out_status, (u32)sizeof(payload))) {
		return 0;
	}
	return out_status;
}

static void se_gl_exec_generate_mipmap(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_framebuffer_status_payload* args = (const se_gl_framebuffer_status_payload*)payload;
	glGenerateMipmap(args->target);
}

void se_gl_dispatchGenerateMipmap(GLenum target) {
	if (se_gl_dispatch_direct()) {
		glGenerateMipmap(target);
		return;
	}
	const se_gl_framebuffer_status_payload payload = {target};
	if (se_gl_dispatch_record_async(se_gl_exec_generate_mipmap, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_generate_mipmap, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_blit_framebuffer(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_blit_framebuffer_payload* args = (const se_gl_blit_framebuffer_payload*)payload;
	glBlitFramebuffer(args->src_x0, args->src_y0, args->src_x1, args->src_y1, args->dst_x0, args->dst_y0, args->dst_x1, args->dst_y1, args->mask, args->filter);
}

void se_gl_dispatchBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
	if (se_gl_dispatch_direct()) {
		glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
		return;
	}
	const se_gl_blit_framebuffer_payload payload = {srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter};
	if (se_gl_dispatch_record_async(se_gl_exec_blit_framebuffer, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_blit_framebuffer, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_enable(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_enable_payload* args = (const se_gl_enable_payload*)payload;
	glEnable(args->cap);
}

void se_gl_dispatchEnable(GLenum cap) {
	if (se_gl_dispatch_direct()) {
		glEnable(cap);
		return;
	}
	const se_gl_enable_payload payload = {cap};
	if (se_gl_dispatch_record_async(se_gl_exec_enable, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_enable, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_disable(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_enable_payload* args = (const se_gl_enable_payload*)payload;
	glDisable(args->cap);
}

void se_gl_dispatchDisable(GLenum cap) {
	if (se_gl_dispatch_direct()) {
		glDisable(cap);
		return;
	}
	const se_gl_enable_payload payload = {cap};
	if (se_gl_dispatch_record_async(se_gl_exec_disable, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_disable, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_blend_equation(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_enable_payload* args = (const se_gl_enable_payload*)payload;
	glBlendEquation(args->cap);
}

void se_gl_dispatchBlendEquation(GLenum mode) {
	if (se_gl_dispatch_direct()) {
		glBlendEquation(mode);
		return;
	}
	const se_gl_enable_payload payload = {mode};
	if (se_gl_dispatch_record_async(se_gl_exec_blend_equation, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_blend_equation, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_blend_func(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_blend_func_payload* args = (const se_gl_blend_func_payload*)payload;
	glBlendFunc(args->sfactor, args->dfactor);
}

void se_gl_dispatchBlendFunc(GLenum sfactor, GLenum dfactor) {
	if (se_gl_dispatch_direct()) {
		glBlendFunc(sfactor, dfactor);
		return;
	}
	const se_gl_blend_func_payload payload = {sfactor, dfactor};
	if (se_gl_dispatch_record_async(se_gl_exec_blend_func, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_blend_func, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_clear(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_clear_payload* args = (const se_gl_clear_payload*)payload;
	glClear(args->mask);
}

void se_gl_dispatchClear(GLbitfield mask) {
	if (se_gl_dispatch_direct()) {
		glClear(mask);
		return;
	}
	const se_gl_clear_payload payload = {mask};
	if (se_gl_dispatch_record_async(se_gl_exec_clear, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_clear, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_clear_color(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_clear_color_payload* args = (const se_gl_clear_color_payload*)payload;
	glClearColor(args->r, args->g, args->b, args->a);
}

void se_gl_dispatchClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	if (se_gl_dispatch_direct()) {
		glClearColor(red, green, blue, alpha);
		return;
	}
	const se_gl_clear_color_payload payload = {red, green, blue, alpha};
	if (se_gl_dispatch_record_async(se_gl_exec_clear_color, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_clear_color, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_color_mask(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_color_mask_payload* args = (const se_gl_color_mask_payload*)payload;
	glColorMask(args->r, args->g, args->b, args->a);
}

void se_gl_dispatchColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	if (se_gl_dispatch_direct()) {
		glColorMask(red, green, blue, alpha);
		return;
	}
	const se_gl_color_mask_payload payload = {red, green, blue, alpha};
	if (se_gl_dispatch_record_async(se_gl_exec_color_mask, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_color_mask, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_depth_mask(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_color_mask_payload* args = (const se_gl_color_mask_payload*)payload;
	glDepthMask(args->r);
}

void se_gl_dispatchDepthMask(GLboolean flag) {
	if (se_gl_dispatch_direct()) {
		glDepthMask(flag);
		return;
	}
	const se_gl_color_mask_payload payload = {flag, 0, 0, 0};
	if (se_gl_dispatch_record_async(se_gl_exec_depth_mask, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_depth_mask, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_stencil_mask(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_u32_payload* args = (const se_gl_u32_payload*)payload;
	glStencilMask(args->value);
}

void se_gl_dispatchStencilMask(GLuint mask) {
	if (se_gl_dispatch_direct()) {
		glStencilMask(mask);
		return;
	}
	const se_gl_u32_payload payload = {mask};
	if (se_gl_dispatch_record_async(se_gl_exec_stencil_mask, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_stencil_mask, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_depth_func(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_enable_payload* args = (const se_gl_enable_payload*)payload;
	glDepthFunc(args->cap);
}

void se_gl_dispatchDepthFunc(GLenum func) {
	if (se_gl_dispatch_direct()) {
		glDepthFunc(func);
		return;
	}
	const se_gl_enable_payload payload = {func};
	if (se_gl_dispatch_record_async(se_gl_exec_depth_func, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_depth_func, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_cull_face(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_enable_payload* args = (const se_gl_enable_payload*)payload;
	glCullFace(args->cap);
}

void se_gl_dispatchCullFace(GLenum mode) {
	if (se_gl_dispatch_direct()) {
		glCullFace(mode);
		return;
	}
	const se_gl_enable_payload payload = {mode};
	if (se_gl_dispatch_record_async(se_gl_exec_cull_face, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_cull_face, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_front_face(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_enable_payload* args = (const se_gl_enable_payload*)payload;
	glFrontFace(args->cap);
}

void se_gl_dispatchFrontFace(GLenum mode) {
	if (se_gl_dispatch_direct()) {
		glFrontFace(mode);
		return;
	}
	const se_gl_enable_payload payload = {mode};
	if (se_gl_dispatch_record_async(se_gl_exec_front_face, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_front_face, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_draw_elements(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_draw_elements_payload* args = (const se_gl_draw_elements_payload*)payload;
	glDrawElements(args->mode, args->count, args->type, args->indices);
}

void se_gl_dispatchDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
	if (se_gl_dispatch_direct()) {
		glDrawElements(mode, count, type, indices);
		return;
	}
	const se_gl_draw_elements_payload payload = {mode, count, type, indices};
	if (se_gl_dispatch_record_async(se_gl_exec_draw_elements, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_draw_elements, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_bind_texture(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_bind_payload* args = (const se_gl_bind_payload*)payload;
	glBindTexture(args->target, args->id);
}

void se_gl_dispatchBindTexture(GLenum target, GLuint texture) {
	if (se_gl_dispatch_direct()) {
		glBindTexture(target, texture);
		return;
	}
	const se_gl_bind_payload payload = {target, texture};
	if (se_gl_dispatch_record_async(se_gl_exec_bind_texture, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_bind_texture, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_delete_textures(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_ids_in_payload* args = (const se_gl_ids_in_payload*)payload;
	glDeleteTextures(args->n, args->ids);
}

void se_gl_dispatchDeleteTextures(GLsizei n, const GLuint *textures) {
	if (se_gl_dispatch_direct()) {
		glDeleteTextures(n, textures);
		return;
	}
	const se_gl_ids_in_payload payload = {n, textures};
	u32 ids_bytes = 0u;
	if (se_gl_dispatch_compute_blob_bytes_i32(n, (u32)sizeof(GLuint), &ids_bytes) && textures != NULL) {
		if (se_gl_dispatch_record_async_blob(se_gl_exec_delete_textures,
			&payload,
			(u32)sizeof(payload),
			(u32)offsetof(se_gl_ids_in_payload, ids),
			textures,
			ids_bytes)) {
			return;
		}
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_delete_textures, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_gen_textures(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_ids_out_payload* args = (const se_gl_ids_out_payload*)payload;
	glGenTextures(args->n, args->ids);
}

void se_gl_dispatchGenTextures(GLsizei n, GLuint *textures) {
	if (se_gl_dispatch_direct()) {
		glGenTextures(n, textures);
		return;
	}
	const se_gl_ids_out_payload payload = {n, textures};
	(void)se_render_queue_call_sync_sized(se_gl_exec_gen_textures, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_tex_image_2d(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_tex_image_2d_payload* args = (const se_gl_tex_image_2d_payload*)payload;
	glTexImage2D(args->target, args->level, args->internal_format, args->width, args->height, args->border, args->format, args->type, args->pixels);
}

void se_gl_dispatchTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
	if (se_gl_dispatch_direct()) {
		glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
		return;
	}
	const se_gl_tex_image_2d_payload payload = {target, level, internalFormat, width, height, border, format, type, pixels};
	(void)se_render_queue_call_sync_sized(se_gl_exec_tex_image_2d, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_tex_param_i(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_tex_param_i_payload* args = (const se_gl_tex_param_i_payload*)payload;
	glTexParameteri(args->target, args->pname, args->param);
}

void se_gl_dispatchTexParameteri(GLenum target, GLenum pname, GLint param) {
	if (se_gl_dispatch_direct()) {
		glTexParameteri(target, pname, param);
		return;
	}
	const se_gl_tex_param_i_payload payload = {target, pname, param};
	if (se_gl_dispatch_record_async(se_gl_exec_tex_param_i, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_tex_param_i, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_active_texture(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_enable_payload* args = (const se_gl_enable_payload*)payload;
	glActiveTexture(args->cap);
}

void se_gl_dispatchActiveTexture(GLenum texture) {
	if (se_gl_dispatch_direct()) {
		glActiveTexture(texture);
		return;
	}
	const se_gl_enable_payload payload = {texture};
	if (se_gl_dispatch_record_async(se_gl_exec_active_texture, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_active_texture, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_viewport(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_viewport_payload* args = (const se_gl_viewport_payload*)payload;
	glViewport(args->x, args->y, args->width, args->height);
}

void se_gl_dispatchViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	if (se_gl_dispatch_direct()) {
		glViewport(x, y, width, height);
		return;
	}
	const se_gl_viewport_payload payload = {x, y, width, height};
	if (se_gl_dispatch_record_async(se_gl_exec_viewport, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_viewport, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_get_integerv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_get_int_payload* args = (const se_gl_get_int_payload*)payload;
	glGetIntegerv(args->pname, args->out_value);
}

void se_gl_dispatchGetIntegerv(GLenum pname, GLint *data) {
	if (se_gl_dispatch_direct()) {
		glGetIntegerv(pname, data);
		return;
	}
	const se_gl_get_int_payload payload = {pname, data};
	(void)se_render_queue_call_sync_sized(se_gl_exec_get_integerv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_get_booleanv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_get_bool_payload* args = (const se_gl_get_bool_payload*)payload;
	glGetBooleanv(args->pname, args->out_value);
}

void se_gl_dispatchGetBooleanv(GLenum pname, GLboolean *data) {
	if (se_gl_dispatch_direct()) {
		glGetBooleanv(pname, data);
		return;
	}
	const se_gl_get_bool_payload payload = {pname, data};
	(void)se_render_queue_call_sync_sized(se_gl_exec_get_booleanv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_get_floatv(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_get_float_payload* args = (const se_gl_get_float_payload*)payload;
	glGetFloatv(args->pname, args->out_value);
}

void se_gl_dispatchGetFloatv(GLenum pname, GLfloat *data) {
	if (se_gl_dispatch_direct()) {
		glGetFloatv(pname, data);
		return;
	}
	const se_gl_get_float_payload payload = {pname, data};
	(void)se_render_queue_call_sync_sized(se_gl_exec_get_floatv, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_is_enabled(const void* payload, void* out_result) {
	const se_gl_enable_payload* args = (const se_gl_enable_payload*)payload;
	if (out_result) {
		*(GLboolean*)out_result = glIsEnabled(args->cap);
	}
}

GLboolean se_gl_dispatchIsEnabled(GLenum cap) {
	if (se_gl_dispatch_direct()) {
		return glIsEnabled(cap);
	}
	const se_gl_enable_payload payload = {cap};
	GLboolean out_enabled = GL_FALSE;
	if (!se_render_queue_call_sync_sized(se_gl_exec_is_enabled, &payload, &out_enabled, (u32)sizeof(payload))) {
		return GL_FALSE;
	}
	return out_enabled;
}

static void se_gl_exec_pixel_store_i(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_pixel_store_i_payload* args = (const se_gl_pixel_store_i_payload*)payload;
	glPixelStorei(args->pname, args->param);
}

void se_gl_dispatchPixelStorei(GLenum pname, GLint param) {
	if (se_gl_dispatch_direct()) {
		glPixelStorei(pname, param);
		return;
	}
	const se_gl_pixel_store_i_payload payload = {pname, param};
	if (se_gl_dispatch_record_async(se_gl_exec_pixel_store_i, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_pixel_store_i, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_read_buffer(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_enable_payload* args = (const se_gl_enable_payload*)payload;
	glReadBuffer(args->cap);
}

void se_gl_dispatchReadBuffer(GLenum src) {
	if (se_gl_dispatch_direct()) {
		glReadBuffer(src);
		return;
	}
	const se_gl_enable_payload payload = {src};
	if (se_gl_dispatch_record_async(se_gl_exec_read_buffer, &payload, (u32)sizeof(payload))) {
		return;
	}
	(void)se_render_queue_call_sync_sized(se_gl_exec_read_buffer, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_read_pixels(const void* payload, void* out_result) {
	(void)out_result;
	const se_gl_read_pixels_payload* args = (const se_gl_read_pixels_payload*)payload;
	glReadPixels(args->x, args->y, args->width, args->height, args->format, args->type, args->pixels);
}

void se_gl_dispatchReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
	if (se_gl_dispatch_direct()) {
		glReadPixels(x, y, width, height, format, type, pixels);
		return;
	}
	const se_gl_read_pixels_payload payload = {x, y, width, height, format, type, pixels};
	(void)se_render_queue_call_sync_sized(se_gl_exec_read_pixels, &payload, NULL, (u32)sizeof(payload));
}

static void se_gl_exec_get_string(const void* payload, void* out_result) {
	const se_gl_get_string_payload* args = (const se_gl_get_string_payload*)payload;
	if (out_result) {
		*(const GLubyte**)out_result = glGetString(args->name);
	}
}

const GLubyte* se_gl_dispatchGetString(GLenum name) {
	if (se_gl_dispatch_direct()) {
		return glGetString(name);
	}
	const se_gl_get_string_payload payload = {name};
	const GLubyte* out_string = NULL;
	if (!se_render_queue_call_sync_sized(se_gl_exec_get_string, &payload, &out_string, (u32)sizeof(payload))) {
		return NULL;
	}
	return out_string;
}
