---
title: sdf_playground
summary: Walkthrough page for sdf_playground.
prerequisites:
  - Build toolchain and resources available.
---

# sdf_playground

> Scope: advanced

![sdf_playground preview](../../assets/img/examples/advanced/sdf_playground.svg)

*Caption: representative preview panel for `sdf_playground`.*

## Goal

Build SDF scene graph primitives and render with stylized shading.


## Learning path

- This example corresponds to [se_sdf Playbook](../../playbooks/se-sdf.md) Step 4.
- Next: apply one change from the linked Playbook step and rerun this target.
## Controls

- Camera controls from controller preset
- Esc: quit

## Build command

```bash
./build.sh sdf_playground
```

## Run command

```bash
./bin/sdf_playground
```

## Edits to try

1. Add a second primitive type.
1. Modify stylized band levels.
1. Adjust camera align padding.

## Related API links

- [Playbook: se_sdf Playbook](../../playbooks/se-sdf.md)
- [API: se_sdf.h](../../api-reference/modules/se_sdf.md)
- [Module guide: se_camera](../../module-guides/se-camera.md)
- [Glossary: scene](../../glossary/terms.md#scene)
