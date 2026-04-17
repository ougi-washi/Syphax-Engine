---
title: Install And Build
summary: Build all targets and individual example targets.
prerequisites:
  - Git with submodule support.
  - CMake 3.22+.
---

# Install And Build


## When to use this

Use when setting up a new machine or CI runner.

## Minimal working snippet

```bash
git submodule update --init --recursive
./build.sh
./build.sh hello_text
./build.sh -render=gles
```

## Step-by-step explanation

1. Initialize submodules.
1. Run `./build.sh` once to configure CMake and build all targets.
1. Use `./build.sh <target>` for fast iteration on one example or tool.
1. Use backend/platform flags only when validating a specific runtime path such as `-render=gles` or `-platform=terminal`.
1. For Android packaging, use `scripts/android/build_native_activity.sh`; `ios` and `web` are planned but not implemented yet.

<div class="next-block" markdown="1">

## Next

1. Next: [Project layout](project-layout.md).
1. Run [First run checklist](first-run-checklist.md).

</div>

## Common mistakes

- Skipping submodule init.
- Using an invalid target name.
- Assuming `./build.sh -platform=android` is the Android build path.

## Related pages

- [Troubleshooting](../troubleshooting/docs-build.md)
- [First run checklist](first-run-checklist.md)
- [Project layout](project-layout.md)
- [Next path page](../path/window.md)
