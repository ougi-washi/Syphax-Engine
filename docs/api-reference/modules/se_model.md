<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_model.h

Source header: `include/se_model.h`

## Overview

Model loading, model handles, and mesh composition APIs.

This page is generated from `include/se_model.h` and is deterministic.

## Functions

### `se_mesh_discard_cpu_data`

<div class="api-signature">

```c
extern void se_mesh_discard_cpu_data(se_mesh *mesh);
```

</div>

No inline description found in header comments.

### `se_mesh_has_cpu_data`

<div class="api-signature">

```c
extern b8 se_mesh_has_cpu_data(const se_mesh *mesh);
```

</div>

No inline description found in header comments.

### `se_mesh_has_gpu_data`

<div class="api-signature">

```c
extern b8 se_mesh_has_gpu_data(const se_mesh *mesh);
```

</div>

No inline description found in header comments.

### `se_mesh_instance_add_buffer`

<div class="api-signature">

```c
extern void se_mesh_instance_add_buffer(se_mesh_instance *instance, const s_mat4 *buffer, const sz instance_count);
```

</div>

No inline description found in header comments.

### `se_mesh_instance_create`

<div class="api-signature">

```c
extern void se_mesh_instance_create(se_mesh_instance *out_instance, const se_mesh *mesh, const u32 instance_count);
```

</div>

No inline description found in header comments.

### `se_mesh_instance_destroy`

<div class="api-signature">

```c
extern void se_mesh_instance_destroy(se_mesh_instance *instance);
```

</div>

No inline description found in header comments.

### `se_mesh_instance_update`

<div class="api-signature">

```c
extern void se_mesh_instance_update(se_mesh_instance *instance);
```

</div>

No inline description found in header comments.

### `se_mesh_rotate`

<div class="api-signature">

```c
extern void se_mesh_rotate(se_mesh *mesh, const s_vec3 *v);
```

</div>

No inline description found in header comments.

### `se_mesh_scale`

<div class="api-signature">

```c
extern void se_mesh_scale(se_mesh *mesh, const s_vec3 *v);
```

</div>

No inline description found in header comments.

### `se_mesh_translate`

<div class="api-signature">

```c
extern void se_mesh_translate(se_mesh *mesh, const s_vec3 *v);
```

</div>

No inline description found in header comments.

### `se_model_destroy`

<div class="api-signature">

```c
extern void se_model_destroy(const se_model_handle model);
```

</div>

No inline description found in header comments.

### `se_model_discard_cpu_data`

<div class="api-signature">

```c
extern void se_model_discard_cpu_data(se_model *model);
```

</div>

No inline description found in header comments.

### `se_model_get_mesh_count`

<div class="api-signature">

```c
extern sz se_model_get_mesh_count(const se_model_handle model);
```

</div>

No inline description found in header comments.

### `se_model_get_mesh_shader`

<div class="api-signature">

```c
extern se_shader_handle se_model_get_mesh_shader(const se_model_handle model, const sz mesh_index);
```

</div>

No inline description found in header comments.

### `se_model_load_obj`

<div class="api-signature">

```c
extern se_model_handle se_model_load_obj(const char *path, const se_shaders_ptr *shaders);
```

</div>

No inline description found in header comments.

### `se_model_load_obj_ex`

<div class="api-signature">

```c
extern se_model_handle se_model_load_obj_ex(const char *path, const se_shaders_ptr *shaders, const se_mesh_data_flags mesh_data_flags);
```

</div>

No inline description found in header comments.

### `se_model_load_obj_simple`

<div class="api-signature">

```c
extern se_model_handle se_model_load_obj_simple(const char *obj_path, const char *vertex_shader_path, const char *fragment_shader_path);
```

</div>

No inline description found in header comments.

### `se_model_load_obj_simple_ex`

<div class="api-signature">

```c
extern se_model_handle se_model_load_obj_simple_ex(const char *obj_path, const char *vertex_shader_path, const char *fragment_shader_path, const se_mesh_data_flags mesh_data_flags);
```

</div>

No inline description found in header comments.

### `se_model_render`

<div class="api-signature">

```c
extern void se_model_render(const se_model_handle model, const se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_model_rotate`

<div class="api-signature">

```c
extern void se_model_rotate(const se_model_handle model, const s_vec3 *v);
```

</div>

No inline description found in header comments.

### `se_model_scale`

<div class="api-signature">

```c
extern void se_model_scale(const se_model_handle model, const s_vec3 *v);
```

</div>

No inline description found in header comments.

### `se_model_translate`

<div class="api-signature">

```c
extern void se_model_translate(const se_model_handle model, const s_vec3 *v);
```

</div>

No inline description found in header comments.

## Enums

### `se_mesh_data_flags`

<div class="api-signature">

```c
typedef enum { SE_MESH_DATA_NONE = 0, SE_MESH_DATA_CPU = 1 << 0, SE_MESH_DATA_GPU = 1 << 1, SE_MESH_DATA_CPU_GPU = SE_MESH_DATA_CPU | SE_MESH_DATA_GPU } se_mesh_data_flags;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_mesh`

<div class="api-signature">

```c
typedef struct { se_mesh_cpu_data cpu; se_mesh_gpu_data gpu; se_shader_handle shader; s_mat4 matrix; se_mesh_data_flags data_flags; } se_mesh;
```

</div>

No inline description found in header comments.

### `se_mesh_cpu_data`

<div class="api-signature">

```c
typedef struct { s_array(c8, file_path); s_array(se_vertex_3d, vertices); s_array(u32, indices); } se_mesh_cpu_data;
```

</div>

No inline description found in header comments.

### `se_mesh_gpu_data`

<div class="api-signature">

```c
typedef struct { u32 vertex_count; u32 index_count; u32 vao; u32 vbo; u32 ebo; } se_mesh_gpu_data;
```

</div>

No inline description found in header comments.

### `se_model`

<div class="api-signature">

```c
typedef struct se_model { se_meshes meshes; } se_model;
```

</div>

No inline description found in header comments.

### `se_model_ptr`

<div class="api-signature">

```c
typedef se_model_handle se_model_ptr;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_mesh, se_meshes);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(se_model, se_models);
```

</div>

No inline description found in header comments.

### `typedef_3`

<div class="api-signature">

```c
typedef s_array(se_model_handle, se_models_ptr);
```

</div>

No inline description found in header comments.
