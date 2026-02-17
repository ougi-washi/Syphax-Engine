---
title: audio_basics
summary: Walkthrough page for audio_basics.
prerequisites:
  - Build toolchain and resources available.
---

# audio_basics

<img src="../../../assets/img/examples/default/audio_basics.png" alt="audio_basics preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/audio_basics.svg';">

*Caption: live runtime capture if available; falls back to placeholder preview card.*

## Goal

Play clips and stream looped audio with bus-level volume control.


## Learning path

- This example corresponds to [Audio path page](../../path/audio.md) Step 3.
- Next: apply one change from the linked path step and rerun this target.
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

## Edits to try

1. Swap to another clip.
1. Change default loop volume.
1. Map volume control to alternate keys.

## Related API links

- [Path: Audio](../../path/audio.md)
- [Module guide: se_audio](../../module-guides/se-audio.md)
- [API: se_audio.h](../../api-reference/modules/se_audio.md)
