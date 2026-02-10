# Syphax-Engine Resources

This directory is split by ownership.

- `internal/`: engine-owned runtime assets used by core systems.
- `public/`: reusable assets exposed for library users and starter projects.
- `examples/`: sample-only assets and datasets used by `examples/*.c`.

Path helpers in `include/se_defines.h`:

- `SE_RESOURCE_INTERNAL("...")`
- `SE_RESOURCE_PUBLIC("...")`
- `SE_RESOURCE_EXAMPLE("...")`

Guidelines:

- Engine modules should only reference `internal/` assets.
- Example binaries can reference `public/` and `examples/` assets.
- Library users should treat `internal/` as unstable implementation detail.
- Shader-based examples should keep shaders in their own `resources/examples/<example_name>/` folder.
