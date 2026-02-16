<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_navigation.h

Source header: `include/se_navigation.h`

## Overview

Grid navigation, pathfinding, smoothing, and occupancy controls.

This page is generated from `include/se_navigation.h` and is deterministic.

## Read the Playbook

- [Deep dive Playbook](../../playbooks/se-navigation.md)

## Functions

### `se_navigation_cell_to_world`

<div class="api-signature">

```c
extern b8 se_navigation_cell_to_world(const se_navigation_grid* grid, const se_navigation_cell cell, s_vec2* out_world_center);
```

</div>

No inline description found in header comments.

### `se_navigation_find_path`

<div class="api-signature">

```c
extern b8 se_navigation_find_path(const se_navigation_grid* grid, const se_navigation_cell start, const se_navigation_cell goal, const b8 allow_diagonal, se_navigation_debug_hook debug_hook, void* debug_user_data, se_navigation_path* out_path);
```

</div>

No inline description found in header comments.

### `se_navigation_find_path_simple`

<div class="api-signature">

```c
extern b8 se_navigation_find_path_simple(const se_navigation_grid* grid, const se_navigation_cell start, const se_navigation_cell goal, const b8 allow_diagonal, se_navigation_path* out_path);
```

</div>

No inline description found in header comments.

### `se_navigation_get_neighbors`

<div class="api-signature">

```c
extern sz se_navigation_get_neighbors(const se_navigation_grid* grid, const se_navigation_cell cell, const b8 allow_diagonal, se_navigation_cell out_neighbors[8]);
```

</div>

No inline description found in header comments.

### `se_navigation_grid_clear`

<div class="api-signature">

```c
extern void se_navigation_grid_clear(se_navigation_grid* grid, const b8 blocked);
```

</div>

No inline description found in header comments.

### `se_navigation_grid_create`

<div class="api-signature">

```c
extern b8 se_navigation_grid_create(se_navigation_grid* out_grid, const i32 width, const i32 height, const f32 cell_size, const s_vec2* origin);
```

</div>

No inline description found in header comments.

### `se_navigation_grid_destroy`

<div class="api-signature">

```c
extern void se_navigation_grid_destroy(se_navigation_grid* grid);
```

</div>

No inline description found in header comments.

### `se_navigation_grid_is_blocked`

<div class="api-signature">

```c
extern b8 se_navigation_grid_is_blocked(const se_navigation_grid* grid, const se_navigation_cell cell);
```

</div>

No inline description found in header comments.

### `se_navigation_grid_is_valid_cell`

<div class="api-signature">

```c
extern b8 se_navigation_grid_is_valid_cell(const se_navigation_grid* grid, const se_navigation_cell cell);
```

</div>

No inline description found in header comments.

### `se_navigation_grid_set_blocked`

<div class="api-signature">

```c
extern b8 se_navigation_grid_set_blocked(se_navigation_grid* grid, const se_navigation_cell cell, const b8 blocked);
```

</div>

No inline description found in header comments.

### `se_navigation_grid_set_blocked_circle`

<div class="api-signature">

```c
extern void se_navigation_grid_set_blocked_circle(se_navigation_grid* grid, const se_navigation_cell center, const i32 radius_cells, const b8 blocked);
```

</div>

No inline description found in header comments.

### `se_navigation_grid_set_blocked_rect`

<div class="api-signature">

```c
extern void se_navigation_grid_set_blocked_rect(se_navigation_grid* grid, const se_navigation_cell min_cell, const se_navigation_cell max_cell, const b8 blocked);
```

</div>

No inline description found in header comments.

### `se_navigation_grid_set_dynamic_obstacle`

<div class="api-signature">

```c
extern void se_navigation_grid_set_dynamic_obstacle(se_navigation_grid* grid, const se_navigation_cell cell, const b8 blocked);
```

</div>

No inline description found in header comments.

### `se_navigation_grid_set_dynamic_obstacle_rect`

<div class="api-signature">

```c
extern void se_navigation_grid_set_dynamic_obstacle_rect(se_navigation_grid* grid, const se_navigation_cell min_cell, const se_navigation_cell max_cell, const b8 blocked);
```

</div>

No inline description found in header comments.

### `se_navigation_path_clear`

<div class="api-signature">

```c
extern void se_navigation_path_clear(se_navigation_path* path);
```

</div>

No inline description found in header comments.

### `se_navigation_path_init`

<div class="api-signature">

```c
extern b8 se_navigation_path_init(se_navigation_path* path, const sz reserve_count);
```

</div>

No inline description found in header comments.

### `se_navigation_path_reset`

<div class="api-signature">

```c
extern void se_navigation_path_reset(se_navigation_path* path);
```

</div>

No inline description found in header comments.

### `se_navigation_raycast_cells`

<div class="api-signature">

```c
extern b8 se_navigation_raycast_cells(const se_navigation_grid* grid, const se_navigation_cell start, const se_navigation_cell end, se_navigation_path* out_cells);
```

</div>

No inline description found in header comments.

### `se_navigation_smooth_path`

<div class="api-signature">

```c
extern void se_navigation_smooth_path(const se_navigation_grid* grid, se_navigation_path* path);
```

</div>

No inline description found in header comments.

### `se_navigation_world_to_cell`

<div class="api-signature">

```c
extern b8 se_navigation_world_to_cell(const se_navigation_grid* grid, const s_vec2* world, se_navigation_cell* out_cell);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

### `se_navigation_cell`

<div class="api-signature">

```c
typedef struct { i32 x; i32 y; } se_navigation_cell;
```

</div>

No inline description found in header comments.

### `se_navigation_debug_hook`

<div class="api-signature">

```c
typedef void (*se_navigation_debug_hook)( const se_navigation_grid* grid, const se_navigation_cell* open_cells, sz open_count, const se_navigation_cell* closed_cells, sz closed_count, void* user_data);
```

</div>

No inline description found in header comments.

### `se_navigation_grid`

<div class="api-signature">

```c
typedef struct { i32 width; i32 height; f32 cell_size; s_vec2 origin; se_navigation_occupancy occupancy; } se_navigation_grid;
```

</div>

No inline description found in header comments.

### `se_navigation_path`

<div class="api-signature">

```c
typedef struct { se_navigation_cells cells; } se_navigation_path;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_navigation_cell, se_navigation_cells);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(u8, se_navigation_occupancy);
```

</div>

No inline description found in header comments.
