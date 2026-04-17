## Syphax Engine - 𐤒𐤐𐤎
<p align="center">
  <img src="docs/assets/img/syphax-engine-icon.png" alt="Syphax Engine icon" width="96">
</p>

Simple, fast C framework for interactive visuals, games, and tools.

[Documentation](https://ougi-washi.github.io/Syphax-Engine/)

### Highlights
- Small public C API in `include/`
- Window, input, text, audio, UI, and graphics modules
- 2D/3D scenes, cameras, models, shaders, textures, and framebuffers
- Physics, navigation, simulation, debug, VFX, and SDF tools
- Runnable examples in `examples/` and `examples/advanced/`

### Requirements
- C11 compiler
- CMake 3.22+
- OpenGL 3.3 with GLFW by default
- GLES is available with `./build.sh -render=gles`

### Quick Start
```bash
git submodule update --init --recursive
./build.sh
./build.sh hello_text
```

Useful variants:
```bash
./build.sh -render=gles
./build.sh -platform=terminal rts_integration
```

### Mobile
- Android builds use `scripts/android/build_native_activity.sh`
- `ios` and `web` platform backends are planned, not implemented yet

### Minimal Example
```c
#include "se_graphics.h"
#include "se_text.h"
#include "se_window.h"

int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Hello", 1280, 720);
	if (window == S_HANDLE_NULL) {
		return 1;
	}

	se_font_handle font = se_font_load(SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_render_clear();
		se_text_draw(font, "Hello Syphax", &s_vec2(0.0f, 0.0f), &s_vec2(1.0f, 1.0f), 0.03f);
		se_window_end_frame(window);
	}

	se_context_destroy(context);
	return 0;
}
```

### Use In Your CMake Project
Add Syphax as a submodule:

```bash
git submodule add https://github.com/ougi-washi/Syphax-Engine.git external/syphax-engine
git submodule update --init --recursive
```

Then wire it into your top-level `CMakeLists.txt`:

```cmake
set(SE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SE_INSTALL OFF CACHE BOOL "" FORCE)
set(SE_BACKEND_RENDER gl CACHE STRING "" FORCE)
set(SE_BACKEND_PLATFORM desktop_glfw CACHE STRING "" FORCE)

add_subdirectory(external/syphax-engine)

add_executable(my_app src/main.c)
target_link_libraries(my_app PRIVATE syphax::engine)
```

### Project Layout
- `include/`: public headers
- `src/`: engine implementation
- `examples/`: beginner runnable samples
- `examples/advanced/`: larger integration examples
- `resources/`: internal, public, and example assets
- `docs/`: documentation source
- `lib/`: vendored dependencies
- `build/`, `bin/`, `site/`: generated artifacts

### License
MIT
