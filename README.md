## Syphax Engine - 𐤒𐤐𐤎
Simple, fast, lightweight 2D/3D Engine in C.

### Highlights
- Window and input handling
- 2D/3D scenes with instancing and physics 
- Audio
- UI layout and widgets
- Navigation 
- Debug
- Simulations/Events 
- VFX (offscreen-first, emitter/particle based)
- Reusable curves/interpolation (`se_curve`)

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
- `6_ui` (helper-first UI showcase)
- `20_debug_tools`
- `22_simulation`
- `24_vfx`

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
- Core entry points: `include/se.h`, `include/se_window.h`, `include/se_render.h`, `include/se_scene.h`, `include/se_ui.h`, `include/se_simulation.h`, `include/se_vfx.h`, `include/se_curve.h`
- Resource layout details: `resources/README.md`

### UI Designer Quick Start
`se_ui` now has a helper-first layer aimed at fast UI authoring with callbacks:

```c
se_ui_handle ui = se_ui_create(window, 256);
se_ui_widget_handle root = se_ui_create_root(ui);
se_ui_vstack(ui, root, 0.02f, se_ui_edge_all(0.02f));

se_ui_widget_handle title = se_ui_label(ui, root, "Settings");
se_ui_widget_handle play = se_ui_button(ui, root, "Play", on_play_click, game_state);
se_ui_widget_handle name = se_ui_textbox(ui, root, "Player name", on_name_change, on_name_submit, game_state);
se_ui_widget_handle list = se_ui_scrollbox(ui, root, s_vec2(0.5f, 0.3f), on_scroll, game_state);
se_ui_theme_apply(ui, SE_UI_THEME_PRESET_DEFAULT);

se_ui_scroll_item_add(ui, list, "Easy");
se_ui_scroll_item_add(ui, list, "Normal");
se_ui_scroll_item_add(ui, list, "Hard");
```

Helper APIs map directly to low-level control when needed:

| Helper | Low-level equivalent |
|---|---|
| `se_ui_button_create` / `SE_UI_BUTTON` | `se_ui_button_add` + callback fields |
| `se_ui_textbox_create` / `SE_UI_TEXTBOX` | `se_ui_textbox_add` + callbacks |
| `se_ui_scroll_item_add` | `se_ui_button_add` child under scrollbox |
| `se_ui_widget_set_stack_vertical` | mutate `se_ui_layout.direction/spacing` |
| `se_ui_widget_set_padding` | mutate `se_ui_layout.padding` |
| `se_ui_widget_use_style_preset` | `se_ui_widget_set_style` |
| `se_ui_widget_find` | explicit handle tracking by `id` |

Migration note:
- Existing descriptor-based APIs remain fully supported.
- Old and new styles can be mixed in the same screen.

Example mapping for a button:

```c
/* Old */
se_ui_button_desc d = SE_UI_BUTTON_DESC_DEFAULTS;
d.text = "Save";
d.callbacks.on_click = on_save;
d.callbacks.user_data = data;
se_ui_widget_handle save = se_ui_button_add(ui, parent, &d);

/* New */
se_ui_widget_handle save = se_ui_button(ui, parent, "Save", on_save, data);
```

VFX note:
- `se_vfx` renders into internal framebuffers/textures via explicit APIs:
  - `se_vfx_*_tick(...)` for simulation cadence.
  - `se_vfx_*_render(...)` for offscreen refresh cadence.
  - `se_vfx_*_draw(window)` for presentation cadence.
- `se_vfx` is scene-independent and does not require `se_scene_2d`/`se_scene_3d` management.

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
