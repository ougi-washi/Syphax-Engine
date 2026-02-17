<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_simulation.h

Source header: `include/se_simulation.h`

## Overview

Deterministic simulation components, systems, events, and snapshots.

This page is generated from `include/se_simulation.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/simulation.md)

## Functions

### `se_simulation_component_get`

<div class="api-signature">

```c
extern b8 se_simulation_component_get(se_simulation_handle sim, se_entity_id entity, se_sim_component_type type, void* out_data, sz out_size);
```

</div>

No inline description found in header comments.

### `se_simulation_component_get_const_ptr`

<div class="api-signature">

```c
extern const void* se_simulation_component_get_const_ptr(se_simulation_handle sim, se_entity_id entity, se_sim_component_type type);
```

</div>

No inline description found in header comments.

### `se_simulation_component_get_ptr`

<div class="api-signature">

```c
extern void* se_simulation_component_get_ptr(se_simulation_handle sim, se_entity_id entity, se_sim_component_type type);
```

</div>

No inline description found in header comments.

### `se_simulation_component_register`

<div class="api-signature">

```c
extern b8 se_simulation_component_register(se_simulation_handle sim, const se_sim_component_desc* desc);
```

</div>

No inline description found in header comments.

### `se_simulation_component_remove`

<div class="api-signature">

```c
extern b8 se_simulation_component_remove(se_simulation_handle sim, se_entity_id entity, se_sim_component_type type);
```

</div>

No inline description found in header comments.

### `se_simulation_component_set`

<div class="api-signature">

```c
extern b8 se_simulation_component_set(se_simulation_handle sim, se_entity_id entity, se_sim_component_type type, const void* data, sz size);
```

</div>

No inline description found in header comments.

### `se_simulation_create`

<div class="api-signature">

```c
extern se_simulation_handle se_simulation_create(const se_simulation_config* config);
```

</div>

No inline description found in header comments.

### `se_simulation_destroy`

<div class="api-signature">

```c
extern void se_simulation_destroy(se_simulation_handle sim);
```

</div>

No inline description found in header comments.

### `se_simulation_entity_alive`

<div class="api-signature">

```c
extern b8 se_simulation_entity_alive(se_simulation_handle sim, se_entity_id entity);
```

</div>

No inline description found in header comments.

### `se_simulation_entity_count`

<div class="api-signature">

```c
extern u32 se_simulation_entity_count(se_simulation_handle sim);
```

</div>

No inline description found in header comments.

### `se_simulation_entity_create`

<div class="api-signature">

```c
extern se_entity_id se_simulation_entity_create(se_simulation_handle sim);
```

</div>

No inline description found in header comments.

### `se_simulation_entity_destroy`

<div class="api-signature">

```c
extern b8 se_simulation_entity_destroy(se_simulation_handle sim, se_entity_id entity);
```

</div>

No inline description found in header comments.

### `se_simulation_event_emit`

<div class="api-signature">

```c
extern b8 se_simulation_event_emit(se_simulation_handle sim, se_entity_id target, se_sim_event_type type, const void* payload, sz size, se_sim_tick deliver_at_tick);
```

</div>

No inline description found in header comments.

### `se_simulation_event_poll`

<div class="api-signature">

```c
extern b8 se_simulation_event_poll(se_simulation_handle sim, se_entity_id target, se_sim_event_type type, void* out_payload, sz out_size, se_sim_tick* out_tick);
```

</div>

No inline description found in header comments.

### `se_simulation_event_register`

<div class="api-signature">

```c
extern b8 se_simulation_event_register(se_simulation_handle sim, const se_sim_event_desc* desc);
```

</div>

No inline description found in header comments.

### `se_simulation_for_each_entity`

<div class="api-signature">

```c
extern void se_simulation_for_each_entity(se_simulation_handle sim, se_sim_entity_iterator_fn fn, void* user_data);
```

</div>

No inline description found in header comments.

### `se_simulation_from_json`

<div class="api-signature">

```c
extern b8 se_simulation_from_json(se_simulation_handle sim, const s_json* root);
```

</div>

No inline description found in header comments.

### `se_simulation_get_diagnostics`

<div class="api-signature">

```c
extern void se_simulation_get_diagnostics(se_simulation_handle sim, se_simulation_diagnostics* out_diag);
```

</div>

No inline description found in header comments.

### `se_simulation_get_fixed_dt`

<div class="api-signature">

```c
extern f32 se_simulation_get_fixed_dt(se_simulation_handle sim);
```

</div>

No inline description found in header comments.

### `se_simulation_get_tick`

<div class="api-signature">

```c
extern se_sim_tick se_simulation_get_tick(se_simulation_handle sim);
```

</div>

No inline description found in header comments.

### `se_simulation_json_load_file`

<div class="api-signature">

```c
extern b8 se_simulation_json_load_file(se_simulation_handle sim, const c8* path);
```

</div>

No inline description found in header comments.

### `se_simulation_json_save_file`

<div class="api-signature">

```c
extern b8 se_simulation_json_save_file(se_simulation_handle sim, const c8* path);
```

</div>

No inline description found in header comments.

### `se_simulation_register_system`

<div class="api-signature">

```c
extern b8 se_simulation_register_system(se_simulation_handle sim, se_sim_system_fn fn, void* user_data, i32 order);
```

</div>

No inline description found in header comments.

### `se_simulation_reset`

<div class="api-signature">

```c
extern void se_simulation_reset(se_simulation_handle sim);
```

</div>

No inline description found in header comments.

### `se_simulation_snapshot_free`

<div class="api-signature">

```c
extern void se_simulation_snapshot_free(void* data);
```

</div>

No inline description found in header comments.

### `se_simulation_snapshot_load_file`

<div class="api-signature">

```c
extern b8 se_simulation_snapshot_load_file(se_simulation_handle sim, const c8* path);
```

</div>

No inline description found in header comments.

### `se_simulation_snapshot_load_memory`

<div class="api-signature">

```c
extern b8 se_simulation_snapshot_load_memory(se_simulation_handle sim, const u8* data, sz size);
```

</div>

No inline description found in header comments.

### `se_simulation_snapshot_save_file`

<div class="api-signature">

```c
extern b8 se_simulation_snapshot_save_file(se_simulation_handle sim, const c8* path);
```

</div>

No inline description found in header comments.

### `se_simulation_snapshot_save_memory`

<div class="api-signature">

```c
extern b8 se_simulation_snapshot_save_memory(se_simulation_handle sim, u8** out_data, sz* out_size);
```

</div>

No inline description found in header comments.

### `se_simulation_step`

<div class="api-signature">

```c
extern b8 se_simulation_step(se_simulation_handle sim, u32 tick_count);
```

</div>

No inline description found in header comments.

### `se_simulation_to_json`

<div class="api-signature">

```c
extern s_json* se_simulation_to_json(se_simulation_handle sim);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

### `s_json`

<div class="api-signature">

```c
typedef struct s_json s_json;
```

</div>

No inline description found in header comments.

### `se_entity`

<div class="api-signature">

```c
typedef struct se_entity { se_entity_id id; } se_entity;
```

</div>

No inline description found in header comments.

### `se_entity_id`

<div class="api-signature">

```c
typedef u64 se_entity_id;
```

</div>

No inline description found in header comments.

### `se_sim_component_desc`

<div class="api-signature">

```c
typedef struct { se_sim_component_type type; u32 size; u32 alignment; c8 name[SE_MAX_NAME_LENGTH]; } se_sim_component_desc;
```

</div>

No inline description found in header comments.

### `se_sim_component_type`

<div class="api-signature">

```c
typedef u32 se_sim_component_type;
```

</div>

No inline description found in header comments.

### `se_sim_entity_iterator_fn`

<div class="api-signature">

```c
typedef void (*se_sim_entity_iterator_fn)(se_simulation_handle sim, se_entity_id entity, void* user_data);
```

</div>

No inline description found in header comments.

### `se_sim_event_desc`

<div class="api-signature">

```c
typedef struct { se_sim_event_type type; u32 payload_size; c8 name[SE_MAX_NAME_LENGTH]; } se_sim_event_desc;
```

</div>

No inline description found in header comments.

### `se_sim_event_type`

<div class="api-signature">

```c
typedef u32 se_sim_event_type;
```

</div>

No inline description found in header comments.

### `se_sim_system_fn`

<div class="api-signature">

```c
typedef void (*se_sim_system_fn)(se_simulation_handle sim, se_sim_tick tick, void* user_data);
```

</div>

No inline description found in header comments.

### `se_sim_tick`

<div class="api-signature">

```c
typedef u64 se_sim_tick;
```

</div>

No inline description found in header comments.

### `se_simulation_config`

<div class="api-signature">

```c
typedef struct { u32 max_entities; u32 max_components_per_entity; u32 max_events; u32 max_event_payload_bytes; f32 fixed_dt; } se_simulation_config;
```

</div>

No inline description found in header comments.

### `se_simulation_diagnostics`

<div class="api-signature">

```c
typedef struct { u32 entity_capacity; u32 entity_count; u32 component_registry_count; u32 event_registry_count; u32 pending_event_count; u32 ready_event_count; u32 max_events; u32 used_event_payload_bytes; u32 max_event_payload_bytes; se_sim_tick current_tick; f32 fixed_dt; } se_simulation_diagnostics;
```

</div>

No inline description found in header comments.
