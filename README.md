## Syphax Engine - 𐤒𐤐𐤎
Syphax is a C framework for interactive visuals, games, tools, and general real-time apps.

### Highlights
- Window and input handling
- 2D/3D scenes with instancing and physics 
- Audio
- UI layout and widgets
- Navigation 
- Debug

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
./build.sh 1_hello
```

Manual build:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSE_BACKEND_RENDER=gl -DSE_BACKEND_PLATFORM=desktop_glfw
cmake --build build -j
```

### Run Examples
```bash
./build.sh 1_hello
./bin/1_hello
```

Integration example:
```bash
./build.sh 99_game
./bin/99_game --autotest --seconds=6
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
	se_context *ctx = se_context_create();
	se_window_handle window = se_window_create("Syphax Hello", 1280, 720);
	se_text_handle *text_handle = se_text_handle_create(0);
	se_font_handle font = se_font_load(text_handle, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.08f, 0.08f, 0.1f, 1.0f));

	se_window_loop(window,
		se_render_clear();
		se_text_render(text_handle, font, "Hello Syphax", &s_vec2(0.0f, 0.0f), &s_vec2(1.0f, 1.0f), 0.03f);
	);

	se_text_handle_destroy(text_handle);
	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
```

### API Docs
- Public API headers: `include/se_*.h`
- Core entry points: `include/se.h`, `include/se_window.h`, `include/se_render.h`, `include/se_scene.h`, `include/se_ui.h`
- Resource layout details: `resources/README.md`

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
- `build/` and `bin/`: generated artifacts

### Submodules
- [Syphax](https://github.com/ougi-washi/syphax)
- [GLFW](https://github.com/glfw/glfw)
- [STB](https://github.com/nothings/stb)
- [miniaudio](https://github.com/mackron/miniaudio)

### License
MIT
