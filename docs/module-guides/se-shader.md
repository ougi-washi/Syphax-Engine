---
title: se_shader
summary: Shader load, bind, and uniform update APIs for custom render paths.
prerequisites:
  - Public header include path available.
---

# se_shader

## When to use this

Use `se_shader` when a feature needs a custom shader handle or per-frame uniform updates that higher-level modules do not hide for you.

## Minimal working snippet

```c
#include "se_shader.h"

se_shader_set_float(shader, "u_time", time_s);
```

## Step-by-step explanation

1. Load or acquire the shader handle through the module that owns it, such as a model, object, render buffer, or emitter.
1. Push uniform values from app state, camera state, simulation state, or effect parameters before the relevant draw call.
1. Keep ownership clear: the shader might belong to a scene object, model, VFX emitter, or post-process pass rather than to loose global state.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_shader.md).
1. Continue with the [Shader path page](../path/shader.md).

</div>

## Common mistakes

- Updating uniforms on an invalid or already-destroyed shader handle.
- Hiding shader ownership so teardown order becomes unclear.
- Reaching for raw shader APIs when a higher-level module already exposes the needed parameter directly.

## Related pages

- [Path: Shader](../path/shader.md)
- [Visual building blocks overview](../visual-building-blocks/index.md)
- [Module guide: se_texture](se-texture.md)
- [Module guide: se_vfx](se-vfx.md)
- [Example: scene2d_click](../examples/default/scene2d_click.md)
