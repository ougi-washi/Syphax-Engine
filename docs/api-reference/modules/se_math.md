<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_math.h

Source header: `include/se_math.h`

## Overview

Geometry utility structs and math helpers used by engine modules.

This page is generated from `include/se_math.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/utilities.md)

## Functions

### `se_box_2d_intersects`

<div class="api-signature">

```c
extern b8 se_box_2d_intersects(const se_box_2d *a, const se_box_2d *b);
```

</div>

No inline description found in header comments.

### `se_box_2d_is_inside`

<div class="api-signature">

```c
extern b8 se_box_2d_is_inside(const se_box_2d *a, const s_vec2 *p);
```

</div>

No inline description found in header comments.

### `se_box_2d_make`

<div class="api-signature">

```c
extern void se_box_2d_make(se_box_2d *out_box, const s_mat3 *transform);
```

</div>

No inline description found in header comments.

### `se_box_3d_intersects`

<div class="api-signature">

```c
extern b8 se_box_3d_intersects(const se_box_3d *a, const se_box_3d *b);
```

</div>

No inline description found in header comments.

### `se_circle_intersects`

<div class="api-signature">

```c
extern b8 se_circle_intersects(const se_circle *a, const se_circle *b);
```

</div>

No inline description found in header comments.

### `se_sphere_intersects`

<div class="api-signature">

```c
extern b8 se_sphere_intersects(const se_sphere *a, const se_sphere *b);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

### `se_box_2d`

<div class="api-signature">

```c
typedef struct { s_vec2 min, max; } se_box_2d;
```

</div>

2D

### `se_box_3d`

<div class="api-signature">

```c
typedef struct { s_vec3 min, max; } se_box_3d;
```

</div>

3D

### `se_circle`

<div class="api-signature">

```c
typedef struct { s_vec2 position; f32 radius; } se_circle;
```

</div>

No inline description found in header comments.

### `se_sphere`

<div class="api-signature">

```c
typedef struct { s_vec3 position; f32 radius; } se_sphere;
```

</div>

No inline description found in header comments.
