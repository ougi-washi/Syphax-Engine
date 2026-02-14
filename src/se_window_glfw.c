// Syphax-Engine - Ougi Washi

#if defined(SE_WINDOW_BACKEND_GLFW)

#include "se_window.h"
#include "se_debug.h"
#include "render/se_gl.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#define SE_MAX_WINDOWS 8
#define SE_MAX_KEY_COMBOS 8
#define SE_MAX_INPUT_EVENTS 1024
#define SE_MAX_RESIZE_HANDLE 1024

typedef struct {
	se_context* ctx;
	se_window_handle window;
} se_window_registry_entry;

typedef s_array(se_window_registry_entry, se_windows_registry);
static se_windows_registry windows_registry = {0};
static se_context* windows_owner_context = NULL;
static GLFWwindow* current_conext_window = NULL;

typedef enum {
	SE_WINDOW_TERMINAL_PARSE_NORMAL = 0,
	SE_WINDOW_TERMINAL_PARSE_ESC,
	SE_WINDOW_TERMINAL_PARSE_CSI,
	SE_WINDOW_TERMINAL_PARSE_SS3
} se_window_terminal_parse_state;

typedef struct {
	se_window_terminal_parse_state state;
	c8 csi_buffer[64];
	sz csi_len;
} se_window_terminal_input_parser;

typedef struct {
	b8 configured;
	b8 enabled;
	b8 hide_window;
	b8 interactive_stdout;
	b8 ansi_initialized;
	b8 emitted_header;
	b8 restore_hooks_installed;
	b8 input_initialized;
	b8 input_raw_enabled;
	u32 forced_columns;
	u32 forced_rows;
	u32 columns;
	u32 rows;
	u32 target_fps;
	f64 frame_interval;
	f64 next_present_time;
	b8 stdout_muted;
	i32 input_original_stdin_flags;
	i32 stdout_original_fd;
	i32 output_fd;
	FILE* output_stream;
	struct termios input_original_termios;
	u8* pixel_buffer;
	sz pixel_buffer_capacity;
} se_window_terminal_mirror_state;

static se_window_terminal_mirror_state g_terminal_mirror = {0};
static se_window_terminal_input_parser g_terminal_input_parser = {0};
static volatile sig_atomic_t g_terminal_restore_emitted = 0;

static i32 se_window_terminal_mirror_output_fd(void) {
	return g_terminal_mirror.output_fd >= 0 ? g_terminal_mirror.output_fd : STDOUT_FILENO;
}

static FILE* se_window_terminal_mirror_output_stream(void) {
	return g_terminal_mirror.output_stream ? g_terminal_mirror.output_stream : stdout;
}

typedef struct {
	se_context* ctx;
	se_window_handle window;
} se_window_user_ptr;

static se_window* se_window_from_handle(se_context* context, const se_window_handle window) {
	return s_array_get(&context->windows, window);
}

static void se_window_terminal_mirror_shutdown(void);
static void se_window_backend_shutdown_if_unused(void) {
	if (s_array_get_size(&windows_registry) != 0) {
		return;
	}
	s_array_clear(&windows_registry);
	windows_owner_context = NULL;
	current_conext_window = NULL;
	glfwMakeContextCurrent(NULL);
	se_window_terminal_mirror_shutdown();
	se_render_shutdown();
	glfwTerminate();
}

static void se_window_registry_refresh_owner_context(void) {
	if (s_array_get_size(&windows_registry) == 0) {
		windows_owner_context = NULL;
		return;
	}
	if (windows_owner_context != NULL) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (entry && entry->ctx) {
			windows_owner_context = entry->ctx;
			return;
		}
	}
}

static se_window_registry_entry* se_window_registry_find(const se_context* preferred_ctx, const se_window_handle window, sz* out_index) {
	if (out_index) {
		*out_index = 0;
	}
	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (!entry || entry->window != window || entry->ctx != preferred_ctx) {
			continue;
		}
		if (out_index) {
			*out_index = i;
		}
		return entry;
	}
	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (!entry || entry->window != window) {
			continue;
		}
		if (out_index) {
			*out_index = i;
		}
		return entry;
	}
	return NULL;
}

static b8 se_window_context_has_live_resources(const se_context* context) {
	if (context == NULL) {
		return false;
	}
	return
		context->ui_text_handle != NULL ||
		s_array_get_size(&context->fonts) > 0 ||
		s_array_get_size(&context->models) > 0 ||
		s_array_get_size(&context->cameras) > 0 ||
		s_array_get_size(&context->framebuffers) > 0 ||
		s_array_get_size(&context->render_buffers) > 0 ||
		s_array_get_size(&context->textures) > 0 ||
		s_array_get_size(&context->shaders) > 0 ||
		s_array_get_size(&context->objects_2d) > 0 ||
		s_array_get_size(&context->scenes_2d) > 0 ||
		s_array_get_size(&context->objects_3d) > 0 ||
		s_array_get_size(&context->scenes_3d) > 0 ||
		s_array_get_size(&context->ui_elements) > 0 ||
		s_array_get_size(&context->ui_texts) > 0;
}

static void se_window_terminal_mirror_sync_size(void);

static b8 se_window_terminal_mirror_get_primary_window(se_window_handle* out_handle, se_window** out_window) {
	if (!out_handle || !out_window) {
		return false;
	}
	*out_handle = S_HANDLE_NULL;
	*out_window = NULL;
	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (!entry || !entry->ctx || entry->window == S_HANDLE_NULL) {
			continue;
		}
		se_window* window_ptr = s_array_get(&entry->ctx->windows, entry->window);
		if (!window_ptr || !window_ptr->handle) {
			continue;
		}
		*out_handle = entry->window;
		*out_window = window_ptr;
		return true;
	}
	return false;
}

static se_key se_window_terminal_map_ascii_key(const c8 ch) {
	if (ch >= 'a' && ch <= 'z') {
		return (se_key)(SE_KEY_A + (ch - 'a'));
	}
	if (ch >= 'A' && ch <= 'Z') {
		return (se_key)(SE_KEY_A + (ch - 'A'));
	}
	if (ch >= '0' && ch <= '9') {
		return (se_key)(SE_KEY_0 + (ch - '0'));
	}
	switch (ch) {
		case ' ':
			return SE_KEY_SPACE;
		case '\'':
			return SE_KEY_APOSTROPHE;
		case ',':
			return SE_KEY_COMMA;
		case '-':
			return SE_KEY_MINUS;
		case '.':
			return SE_KEY_PERIOD;
		case '/':
			return SE_KEY_SLASH;
		case ';':
			return SE_KEY_SEMICOLON;
		case '=':
			return SE_KEY_EQUAL;
		case '[':
			return SE_KEY_LEFT_BRACKET;
		case '\\':
			return SE_KEY_BACKSLASH;
		case ']':
			return SE_KEY_RIGHT_BRACKET;
		case '`':
			return SE_KEY_GRAVE_ACCENT;
		default:
			return SE_KEY_UNKNOWN;
	}
}

static void se_window_terminal_mirror_emit_key_press(se_window* window_ptr, const se_key key) {
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT || key == SE_KEY_UNKNOWN) {
		return;
	}
	window_ptr->keys[key] = true;
	window_ptr->diagnostics.key_events++;
}

static void se_window_terminal_mirror_emit_text_char(const se_window_handle window, se_window* window_ptr, const c8 ch) {
	if (!window_ptr || !window_ptr->text_callback || !isprint((unsigned char)ch)) {
		return;
	}
	c8 utf8[2] = {ch, '\0'};
	window_ptr->text_callback(window, utf8, window_ptr->text_callback_data);
}

static i32 se_window_terminal_map_mouse_button(const i32 terminal_button_code) {
	switch (terminal_button_code) {
		case 0:
			return SE_MOUSE_LEFT;
		case 1:
			return SE_MOUSE_MIDDLE;
		case 2:
			return SE_MOUSE_RIGHT;
		default:
			return -1;
	}
}

static void se_window_terminal_mirror_handle_mouse_sgr(
	se_window* window_ptr,
	const c8* csi_data,
	const c8 final_char) {
	if (!window_ptr || !csi_data) {
		return;
	}
	i32 code = 0;
	i32 x = 0;
	i32 y = 0;
	if (sscanf(csi_data, "<%d;%d;%d", &code, &x, &y) != 3) {
		return;
	}

	const f64 grid_x = (f64)s_max(0, x - 1);
	const f64 grid_y = (f64)s_max(0, y - 1);
	const f64 denom_x = (f64)s_max((u32)1, g_terminal_mirror.columns - 1u);
	const f64 denom_y = (f64)s_max((u32)1, g_terminal_mirror.rows - 1u);
	const f64 scale_x = (f64)s_max((u32)1, window_ptr->width - 1u);
	const f64 scale_y = (f64)s_max((u32)1, window_ptr->height - 1u);
	const f64 next_x = s_max(0.0, s_min((grid_x / denom_x) * scale_x, (f64)window_ptr->width));
	const f64 next_y = s_max(0.0, s_min((grid_y / denom_y) * scale_y, (f64)window_ptr->height));

	window_ptr->mouse_dx = next_x - window_ptr->mouse_x;
	window_ptr->mouse_dy = next_y - window_ptr->mouse_y;
	window_ptr->mouse_x = next_x;
	window_ptr->mouse_y = next_y;
	window_ptr->diagnostics.mouse_move_events++;

	if ((code & 64) != 0) {
		const i32 wheel_code = code & 0x3;
		if (wheel_code == 0) {
			window_ptr->scroll_dy += 1.0;
		} else if (wheel_code == 1) {
			window_ptr->scroll_dy -= 1.0;
		} else if (wheel_code == 2) {
			window_ptr->scroll_dx -= 1.0;
		} else if (wheel_code == 3) {
			window_ptr->scroll_dx += 1.0;
		}
		window_ptr->diagnostics.scroll_events++;
		return;
	}

	const i32 terminal_button_code = code & 0x3;
	const i32 button_code = se_window_terminal_map_mouse_button(terminal_button_code);
	if (button_code < 0 || button_code >= SE_MOUSE_BUTTON_COUNT) {
		return;
	}
	const b8 is_release = (final_char == 'm');
	window_ptr->mouse_buttons[button_code] = !is_release;
	window_ptr->diagnostics.mouse_button_events++;
}

static void se_window_terminal_mirror_handle_csi(
	const se_window_handle window,
	se_window* window_ptr,
	const c8* csi_data,
	const c8 final_char) {
	if (!window_ptr || !csi_data) {
		return;
	}
	if (csi_data[0] == '<' && (final_char == 'M' || final_char == 'm')) {
		se_window_terminal_mirror_handle_mouse_sgr(window_ptr, csi_data, final_char);
		return;
	}

	if (final_char == 'A') {
		se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_UP);
		return;
	}
	if (final_char == 'B') {
		se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_DOWN);
		return;
	}
	if (final_char == 'C') {
		se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_RIGHT);
		return;
	}
	if (final_char == 'D') {
		se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_LEFT);
		return;
	}
	if (final_char == 'H') {
		se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_HOME);
		return;
	}
	if (final_char == 'F') {
		se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_END);
		return;
	}
	if (final_char == 'Z') {
		se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_TAB);
		return;
	}
	if (final_char != '~') {
		return;
	}
	const i32 code = atoi(csi_data);
	switch (code) {
		case 1:
		case 7:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_HOME);
			break;
		case 2:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_INSERT);
			break;
		case 3:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_DELETE);
			break;
		case 4:
		case 8:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_END);
			break;
		case 5:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_PAGE_UP);
			break;
		case 6:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_PAGE_DOWN);
			break;
		case 15:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F5);
			break;
		case 17:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F6);
			break;
		case 18:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F7);
			break;
		case 19:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F8);
			break;
		case 20:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F9);
			break;
		case 21:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F10);
			break;
		case 23:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F11);
			break;
		case 24:
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F12);
			break;
		default:
			break;
	}
}

static void se_window_terminal_mirror_handle_input_byte(const se_window_handle window, se_window* window_ptr, const u8 byte) {
	if (!window_ptr) {
		return;
	}
	switch (g_terminal_input_parser.state) {
		case SE_WINDOW_TERMINAL_PARSE_NORMAL: {
			if (byte == 0x1B) {
				g_terminal_input_parser.state = SE_WINDOW_TERMINAL_PARSE_ESC;
				return;
			}
			if (byte >= 1 && byte <= 26) {
				se_window_terminal_mirror_emit_key_press(window_ptr, (se_key)(SE_KEY_A + (byte - 1)));
				return;
			}
			if (byte == '\r' || byte == '\n') {
				se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_ENTER);
				return;
			}
			if (byte == '\t') {
				se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_TAB);
				return;
			}
			if (byte == 0x7F || byte == 0x08) {
				se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_BACKSPACE);
				return;
			}
			const c8 ch = (c8)byte;
			const se_key mapped_key = se_window_terminal_map_ascii_key(ch);
			if (mapped_key != SE_KEY_UNKNOWN) {
				se_window_terminal_mirror_emit_key_press(window_ptr, mapped_key);
			}
			se_window_terminal_mirror_emit_text_char(window, window_ptr, ch);
			return;
		}
		case SE_WINDOW_TERMINAL_PARSE_ESC: {
			if (byte == '[') {
				g_terminal_input_parser.state = SE_WINDOW_TERMINAL_PARSE_CSI;
				g_terminal_input_parser.csi_len = 0;
				g_terminal_input_parser.csi_buffer[0] = '\0';
				return;
			}
			if (byte == 'O') {
				g_terminal_input_parser.state = SE_WINDOW_TERMINAL_PARSE_SS3;
				return;
			}
			se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_ESCAPE);
			g_terminal_input_parser.state = SE_WINDOW_TERMINAL_PARSE_NORMAL;
			se_window_terminal_mirror_handle_input_byte(window, window_ptr, byte);
			return;
		}
		case SE_WINDOW_TERMINAL_PARSE_CSI: {
			if (g_terminal_input_parser.csi_len + 1 < sizeof(g_terminal_input_parser.csi_buffer)) {
				g_terminal_input_parser.csi_buffer[g_terminal_input_parser.csi_len++] = (c8)byte;
				g_terminal_input_parser.csi_buffer[g_terminal_input_parser.csi_len] = '\0';
			}
			if (byte >= 0x40 && byte <= 0x7E) {
				if (g_terminal_input_parser.csi_len > 0) {
					g_terminal_input_parser.csi_buffer[g_terminal_input_parser.csi_len - 1] = '\0';
					se_window_terminal_mirror_handle_csi(
						window,
						window_ptr,
						g_terminal_input_parser.csi_buffer,
						(c8)byte);
				}
				g_terminal_input_parser.state = SE_WINDOW_TERMINAL_PARSE_NORMAL;
				g_terminal_input_parser.csi_len = 0;
				g_terminal_input_parser.csi_buffer[0] = '\0';
			}
			return;
		}
		case SE_WINDOW_TERMINAL_PARSE_SS3: {
			switch (byte) {
				case 'P':
					se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F1);
					break;
				case 'Q':
					se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F2);
					break;
				case 'R':
					se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F3);
					break;
				case 'S':
					se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_F4);
					break;
				case 'H':
					se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_HOME);
					break;
				case 'F':
					se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_END);
					break;
				default:
					break;
			}
			g_terminal_input_parser.state = SE_WINDOW_TERMINAL_PARSE_NORMAL;
			return;
		}
		default:
			g_terminal_input_parser.state = SE_WINDOW_TERMINAL_PARSE_NORMAL;
			return;
	}
}

static b8 se_window_terminal_mirror_should_use_terminal_input(void) {
	return g_terminal_mirror.enabled && g_terminal_mirror.hide_window && isatty(STDIN_FILENO);
}

static void se_window_terminal_mirror_input_enable(void) {
	if (g_terminal_mirror.input_initialized) {
		return;
	}
	g_terminal_mirror.input_initialized = true;
	g_terminal_mirror.input_original_stdin_flags = -1;
	memset(&g_terminal_input_parser, 0, sizeof(g_terminal_input_parser));

	if (!se_window_terminal_mirror_should_use_terminal_input()) {
		return;
	}

	if (tcgetattr(STDIN_FILENO, &g_terminal_mirror.input_original_termios) != 0) {
		return;
	}
	struct termios raw = g_terminal_mirror.input_original_termios;
	raw.c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= (tcflag_t) ~(OPOST);
	raw.c_cflag |= (tcflag_t)(CS8);
	raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
		return;
	}

	g_terminal_mirror.input_original_stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	if (g_terminal_mirror.input_original_stdin_flags >= 0) {
		(void)fcntl(STDIN_FILENO, F_SETFL, g_terminal_mirror.input_original_stdin_flags | O_NONBLOCK);
	}
	static const c8 enable_mouse_sequence[] = "\033[?1000h\033[?1002h\033[?1006h";
	(void)write(se_window_terminal_mirror_output_fd(), enable_mouse_sequence, sizeof(enable_mouse_sequence) - 1);
	g_terminal_mirror.input_raw_enabled = true;
}

static void se_window_terminal_mirror_input_disable(void) {
	if (!g_terminal_mirror.input_initialized) {
		return;
	}
	if (g_terminal_mirror.input_raw_enabled) {
		if (g_terminal_mirror.input_original_stdin_flags >= 0) {
			(void)fcntl(STDIN_FILENO, F_SETFL, g_terminal_mirror.input_original_stdin_flags);
		}
		(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_terminal_mirror.input_original_termios);
	}
	g_terminal_mirror.input_raw_enabled = false;
	g_terminal_mirror.input_initialized = false;
	g_terminal_mirror.input_original_stdin_flags = -1;
	memset(&g_terminal_input_parser, 0, sizeof(g_terminal_input_parser));
}

static void se_window_terminal_mirror_poll_input(void) {
	if (!g_terminal_mirror.input_raw_enabled) {
		return;
	}
	se_window_terminal_mirror_sync_size();
	se_window_handle window = S_HANDLE_NULL;
	se_window* window_ptr = NULL;
	if (!se_window_terminal_mirror_get_primary_window(&window, &window_ptr)) {
		return;
	}
	memset(window_ptr->keys, 0, sizeof(window_ptr->keys));

	u8 buffer[256] = {0};
	for (;;) {
		const ssize_t read_count = read(STDIN_FILENO, buffer, sizeof(buffer));
		if (read_count <= 0) {
			if (read_count < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
				break;
			}
			break;
		}
		for (ssize_t i = 0; i < read_count; ++i) {
			se_window_terminal_mirror_handle_input_byte(window, window_ptr, buffer[i]);
		}
	}

	if (g_terminal_input_parser.state == SE_WINDOW_TERMINAL_PARSE_ESC) {
		se_window_terminal_mirror_emit_key_press(window_ptr, SE_KEY_ESCAPE);
		g_terminal_input_parser.state = SE_WINDOW_TERMINAL_PARSE_NORMAL;
	}
}

static void se_window_terminal_mirror_write_restore_sequence(void) {
	if (g_terminal_restore_emitted) {
		return;
	}
	g_terminal_restore_emitted = 1;
	static const c8 restore_sequence[] = "\033[?1000l\033[?1002l\033[?1006l\033[0m\033[?25h\033[?1049l";
	(void)write(se_window_terminal_mirror_output_fd(), restore_sequence, sizeof(restore_sequence) - 1);
}

static void se_window_terminal_mirror_restore_on_exit(void) {
	se_window_terminal_mirror_input_disable();
	se_window_terminal_mirror_write_restore_sequence();
}

static void se_window_terminal_mirror_restore_on_signal(const i32 signal_id) {
	se_window_terminal_mirror_input_disable();
	se_window_terminal_mirror_write_restore_sequence();
	_exit(128 + signal_id);
}

static void se_window_terminal_mirror_install_restore_hooks(void) {
	if (g_terminal_mirror.restore_hooks_installed ||
		!g_terminal_mirror.enabled ||
		!g_terminal_mirror.interactive_stdout) {
		return;
	}
	(void)atexit(se_window_terminal_mirror_restore_on_exit);
	(void)signal(SIGINT, se_window_terminal_mirror_restore_on_signal);
	(void)signal(SIGTERM, se_window_terminal_mirror_restore_on_signal);
#ifdef SIGHUP
	(void)signal(SIGHUP, se_window_terminal_mirror_restore_on_signal);
#endif
#ifdef SIGABRT
	(void)signal(SIGABRT, se_window_terminal_mirror_restore_on_signal);
#endif
	g_terminal_mirror.restore_hooks_installed = true;
}

static b8 se_window_string_equal_ci(const c8* lhs, const c8* rhs) {
	if (!lhs || !rhs) {
		return false;
	}
	while (*lhs && *rhs) {
		if (tolower((unsigned char)*lhs) != tolower((unsigned char)*rhs)) {
			return false;
		}
		++lhs;
		++rhs;
	}
	return *lhs == '\0' && *rhs == '\0';
}

static b8 se_window_env_flag_enabled(const c8* key) {
	const c8* value = getenv(key);
	if (!value || value[0] == '\0') {
		return false;
	}
	if (strcmp(value, "0") == 0 ||
		se_window_string_equal_ci(value, "false") ||
		se_window_string_equal_ci(value, "off") ||
		se_window_string_equal_ci(value, "no")) {
		return false;
	}
	return true;
}

static b8 se_window_terminal_allow_logs(void) {
	return
		se_window_env_flag_enabled("SE_TERMINAL_ALLOW_LOGS") ||
		se_window_env_flag_enabled("SE_TERMINAL_ALLOW_STDOUT_LOGS");
}

static u32 se_window_env_u32(const c8* key, const u32 fallback, const u32 min_value, const u32 max_value) {
	const c8* value = getenv(key);
	if (!value || value[0] == '\0') {
		return fallback;
	}
	errno = 0;
	c8* end_ptr = NULL;
	unsigned long parsed = strtoul(value, &end_ptr, 10);
	if (errno != 0 || end_ptr == value || (end_ptr && *end_ptr != '\0')) {
		return fallback;
	}
	if (parsed < min_value) {
		return min_value;
	}
	if (parsed > max_value) {
		return max_value;
	}
	return (u32)parsed;
}

static void se_window_terminal_mirror_configure(void) {
	if (g_terminal_mirror.configured) {
		return;
	}
	memset(&g_terminal_mirror, 0, sizeof(g_terminal_mirror));
	g_terminal_restore_emitted = 0;
	g_terminal_mirror.configured = true;
	g_terminal_mirror.enabled =
		se_window_env_flag_enabled("SE_TERMINAL_RENDER") ||
		se_window_env_flag_enabled("SE_TERMINAL_MIRROR") ||
		se_window_env_flag_enabled("SE_WINDOW_TERMINAL");
	if (!g_terminal_mirror.enabled) {
		return;
	}
	g_terminal_mirror.hide_window = se_window_env_flag_enabled("SE_TERMINAL_HIDE_WINDOW");
	g_terminal_mirror.target_fps = se_window_env_u32("SE_TERMINAL_FPS", 12, 1, 60);
	g_terminal_mirror.frame_interval = 1.0 / (f64)g_terminal_mirror.target_fps;
	g_terminal_mirror.forced_columns = se_window_env_u32("SE_TERMINAL_COLS", 0, 0, 240);
	g_terminal_mirror.forced_rows = se_window_env_u32("SE_TERMINAL_ROWS", 0, 0, 120);
	g_terminal_mirror.interactive_stdout = isatty(STDOUT_FILENO);
	g_terminal_mirror.output_fd = -1;
	g_terminal_mirror.stdout_original_fd = -1;
	g_terminal_mirror.output_stream = NULL;
	g_terminal_mirror.stdout_muted = false;
	if (g_terminal_mirror.interactive_stdout) {
		const b8 allow_stdout_logs = se_window_terminal_allow_logs();
		const i32 output_fd = dup(STDOUT_FILENO);
		if (output_fd >= 0) {
			g_terminal_mirror.output_fd = output_fd;
			const i32 stream_fd = dup(output_fd);
			if (stream_fd >= 0) {
				g_terminal_mirror.output_stream = fdopen(stream_fd, "w");
				if (!g_terminal_mirror.output_stream) {
					(void)close(stream_fd);
				}
			}
		}
		if (!allow_stdout_logs) {
			const i32 null_fd = open("/dev/null", O_WRONLY);
			if (null_fd >= 0) {
				g_terminal_mirror.stdout_original_fd = dup(STDOUT_FILENO);
				if (g_terminal_mirror.stdout_original_fd >= 0) {
					fflush(stdout);
					if (dup2(null_fd, STDOUT_FILENO) >= 0) {
						g_terminal_mirror.stdout_muted = true;
					} else {
						(void)close(g_terminal_mirror.stdout_original_fd);
						g_terminal_mirror.stdout_original_fd = -1;
					}
				}
				(void)close(null_fd);
			}
		}
	}
	se_window_terminal_mirror_install_restore_hooks();
}

static void se_window_terminal_mirror_sync_size(void) {
	if (!g_terminal_mirror.enabled) {
		return;
	}
	u32 next_columns = g_terminal_mirror.forced_columns;
	u32 next_rows = g_terminal_mirror.forced_rows;
	if (next_columns == 0 || next_rows == 0) {
		struct winsize ws = {0};
		if (ioctl(se_window_terminal_mirror_output_fd(), TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
			next_columns = (u32)ws.ws_col;
			next_rows = (u32)ws.ws_row;
		}
	}
	if (next_columns == 0) {
		next_columns = 120;
	}
	if (next_rows == 0) {
		next_rows = 40;
	}
	next_columns = s_max(16, s_min(next_columns, 240));
	next_rows = s_max(8, s_min(next_rows, 120));
	if (next_columns != g_terminal_mirror.columns || next_rows != g_terminal_mirror.rows) {
		g_terminal_mirror.columns = next_columns;
		g_terminal_mirror.rows = next_rows;
		g_terminal_mirror.ansi_initialized = false;
	}
}

static void se_window_terminal_mirror_begin_frame(void) {
	if (!g_terminal_mirror.enabled || !g_terminal_mirror.interactive_stdout) {
		return;
	}
	if (!g_terminal_mirror.ansi_initialized) {
		FILE* out = se_window_terminal_mirror_output_stream();
		fprintf(out, "\033[?1049h\033[2J\033[H\033[?25l");
		fflush(out);
		g_terminal_mirror.ansi_initialized = true;
	}
}

static b8 se_window_terminal_mirror_reserve_pixels(const u32 width, const u32 height) {
	if (!g_terminal_mirror.enabled) {
		return false;
	}
	const sz required = (sz)width * (sz)height * 3;
	if (required == 0) {
		return false;
	}
	if (required <= g_terminal_mirror.pixel_buffer_capacity && g_terminal_mirror.pixel_buffer) {
		return true;
	}
	u8* resized = realloc(g_terminal_mirror.pixel_buffer, required);
	if (!resized) {
		return false;
	}
	g_terminal_mirror.pixel_buffer = resized;
	g_terminal_mirror.pixel_buffer_capacity = required;
	return true;
}

static void se_window_terminal_mirror_present(se_window* window_ptr) {
	if (!window_ptr || !window_ptr->handle) {
		return;
	}
	se_window_terminal_mirror_configure();
	if (!g_terminal_mirror.enabled || !g_terminal_mirror.interactive_stdout) {
		return;
	}
	const f64 now = glfwGetTime();
	if (now < g_terminal_mirror.next_present_time) {
		return;
	}
	g_terminal_mirror.next_present_time = now + g_terminal_mirror.frame_interval;

	se_window_terminal_mirror_sync_size();
	se_window_terminal_mirror_begin_frame();
	if (g_terminal_mirror.columns == 0 || g_terminal_mirror.rows == 0) {
		return;
	}

	i32 fb_width = 0;
	i32 fb_height = 0;
	glfwGetFramebufferSize((GLFWwindow*)window_ptr->handle, &fb_width, &fb_height);
	if (fb_width <= 0 || fb_height <= 0) {
		return;
	}
	if (!se_window_terminal_mirror_reserve_pixels((u32)fb_width, (u32)fb_height)) {
		return;
	}

	if (!g_terminal_mirror.emitted_header) {
		if (se_window_terminal_allow_logs()) {
			se_debug_log(
				SE_DEBUG_LEVEL_INFO,
				SE_DEBUG_CATEGORY_WINDOW,
				"Terminal mirror enabled (fps=%u, cols=%u, rows=%u, hide_window=%d)",
				g_terminal_mirror.target_fps,
				g_terminal_mirror.columns,
				g_terminal_mirror.rows,
				g_terminal_mirror.hide_window ? 1 : 0);
		}
		g_terminal_mirror.emitted_header = true;
	}

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, fb_width, fb_height, GL_RGB, GL_UNSIGNED_BYTE, g_terminal_mirror.pixel_buffer);

	static const c8 block_glyph[] = "\xE2\x96\x88";
	FILE* out = se_window_terminal_mirror_output_stream();
	fprintf(out, "\033[H");

	u8 current_fg[3] = {255, 255, 255};
	b8 has_color = false;

	for (u32 row = 0; row < g_terminal_mirror.rows; ++row) {
		for (u32 column = 0; column < g_terminal_mirror.columns; ++column) {
			const u32 src_x = s_min(
				(u32)(((column * (u64)fb_width) + (g_terminal_mirror.columns / 2u)) / g_terminal_mirror.columns),
				(u32)(fb_width - 1));
			const u32 src_y_unflipped = s_min(
				(u32)(((row * (u64)fb_height) + (g_terminal_mirror.rows / 2u)) / g_terminal_mirror.rows),
				(u32)(fb_height - 1));
			const u32 src_y = (u32)(fb_height - 1) - src_y_unflipped;
			const sz pixel_index = ((sz)src_y * (sz)fb_width + (sz)src_x) * 3;
			const u8 r = g_terminal_mirror.pixel_buffer[pixel_index + 0];
			const u8 g = g_terminal_mirror.pixel_buffer[pixel_index + 1];
			const u8 b = g_terminal_mirror.pixel_buffer[pixel_index + 2];
			if (!has_color || r != current_fg[0] || g != current_fg[1] || b != current_fg[2]) {
				fprintf(out, "\033[38;2;%u;%u;%um", (unsigned)r, (unsigned)g, (unsigned)b);
				current_fg[0] = r;
				current_fg[1] = g;
				current_fg[2] = b;
				has_color = true;
			}
			fputs(block_glyph, out);
		}
		if (row + 1 < g_terminal_mirror.rows) {
			fputc('\n', out);
		}
	}
	// Return cursor to home so periodic logs do not cause terminal scroll/jitter.
	fprintf(out, "\033[0m\033[H");
	fflush(out);
}

static void se_window_terminal_mirror_shutdown(void) {
	if (!g_terminal_mirror.configured) {
		return;
	}
	se_window_terminal_mirror_input_disable();
	if (g_terminal_mirror.enabled && g_terminal_mirror.interactive_stdout) {
		se_window_terminal_mirror_write_restore_sequence();
	}
	if (g_terminal_mirror.output_stream) {
		fflush(g_terminal_mirror.output_stream);
		fclose(g_terminal_mirror.output_stream);
		g_terminal_mirror.output_stream = NULL;
	}
	if (g_terminal_mirror.output_fd >= 0) {
		(void)close(g_terminal_mirror.output_fd);
		g_terminal_mirror.output_fd = -1;
	}
	if (g_terminal_mirror.stdout_muted && g_terminal_mirror.stdout_original_fd >= 0) {
		fflush(stdout);
		(void)dup2(g_terminal_mirror.stdout_original_fd, STDOUT_FILENO);
	}
	if (g_terminal_mirror.stdout_original_fd >= 0) {
		(void)close(g_terminal_mirror.stdout_original_fd);
		g_terminal_mirror.stdout_original_fd = -1;
	}
	free(g_terminal_mirror.pixel_buffer);
	memset(&g_terminal_mirror, 0, sizeof(g_terminal_mirror));
}

static se_window* se_window_from_glfw(GLFWwindow* glfw_handle) {
	se_window_user_ptr* user_ptr = (se_window_user_ptr*)glfwGetWindowUserPointer(glfw_handle);
	if (!user_ptr) {
		return NULL;
	}
	return s_array_get(&user_ptr->ctx->windows, user_ptr->window);
}

static se_result se_window_init_render(se_window* window, se_context* context) {
	s_assertf(window, "se_window_init_render :: window is null");
	if (!context) {
		return SE_RESULT_INVALID_ARGUMENT;
	}
	if (window->context == context && window->shader != S_HANDLE_NULL) {
		return SE_RESULT_OK;
	}
	window->context = context;
	if (window->quad.vao == 0) {
		se_quad_2d_create(&window->quad, 0);
	}
	if (window->shader == S_HANDLE_NULL) {
		se_shader_handle shader = se_shader_load(SE_RESOURCE_INTERNAL("shaders/render_quad_vert.glsl"), SE_RESOURCE_INTERNAL("shaders/render_quad_frag.glsl"));
		if (shader == S_HANDLE_NULL) {
			return se_get_last_error();
		}
		window->shader = shader;
	}
	return SE_RESULT_OK;
}

static se_key se_window_map_glfw_key(const i32 key) {
	if (key >= SE_KEY_SPACE && key <= SE_KEY_MENU) {
		return (se_key)key;
	}
	return SE_KEY_UNKNOWN;
}

static sz se_window_utf8_encode(const u32 codepoint, c8 out_utf8[5]) {
	if (!out_utf8) {
		return 0;
	}
	if (codepoint <= 0x7FU) {
		out_utf8[0] = (c8)codepoint;
		out_utf8[1] = '\0';
		return 1;
	}
	if (codepoint <= 0x7FFU) {
		out_utf8[0] = (c8)(0xC0U | ((codepoint >> 6) & 0x1FU));
		out_utf8[1] = (c8)(0x80U | (codepoint & 0x3FU));
		out_utf8[2] = '\0';
		return 2;
	}
	if (codepoint <= 0xFFFFU) {
		out_utf8[0] = (c8)(0xE0U | ((codepoint >> 12) & 0x0FU));
		out_utf8[1] = (c8)(0x80U | ((codepoint >> 6) & 0x3FU));
		out_utf8[2] = (c8)(0x80U | (codepoint & 0x3FU));
		out_utf8[3] = '\0';
		return 3;
	}
	if (codepoint <= 0x10FFFFU) {
		out_utf8[0] = (c8)(0xF0U | ((codepoint >> 18) & 0x07U));
		out_utf8[1] = (c8)(0x80U | ((codepoint >> 12) & 0x3FU));
		out_utf8[2] = (c8)(0x80U | ((codepoint >> 6) & 0x3FU));
		out_utf8[3] = (c8)(0x80U | (codepoint & 0x3FU));
		out_utf8[4] = '\0';
		return 4;
	}
	return 0;
}

static void key_callback(GLFWwindow* glfw_handle, i32 key, i32 scancode, i32 action, i32 mods) {
	(void)scancode;
	(void)mods;
	se_window* window = se_window_from_glfw(glfw_handle);
	se_key mapped = se_window_map_glfw_key(key);
	if (mapped != SE_KEY_UNKNOWN) {
		if (action == GLFW_PRESS) {
			window->keys[mapped] = true;
		} else if (action == GLFW_RELEASE) {
			window->keys[mapped] = false;
		}
		window->diagnostics.key_events++;
	}
}

static void mouse_callback(GLFWwindow* glfw_handle, double xpos, double ypos) {
	se_window* window = se_window_from_glfw(glfw_handle);
	window->mouse_dx = xpos - window->mouse_x;
	window->mouse_dy = ypos - window->mouse_y;
	window->mouse_x = xpos;
	window->mouse_y = ypos;
	window->diagnostics.mouse_move_events++;
}

static void mouse_button_callback(GLFWwindow* glfw_handle, i32 button, i32 action, i32 mods) {
	(void)mods;
	se_window* window = se_window_from_glfw(glfw_handle);
	if (button >= 0 && button < SE_MOUSE_BUTTON_COUNT) {
		if (action == GLFW_PRESS) {
			window->mouse_buttons[button] = true;
		} else if (action == GLFW_RELEASE) {
			window->mouse_buttons[button] = false;
		}
		window->diagnostics.mouse_button_events++;
	}
}

static void scroll_callback(GLFWwindow* glfw_handle, double xoffset, double yoffset) {
	se_window* window = se_window_from_glfw(glfw_handle);
	window->scroll_dx += xoffset;
	window->scroll_dy += yoffset;
	window->diagnostics.scroll_events++;
}

static void char_callback(GLFWwindow* glfw_handle, unsigned int codepoint) {
	se_window_user_ptr* user_ptr = (se_window_user_ptr*)glfwGetWindowUserPointer(glfw_handle);
	if (!user_ptr || !user_ptr->ctx || user_ptr->window == S_HANDLE_NULL) {
		return;
	}
	se_window* window = s_array_get(&user_ptr->ctx->windows, user_ptr->window);
	if (!window || !window->text_callback) {
		return;
	}
	c8 utf8[5] = {0};
	if (se_window_utf8_encode((u32)codepoint, utf8) == 0) {
		return;
	}
	window->text_callback(user_ptr->window, utf8, window->text_callback_data);
}

static void framebuffer_size_callback(GLFWwindow* glfw_handle, i32 width, i32 height) {
	se_window_user_ptr* user_ptr = (se_window_user_ptr*)glfwGetWindowUserPointer(glfw_handle);
	if (!user_ptr) {
		return;
	}
	se_window* window = s_array_get(&user_ptr->ctx->windows, user_ptr->window);
	window->width = width;
	window->height = height;
	se_debug_log(SE_DEBUG_LEVEL_DEBUG, SE_DEBUG_CATEGORY_WINDOW, "Framebuffer resized: %d x %d", width, height);
	glViewport(0, 0, width, height);
	printf("frambuffer_size_change_fallback\n");
	for (sz i = 0; i < s_array_get_size(&window->resize_handles); ++i) {
		se_resize_handle* current_event_ptr = s_array_get(&window->resize_handles, s_array_handle(&window->resize_handles, (u32)i));
		if (current_event_ptr && current_event_ptr->callback) {
			current_event_ptr->callback(user_ptr->window, current_event_ptr->data);
		}
	}
}

void gl_error_callback(i32 error, const c8* description) {
	printf("GLFW Error %d: %s\n", error, description);
}

se_window_handle se_window_create(const char* title, const u32 width, const u32 height) {
	se_context* context = se_current_context();
	if (!context || !title) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_window_registry_refresh_owner_context();
	if (windows_owner_context != NULL && windows_owner_context != context) {
		se_debug_log(
			SE_DEBUG_LEVEL_ERROR,
			SE_DEBUG_CATEGORY_WINDOW,
			"se_window_create currently supports one active context at a time (owner=%p, requested=%p)",
			(void*)windows_owner_context,
			(void*)context);
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return S_HANDLE_NULL;
	}

	glfwSetErrorCallback(gl_error_callback);
	se_window_terminal_mirror_configure();
	if (g_terminal_mirror.enabled && g_terminal_mirror.hide_window) {
		// Null platform lets GLFW create a context without a window system.
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_NULL);
	}
	
	if (!glfwInit()) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		if (s_array_get_size(&windows_registry) == 0) {
			se_window_terminal_mirror_shutdown();
		}
		return S_HANDLE_NULL;
	}

	if (s_array_get_capacity(&context->windows) == 0) {
		s_array_init(&context->windows);
	}
	se_window_handle window_handle = s_array_increment(&context->windows);
	se_window* new_window = s_array_get(&context->windows, window_handle);

	memset(new_window, 0, sizeof(se_window));
	s_array_init(&new_window->input_events);
	s_array_init(&new_window->resize_handles);

	// Set graphics context version based on selected render backend.
#if defined(SE_RENDER_BACKEND_GLES)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
#endif
	if (g_terminal_mirror.enabled && g_terminal_mirror.hide_window) {
		glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	}
	 
	new_window->handle = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!new_window->handle) {
		s_array_clear(&new_window->input_events);
		s_array_clear(&new_window->resize_handles);
		s_array_remove(&context->windows, window_handle);
		if (s_array_get_size(&windows_registry) == 0) {
			se_window_terminal_mirror_shutdown();
			glfwTerminate();
		}
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return S_HANDLE_NULL;
	}
	
	
	new_window->width = width;
	new_window->height = height;
	new_window->cursor_mode = SE_WINDOW_CURSOR_NORMAL;
	new_window->raw_mouse_motion_supported = glfwRawMouseMotionSupported();
	new_window->raw_mouse_motion_enabled = false;
	new_window->vsync_enabled = false;
	new_window->should_close = false;
	
	se_window_set_current_context(window_handle);
	se_window_user_ptr* user_ptr = malloc(sizeof(*user_ptr));
	if (!user_ptr) {
		glfwDestroyWindow((GLFWwindow *)new_window->handle);
		new_window->handle = NULL;
		s_array_clear(&new_window->input_events);
		s_array_clear(&new_window->resize_handles);
		s_array_remove(&context->windows, window_handle);
		if (s_array_get_size(&windows_registry) == 0) {
			se_window_terminal_mirror_shutdown();
			glfwTerminate();
		}
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	user_ptr->ctx = context;
	user_ptr->window = window_handle;
	glfwSetWindowUserPointer((GLFWwindow*)new_window->handle, user_ptr);
	glfwSwapInterval(0);
	
	// Set callbacks
	glfwSetKeyCallback((GLFWwindow*)new_window->handle, key_callback);
	glfwSetCursorPosCallback((GLFWwindow*)new_window->handle, mouse_callback);
	glfwSetMouseButtonCallback((GLFWwindow*)new_window->handle, mouse_button_callback);
	glfwSetScrollCallback((GLFWwindow*)new_window->handle, scroll_callback);
	glfwSetCharCallback((GLFWwindow*)new_window->handle, char_callback);
	glfwSetFramebufferSizeCallback((GLFWwindow*)new_window->handle, framebuffer_size_callback);
	
	if (!se_render_init()) {
		se_window_user_ptr* window_user_ptr = (se_window_user_ptr*)glfwGetWindowUserPointer((GLFWwindow*)new_window->handle);
		if (window_user_ptr) {
			free(window_user_ptr);
			glfwSetWindowUserPointer((GLFWwindow*)new_window->handle, NULL);
		}
		glfwDestroyWindow((GLFWwindow *)new_window->handle);
		new_window->handle = NULL;
		s_array_clear(&new_window->input_events);
		s_array_clear(&new_window->resize_handles);
		s_array_remove(&context->windows, window_handle);
		if (s_array_get_size(&windows_registry) == 0) {
			se_window_terminal_mirror_shutdown();
			glfwTerminate();
		}
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return S_HANDLE_NULL;
	}
	
	glEnable(GL_DEPTH_TEST);
	
	//create_fullscreen_quad(&new_window->quad_vao, &new_window->quad_vbo, &new_window->quad_ebo);
	se_result render_result = se_window_init_render(new_window, context);
	if (render_result != SE_RESULT_OK) {
		if (new_window->quad.vao != 0) {
			se_quad_destroy(&new_window->quad);
		}
		se_window_user_ptr* window_user_ptr = (se_window_user_ptr*)glfwGetWindowUserPointer((GLFWwindow*)new_window->handle);
		if (window_user_ptr) {
			free(window_user_ptr);
			glfwSetWindowUserPointer((GLFWwindow*)new_window->handle, NULL);
		}
		glfwDestroyWindow((GLFWwindow *)new_window->handle);
		new_window->handle = NULL;
		s_array_clear(&new_window->input_events);
		s_array_clear(&new_window->resize_handles);
		s_array_remove(&context->windows, window_handle);
		if (s_array_get_size(&windows_registry) == 0) {
			se_window_terminal_mirror_shutdown();
			glfwTerminate();
		}
		se_set_last_error(render_result);
		return S_HANDLE_NULL;
	}

	new_window->time.current = glfwGetTime();
	new_window->time.last_frame = new_window->time.current;
	new_window->time.delta = 0;
	new_window->frame_count = 0;
	new_window->target_fps = 30;

	if (s_array_get_capacity(&windows_registry) == 0) {
		s_array_init(&windows_registry);
	}
	se_window_registry_entry entry = {0};
	entry.ctx = context;
	entry.window = window_handle;
	s_handle entry_handle = s_array_increment(&windows_registry);
	se_window_registry_entry* entry_slot = s_array_get(&windows_registry, entry_handle);
	*entry_slot = entry;
	windows_owner_context = context;
	if (g_terminal_mirror.enabled && g_terminal_mirror.hide_window) {
		se_window_terminal_mirror_input_enable();
	}

	se_set_last_error(SE_RESULT_OK);
	return window_handle;
}

void se_window_attach_render(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_attach_render :: window is null");
	se_window_init_render(window_ptr, context);
}

extern void se_window_update(const se_window_handle window) {
	se_debug_frame_begin();
	se_debug_trace_begin("window_update");
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	se_window_check_exit_keys(window);
	memcpy(window_ptr->keys_prev, window_ptr->keys, sizeof(window_ptr->keys));
	memcpy(window_ptr->mouse_buttons_prev, window_ptr->mouse_buttons, sizeof(window_ptr->mouse_buttons));
	window_ptr->mouse_dx = 0.0;
	window_ptr->mouse_dy = 0.0;
	window_ptr->scroll_dx = 0.0;
	window_ptr->scroll_dy = 0.0;
	window_ptr->time.last_frame = window_ptr->time.current;
	window_ptr->time.current = glfwGetTime();
	window_ptr->time.delta = window_ptr->time.current - window_ptr->time.last_frame;
	window_ptr->time.frame_start = window_ptr->time.current;
	window_ptr->frame_count++;
	se_debug_trace_end("window_update");
}

void se_window_tick(const se_window_handle window) {
	se_window_update(window);
	se_debug_trace_begin("input_tick");
	se_window_poll_events();
	se_debug_trace_end("input_tick");
}

void se_window_set_current_context(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	current_conext_window = (GLFWwindow*)window_ptr->handle;
	glfwMakeContextCurrent((GLFWwindow*)window_ptr->handle);
}

void se_window_render_quad(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr->shader != S_HANDLE_NULL, "se_window_render_quad :: shader is null");
	se_shader_use(window_ptr->shader, true, false);
	se_quad_render(&window_ptr->quad, 0);
}

void se_window_render_screen(const se_window_handle window) {
	se_debug_trace_begin("window_present");
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	const f64 present_begin = glfwGetTime();
	window_ptr->diagnostics.last_sleep_duration = 0.0;
	if (!window_ptr->vsync_enabled && window_ptr->target_fps > 0) {
		const f64 target_frame_time = 1.0 / window_ptr->target_fps;
		const f64 now = glfwGetTime();
		const f64 elapsed = now - window_ptr->time.frame_start;
		f64 time_left = target_frame_time - elapsed;
		if (time_left > 0.0) {
			/*
			 * Coarse sleep first, then spin for the final sub-millisecond window.
			 * This avoids repeatedly sleeping in 1ms chunks, which caused
			 * significant oversleep and unstable high-FPS pacing.
			 */
			const f64 coarse_guard = 0.001;
			if (time_left > coarse_guard) {
				const f64 coarse_sleep = time_left - coarse_guard;
				usleep((useconds_t)(coarse_sleep * 1000000.0));
				window_ptr->diagnostics.last_sleep_duration += coarse_sleep;
			}
			const f64 wait_end = window_ptr->time.frame_start + target_frame_time;
			while (glfwGetTime() < wait_end) {
				/* busy wait for precise frame boundary */
			}
			const f64 after_wait = glfwGetTime();
			const f64 slept = after_wait - now;
			if (slept > window_ptr->diagnostics.last_sleep_duration) {
				window_ptr->diagnostics.last_sleep_duration = slept;
			}
		}
	}
	se_debug_render_overlay(window, NULL);
	se_window_terminal_mirror_present(window_ptr);
	glfwSwapBuffers((GLFWwindow*)window_ptr->handle);
	window_ptr->diagnostics.frames_presented++;
	window_ptr->diagnostics.last_present_duration = glfwGetTime() - present_begin;
	se_debug_trace_end("window_present");
	se_debug_frame_end();
}

void se_window_set_vsync(const se_window_handle window, const b8 enabled) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_vsync :: window is null");
	se_window_set_current_context(window);
	glfwSwapInterval(enabled ? 1 : 0);
	window_ptr->vsync_enabled = enabled;
}

b8 se_window_is_vsync_enabled(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_vsync_enabled :: window is null");
	return window_ptr->vsync_enabled;
}

void se_window_present(const se_window_handle window) {
	se_window_render_screen(window);
}

void se_window_present_frame(const se_window_handle window, const s_vec4* clear_color) {
	if (clear_color) {
		se_render_set_background_color(*clear_color);
	}
	se_render_clear();
	se_window_render_screen(window);
}

void se_window_poll_events(){
	glfwPollEvents();
	se_window_terminal_mirror_poll_input();
	for (sz i = 0; i < s_array_get_size(&windows_registry); ++i) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (!entry || entry->window == S_HANDLE_NULL || !entry->ctx) {
			continue;
		}
		se_window* window = s_array_get(&entry->ctx->windows, entry->window);
		if (!window || window->handle == NULL) {
			continue;
		}
		s_vec2 mouse_position = {0};
		se_window_get_mouse_position_normalized(entry->window, &mouse_position);
		se_input_event* out_event = NULL;
		i32 out_depth = INT_MIN;
		b8 interacted = false;
		for (sz j = 0; j < s_array_get_size(&window->input_events); ++j) {
			se_input_event* current_event = s_array_get(&window->input_events, s_array_handle(&window->input_events, (u32)j));
			if (current_event->active &&
				mouse_position.x >= current_event->box.min.x && mouse_position.x <= current_event->box.max.x &&
				mouse_position.y >= current_event->box.min.y && mouse_position.y <= current_event->box.max.y) {
				if (current_event->depth > out_depth) {
					out_event = current_event;
					out_depth = current_event->depth;
					interacted = true;
				}
			}
		}
		if (interacted) {
			if (out_event->on_interact_callback) {
				out_event->on_interact_callback(entry->window, out_event->callback_data);
			}
			out_event->interacted = true;
		} else {
			for (sz j = 0; j < s_array_get_size(&window->input_events); ++j) {
				se_input_event* current_event = s_array_get(&window->input_events, s_array_handle(&window->input_events, (u32)j));
				if (current_event->interacted) {
					if (current_event->on_stop_interact_callback) {
						current_event->on_stop_interact_callback(entry->window, current_event->callback_data);
					}
					current_event->interacted = false;
				}
			}
		}
	}
}

b8 se_window_is_key_down(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_key_down :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window_ptr->keys[key];
}

b8 se_window_is_key_pressed(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_key_pressed :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window_ptr->keys[key] && !window_ptr->keys_prev[key];
}

b8 se_window_is_key_released(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_key_released :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return !window_ptr->keys[key] && window_ptr->keys_prev[key];
}

b8 se_window_is_mouse_down(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_mouse_down :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window_ptr->mouse_buttons[button];
}

b8 se_window_is_mouse_pressed(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_mouse_pressed :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window_ptr->mouse_buttons[button] && !window_ptr->mouse_buttons_prev[button];
}

b8 se_window_is_mouse_released(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_mouse_released :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return !window_ptr->mouse_buttons[button] && window_ptr->mouse_buttons_prev[button];
}

void se_window_clear_input_state(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_clear_input_state :: window is null");
	memset(window_ptr->keys, 0, sizeof(window_ptr->keys));
	memset(window_ptr->mouse_buttons, 0, sizeof(window_ptr->mouse_buttons));
	window_ptr->mouse_dx = 0.0;
	window_ptr->mouse_dy = 0.0;
	window_ptr->scroll_dx = 0.0;
	window_ptr->scroll_dy = 0.0;
}

void se_window_inject_key_state(const se_window_handle window, const se_key key, const b8 down) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_inject_key_state :: window is null");
	if (key < 0 || key >= SE_KEY_COUNT) {
		return;
	}
	window_ptr->keys[key] = down;
}

void se_window_inject_mouse_button_state(const se_window_handle window, const se_mouse_button button, const b8 down) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_inject_mouse_button_state :: window is null");
	if (button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return;
	}
	window_ptr->mouse_buttons[button] = down;
}

void se_window_inject_mouse_position(const se_window_handle window, const f32 x, const f32 y) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_inject_mouse_position :: window is null");
	const f64 prev_x = window_ptr->mouse_x;
	const f64 prev_y = window_ptr->mouse_y;
	window_ptr->mouse_x = x;
	window_ptr->mouse_y = y;
	window_ptr->mouse_dx = window_ptr->mouse_x - prev_x;
	window_ptr->mouse_dy = window_ptr->mouse_y - prev_y;
}

void se_window_inject_scroll_delta(const se_window_handle window, const f32 x, const f32 y) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_inject_scroll_delta :: window is null");
	window_ptr->scroll_dx = x;
	window_ptr->scroll_dy = y;
}

f32 se_window_get_mouse_position_x(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_mouse_position_x :: window is null");
	return (f32)window_ptr->mouse_x;
}

f32 se_window_get_mouse_position_y(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_mouse_position_y :: window is null");
	return (f32)window_ptr->mouse_y;
}

void se_window_get_size(const se_window_handle window, u32* out_width, u32* out_height) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_size :: window is null");
	if (!out_width && !out_height) {
		return;
	}

	i32 width = (i32)window_ptr->width;
	i32 height = (i32)window_ptr->height;
	if (window_ptr->handle) {
		glfwGetWindowSize((GLFWwindow*)window_ptr->handle, &width, &height);
	}

	if (out_width) {
		*out_width = (u32)s_max(width, 0);
	}
	if (out_height) {
		*out_height = (u32)s_max(height, 0);
	}
}

void se_window_get_framebuffer_size(const se_window_handle window, u32* out_width, u32* out_height) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_framebuffer_size :: window is null");
	if (!out_width && !out_height) {
		return;
	}

	i32 width = (i32)window_ptr->width;
	i32 height = (i32)window_ptr->height;
	if (window_ptr->handle) {
		glfwGetFramebufferSize((GLFWwindow*)window_ptr->handle, &width, &height);
	}

	if (out_width) {
		*out_width = (u32)s_max(width, 0);
	}
	if (out_height) {
		*out_height = (u32)s_max(height, 0);
	}
}

void se_window_get_content_scale(const se_window_handle window, f32* out_xscale, f32* out_yscale) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_content_scale :: window is null");
	if (!out_xscale && !out_yscale) {
		return;
	}

	f32 xscale = 1.0f;
	f32 yscale = 1.0f;
	if (window_ptr->handle) {
		glfwGetWindowContentScale((GLFWwindow*)window_ptr->handle, &xscale, &yscale);
	}

	if (out_xscale) {
		*out_xscale = xscale;
	}
	if (out_yscale) {
		*out_yscale = yscale;
	}
}

f32 se_window_get_aspect(const se_window_handle window) {
	u32 width = 0;
	u32 height = 0;
	se_window_get_framebuffer_size(window, &width, &height);
	if (height == 0) {
		return 1.0f;
	}
	return (f32)width / (f32)height;
}

b8 se_window_pixel_to_normalized(const se_window_handle window, const f32 x, const f32 y, s_vec2* out_normalized) {
	if (!out_normalized) {
		return false;
	}
	u32 width = 0;
	u32 height = 0;
	se_window_get_size(window, &width, &height);
	if (width <= 1 || height <= 1) {
		return false;
	}
	out_normalized->x = (x / (f32)width) * 2.0f - 1.0f;
	out_normalized->y = 1.0f - (y / (f32)height) * 2.0f;
	return true;
}

b8 se_window_normalized_to_pixel(const se_window_handle window, const f32 nx, const f32 ny, s_vec2* out_pixel) {
	if (!out_pixel) {
		return false;
	}
	u32 width = 0;
	u32 height = 0;
	se_window_get_size(window, &width, &height);
	if (width <= 1 || height <= 1) {
		return false;
	}
	out_pixel->x = (nx * 0.5f + 0.5f) * (f32)width;
	out_pixel->y = (1.0f - (ny * 0.5f + 0.5f)) * (f32)height;
	return true;
}

b8 se_window_window_to_framebuffer(const se_window_handle window, const f32 x, const f32 y, s_vec2* out_framebuffer) {
	if (!out_framebuffer) {
		return false;
	}
	u32 window_w = 0;
	u32 window_h = 0;
	u32 fb_w = 0;
	u32 fb_h = 0;
	se_window_get_size(window, &window_w, &window_h);
	se_window_get_framebuffer_size(window, &fb_w, &fb_h);
	if (window_w <= 1 || window_h <= 1 || fb_w <= 1 || fb_h <= 1) {
		return false;
	}
	out_framebuffer->x = x * ((f32)fb_w / (f32)window_w);
	out_framebuffer->y = y * ((f32)fb_h / (f32)window_h);
	return true;
}

b8 se_window_framebuffer_to_window(const se_window_handle window, const f32 x, const f32 y, s_vec2* out_window) {
	if (!out_window) {
		return false;
	}
	u32 window_w = 0;
	u32 window_h = 0;
	u32 fb_w = 0;
	u32 fb_h = 0;
	se_window_get_size(window, &window_w, &window_h);
	se_window_get_framebuffer_size(window, &fb_w, &fb_h);
	if (window_w <= 1 || window_h <= 1 || fb_w <= 1 || fb_h <= 1) {
		return false;
	}
	out_window->x = x * ((f32)window_w / (f32)fb_w);
	out_window->y = y * ((f32)window_h / (f32)fb_h);
	return true;
}

void se_window_get_diagnostics(const se_window_handle window, se_window_diagnostics* out_diagnostics) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_diagnostics :: window is null");
	if (!out_diagnostics) {
		return;
	}
	*out_diagnostics = window_ptr->diagnostics;
}

void se_window_reset_diagnostics(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_reset_diagnostics :: window is null");
	memset(&window_ptr->diagnostics, 0, sizeof(window_ptr->diagnostics));
}

void se_window_get_mouse_position_normalized(const se_window_handle window, s_vec2* out_mouse_position) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_mouse_position_normalized :: window is null");
	s_assertf(out_mouse_position, "se_window_get_mouse_position_normalized :: out_mouse_position is null");
	*out_mouse_position = s_vec2((window_ptr->mouse_x / window_ptr->width) - .5, window_ptr->mouse_y / window_ptr->height - .5);
	out_mouse_position->x *= 2.;
	out_mouse_position->y *= 2.;
}

void se_window_get_mouse_delta(const se_window_handle window, s_vec2* out_mouse_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_mouse_delta :: window is null");
	s_assertf(out_mouse_delta, "se_window_get_mouse_delta :: out_mouse_delta is null");
	*out_mouse_delta = s_vec2(window_ptr->mouse_dx, window_ptr->mouse_dy);
}

void se_window_get_mouse_delta_normalized(const se_window_handle window, s_vec2* out_mouse_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_mouse_delta_normalized :: window is null");
	s_assertf(out_mouse_delta, "se_window_get_mouse_delta_normalized :: out_mouse_delta is null");
	*out_mouse_delta = s_vec2((window_ptr->mouse_dx / window_ptr->width), (window_ptr->mouse_dy / window_ptr->height));
}

void se_window_get_scroll_delta(const se_window_handle window, s_vec2* out_scroll_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_scroll_delta :: window is null");
	s_assertf(out_scroll_delta, "se_window_get_scroll_delta :: out_scroll_delta is null");
	*out_scroll_delta = s_vec2(window_ptr->scroll_dx, window_ptr->scroll_dy);
}

void se_window_set_cursor_mode(const se_window_handle window, const se_window_cursor_mode mode) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_cursor_mode :: window is null");
	i32 glfw_mode = GLFW_CURSOR_NORMAL;
	switch (mode) {
		case SE_WINDOW_CURSOR_NORMAL: glfw_mode = GLFW_CURSOR_NORMAL; break;
		case SE_WINDOW_CURSOR_HIDDEN: glfw_mode = GLFW_CURSOR_HIDDEN; break;
		case SE_WINDOW_CURSOR_DISABLED: glfw_mode = GLFW_CURSOR_DISABLED; break;
		default: glfw_mode = GLFW_CURSOR_NORMAL; break;
	}
	glfwSetInputMode((GLFWwindow*)window_ptr->handle, GLFW_CURSOR, glfw_mode);
	window_ptr->cursor_mode = mode;
}

se_window_cursor_mode se_window_get_cursor_mode(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_get_cursor_mode :: window is null");
	return window_ptr->cursor_mode;
}

b8 se_window_is_raw_mouse_motion_supported(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_raw_mouse_motion_supported :: window is null");
	return window_ptr->raw_mouse_motion_supported;
}

void se_window_set_raw_mouse_motion(const se_window_handle window, const b8 enabled) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_raw_mouse_motion :: window is null");
	if (!window_ptr->raw_mouse_motion_supported) {
		window_ptr->raw_mouse_motion_enabled = false;
		return;
	}
	const i32 glfw_enabled = enabled ? GLFW_TRUE : GLFW_FALSE;
	glfwSetInputMode((GLFWwindow*)window_ptr->handle, GLFW_RAW_MOUSE_MOTION, glfw_enabled);
	window_ptr->raw_mouse_motion_enabled = enabled;
}

b8 se_window_is_raw_mouse_motion_enabled(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_is_raw_mouse_motion_enabled :: window is null");
	return window_ptr->raw_mouse_motion_enabled;
}

b8 se_window_should_close(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_should_close :: window is null");
	return glfwWindowShouldClose((GLFWwindow*)window_ptr->handle);
}

void se_window_set_should_close(const se_window_handle window, const b8 should_close) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_should_close :: window is null");
	glfwSetWindowShouldClose((GLFWwindow*)window_ptr->handle, should_close);
}

void se_window_set_exit_keys(const se_window_handle window, se_key_combo* keys) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_exit_keys :: window is null");
	s_assertf(keys, "se_window_set_exit_keys :: keys is null");
	window_ptr->exit_keys = keys;
	window_ptr->use_exit_key = false;
}

void se_window_set_exit_key(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_exit_key :: window is null");
	window_ptr->exit_key = key;
	window_ptr->use_exit_key = true;
}

void se_window_check_exit_keys(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_check_exit_keys :: window is null");
	if (window_ptr->use_exit_key) {
		if (window_ptr->exit_key != SE_KEY_UNKNOWN && se_window_is_key_down(window, window_ptr->exit_key)) {
			glfwSetWindowShouldClose((GLFWwindow*)window_ptr->handle, GLFW_TRUE);
			return;
		}
	}
	if (!window_ptr->exit_keys) {
		return;
	}
	se_key_combo* keys = window_ptr->exit_keys;
	if (s_array_get_size(keys) == 0) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(keys); ++i) {
		se_key* current_key = s_array_get(keys, s_array_handle(keys, (u32)i));
		if (!se_window_is_key_down(window, *current_key)) {
			return;
		}
	}
	glfwSetWindowShouldClose((GLFWwindow*)window_ptr->handle, GLFW_TRUE);
}

f64 se_window_get_delta_time(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr->time.delta;
}

f64 se_window_get_fps(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return 1. / window_ptr->time.delta;
}

f64 se_window_get_time(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr->time.current;
}

void se_window_set_target_fps(const se_window_handle window, const u16 fps) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_target_fps :: window is null");
	if (fps == 0) {
		se_debug_log(SE_DEBUG_LEVEL_WARN, SE_DEBUG_CATEGORY_WINDOW, "Ignored target_fps=0");
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	window_ptr->target_fps = fps;
	se_set_last_error(SE_RESULT_OK);
}

// TODO: move somewhere else and optimize it if it used a lot
i32 se_hash(const c8* str1, const i32 len1, const c8* str2, const i32 len2) {
	u32 hash = 5381;
	for (i32 i = 0; i < len1; i++) {
		hash = ((hash << 5) + hash) + str1[i];
	}
	for (i32 i = 0; i < len2; i++) {
		hash = ((hash << 5) + hash) + str2[i];
	}
	return hash;
}

i32 se_window_register_input_event(const se_window_handle window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_register_input_event :: window is null");
	s_assertf(box, "se_window_register_input_event :: box is null");

	if (s_array_get_size(&window_ptr->input_events) >= SE_MAX_INPUT_EVENTS) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return -1;
	}
	s_handle event_handle = s_array_increment(&window_ptr->input_events);
	se_input_event* new_event = s_array_get(&window_ptr->input_events, event_handle);
	s_assertf(new_event, "se_window_register_input_event :: Array is full");
	c8 event_ptr[16];
	c8 box_ptr[16];
	sprintf(event_ptr, "%p", new_event);
	sprintf(box_ptr, "%p", box);
	new_event->id = se_hash(event_ptr, strlen(event_ptr), box_ptr, strlen(box_ptr));
	new_event->box = *box;
	new_event->depth = depth;
	new_event->active = true;
	new_event->on_interact_callback = on_interact_callback;
	new_event->on_stop_interact_callback = on_stop_interact_callback;
	new_event->callback_data = callback_data;
	return new_event->id;
}

void se_window_update_input_event(const i32 input_event_id, const se_window_handle window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_update_input_event :: window is null");
	s_assertf(box, "se_window_update_input_event :: box is null");

	se_input_event* found_event = NULL;
	for (sz i = 0; i < s_array_get_size(&window_ptr->input_events); ++i) {
		se_input_event* current_event = s_array_get(&window_ptr->input_events, s_array_handle(&window_ptr->input_events, (u32)i));
		if (current_event->id == input_event_id) {
			found_event = current_event;
			break;
		}
	}
	if (found_event) {
		found_event->box = *box;
		found_event->depth = depth;
		found_event->on_interact_callback = on_interact_callback;
		found_event->on_stop_interact_callback = on_stop_interact_callback;
		found_event->callback_data = callback_data;
	} else {
		printf("se_window_update_input_event :: failed to find event with id: %d\n", input_event_id);
	}
}

void se_window_register_resize_event(const se_window_handle window, se_resize_event_callback callback, void* data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_register_resize_event :: window is null");
	s_assertf(callback, "se_window_register_resize_event :: callback is null");
	if (s_array_get_size(&window_ptr->resize_handles) >= SE_MAX_RESIZE_HANDLE) {
		return;
	}
	s_handle handle = s_array_increment(&window_ptr->resize_handles);
	se_resize_handle* new_event = s_array_get(&window_ptr->resize_handles, handle);
	new_event->callback = callback;
	new_event->data = data;
}

void se_window_set_text_callback(const se_window_handle window, se_window_text_callback callback, void* data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_set_text_callback :: window is null");
	window_ptr->text_callback = callback;
	window_ptr->text_callback_data = data;
}

void se_window_emit_text(const se_window_handle window, const c8* utf8_text) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_emit_text :: window is null");
	if (!utf8_text || !window_ptr->text_callback) {
		return;
	}
	window_ptr->text_callback(window, utf8_text, window_ptr->text_callback_data);
}

void se_window_destroy(const se_window_handle window) {
	if (window == S_HANDLE_NULL) {
		return;
	}
	se_window_registry_refresh_owner_context();
	se_context* current_context = se_current_context();
	sz registry_index = 0;
	se_window_registry_entry* registry_entry = se_window_registry_find(current_context, window, &registry_index);
	if (registry_entry == NULL || registry_entry->ctx == NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return;
	}

	se_context* owner_context = registry_entry->ctx;
	const se_window_handle owner_window = registry_entry->window;
	se_window* window_ptr = s_array_get(&owner_context->windows, owner_window);
	if (window_ptr == NULL) {
		s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)registry_index));
		se_window_backend_shutdown_if_unused();
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return;
	}

	if (s_array_get_size(&owner_context->windows) == 1 && se_window_context_has_live_resources(owner_context)) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_WINDOW,
			"Refusing to destroy last window while context resources are still alive; call se_context_destroy() instead.");
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return;
	}

	se_context* previous_context = se_push_tls_context(owner_context);
	GLFWwindow* glfw_window = (GLFWwindow*)window_ptr->handle;
	if (window_ptr->quad.vao != 0) {
		se_quad_destroy(&window_ptr->quad);
	}
	if (window_ptr->shader != S_HANDLE_NULL &&
		s_array_get(&owner_context->shaders, window_ptr->shader) != NULL) {
		se_shader_destroy(window_ptr->shader);
	}
	window_ptr->shader = S_HANDLE_NULL;
	s_array_clear(&window_ptr->input_events);
	s_array_clear(&window_ptr->resize_handles);

	if (glfw_window) {
		se_window_user_ptr* user_ptr = (se_window_user_ptr*)glfwGetWindowUserPointer(glfw_window);
		if (user_ptr) {
			free(user_ptr);
		}
		glfwDestroyWindow(glfw_window);
	}
	window_ptr->handle = NULL;

	s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)registry_index));
	s_array_remove(&owner_context->windows, owner_window);

	if (current_conext_window == glfw_window) {
		current_conext_window = NULL;
		glfwMakeContextCurrent(NULL);
	}

	se_pop_tls_context(previous_context);
	se_window_backend_shutdown_if_unused();
	se_set_last_error(SE_RESULT_OK);
}

void se_window_destroy_all(void){
	while (s_array_get_size(&windows_registry) > 0) {
		se_window_registry_entry* entry = s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)(s_array_get_size(&windows_registry) - 1)));
		if (!entry || entry->window == S_HANDLE_NULL) {
			s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)(s_array_get_size(&windows_registry) - 1)));
			continue;
		}
		const sz windows_before = s_array_get_size(&windows_registry);
		se_window_destroy(entry->window);
		if (s_array_get_size(&windows_registry) == windows_before) {
			se_debug_log(SE_DEBUG_LEVEL_WARN, SE_DEBUG_CATEGORY_WINDOW, "se_window_destroy_all stopped because teardown made no progress");
			break;
		}
	}
	se_window_backend_shutdown_if_unused();
}

#endif // SE_WINDOW_BACKEND_GLFW
