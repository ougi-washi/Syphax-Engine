## Syphax Engine - 𐤒𐤐𐤎
Simple, fast, and lightweight 2D/3D engine in C.

### Features
* Handle shaders and their uniforms
* (WIP) Handle SDF objects and operations

### Building
CMake 3.22 is required.
Make sure to:
git clone with `--recurse-submodules` or run `git submodule update --init --recursive`

Simply run:
```bash
build.sh
```
or manually:
```bash
mkdir build
cd build
cmake ..
make
```

### Usage

Examples are located in the `examples` directory.
To run an example, simply run:
```bash
./bin/example_name
```

#### Minimal Hello World
```c
#include "se_window.h"
#include "se_render.h"

int main() {
	se_render_handle *render = se_render_handle_create(NULL);
	se_window *window = se_window_create(render, "Syphax Hello", 1280, 720);
	se_render_set_background_color(s_vec4(0.08f, 0.08f, 0.1f, 1.0f));

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_clear();
		se_window_render_screen(window);
	}

	se_window_destroy(window);
	se_render_handle_cleanup(render);
	return 0;
}
```

Notes:
- `se_window_tick(...)` is a single-window convenience: it calls `se_window_update(...)` then `se_window_poll_events()`.
- `se_render_command(...)` is a grouping macro only; it does not manage state or perform implicit setup.

### Submodules
These submodules are added under `lib/`
* [GLFW](https://github.com/glfw/glfw)
* [STB](https://github.com/nothings/stb)
* [miniaudio](https://github.com/mackron/miniaudio)

### License
MIT License
