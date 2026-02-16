---
title: Add Sound
summary: Play clips and control music bus volume.
prerequisites:
  - Start Here pages complete.
---

# Add Sound


## When to use this

Use this concept when implementing `add-sound` behavior in a runtime target.

## Minimal working snippet

```c
se_audio_engine* audio = se_audio_init(NULL);
se_audio_clip* clip = se_audio_clip_load(audio, SE_RESOURCE_PUBLIC("audio/chime.wav"));
se_audio_update(audio);
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown="1">

## Next

1. Next: [Your First 3D Object](your-first-3d-object.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
