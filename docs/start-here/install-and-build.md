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

```c
git submodule update --init --recursive
./build.sh
./build.sh hello_text
./build.sh -render=opengl
```

## Step-by-step explanation

1. Initialize submodules.
1. Run full build once.
1. Use single-target build for iteration.

<div class="next-block" markdown="1">

## Next

1. Next: [Project layout](project-layout.md).
1. Run [First run checklist](first-run-checklist.md).

</div>

## Common mistakes

- Skipping submodule init.
- Using an invalid target name.

## Related pages

- [Troubleshooting](../troubleshooting/docs-build.md)
- [First run checklist](first-run-checklist.md)
- [README](../index.md)
- [Next path page](../path/window.md)
