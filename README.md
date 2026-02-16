## Syphax Engine - 𐤒𐤐𐤎
Simple, fast, lightweight framework for interactive visuals, games, and tools in C.

### Highlights
- Window + input handling
- 2D/3D scenes with instancing and physics
- Audio, UI, navigation, simulation, debug, and VFX
- Public APIs in `include/se_*.h`

## [Documentation here](https://syphax-engine.github.io/syphax-engine/)

### Requirements
- C11 compiler
- CMake 3.22+
- OpenGL 3.3 (default `-render=gl`) or GLES2/EGL (`-render=gles`)
- GLFW development packages

### Initial Setup
```bash
git submodule update --init --recursive
```

### Build
```bash
./build.sh
```

Build one target:
```bash
./build.sh hello_text
```

Manual build:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSE_BACKEND_RENDER=gl -DSE_BACKEND_PLATFORM=desktop_glfw
cmake --build build -j
```

### Docs Checks
```bash
./scripts/docs/generate_api_reference.sh
./scripts/docs/check_nav_consistency.sh
./scripts/docs/check_links.sh
./scripts/docs/check_content_quality.sh
./scripts/docs/check_playbook_coverage.sh
./scripts/docs/verify_snippets.sh
mkdocs build --strict --config-file mkdocs.yml
./scripts/docs/check_site_size.sh
```

### Minimal Usage
```c
#include "se_graphics.h"
#include "se_text.h"
#include "se_window.h"

int main(void) {
	se_context* ctx = se_context_create();
	se_window_handle window = se_window_create("Hello", 1280, 720);
	if (window == S_HANDLE_NULL) {
		return 1;
	}

	se_text_handle* text = se_text_handle_create(0);
	se_font_handle font = se_font_load(text, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_render_clear();
		se_text_render(text, font, "Hello Syphax", &s_vec2(0.0f, 0.0f), &s_vec2(1.0f, 1.0f), 0.03f);
		se_window_end_frame(window);
	}

	se_text_handle_destroy(text);
	se_context_destroy(ctx); // canonical final teardown
	return 0;
}
```

### Error Handling
- Check `se_get_last_error()` + `se_result_str(...)` when creation calls fail.
- `se_window_begin_frame` / `se_window_end_frame` no-op and set `SE_RESULT_INVALID_ARGUMENT` for invalid handles.

### Resource Scopes
- `SE_RESOURCE_INTERNAL("...")`: engine implementation assets
- `SE_RESOURCE_PUBLIC("...")`: reusable assets for framework users
- `SE_RESOURCE_EXAMPLE("...")`: example/demo-only assets

### Project Layout
- `include/`: public headers
- `src/`: engine modules
- `examples/`: default examples
- `examples/advanced/`: advanced examples
- `resources/`: `internal/`, `public/`, `examples/`
- `lib/`: vendored dependencies
- `build/` and `bin/`: generated artifacts

### License
MIT
