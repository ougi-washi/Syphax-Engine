---
title: Error Handling
summary: Use result codes and readable error strings.
prerequisites:
  - Core setup flow understood.
---

# Error Handling


## When to use this

Use for all creation and load operations where failure is possible.

## Minimal working snippet

```c
if (window == S_HANDLE_NULL) {
	printf("error: %s\n", se_result_str(se_get_last_error()));
	se_context_destroy(ctx);
	return 1;
}
```

## Step-by-step explanation

1. Check return values immediately.
1. Print result string with context.
1. Perform explicit cleanup before return.

<div class="next-block" markdown="1">

## Next

1. Next: [Resource paths](resource-paths.md).
1. Inspect failure paths in [audio_basics](../examples/default/audio_basics.md).

</div>

## Common mistakes

- Ignoring failed returns and continuing.
- Logging without `se_result_str`.

## Related pages

- [First run checklist](first-run-checklist.md)
- [Troubleshooting](../troubleshooting/docs-build.md)
- [API: se_defines.h](../api-reference/modules/se_defines.md)
- [Next path page](../path/utilities.md)
