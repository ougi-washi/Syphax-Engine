## Syphax Engine - 𐤒𐤐𐤎
Simple, fast, and lightweight 2D/3D engine in C.

### Highlights
- 2D/3D scenes
- Render-to-texture and post-processing
- Window + input helpers
- Text and UI
- Audio playback/capture

### Requirements
- CMake 3.22+
- OpenGL 3.3
- GLFW development packages

Linux packages (Ubuntu): `libglfw3-dev libgl1-mesa-dev mesa-common-dev`

### Build
```bash
./build.sh
```

Manual build:
```bash
mkdir -p build
cd build
cmake ..
make -j
```

### Run examples
```bash
./build.sh 1_hello
./bin/1_hello
```

### Minimal usage
```c
#include "se_window.h"
#include "se_render.h"
#include "se_text.h"

int main() {
	se_render_handle *render = se_render_handle_create(NULL);
	se_window *window = se_window_create(render, "Syphax Hello", 1280, 720);
	se_text_handle *text_handle = se_text_handle_create(render, 0);
	se_font *font = se_font_load(text_handle, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.08f, 0.08f, 0.1f, 1.0f));

	se_window_loop(window,
		se_render_clear();
		se_text_render(text_handle, font, "Hello Syphax", &s_vec2(0.0f, 0.0f), &s_vec2(1.0f, 1.0f), 0.03f);
	);

	se_text_handle_destroy(text_handle);
	se_window_destroy(window);
	se_render_handle_destroy(render);
	return 0;
}
```

### Resource scopes
- `SE_RESOURCE_INTERNAL("...")`: engine-only implementation assets.
- `SE_RESOURCE_PUBLIC("...")`: reusable assets intended for library users.
- `SE_RESOURCE_EXAMPLE("...")`: sample/demo-only assets.
- Scope roots live under `resources/internal`, `resources/public`, and `resources/examples`.

### Project layout
- `src/` engine modules (`se_*.c` / `se_*.h`)
- `examples/` runnable samples (build to `bin/<name>`)
- `resources/` split by ownership (`internal/`, `public/`, `examples/`)
- `lib/` vendored dependencies

### Submodules
- [Syphax](https://github.com/ougi-washi/syphax)
- [GLFW](https://github.com/glfw/glfw)
- [STB](https://github.com/nothings/stb)
- [miniaudio](https://github.com/mackron/miniaudio)

### License
MIT License
