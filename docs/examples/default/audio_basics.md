---
title: audio_basics
summary: Reference for audio_basics example.
prerequisites:
  - Build toolchain and resources available.
---

# audio_basics

<img src="../../../assets/img/examples/default/audio_basics.png" alt="audio_basics preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/audio_basics.svg';">


## Goal

Play clips and stream looped audio with bus-level volume control.


## Learning path

- This example corresponds to [Audio path page](../../path/audio.md) Step 3.

## Controls

- Space: play chime
- M: mute or unmute loop
- Up/Down: master volume
- Esc: quit

## Build command

```bash
./build.sh audio_basics
```

## Run command

```bash
./bin/audio_basics
```

## Internal flow

- `se_audio_init` creates the mixer, then clip/stream handles are loaded once at startup.
- Per-frame `se_audio_update` advances stream state while input edges trigger clip play/mute toggles.
- Bus and stream volume updates (`se_audio_bus_set_volume`, `se_audio_stream_set_volume`) apply in the mixer path.

## Related API links

- [Path: Audio](../../path/audio.md)
- [Module guide: se_audio](../../module-guides/se-audio.md)
- [API: se_audio.h](../../api-reference/modules/se_audio.md)
