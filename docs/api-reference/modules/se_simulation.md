<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_simulation.h

Source header: `include/se_simulation.h`

## Overview

Deterministic simulation components, systems, events, and snapshots.

This page is generated from `include/se_simulation.h` and is deterministic.

## Read the Playbook

- [Deep dive Playbook](../../playbooks/se-simulation.md)

## Functions

No functions found in this header.

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

### `se_sim_component_type`

<div class="api-signature">

```c
typedef u32 se_sim_component_type;
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
