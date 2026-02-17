<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_defines.h

Source header: `include/se_defines.h`

## Overview

Global constants, resource scope macros, and engine-wide result codes.

This page is generated from `include/se_defines.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/utilities.md)

## Functions

### `se_get_last_error`

<div class="api-signature">

```c
extern se_result se_get_last_error(void);
```

</div>

No inline description found in header comments.

### `se_paths_get_resource_root`

<div class="api-signature">

```c
extern const char* se_paths_get_resource_root(void);
```

</div>

No inline description found in header comments.

### `se_paths_resolve_resource_path`

<div class="api-signature">

```c
extern b8 se_paths_resolve_resource_path(char* out_path, sz out_path_size, const char* path);
```

</div>

No inline description found in header comments.

### `se_paths_set_resource_root`

<div class="api-signature">

```c
extern void se_paths_set_resource_root(const char* path);
```

</div>

No inline description found in header comments.

### `se_result_str`

<div class="api-signature">

```c
extern const char* se_result_str(se_result result);
```

</div>

No inline description found in header comments.

### `se_set_last_error`

<div class="api-signature">

```c
extern void se_set_last_error(se_result result);
```

</div>

No inline description found in header comments.

## Enums

### `se_result`

<div class="api-signature">

```c
typedef enum { SE_RESULT_OK = 0, SE_RESULT_INVALID_ARGUMENT, SE_RESULT_OUT_OF_MEMORY, SE_RESULT_NOT_FOUND, SE_RESULT_IO, SE_RESULT_BACKEND_FAILURE, SE_RESULT_UNSUPPORTED, SE_RESULT_CAPACITY_EXCEEDED } se_result;
```

</div>

No inline description found in header comments.

## Typedefs

No typedefs found in this header.
