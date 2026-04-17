---
title: Examples Index
summary: Reference pages for documented runnable example targets.
prerequisites:
  - Build toolchain available.
---

# Examples Index

Use this section to find build/run commands, controls, and small edits for documented targets.

## Default examples

- [hello_text](default/hello_text.md)
- [input_actions](default/input_actions.md)
- [scene2d_click](default/scene2d_click.md)
- [scene2d_instancing](default/scene2d_instancing.md)
- [scene2d_json](default/scene2d_json.md)
- [scene3d_orbit](default/scene3d_orbit.md)
- [scene3d_json](default/scene3d_json.md)
- [physics2d_playground](default/physics2d_playground.md)
- [physics3d_playground](default/physics3d_playground.md)
- [ui_basics](default/ui_basics.md)
- [audio_basics](default/audio_basics.md)
- [model_viewer](default/model_viewer.md)
- [sdf](default/sdf.md)

## Advanced examples

- [array_handles](advanced/array_handles.md)
- [civilization](advanced/civilization.md)
- [context_lifecycle](advanced/context_lifecycle.md)
- [debug_tools](advanced/debug_tools.md)
- [editor_sdf](advanced/editor_sdf.md)
- [gltf_roundtrip](advanced/gltf_roundtrip.md)
- [multi_window](advanced/multi_window.md)
- [navigation_grid](advanced/navigation_grid.md)
- [noise_texture](advanced/noise_texture.md)
- [rts_integration](advanced/rts_integration.md)
- [sdf_path_steps](advanced/sdf_path_steps.md)
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
- Add a `.png` when a live capture exists.
- Keep the `.svg` placeholder file for every example page.

## Generate captures locally

Run this locally to avoid workflow cost and review images before commit:

```bash
./scripts/docs/capture_examples.sh --build
```

Useful options:

- `--track default` or `--track advanced`
- `--frame 30` for later capture timing
- `--show-window` to capture with visible windows instead of hidden terminal mode
