---
title: se_curve
summary: Typed curve-key arrays and interpolation helpers for scalar and vector values.
prerequisites:
  - Public header include path available.
---

# se_curve

## When to use this

Use `se_curve` when a value should change over normalized time with explicit interpolation instead of ad-hoc math scattered across gameplay, tools, or VFX code.

## Minimal working snippet

```c
#include "se_curve.h"

se_curve_float_keys keys;
se_curve_float_init(&keys);
se_curve_float_add_key(&keys, 0.0f, 0.0f);
se_curve_float_eval(&keys, SE_CURVE_SMOOTH, t, &value);
```

## Step-by-step explanation

1. Initialize the typed key array that matches the value you want to animate or interpolate.
1. Add keys in time order or sort them before evaluation if the authoring flow does not guarantee order.
1. Evaluate the curve with the right mode each frame or each effect tick, then clear or reset the array when reauthoring it.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_curve.md).
1. Continue with the [Curve path page](../path/curve.md).

</div>

## Common mistakes

- Forgetting to initialize the key array before adding keys.
- Mixing world units, normalized time, and frame counts in the same curve without naming that decision clearly.
- Reimplementing curve math inside VFX or UI code when the shared curve helpers already fit.

## Related pages

- [Path: Curve](../path/curve.md)
- [VFX overview](../vfx/index.md)
- [Module guide: se_vfx](se-vfx.md)
- [Example: vfx_emitters](../examples/advanced/vfx_emitters.md)
