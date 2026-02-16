// Syphax-Engine - Ougi Washi

#if defined(SE_WINDOW_BACKEND_TERMINAL)

#include "se_window.h"
#include "se_debug.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

typedef s_array(se_window_handle, se_windows_registry);
static se_windows_registry windows_registry = {0};

static se_window* se_window_from_handle(se_context* context, const se_window_handle window) {
	return s_array_get(&context->windows, window);
}

typedef struct {
	c8 glyph;
	s_vec4 fg_color;
	s_vec4 bg_color;
} se_window_terminal_cell;

typedef struct {
	u32 columns;
	u32 rows;
	s_array(se_window_terminal_cell, cells);
	b8 present_initialized;
} se_window_terminal_surface;

typedef struct {
	b8 initialized;
	b8 interactive;
	struct termios original_termios;
	i32 original_stdin_flags;
} se_window_terminal_io_state;

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

static se_window_terminal_io_state g_terminal_io = {0};
static se_window_terminal_input_parser g_terminal_parser = {0};
static volatile sig_atomic_t g_terminal_restore_emitted = 0;
static b8 g_terminal_restore_hooks_installed = false;

static void se_window_terminal_input_disable(void);

static void se_window_terminal_write_restore_sequence(void) {
	if (g_terminal_restore_emitted) {
		return;
	}
	g_terminal_restore_emitted = 1;
	static const c8 restore_sequence[] = "\033[?1000l\033[?1002l\033[?1006l\033[0m\033[?25h";
	(void)write(STDOUT_FILENO, restore_sequence, sizeof(restore_sequence) - 1);
}

static void se_window_terminal_restore_on_exit(void) {
	se_window_terminal_input_disable();
	se_window_terminal_write_restore_sequence();
}

static void se_window_terminal_restore_on_signal(const i32 signal_id) {
	se_window_terminal_input_disable();
	se_window_terminal_write_restore_sequence();
	_exit(128 + signal_id);
}

static void se_window_terminal_install_restore_hooks(void) {
	if (g_terminal_restore_hooks_installed || !g_terminal_io.interactive) {
		return;
	}
	(void)atexit(se_window_terminal_restore_on_exit);
	(void)signal(SIGINT, se_window_terminal_restore_on_signal);
	(void)signal(SIGTERM, se_window_terminal_restore_on_signal);
#ifdef SIGHUP
	(void)signal(SIGHUP, se_window_terminal_restore_on_signal);
#endif
#ifdef SIGABRT
	(void)signal(SIGABRT, se_window_terminal_restore_on_signal);
#endif
	g_terminal_restore_hooks_installed = true;
}

static void se_window_terminal_resolve_size(const u32 requested_width, const u32 requested_height, u32* out_width, u32* out_height) {
	u32 width = requested_width;
	u32 height = requested_height;

	// Most examples pass pixel dimensions. For terminal backend, prefer actual terminal grid size.
	if (requested_width > 300 || requested_height > 120) {
		struct winsize ws = {0};
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
			width = (u32)ws.ws_col;
			height = (u32)ws.ws_row;
		} else {
			width = 120;
			height = 40;
		}
	}

	width = s_max(1, s_min(width, 240));
	height = s_max(1, s_min(height, 100));
	*out_width = width;
	*out_height = height;
}

static b8 se_window_terminal_is_interactive(void) {
	return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

static void se_window_terminal_input_enable(void) {
	if (g_terminal_io.initialized) {
		return;
	}
	g_terminal_io.initialized = true;
	g_terminal_io.interactive = se_window_terminal_is_interactive();
	g_terminal_io.original_stdin_flags = -1;
	g_terminal_restore_emitted = 0;
	if (!g_terminal_io.interactive) {
		return;
	}
	se_window_terminal_install_restore_hooks();
	if (tcgetattr(STDIN_FILENO, &g_terminal_io.original_termios) != 0) {
		g_terminal_io.interactive = false;
		return;
	}
	struct termios raw = g_terminal_io.original_termios;
	raw.c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= (tcflag_t) ~(OPOST);
	raw.c_cflag |= (tcflag_t)(CS8);
	raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
		g_terminal_io.interactive = false;
		return;
	}
	g_terminal_io.original_stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	if (g_terminal_io.original_stdin_flags >= 0) {
		(void)fcntl(STDIN_FILENO, F_SETFL, g_terminal_io.original_stdin_flags | O_NONBLOCK);
	}

	// Enable UTF-8 SGR mouse tracking for button, drag and scroll events.
	fputs("\033[?1000h\033[?1002h\033[?1006h", stdout);
	fflush(stdout);
}

static void se_window_terminal_input_disable(void) {
	if (!g_terminal_io.initialized) {
		return;
	}
	if (g_terminal_io.interactive) {
		se_window_terminal_write_restore_sequence();
		if (g_terminal_io.original_stdin_flags >= 0) {
			(void)fcntl(STDIN_FILENO, F_SETFL, g_terminal_io.original_stdin_flags);
		}
		(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_terminal_io.original_termios);
	}
	memset(&g_terminal_io, 0, sizeof(g_terminal_io));
	memset(&g_terminal_parser, 0, sizeof(g_terminal_parser));
}

static se_window_terminal_cell se_window_terminal_default_cell(void) {
	se_window_terminal_cell cell = {0};
	cell.glyph = ' ';
	cell.fg_color = s_vec4(1.0f, 1.0f, 1.0f, 1.0f);
	cell.bg_color = s_vec4(0.0f, 0.0f, 0.0f, 1.0f);
	return cell;
}

static u8 se_window_terminal_color_to_u8(const f32 value) {
	f32 clamped = value;
	if (clamped < 0.0f) {
		clamped = 0.0f;
	} else if (clamped > 1.0f) {
		clamped = 1.0f;
	}
	return (u8)lroundf(clamped * 255.0f);
}

static se_window_terminal_surface* se_window_terminal_surface_get_or_create(se_window* window_ptr) {
	if (!window_ptr) {
		return NULL;
	}
	if (window_ptr->handle != NULL) {
		return (se_window_terminal_surface*)window_ptr->handle;
	}
	se_window_terminal_surface* surface = malloc(sizeof(*surface));
	if (!surface) {
		return NULL;
	}
	memset(surface, 0, sizeof(*surface));
	surface->columns = s_max(1, window_ptr->width);
	surface->rows = s_max(1, window_ptr->height);
	s_array_init(&surface->cells);
	s_array_reserve(&surface->cells, (sz)(surface->columns * surface->rows));
	const se_window_terminal_cell default_cell = se_window_terminal_default_cell();
	for (u32 i = 0; i < surface->columns * surface->rows; ++i) {
		s_array_add(&surface->cells, default_cell);
	}
	window_ptr->handle = surface;
	return surface;
}

static void se_window_terminal_surface_resize(se_window* window_ptr, const u32 columns, const u32 rows) {
	se_window_terminal_surface* surface = se_window_terminal_surface_get_or_create(window_ptr);
	if (!surface) {
		return;
	}
	if (surface->columns == columns && surface->rows == rows) {
		return;
	}
	surface->columns = columns;
	surface->rows = rows;
	s_array_clear(&surface->cells);
	s_array_init(&surface->cells);
	s_array_reserve(&surface->cells, (sz)(columns * rows));
	const se_window_terminal_cell default_cell = se_window_terminal_default_cell();
	for (u32 i = 0; i < columns * rows; ++i) {
		s_array_add(&surface->cells, default_cell);
	}
	surface->present_initialized = false;
}

static void se_window_terminal_sync_size(se_window* window_ptr) {
	if (!window_ptr || !g_terminal_io.interactive) {
		return;
	}
	struct winsize ws = {0};
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0 || ws.ws_row == 0) {
		return;
	}
	u32 resolved_width = s_max(1, s_min((u32)ws.ws_col, 240));
	u32 resolved_height = s_max(1, s_min((u32)ws.ws_row, 100));
	if (resolved_width == window_ptr->width && resolved_height == window_ptr->height) {
		return;
	}
	window_ptr->width = resolved_width;
	window_ptr->height = resolved_height;
	se_window_terminal_surface_resize(window_ptr, resolved_width, resolved_height);
}

static void se_window_terminal_clear(se_window* window_ptr, const s_vec4* bg_color) {
	se_window_terminal_surface* surface = se_window_terminal_surface_get_or_create(window_ptr);
	if (!surface) {
		return;
	}
	se_window_terminal_cell clear_cell = se_window_terminal_default_cell();
	if (bg_color) {
		clear_cell.bg_color = *bg_color;
	}
	for (u32 i = 0; i < surface->columns * surface->rows; ++i) {
		se_window_terminal_cell* slot = s_array_get(&surface->cells, s_array_handle(&surface->cells, i));
		if (!slot) {
			continue;
		}
		*slot = clear_cell;
	}
}

static void se_window_terminal_present(se_window* window_ptr) {
	se_window_terminal_surface* surface = se_window_terminal_surface_get_or_create(window_ptr);
	if (!surface) {
		return;
	}
	if (!surface->present_initialized) {
		fputs("\033[2J", stdout);
		surface->present_initialized = true;
	}
	fputs("\033[H\033[?25l", stdout);

	u8 current_fg[3] = {255, 255, 255};
	u8 current_bg[3] = {0, 0, 0};
	b8 has_color = false;
	for (u32 row = 0; row < surface->rows; ++row) {
		for (u32 column = 0; column < surface->columns; ++column) {
			const u32 index = row * surface->columns + column;
			const se_window_terminal_cell* cell = s_array_get(&surface->cells, s_array_handle(&surface->cells, index));
			if (!cell) {
				putchar(' ');
				continue;
			}
			u8 fg[3] = {
				se_window_terminal_color_to_u8(cell->fg_color.x),
				se_window_terminal_color_to_u8(cell->fg_color.y),
				se_window_terminal_color_to_u8(cell->fg_color.z)};
			u8 bg[3] = {
				se_window_terminal_color_to_u8(cell->bg_color.x),
				se_window_terminal_color_to_u8(cell->bg_color.y),
				se_window_terminal_color_to_u8(cell->bg_color.z)};
			if (!has_color ||
				fg[0] != current_fg[0] || fg[1] != current_fg[1] || fg[2] != current_fg[2] ||
				bg[0] != current_bg[0] || bg[1] != current_bg[1] || bg[2] != current_bg[2]) {
				c8 color_ansi[72] = {0};
				const i32 color_ansi_len = snprintf(
					color_ansi,
					sizeof(color_ansi),
					"\033[38;2;%u;%u;%um\033[48;2;%u;%u;%um",
					(unsigned)fg[0],
					(unsigned)fg[1],
					(unsigned)fg[2],
					(unsigned)bg[0],
					(unsigned)bg[1],
					(unsigned)bg[2]);
				if (color_ansi_len > 0) {
					fputs(color_ansi, stdout);
				}
				current_fg[0] = fg[0];
				current_fg[1] = fg[1];
				current_fg[2] = fg[2];
				current_bg[0] = bg[0];
				current_bg[1] = bg[1];
				current_bg[2] = bg[2];
				has_color = true;
			}
			const c8 glyph = cell->glyph == '\0' ? ' ' : cell->glyph;
			putchar(glyph);
		}
		if (row + 1 < surface->rows) {
			putchar('\n');
		}
	}
	fputs("\033[0m", stdout);
	fflush(stdout);
}

static void se_window_terminal_shutdown_surface(se_window* window_ptr) {
	if (!window_ptr || !window_ptr->handle) {
		return;
	}
	se_window_terminal_surface* surface = (se_window_terminal_surface*)window_ptr->handle;
	s_array_clear(&surface->cells);
	free(surface);
	window_ptr->handle = NULL;
	fputs("\033[0m\033[?25h", stdout);
	fflush(stdout);
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

static void se_window_terminal_emit_key_press(const se_window_handle window, se_window* window_ptr, const se_key key) {
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT || key == SE_KEY_UNKNOWN) {
		return;
	}
	window_ptr->keys[key] = true;
	window_ptr->diagnostics.key_events++;
	(void)window;
}

static void se_window_terminal_emit_text_char(const se_window_handle window, se_window* window_ptr, const c8 ch) {
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

static void se_window_terminal_handle_mouse_sgr(
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

	const f64 next_x = (f64)s_max(0, x - 1);
	const f64 next_y = (f64)s_max(0, y - 1);
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

static void se_window_terminal_handle_csi(
	const se_window_handle window,
	se_window* window_ptr,
	const c8* csi_data,
	const c8 final_char) {
	if (!window_ptr || !csi_data) {
		return;
	}
	if (csi_data[0] == '<' && (final_char == 'M' || final_char == 'm')) {
		se_window_terminal_handle_mouse_sgr(window_ptr, csi_data, final_char);
		return;
	}

	if (final_char == 'A') {
		se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_UP);
		return;
	}
	if (final_char == 'B') {
		se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_DOWN);
		return;
	}
	if (final_char == 'C') {
		se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_RIGHT);
		return;
	}
	if (final_char == 'D') {
		se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_LEFT);
		return;
	}
	if (final_char == 'H') {
		se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_HOME);
		return;
	}
	if (final_char == 'F') {
		se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_END);
		return;
	}
	if (final_char == 'Z') {
		se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_TAB);
		return;
	}
	if (final_char != '~') {
		return;
	}
	const i32 code = atoi(csi_data);
	switch (code) {
		case 1:
		case 7:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_HOME);
			break;
		case 2:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_INSERT);
			break;
		case 3:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_DELETE);
			break;
		case 4:
		case 8:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_END);
			break;
		case 5:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_PAGE_UP);
			break;
		case 6:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_PAGE_DOWN);
			break;
		case 15:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F5);
			break;
		case 17:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F6);
			break;
		case 18:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F7);
			break;
		case 19:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F8);
			break;
		case 20:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F9);
			break;
		case 21:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F10);
			break;
		case 23:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F11);
			break;
		case 24:
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F12);
			break;
		default:
			break;
	}
}

static void se_window_terminal_handle_input_byte(const se_window_handle window, se_window* window_ptr, const u8 byte) {
	if (!window_ptr) {
		return;
	}
	switch (g_terminal_parser.state) {
		case SE_WINDOW_TERMINAL_PARSE_NORMAL: {
			if (byte == 0x1B) {
				g_terminal_parser.state = SE_WINDOW_TERMINAL_PARSE_ESC;
				return;
			}
			if (byte >= 1 && byte <= 26) {
				// Ctrl+A..Ctrl+Z produce 0x01..0x1A in canonical raw terminal input.
				se_window_terminal_emit_key_press(window, window_ptr, (se_key)(SE_KEY_A + (byte - 1)));
				return;
			}
			if (byte == '\r' || byte == '\n') {
				se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_ENTER);
				return;
			}
			if (byte == '\t') {
				se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_TAB);
				return;
			}
			if (byte == 0x7F || byte == 0x08) {
				se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_BACKSPACE);
				return;
			}
			const c8 ch = (c8)byte;
			const se_key mapped_key = se_window_terminal_map_ascii_key(ch);
			if (mapped_key != SE_KEY_UNKNOWN) {
				se_window_terminal_emit_key_press(window, window_ptr, mapped_key);
			}
			se_window_terminal_emit_text_char(window, window_ptr, ch);
			return;
		}
		case SE_WINDOW_TERMINAL_PARSE_ESC: {
			if (byte == '[') {
				g_terminal_parser.state = SE_WINDOW_TERMINAL_PARSE_CSI;
				g_terminal_parser.csi_len = 0;
				g_terminal_parser.csi_buffer[0] = '\0';
				return;
			}
			if (byte == 'O') {
				g_terminal_parser.state = SE_WINDOW_TERMINAL_PARSE_SS3;
				return;
			}
			se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_ESCAPE);
			g_terminal_parser.state = SE_WINDOW_TERMINAL_PARSE_NORMAL;
			se_window_terminal_handle_input_byte(window, window_ptr, byte);
			return;
		}
		case SE_WINDOW_TERMINAL_PARSE_CSI: {
			if (g_terminal_parser.csi_len + 1 < sizeof(g_terminal_parser.csi_buffer)) {
				g_terminal_parser.csi_buffer[g_terminal_parser.csi_len++] = (c8)byte;
				g_terminal_parser.csi_buffer[g_terminal_parser.csi_len] = '\0';
			}
			if (byte >= 0x40 && byte <= 0x7E) {
				if (g_terminal_parser.csi_len > 0) {
					g_terminal_parser.csi_buffer[g_terminal_parser.csi_len - 1] = '\0';
					se_window_terminal_handle_csi(
						window,
						window_ptr,
						g_terminal_parser.csi_buffer,
						(c8)byte);
				}
				g_terminal_parser.state = SE_WINDOW_TERMINAL_PARSE_NORMAL;
				g_terminal_parser.csi_len = 0;
				g_terminal_parser.csi_buffer[0] = '\0';
			}
			return;
		}
		case SE_WINDOW_TERMINAL_PARSE_SS3: {
			switch (byte) {
				case 'P':
					se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F1);
					break;
				case 'Q':
					se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F2);
					break;
				case 'R':
					se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F3);
					break;
				case 'S':
					se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_F4);
					break;
				case 'H':
					se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_HOME);
					break;
				case 'F':
					se_window_terminal_emit_key_press(window, window_ptr, SE_KEY_END);
					break;
				default:
					break;
			}
			g_terminal_parser.state = SE_WINDOW_TERMINAL_PARSE_NORMAL;
			return;
		}
		default:
			g_terminal_parser.state = SE_WINDOW_TERMINAL_PARSE_NORMAL;
			return;
	}
}

se_window_handle se_window_create(const char* title, const u32 width, const u32 height) {
	se_context* context = se_current_context();
	if (!context || !title) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&context->windows) == 0) {
		s_array_init(&context->windows);
	}

	se_window_handle window_handle = s_array_increment(&context->windows);
	se_window* new_window = s_array_get(&context->windows, window_handle);

	memset(new_window, 0, sizeof(se_window));
	new_window->context = context;
	se_window_terminal_resolve_size(width, height, &new_window->width, &new_window->height);
	new_window->handle = NULL;
	new_window->cursor_mode = SE_WINDOW_CURSOR_NORMAL;
	new_window->raw_mouse_motion_supported = false;
	new_window->raw_mouse_motion_enabled = false;
	new_window->vsync_enabled = false;
	new_window->should_close = false;
	new_window->target_fps = 30;
	new_window->time.current = 0.0;

	if (s_array_get_capacity(&windows_registry) == 0) {
		s_array_init(&windows_registry);
	}
	s_array_add(&windows_registry, window_handle);
	se_window_terminal_input_enable();

	se_log("se_window_create :: [terminal] window: %s (%u x %u)", title, new_window->width, new_window->height);
	se_set_last_error(SE_RESULT_OK);
	return window_handle;
}

void se_window_attach_render(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_attach_render :: window is null");
	window_ptr->context = context;
}

void se_window_update(const se_window_handle window) {
	se_debug_frame_begin();
	se_debug_trace_begin("window_update");
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	se_window_terminal_sync_size(window_ptr);
	se_window_check_exit_keys(window);
	memcpy(window_ptr->keys_prev, window_ptr->keys, sizeof(window_ptr->keys));
	memcpy(window_ptr->mouse_buttons_prev, window_ptr->mouse_buttons, sizeof(window_ptr->mouse_buttons));
	window_ptr->mouse_dx = 0.0;
	window_ptr->mouse_dy = 0.0;
	window_ptr->scroll_dx = 0.0;
	window_ptr->scroll_dy = 0.0;
	window_ptr->time.last_frame = window_ptr->time.current;
	window_ptr->time.current += 1.0 / (f64)window_ptr->target_fps;
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
	(void)window;
}

void se_window_render_quad(const se_window_handle window) {
	(void)window;
}

void se_window_render_screen(const se_window_handle window) {
	se_debug_trace_begin("window_present");
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		se_debug_trace_end("window_present");
		se_debug_frame_end();
		return;
	}
	se_window_terminal_present(window_ptr);
	window_ptr->diagnostics.frames_presented++;
	window_ptr->diagnostics.last_present_duration = 0.0;
	window_ptr->diagnostics.last_sleep_duration = 0.0;
	se_debug_trace_end("window_present");
	se_debug_frame_end();
}

void se_window_set_vsync(const se_window_handle window, const b8 enabled) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->vsync_enabled = enabled;
}

b8 se_window_is_vsync_enabled(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return false;
	}
	return window_ptr->vsync_enabled;
}

void se_window_present(const se_window_handle window) {
	se_window_render_screen(window);
}

void se_window_present_frame(const se_window_handle window, const s_vec4* clear_color) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	const s_vec4 resolved_clear = clear_color ? *clear_color : s_vec4(0.0f, 0.0f, 0.0f, 1.0f);
	se_window_terminal_clear(window_ptr, &resolved_clear);
	se_window_render_screen(window);
}

void se_window_poll_events() {
	if (!g_terminal_io.interactive || s_array_get_size(&windows_registry) == 0) {
		return;
	}
	se_context* context = se_current_context();
	if (!context) {
		return;
	}

	se_window_handle* first_handle_ptr =
		s_array_get(&windows_registry, s_array_handle(&windows_registry, 0));
	if (!first_handle_ptr || *first_handle_ptr == S_HANDLE_NULL) {
		return;
	}
	se_window* window_ptr = se_window_from_handle(context, *first_handle_ptr);
	if (!window_ptr) {
		return;
	}

	// Terminal key input is event-based; clear key-down state before consuming new bytes.
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
			se_window_terminal_handle_input_byte(*first_handle_ptr, window_ptr, buffer[i]);
		}
	}

	if (g_terminal_parser.state == SE_WINDOW_TERMINAL_PARSE_ESC) {
		se_window_terminal_emit_key_press(*first_handle_ptr, window_ptr, SE_KEY_ESCAPE);
		g_terminal_parser.state = SE_WINDOW_TERMINAL_PARSE_NORMAL;
	}
}

b8 se_window_is_key_down(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window_ptr->keys[key];
}

b8 se_window_is_key_pressed(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window_ptr->keys[key] && !window_ptr->keys_prev[key];
}

b8 se_window_is_key_released(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return !window_ptr->keys[key] && window_ptr->keys_prev[key];
}

b8 se_window_is_mouse_down(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window_ptr->mouse_buttons[button];
}

b8 se_window_is_mouse_pressed(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window_ptr->mouse_buttons[button] && !window_ptr->mouse_buttons_prev[button];
}

b8 se_window_is_mouse_released(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return !window_ptr->mouse_buttons[button] && window_ptr->mouse_buttons_prev[button];
}

void se_window_clear_input_state(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
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
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return;
	}
	window_ptr->keys[key] = down;
}

void se_window_inject_mouse_button_state(const se_window_handle window, const se_mouse_button button, const b8 down) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return;
	}
	window_ptr->mouse_buttons[button] = down;
}

void se_window_inject_mouse_position(const se_window_handle window, const f32 x, const f32 y) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
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
	if (!window_ptr) {
		return;
	}
	window_ptr->scroll_dx = x;
	window_ptr->scroll_dy = y;
}

f32 se_window_get_mouse_position_x(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? (f32)window_ptr->mouse_x : 0.0f;
}

f32 se_window_get_mouse_position_y(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? (f32)window_ptr->mouse_y : 0.0f;
}

void se_window_get_size(const se_window_handle window, u32* out_width, u32* out_height) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	if (out_width) {
		*out_width = window_ptr->width;
	}
	if (out_height) {
		*out_height = window_ptr->height;
	}
}

void se_window_get_framebuffer_size(const se_window_handle window, u32* out_width, u32* out_height) {
	se_window_get_size(window, out_width, out_height);
}

void se_window_get_content_scale(const se_window_handle window, f32* out_xscale, f32* out_yscale) {
	(void)window;
	if (out_xscale) {
		*out_xscale = 1.0f;
	}
	if (out_yscale) {
		*out_yscale = 1.0f;
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
	*out_framebuffer = s_vec2(x, y);
	(void)window;
	return true;
}

b8 se_window_framebuffer_to_window(const se_window_handle window, const f32 x, const f32 y, s_vec2* out_window) {
	if (!out_window) {
		return false;
	}
	*out_window = s_vec2(x, y);
	(void)window;
	return true;
}

void se_window_get_diagnostics(const se_window_handle window, se_window_diagnostics* out_diagnostics) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_diagnostics) {
		return;
	}
	*out_diagnostics = window_ptr->diagnostics;
}

void se_window_reset_diagnostics(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	memset(&window_ptr->diagnostics, 0, sizeof(window_ptr->diagnostics));
}

void se_window_get_mouse_position_normalized(const se_window_handle window, s_vec2* out_mouse_position) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_mouse_position) {
		return;
	}
	if (!se_window_pixel_to_normalized(window, (f32)window_ptr->mouse_x, (f32)window_ptr->mouse_y, out_mouse_position)) {
		*out_mouse_position = s_vec2(0.0f, 0.0f);
	}
}

void se_window_get_mouse_delta(const se_window_handle window, s_vec2* out_mouse_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_mouse_delta) {
		return;
	}
	*out_mouse_delta = s_vec2(window_ptr->mouse_dx, window_ptr->mouse_dy);
}

void se_window_get_mouse_delta_normalized(const se_window_handle window, s_vec2* out_mouse_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_mouse_delta) {
		return;
	}
	*out_mouse_delta = s_vec2((window_ptr->mouse_dx / window_ptr->width), (window_ptr->mouse_dy / window_ptr->height));
}

void se_window_get_scroll_delta(const se_window_handle window, s_vec2* out_scroll_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_scroll_delta) {
		return;
	}
	*out_scroll_delta = s_vec2(window_ptr->scroll_dx, window_ptr->scroll_dy);
}

void se_window_set_cursor_mode(const se_window_handle window, const se_window_cursor_mode mode) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->cursor_mode = mode;
}

se_window_cursor_mode se_window_get_cursor_mode(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return SE_WINDOW_CURSOR_NORMAL;
	}
	return window_ptr->cursor_mode;
}

b8 se_window_is_raw_mouse_motion_supported(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return false;
	}
	return window_ptr->raw_mouse_motion_supported;
}

void se_window_set_raw_mouse_motion(const se_window_handle window, const b8 enabled) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	if (!window_ptr->raw_mouse_motion_supported) {
		window_ptr->raw_mouse_motion_enabled = false;
		return;
	}
	window_ptr->raw_mouse_motion_enabled = enabled;
}

b8 se_window_is_raw_mouse_motion_enabled(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return false;
	}
	return window_ptr->raw_mouse_motion_enabled;
}

b8 se_window_should_close(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? window_ptr->should_close : true;
}

void se_window_set_should_close(const se_window_handle window, const b8 should_close) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->should_close = should_close;
}

void se_window_set_exit_keys(const se_window_handle window, se_key_combo* keys) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !keys) {
		return;
	}
	window_ptr->exit_keys = keys;
	window_ptr->use_exit_key = false;
}

void se_window_set_exit_key(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->exit_key = key;
	window_ptr->use_exit_key = true;
}

void se_window_check_exit_keys(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	if (window_ptr->use_exit_key) {
		if (window_ptr->exit_key != SE_KEY_UNKNOWN && se_window_is_key_down(window, window_ptr->exit_key)) {
			window_ptr->should_close = true;
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
	window_ptr->should_close = true;
}

f64 se_window_get_delta_time(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? window_ptr->time.delta : 0.0;
}

f64 se_window_get_fps(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? (1.0 / window_ptr->time.delta) : 0.0;
}

f64 se_window_get_time(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? window_ptr->time.current : 0.0;
}

void se_window_set_target_fps(const se_window_handle window, const u16 fps) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	if (fps == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	window_ptr->target_fps = fps;
	se_set_last_error(SE_RESULT_OK);
}

i32 se_window_register_input_event(const se_window_handle window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	(void)window;
	(void)box;
	(void)depth;
	(void)on_interact_callback;
	(void)on_stop_interact_callback;
	(void)callback_data;
	return -1;
}

void se_window_update_input_event(const i32 input_event_id, const se_window_handle window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	(void)input_event_id;
	(void)window;
	(void)box;
	(void)depth;
	(void)on_interact_callback;
	(void)on_stop_interact_callback;
	(void)callback_data;
}

void se_window_register_resize_event(const se_window_handle window, se_resize_event_callback callback, void* data) {
	(void)window;
	(void)callback;
	(void)data;
}

void se_window_set_text_callback(const se_window_handle window, se_window_text_callback callback, void* data) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->text_callback = callback;
	window_ptr->text_callback_data = data;
}

void se_window_emit_text(const se_window_handle window, const c8* utf8_text) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !utf8_text || !window_ptr->text_callback) {
		return;
	}
	window_ptr->text_callback(window, utf8_text, window_ptr->text_callback_data);
}

void se_window_destroy(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	se_window_terminal_shutdown_surface(window_ptr);
	if (window_ptr->quad.vao != 0) {
		se_quad_destroy(&window_ptr->quad);
	}
	for (sz i = 0; i < s_array_get_size(&windows_registry); i++) {
		se_window_handle slot = *s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (slot == window) {
			s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)i));
			break;
		}
	}
	s_array_remove(&context->windows, window);
	if (s_array_get_size(&windows_registry) == 0) {
		se_window_terminal_input_disable();
		s_array_clear(&windows_registry);
	}
}

void se_window_destroy_all(void){
	while (s_array_get_size(&windows_registry) > 0) {
		se_window_handle window_handle = *s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)(s_array_get_size(&windows_registry) - 1)));
		if (window_handle == S_HANDLE_NULL) {
			s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)(s_array_get_size(&windows_registry) - 1)));
			continue;
		}
		se_window_destroy(window_handle);
	}
}

#endif // SE_WINDOW_BACKEND_TERMINAL
