---
title: First Run Checklist
summary: Sanity checks after setup and before deeper work.
prerequisites:
  - Build succeeded locally.
---

# First Run Checklist


## When to use this

Use after setup or after large environment changes.

## Minimal working snippet

```c
./build.sh hello_text
./bin/hello_text
./build.sh input_actions
./bin/input_actions
```

## Step-by-step explanation

1. Run one visual and one interaction target.
1. Confirm exit-key behavior.
1. Capture command + error on failure.

<div class="next-block" markdown="1">

## Next

1. Next: [Path](../path/index.md).
1. Start with [Colors and clear](../path/colors-and-clear.md).

</div>

## Common mistakes

- Assuming build success implies runtime success.
- Reporting issues without exact reproduction commands.

## Related pages

- [Install and build](install-and-build.md)
- [Examples index](../examples/index.md)
- [Troubleshooting](../troubleshooting/docs-build.md)
