## Syphax Engine - 𐤒𐤐𐤎
Syphax is a C framework for interactive visuals, games, tools, and general real-time apps.

### Highlights
- Window/runtime loop helpers with diagnostics and synthetic input injection
- Input layer with low-level polling and high-level action/context mapping
- 2D/3D scene APIs with instancing, picking, and debug markers
- Camera helpers for projection, orbit/pan/dolly, and screen/world conversion
- UI system with layout rules, widgets, clipping, and interaction dispatch
- Generic navigation/pathfinding utilities (`se_navigation`)
- Unified debug/tracing/stat overlays (`se_debug`)

### Requirements
- CMake 3.22+
- OpenGL 3.3
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
./build.sh 1_hello
```

Manual build:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j
```

### Run Examples
```bash
./build.sh 99_game
./bin/99_game
```

New focused examples:
- `17_navigation`
- `18_input_actions`
- `19_ui_widgets`
- `20_debug_tools`
- `21_scene_qol`

### Minimal Usage
```c
#include "se_window.h"
#include "se_render.h"
#include "se_text.h"

int main(void) {
	se_context* ctx = se_context_create();
	se_window_handle window = se_window_create("Syphax Hello", 1280, 720);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(ctx);
		return 1;
	}

	se_text_handle* text = se_text_handle_create(0);
	se_font_handle font = se_font_load(text, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.08f, 0.08f, 0.1f, 1.0f));

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_clear();
		se_text_render(text, font, "Hello Syphax", &s_vec2(0.0f, 0.0f), &s_vec2(1.0f, 1.0f), 0.03f);
		se_window_render_screen(window);
	}

	se_text_handle_destroy(text);
	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
```

### API Docs
- Module overview and API map: `docs/MODULE_GUIDE.md`
- Migration notes: `docs/MIGRATION.md`

### Resource Scopes
- `SE_RESOURCE_INTERNAL("...")`: engine implementation assets
- `SE_RESOURCE_PUBLIC("...")`: reusable assets for framework users
- `SE_RESOURCE_EXAMPLE("...")`: example/demo-only assets

### Project Layout
- `include/`: public headers
- `src/`: framework modules (`se_*.c`)
- `examples/`: runnable samples (output in `bin/`)
- `resources/`: `internal/`, `public/`, `examples/`
- `lib/`: vendored dependencies

### Submodules
- [Syphax](https://github.com/ougi-washi/syphax)
- [GLFW](https://github.com/glfw/glfw)
- [STB](https://github.com/nothings/stb)
- [miniaudio](https://github.com/mackron/miniaudio)

### License
MIT
