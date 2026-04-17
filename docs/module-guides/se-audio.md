---
title: se_audio
summary: Audio engine, clip, stream, capture, and bus control APIs.
prerequisites:
  - Public header include path available.
---

# se_audio

## When to use this

Use `se_audio` when a runtime needs music, sound effects, streamed playback, capture input, or bus-level volume control.

## Minimal working snippet

```c
#include "se_audio.h"

se_audio_engine* audio = se_audio_init(NULL);
se_audio_update(audio);
```

## Step-by-step explanation

1. Initialize one audio engine, then load clips or open streams from packaged resource paths.
1. Update the engine each frame so playback, meters, and capture state stay current.
1. Control mix behavior through buses and stop or close owned resources before shutting the engine down.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_audio.md).
1. Continue with the [Audio path page](../path/audio.md).

</div>

## Common mistakes

- Forgetting to call `se_audio_update(...)` during the main loop.
- Treating streams and clips as interchangeable when one is intended for long playback and the other for short reusable sounds.
- Shutting down the audio engine while clips, streams, or capture flows are still actively referenced by gameplay or UI code.

## Related pages

- [Path: Audio](../path/audio.md)
- [Audio overview](../audio/index.md)
- [Example: audio_basics](../examples/default/audio_basics.md)
