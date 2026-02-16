<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_shader.h

Source header: `include/se_shader.h`

## Overview

Shader loading, uniform access, and shader handle lifecycle.

This page is generated from `include/se_shader.h` and is deterministic.

## Functions

### `se_context_get_global_uniforms`

<div class="api-signature">

```c
extern se_uniforms *se_context_get_global_uniforms(void);
```

</div>

No inline description found in header comments.

### `se_context_reload_changed_shaders`

<div class="api-signature">

```c
extern void se_context_reload_changed_shaders(void);
```

</div>

No inline description found in header comments.

### `se_shader_destroy`

<div class="api-signature">

```c
extern void se_shader_destroy(const se_shader_handle shader);
```

</div>

No inline description found in header comments.

### `se_shader_get_uniform_float`

<div class="api-signature">

```c
extern f32 *se_shader_get_uniform_float(const se_shader_handle shader, const char *name);
```

</div>

No inline description found in header comments.

### `se_shader_get_uniform_int`

<div class="api-signature">

```c
extern i32 *se_shader_get_uniform_int(const se_shader_handle shader, const char *name);
```

</div>

No inline description found in header comments.

### `se_shader_get_uniform_location`

<div class="api-signature">

```c
extern i32 se_shader_get_uniform_location(const se_shader_handle shader, const char *name);
```

</div>

No inline description found in header comments.

### `se_shader_get_uniform_mat4`

<div class="api-signature">

```c
extern s_mat4 *se_shader_get_uniform_mat4(const se_shader_handle shader, const char *name);
```

</div>

No inline description found in header comments.

### `se_shader_get_uniform_texture`

<div class="api-signature">

```c
extern u32 *se_shader_get_uniform_texture(const se_shader_handle shader, const char *name);
```

</div>

No inline description found in header comments.

### `se_shader_get_uniform_vec2`

<div class="api-signature">

```c
extern s_vec2 *se_shader_get_uniform_vec2(const se_shader_handle shader, const char *name);
```

</div>

No inline description found in header comments.

### `se_shader_get_uniform_vec3`

<div class="api-signature">

```c
extern s_vec3 *se_shader_get_uniform_vec3(const se_shader_handle shader, const char *name);
```

</div>

No inline description found in header comments.

### `se_shader_get_uniform_vec4`

<div class="api-signature">

```c
extern s_vec4 *se_shader_get_uniform_vec4(const se_shader_handle shader, const char *name);
```

</div>

No inline description found in header comments.

### `se_shader_load`

<div class="api-signature">

```c
extern se_shader_handle se_shader_load(const char *vs_path, const char *fs_path);
```

</div>

No inline description found in header comments.

### `se_shader_load_from_memory`

<div class="api-signature">

```c
extern se_shader_handle se_shader_load_from_memory(const char *vs_data, const char *fs_data);
```

</div>

No inline description found in header comments.

### `se_shader_reload_if_changed`

<div class="api-signature">

```c
extern b8 se_shader_reload_if_changed(const se_shader_handle shader);
```

</div>

No inline description found in header comments.

### `se_shader_set_float`

<div class="api-signature">

```c
extern void se_shader_set_float(const se_shader_handle shader, const char *name, f32 value);
```

</div>

No inline description found in header comments.

### `se_shader_set_int`

<div class="api-signature">

```c
extern void se_shader_set_int(const se_shader_handle shader, const char *name, i32 value);
```

</div>

No inline description found in header comments.

### `se_shader_set_mat3`

<div class="api-signature">

```c
extern void se_shader_set_mat3(const se_shader_handle shader, const char *name, const s_mat3 *value);
```

</div>

No inline description found in header comments.

### `se_shader_set_mat4`

<div class="api-signature">

```c
extern void se_shader_set_mat4(const se_shader_handle shader, const char *name, const s_mat4 *value);
```

</div>

No inline description found in header comments.

### `se_shader_set_texture`

<div class="api-signature">

```c
extern void se_shader_set_texture(const se_shader_handle shader, const char *name, const u32 texture);
```

</div>

No inline description found in header comments.

### `se_shader_set_vec2`

<div class="api-signature">

```c
extern void se_shader_set_vec2(const se_shader_handle shader, const char *name, const s_vec2 *value);
```

</div>

No inline description found in header comments.

### `se_shader_set_vec3`

<div class="api-signature">

```c
extern void se_shader_set_vec3(const se_shader_handle shader, const char *name, const s_vec3 *value);
```

</div>

No inline description found in header comments.

### `se_shader_set_vec4`

<div class="api-signature">

```c
extern void se_shader_set_vec4(const se_shader_handle shader, const char *name, const s_vec4 *value);
```

</div>

No inline description found in header comments.

### `se_shader_use`

<div class="api-signature">

```c
extern void se_shader_use(const se_shader_handle shader, const b8 update_uniforms, const b8 update_global_uniforms);
```

</div>

No inline description found in header comments.

### `se_uniform_apply`

<div class="api-signature">

```c
extern void se_uniform_apply(const se_shader_handle shader, const b8 update_global_uniforms);
```

</div>

No inline description found in header comments.

### `se_uniform_set_float`

<div class="api-signature">

```c
extern void se_uniform_set_float(se_uniforms *uniforms, const char *name, f32 value);
```

</div>

Uniform functions

### `se_uniform_set_int`

<div class="api-signature">

```c
extern void se_uniform_set_int(se_uniforms *uniforms, const char *name, i32 value);
```

</div>

No inline description found in header comments.

### `se_uniform_set_mat3`

<div class="api-signature">

```c
extern void se_uniform_set_mat3(se_uniforms *uniforms, const char *name, const s_mat3 *value);
```

</div>

No inline description found in header comments.

### `se_uniform_set_mat4`

<div class="api-signature">

```c
extern void se_uniform_set_mat4(se_uniforms *uniforms, const char *name, const s_mat4 *value);
```

</div>

No inline description found in header comments.

### `se_uniform_set_texture`

<div class="api-signature">

```c
extern void se_uniform_set_texture(se_uniforms *uniforms, const char *name, const u32 texture);
```

</div>

No inline description found in header comments.

### `se_uniform_set_vec2`

<div class="api-signature">

```c
extern void se_uniform_set_vec2(se_uniforms *uniforms, const char *name, const s_vec2 *value);
```

</div>

No inline description found in header comments.

### `se_uniform_set_vec3`

<div class="api-signature">

```c
extern void se_uniform_set_vec3(se_uniforms *uniforms, const char *name, const s_vec3 *value);
```

</div>

No inline description found in header comments.

### `se_uniform_set_vec4`

<div class="api-signature">

```c
extern void se_uniform_set_vec4(se_uniforms *uniforms, const char *name, const s_vec4 *value);
```

</div>

No inline description found in header comments.

## Enums

### `se_uniform_type`

<div class="api-signature">

```c
typedef enum { SE_UNIFORM_FLOAT, SE_UNIFORM_VEC2, SE_UNIFORM_VEC3, SE_UNIFORM_VEC4, SE_UNIFORM_INT, SE_UNIFORM_MAT3, SE_UNIFORM_MAT4, SE_UNIFORM_TEXTURE } se_uniform_type;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_shader`

<div class="api-signature">

```c
typedef struct se_shader { u32 program; u32 vertex_shader; u32 fragment_shader; c8 vertex_path[SE_MAX_PATH_LENGTH]; c8 fragment_path[SE_MAX_PATH_LENGTH]; time_t vertex_mtime; time_t fragment_mtime; se_uniforms uniforms; b8 needs_reload; } se_shader;
```

</div>

No inline description found in header comments.

### `se_shader_ptr`

<div class="api-signature">

```c
typedef se_shader_handle se_shader_ptr;
```

</div>

No inline description found in header comments.

### `se_uniform`

<div class="api-signature">

```c
typedef struct se_uniform { char name[SE_MAX_NAME_LENGTH]; se_uniform_type type; union { f32 f; s_vec2 vec2; s_vec3 vec3; s_vec4 vec4; i32 i; s_mat3 mat3; s_mat4 mat4; u32 texture; } value; } se_uniform;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_shader, se_shaders);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(se_shader_handle, se_shaders_ptr);
```

</div>

No inline description found in header comments.

### `typedef_3`

<div class="api-signature">

```c
typedef s_array(se_uniform, se_uniforms);
```

</div>

No inline description found in header comments.
