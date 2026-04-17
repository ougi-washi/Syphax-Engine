---
title: array_handles
summary: Reference for array_handles example.
prerequisites:
  - Build toolchain and resources available.
---

# array_handles

> Scope: advanced

<img src="../../../assets/img/examples/advanced/array_handles.svg" alt="array_handles preview image">


## Goal

Reference operations for `s_array` and handle-based remove behavior.


## Learning path

- This example corresponds to the [Utilities path page](../../path/utilities.md) Step 2.

## Controls

- Non-interactive example. Inspect stdout diagnostics.

## Build command

```bash
./build.sh array_handles
```

## Run command

```bash
./bin/array_handles
```

## Internal flow

- `s_array_init` + `s_array_reserve` allocate typed storage for handle-based access.
- Removals are executed through `s_array_remove` using handles, not raw pointer arithmetic.
- Logged output after each mutation shows size/content evolution for safe container operations.

## Related API links

- [Source code: `examples/advanced/array_handles.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/advanced/array_handles.c)
- [Path: Utilities](../../path/utilities.md)
- [Glossary: handle](../../glossary/terms.md#handle)
- [API: se_defines.h](../../api-reference/modules/se_defines.md)
- [Module guide: se_math](../../module-guides/se-math.md)
- [Module guide: se_defines](../../module-guides/se-defines.md)
