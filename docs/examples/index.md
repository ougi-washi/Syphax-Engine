---
title: Examples Index
summary: One page per runnable example target.
prerequisites:
  - Build toolchain available.
---

# Examples Index

Use this section to find build/run commands, controls, and small edits for each target.

## Default examples

- [hello_text](default/hello_text.md)
- [input_actions](default/input_actions.md)
- [scene2d_click](default/scene2d_click.md)
- [scene2d_instancing](default/scene2d_instancing.md)
- [scene3d_orbit](default/scene3d_orbit.md)
- [physics2d_playground](default/physics2d_playground.md)
- [physics3d_playground](default/physics3d_playground.md)
- [ui_basics](default/ui_basics.md)
- [audio_basics](default/audio_basics.md)
- [model_viewer](default/model_viewer.md)

## Advanced examples

- [array_handles](advanced/array_handles.md)
- [context_lifecycle](advanced/context_lifecycle.md)
- [debug_tools](advanced/debug_tools.md)
- [gltf_roundtrip](advanced/gltf_roundtrip.md)
- [multi_window](advanced/multi_window.md)
- [navigation_grid](advanced/navigation_grid.md)
- [rts_integration](advanced/rts_integration.md)
- [sdf_playground](advanced/sdf_playground.md)
- [simulation_intro](advanced/simulation_intro.md)
- [simulation_advanced](advanced/simulation_advanced.md)
- [ui_showcase](advanced/ui_showcase.md)
- [vfx_emitters](advanced/vfx_emitters.md)

## Screenshot naming convention

Generated runtime captures are stored under:

`docs/assets/img/examples/<track>/<target>.png`

Rules:

- `<track>` is `default` or `advanced`.
- `<target>` matches the executable name exactly.
- Use lowercase and underscores.
- Keep the existing `.svg` placeholder file as a fallback for each example page.

## Generate captures locally

Run this locally to avoid workflow cost and review images before commit:

```bash
./scripts/docs/capture_examples.sh --build
```

Useful options:

- `--track default` or `--track advanced`
- `--frame 30` for later capture timing
- `--show-window` to capture with visible windows instead of hidden terminal mode
