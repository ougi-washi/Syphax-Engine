<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_ext.h

Source header: `include/se_ext.h`

## Overview

Optional extension capability checks.

This page is generated from `include/se_ext.h` and is deterministic.

## Read the Playbook

- [Deep dive Playbook](../../playbooks/se-math-defines-ext-quad.md)

## Functions

### `se_ext_feature_name`

<div class="api-signature">

```c
extern const char *se_ext_feature_name(const se_ext_feature feature);
```

</div>

No inline description found in header comments.

### `se_ext_is_supported`

<div class="api-signature">

```c
extern b8 se_ext_is_supported(const se_ext_feature feature);
```

</div>

No inline description found in header comments.

### `se_ext_require`

<div class="api-signature">

```c
extern b8 se_ext_require(const se_ext_feature feature);
```

</div>

No inline description found in header comments.

## Enums

### `se_ext_feature`

<div class="api-signature">

```c
typedef enum { SE_EXT_FEATURE_INSTANCING = 1 << 0, SE_EXT_FEATURE_MULTI_RENDER_TARGET = 1 << 1, SE_EXT_FEATURE_FLOAT_RENDER_TARGET = 1 << 2, SE_EXT_FEATURE_COMPUTE = 1 << 3 } se_ext_feature;
```

</div>

No inline description found in header comments.

## Typedefs

No typedefs found in this header.
