// Syphax-Engine - Ougi Washi

#include "se_ui.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 1600
#define HEIGHT 900

typedef s_array(se_ui_widget_handle, ui_widget_handles);

typedef struct {
	se_window_handle window;
	se_ui_handle ui;

	se_ui_widget_handle title_text;
	se_ui_widget_handle summary_text;
	se_ui_widget_handle hover_text;
	se_ui_widget_handle focus_text;
	se_ui_widget_handle textbox_state_text;
	se_ui_widget_handle submit_state_text;
	se_ui_widget_handle scroll_state_text;
	se_ui_widget_handle layout_state_text;
	se_ui_widget_handle clip_state_text;
	se_ui_widget_handle scene_state_text;
	se_ui_widget_handle card_state_text;

	se_ui_widget_handle demo_button;
	se_ui_widget_handle textbox;
	se_ui_widget_handle scrollbox;

	se_ui_widget_handle align_container;
	se_ui_widget_handle align_sample;

	se_ui_widget_handle dock_left;
	se_ui_widget_handle dock_right;
	se_ui_widget_handle card_panel;
	se_ui_widget_handle card_button;
	se_ui_widget_handle floating_panel;

	ui_widget_handles scroll_items;

	i32 click_count;
	i32 hover_count;
	i32 press_count;
	i32 release_count;
	i32 submit_count;
	i32 scroll_event_count;
	i32 textbox_change_count;
	i32 card_click_count;
	i32 scroll_item_click_count;
	i32 added_items_count;
	i32 removed_items_count;

	i32 align_h_index;
	i32 align_v_index;
	i32 align_direction_index;

	b8 overlay_enabled;
	b8 scroll_clipping;
	b8 demo_button_visible;
	b8 demo_button_enabled;
	b8 demo_button_interactable;
	b8 accent_style_enabled;
	b8 card_attached_left;
	b8 card_detached;
	b8 floating_front;
	b8 card_dragging;

	s_vec2 card_drag_pointer_offset;
	se_ui_widget_handle selected_scroll_item;

	se_ui_widget_handle last_hovered;
	se_ui_widget_handle last_focused;
	f32 last_scroll_value;
	b8 last_dirty;
} ui_example_state;

static const se_ui_alignment g_align_values[4] = {
	SE_UI_ALIGN_START,
	SE_UI_ALIGN_CENTER,
	SE_UI_ALIGN_END,
	SE_UI_ALIGN_STRETCH
};

static const c8* g_align_names[4] = {
	"start",
	"center",
	"end",
	"stretch"
};

static const se_ui_layout_direction g_direction_values[2] = {
	SE_UI_LAYOUT_VERTICAL,
	SE_UI_LAYOUT_HORIZONTAL
};

static const c8* g_direction_names[2] = {
	"vertical",
	"horizontal"
};

static void ui_refresh_summary(ui_example_state* state);
static void ui_refresh_scroll_state(ui_example_state* state);
static void ui_refresh_layout_state(ui_example_state* state);
static void ui_refresh_clip_state(ui_example_state* state);
static void ui_refresh_scene_state(ui_example_state* state);
static void ui_refresh_card_state(ui_example_state* state);
static void ui_apply_alignment_state(ui_example_state* state);
static void ui_add_scroll_item(ui_example_state* state);
static void ui_remove_scroll_item(ui_example_state* state);
static void ui_update_card_drag(ui_example_state* state, const s_vec2* pointer_ndc);
static void ui_refresh_scroll_item_selection(ui_example_state* state);
static void ui_on_hover_start(const se_ui_hover_event* event, void* user_data);
static void ui_on_hover_end(const se_ui_hover_event* event, void* user_data);
static void ui_on_press(const se_ui_press_event* event, void* user_data);
static void ui_on_release(const se_ui_press_event* event, void* user_data);

static void ui_write_text(ui_example_state* state, const se_ui_widget_handle widget, const c8* format, ...) {
	if (!state || state->ui == S_HANDLE_NULL || widget == S_HANDLE_NULL || !format) {
		return;
	}

	c8 line[256] = {0};
	va_list args;
	va_start(args, format);
	(void)vsnprintf(line, sizeof(line), format, args);
	va_end(args);

	c8 current[256] = {0};
	(void)se_ui_widget_get_text(state->ui, widget, current, sizeof(current));
	if (strncmp(line, current, sizeof(current) - 1) == 0) {
		return;
	}
	(void)se_ui_widget_set_text(state->ui, widget, line);
}

static b8 ui_point_inside_box(const se_box_2d* box, const s_vec2* point) {
	if (!box || !point) {
		return false;
	}
	return point->x >= box->min.x && point->x <= box->max.x && point->y >= box->min.y && point->y <= box->max.y;
}

static i32 ui_find_scroll_item_index(ui_example_state* state, const se_ui_widget_handle item) {
	if (!state || item == S_HANDLE_NULL) {
		return -1;
	}
	for (sz i = 0; i < s_array_get_size(&state->scroll_items); ++i) {
		se_ui_widget_handle* handle = s_array_get(&state->scroll_items, s_array_handle(&state->scroll_items, (u32)i));
		if (handle && *handle == item) {
			return (i32)i;
		}
	}
	return -1;
}

static se_ui_widget_handle ui_add_panel(
	ui_example_state* state,
	const se_ui_widget_handle parent,
	const s_vec2 size,
	const se_ui_layout_direction direction,
	const f32 spacing,
	const se_ui_edge padding,
	const se_ui_alignment align_h,
	const se_ui_alignment align_v,
	const b8 clip_children) {
	if (!state) {
		return S_HANDLE_NULL;
	}
	se_ui_widget_handle panel = se_ui_add_panel(parent, { .size = size });
	if (panel == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	if (direction == SE_UI_LAYOUT_VERTICAL) {
		(void)se_ui_vstack(state->ui, panel, spacing, padding);
	} else {
		(void)se_ui_hstack(state->ui, panel, spacing, padding);
	}
	(void)se_ui_widget_set_alignment(state->ui, panel, align_h, align_v);
	(void)se_ui_widget_set_clipping(state->ui, panel, clip_children);
	return panel;
}

static se_ui_widget_handle ui_add_label(
	ui_example_state* state,
	const se_ui_widget_handle parent,
	const c8* text,
	const s_vec2 size,
	const se_ui_alignment align_h,
	const se_ui_alignment align_v) {
	if (!state) {
		return S_HANDLE_NULL;
	}
	se_ui_widget_handle label = se_ui_add_text(parent, {
		.text = text,
		.size = size
	});
	if (label == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	(void)se_ui_widget_set_alignment(state->ui, label, align_h, align_v);
	return label;
}

static se_ui_widget_handle ui_add_button_row(ui_example_state* state, const se_ui_widget_handle parent) {
	return ui_add_panel(
		state,
		parent,
		s_vec2(0.42f, 0.060f),
		SE_UI_LAYOUT_HORIZONTAL,
		0.008f,
		se_ui_edge_all(0.006f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_CENTER,
		true);
}

static void ui_on_hover_start(const se_ui_hover_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	state->hover_count++;
	se_box_2d bounds = {0};
	if (se_ui_widget_get_bounds(state->ui, event->widget, &bounds)) {
		ui_write_text(
			state,
			state->hover_text,
			"hover start #%d widget=%llu pointer=(%.2f, %.2f) box=[%.2f %.2f %.2f %.2f]",
			state->hover_count,
			(unsigned long long)event->widget,
			event->pointer_ndc.x,
			event->pointer_ndc.y,
			bounds.min.x,
			bounds.min.y,
			bounds.max.x,
			bounds.max.y);
	} else {
		ui_write_text(
			state,
			state->hover_text,
			"hover start #%d widget=%llu pointer=(%.2f, %.2f)",
			state->hover_count,
			(unsigned long long)event->widget,
			event->pointer_ndc.x,
			event->pointer_ndc.y);
	}
	ui_refresh_summary(state);
}

static void ui_on_hover_end(const se_ui_hover_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	ui_write_text(
		state,
		state->hover_text,
		"hover end widget=%llu pointer=(%.2f, %.2f)",
		(unsigned long long)event->widget,
		event->pointer_ndc.x,
		event->pointer_ndc.y);
}

static void ui_on_focus(const se_ui_focus_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	ui_write_text(state, state->focus_text, "focus widget=%llu", (unsigned long long)event->widget);
	ui_refresh_scene_state(state);
}

static void ui_on_blur(const se_ui_focus_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	ui_write_text(state, state->focus_text, "blur widget=%llu", (unsigned long long)event->widget);
	ui_refresh_scene_state(state);
}

static void ui_on_press(const se_ui_press_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	state->press_count++;
	ui_write_text(
		state,
		state->summary_text,
		"press #%d release #%d click #%d submit #%d",
		state->press_count,
		state->release_count,
		state->click_count,
		state->submit_count);
}

static void ui_on_release(const se_ui_press_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	state->release_count++;
	ui_write_text(
		state,
		state->summary_text,
		"press #%d release #%d click #%d submit #%d",
		state->press_count,
		state->release_count,
		state->click_count,
		state->submit_count);
}

static void ui_on_change(const se_ui_change_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	if (event->widget == state->textbox) {
		state->textbox_change_count++;
		ui_write_text(
			state,
			state->textbox_state_text,
			"textbox change #%d value='%s'",
			state->textbox_change_count,
			event->text ? event->text : "");
	} else if (event->widget == state->scrollbox) {
		ui_refresh_scroll_state(state);
	}
}

static void ui_on_submit(const se_ui_submit_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	state->submit_count++;

	c8 value[256] = {0};
	(void)se_ui_widget_get_text(state->ui, state->textbox, value, sizeof(value));
	ui_write_text(state, state->submit_state_text, "submit #%d text='%s'", state->submit_count, value);
	ui_refresh_summary(state);
}

static void ui_on_scroll(const se_ui_scroll_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	state->scroll_event_count++;
	ui_refresh_scroll_state(state);
}

static void ui_on_scroll_item_press(const se_ui_press_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	ui_on_press(event, user_data);
}

static void ui_on_scroll_item_click(const se_ui_click_event* event, void* user_data) {
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state || !event) {
		return;
	}
	state->selected_scroll_item = se_ui_scrollbox_get_selected(state->ui, state->scrollbox);
	state->scroll_item_click_count++;
	ui_refresh_scroll_item_selection(state);
	ui_refresh_scroll_state(state);
	ui_refresh_summary(state);
}

static void ui_on_exit(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	se_window_set_should_close(state->window, true);
}

static void ui_on_demo_button_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	state->click_count++;
	ui_write_text(
		state,
		state->title_text,
		"6_ui :: full widget showcase (demo clicks=%d)",
		state->click_count);
	ui_refresh_summary(state);
}

static void ui_on_focus_textbox_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	(void)se_ui_focus_widget(state->ui, state->textbox);
	ui_write_text(state, state->focus_text, "focus requested for textbox");
	ui_refresh_scene_state(state);
}

static void ui_on_clear_focus_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	se_ui_clear_focus(state->ui);
	ui_write_text(state, state->focus_text, "focus cleared");
	ui_refresh_scene_state(state);
}

static void ui_on_toggle_overlay_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	state->overlay_enabled = !state->overlay_enabled;
	se_ui_set_debug_overlay(state->overlay_enabled);
	ui_write_text(state, state->scene_state_text, "debug overlay=%s", state->overlay_enabled ? "on" : "off");
	ui_refresh_scene_state(state);
}

static void ui_on_toggle_visible_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	state->demo_button_visible = !state->demo_button_visible;
	(void)se_ui_widget_set_visible(state->ui, state->demo_button, state->demo_button_visible);
	ui_write_text(state, state->clip_state_text, "demo button visible=%s", state->demo_button_visible ? "true" : "false");
}

static void ui_on_toggle_enabled_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	state->demo_button_enabled = !state->demo_button_enabled;
	(void)se_ui_widget_set_enabled(state->ui, state->demo_button, state->demo_button_enabled);
	ui_write_text(state, state->clip_state_text, "demo button enabled=%s", state->demo_button_enabled ? "true" : "false");
}

static void ui_on_toggle_interactable_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	state->demo_button_interactable = !state->demo_button_interactable;
	(void)se_ui_widget_set_interactable(state->ui, state->demo_button, state->demo_button_interactable);
	ui_write_text(state, state->clip_state_text, "demo button interactable=%s", state->demo_button_interactable ? "true" : "false");
}

static void ui_on_toggle_style_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}

	se_ui_style style = SE_UI_STYLE_DEFAULT;
	if (!se_ui_widget_get_style(state->ui, state->demo_button, &style)) {
		return;
	}

	if (state->accent_style_enabled) {
		se_ui_get_default_style(&style);
		state->accent_style_enabled = false;
	} else {
		style.normal.background_color = s_vec4(0.18f, 0.11f, 0.26f, 0.92f);
		style.hovered.background_color = s_vec4(0.25f, 0.15f, 0.34f, 0.95f);
		style.pressed.background_color = s_vec4(0.12f, 0.07f, 0.20f, 0.96f);
		style.focused.border_color = s_vec4(0.82f, 0.58f, 0.96f, 1.0f);
		state->accent_style_enabled = true;
	}

	(void)se_ui_widget_set_style(state->ui, state->demo_button, &style);
	ui_write_text(state, state->clip_state_text, "demo style=%s", state->accent_style_enabled ? "accent" : "default");
}

static void ui_on_force_dirty_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	se_ui_mark_layout_dirty(state->ui);
	se_ui_mark_visual_dirty(state->ui);
	se_ui_mark_text_dirty(state->ui);
	se_ui_mark_structure_dirty(state->ui);
	ui_write_text(state, state->scene_state_text, "manual dirty mark: layout+visual+text+structure");
	ui_refresh_scene_state(state);
}

static void ui_on_add_item_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	ui_add_scroll_item(state);
}

static void ui_on_remove_item_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	ui_remove_scroll_item(state);
}

static void ui_on_scroll_top_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	(void)se_ui_widget_set_value(state->ui, state->scrollbox, 0.0f);
	ui_refresh_scroll_state(state);
}

static void ui_on_scroll_bottom_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	(void)se_ui_widget_set_value(state->ui, state->scrollbox, 1.0f);
	ui_refresh_scroll_state(state);
}

static void ui_on_toggle_scroll_clip_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	state->scroll_clipping = !state->scroll_clipping;
	(void)se_ui_widget_set_clipping(state->ui, state->scrollbox, state->scroll_clipping);
	ui_refresh_clip_state(state);
}

static void ui_on_cycle_align_h_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	state->align_h_index = (state->align_h_index + 1) % 4;
	ui_apply_alignment_state(state);
}

static void ui_on_cycle_align_v_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	state->align_v_index = (state->align_v_index + 1) % 4;
	ui_apply_alignment_state(state);
}

static void ui_on_cycle_direction_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	state->align_direction_index = (state->align_direction_index + 1) % 2;
	ui_apply_alignment_state(state);
}

static void ui_on_swap_dock_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}

	state->card_attached_left = !state->card_attached_left;
	if (!state->card_detached) {
		const se_ui_widget_handle target_dock = state->card_attached_left ? state->dock_left : state->dock_right;
		(void)se_ui_widget_attach(state->ui, target_dock, state->card_panel);
	}
	ui_refresh_card_state(state);
}

static void ui_on_detach_attach_card_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}

	if (state->card_detached) {
		const se_ui_widget_handle target_dock = state->card_attached_left ? state->dock_left : state->dock_right;
		(void)se_ui_widget_attach(state->ui, target_dock, state->card_panel);
		(void)se_ui_widget_set_z_order(state->ui, state->card_panel, 0);
		state->card_detached = false;
	} else {
		(void)se_ui_widget_detach(state->ui, state->card_panel);
		(void)se_ui_widget_set_position(state->ui, state->card_panel, &s_vec2(0.64f, -0.56f));
		(void)se_ui_widget_set_z_order(state->ui, state->card_panel, 30);
		state->card_detached = true;
	}
	ui_refresh_card_state(state);
}

static void ui_on_toggle_floating_z_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}

	state->floating_front = !state->floating_front;
	const i32 z = state->floating_front ? 40 : -20;
	(void)se_ui_widget_set_z_order(state->ui, state->floating_panel, z);
	ui_refresh_card_state(state);
}

static void ui_on_card_button_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_example_state* state = (ui_example_state*)user_data;
	if (!state) {
		return;
	}
	state->card_click_count++;
	ui_write_text(state, state->card_state_text, "card clicks=%d", state->card_click_count);
	ui_refresh_summary(state);
}

static se_ui_widget_handle ui_add_action_button(
	ui_example_state* state,
	const se_ui_widget_handle parent,
	const c8* text,
	const se_ui_click_callback on_click) {
	se_ui_widget_handle button = se_ui_add_button(parent, {
		.text = text,
		.size = s_vec2(0.125f, 0.042f),
		.on_click_fn = on_click,
		.on_click_data = state,
		.on_hover_start_fn = ui_on_hover_start,
		.on_hover_start_data = state,
		.on_hover_end_fn = ui_on_hover_end,
		.on_hover_end_data = state,
		.on_press_fn = ui_on_press,
		.on_press_data = state,
		.on_release_fn = ui_on_release,
		.on_release_data = state
	});
	if (button == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	(void)se_ui_widget_set_alignment(state->ui, button, SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_CENTER);
	return button;
}

static void ui_refresh_summary(ui_example_state* state) {
	if (!state) {
		return;
	}
	ui_write_text(
		state,
		state->summary_text,
		"events: hover=%d press=%d release=%d click=%d submit=%d scroll=%d item_click=%d",
		state->hover_count,
		state->press_count,
		state->release_count,
		state->click_count,
		state->submit_count,
		state->scroll_event_count,
		state->scroll_item_click_count);
}

static void ui_refresh_scroll_state(ui_example_state* state) {
	if (!state) {
		return;
	}
	state->selected_scroll_item = se_ui_scrollbox_get_selected(state->ui, state->scrollbox);
	const i32 selected_index = ui_find_scroll_item_index(state, state->selected_scroll_item);
	const u64 selected_handle = (selected_index >= 0) ? (u64)state->selected_scroll_item : 0ull;
	const f32 value = se_ui_widget_get_value(state->ui, state->scrollbox);
	ui_write_text(
		state,
		state->scroll_state_text,
		"scroll value=%.3f items=%zu selected=%d handle=%llu added=%d removed=%d callbacks=%d",
		value,
		s_array_get_size(&state->scroll_items),
		selected_index + 1,
		(unsigned long long)selected_handle,
		state->added_items_count,
		state->removed_items_count,
		state->scroll_event_count);
}

static void ui_refresh_scroll_item_selection(ui_example_state* state) {
	if (!state) {
		return;
	}
	state->selected_scroll_item = se_ui_scrollbox_get_selected(state->ui, state->scrollbox);
	for (sz i = 0; i < s_array_get_size(&state->scroll_items); ++i) {
		se_ui_widget_handle* item_handle = s_array_get(&state->scroll_items, s_array_handle(&state->scroll_items, (u32)i));
		if (!item_handle || *item_handle == S_HANDLE_NULL || !se_ui_widget_exists(state->ui, *item_handle)) {
			continue;
		}
		const b8 selected = (*item_handle == state->selected_scroll_item);
		c8 line[96] = {0};
		if (selected) {
			snprintf(line, sizeof(line), ">> Item %02d [SELECTED]", (i32)i + 1);
		} else {
			snprintf(line, sizeof(line), "Item %02d :: click to select", (i32)i + 1);
		}
		(void)se_ui_widget_set_text(state->ui, *item_handle, line);
	}
}

static void ui_refresh_layout_state(ui_example_state* state) {
	if (!state) {
		return;
	}
	ui_write_text(
		state,
		state->layout_state_text,
		"layout dir=%s h=%s v=%s (F2/F3/F4)",
		g_direction_names[state->align_direction_index],
		g_align_names[state->align_h_index],
		g_align_names[state->align_v_index]);
}

static void ui_refresh_clip_state(ui_example_state* state) {
	if (!state) {
		return;
	}
	ui_write_text(
		state,
		state->clip_state_text,
		"clip=%s visible=%s enabled=%s interactable=%s style=%s",
		state->scroll_clipping ? "on" : "off",
		state->demo_button_visible ? "on" : "off",
		state->demo_button_enabled ? "on" : "off",
		state->demo_button_interactable ? "on" : "off",
		state->accent_style_enabled ? "accent" : "default");
}

static void ui_refresh_scene_state(ui_example_state* state) {
	if (!state) {
		return;
	}
	const se_scene_2d_handle scene = se_ui_get_scene(state->ui);
	const se_window_handle window = se_ui_get_window(state->ui);
	const se_ui_widget_handle hovered = se_ui_get_hovered_widget(state->ui);
	const se_ui_widget_handle focused = se_ui_get_focused_widget(state->ui);
	ui_write_text(
		state,
		state->scene_state_text,
		"ui=%llu scene=%llu window=%llu hovered=%llu focused=%llu dirty=%d overlay=%s",
		(unsigned long long)state->ui,
		(unsigned long long)scene,
		(unsigned long long)window,
		(unsigned long long)hovered,
		(unsigned long long)focused,
		se_ui_is_dirty(state->ui),
		state->overlay_enabled ? "on" : "off");
}

static void ui_refresh_card_state(ui_example_state* state) {
	if (!state) {
		return;
	}
	const c8* dock = state->card_attached_left ? "left" : "right";
	const c8* attached = state->card_detached ? "detached" : "attached";
	const i32 floating_z = se_ui_widget_get_z_order(state->ui, state->floating_panel);
	se_box_2d card_bounds = {0};
	f32 card_x = 0.0f;
	f32 card_y = 0.0f;
	if (se_ui_widget_get_bounds(state->ui, state->card_panel, &card_bounds)) {
		card_x = (card_bounds.min.x + card_bounds.max.x) * 0.5f;
		card_y = (card_bounds.min.y + card_bounds.max.y) * 0.5f;
	}
	ui_write_text(
		state,
		state->card_state_text,
		"card=%s dock=%s drag=%s pos=(%.2f, %.2f) clicks=%d floating_z=%d",
		attached,
		dock,
		state->card_dragging ? "yes" : "no",
		card_x,
		card_y,
		state->card_click_count,
		floating_z);
}

static void ui_update_card_drag(ui_example_state* state, const s_vec2* pointer_ndc) {
	if (!state || !pointer_ndc) {
		return;
	}

	const b8 mouse_pressed = se_window_is_mouse_pressed(state->window, SE_MOUSE_LEFT);
	const b8 mouse_down = se_window_is_mouse_down(state->window, SE_MOUSE_LEFT);
	const b8 mouse_released = se_window_is_mouse_released(state->window, SE_MOUSE_LEFT);

	if (!state->card_dragging && mouse_pressed) {
		const se_ui_widget_handle hovered = se_ui_get_hovered_widget(state->ui);
		if (hovered == state->card_panel) {
			se_box_2d card_bounds = {0};
			if (se_ui_widget_get_bounds(state->ui, state->card_panel, &card_bounds)) {
				const s_vec2 card_center = s_vec2(
					(card_bounds.min.x + card_bounds.max.x) * 0.5f,
					(card_bounds.min.y + card_bounds.max.y) * 0.5f);
				state->card_drag_pointer_offset = s_vec2(pointer_ndc->x - card_center.x, pointer_ndc->y - card_center.y);
			} else {
				state->card_drag_pointer_offset = s_vec2(0.0f, 0.0f);
			}

			if (!state->card_detached) {
				(void)se_ui_widget_detach(state->ui, state->card_panel);
				state->card_detached = true;
			}
			(void)se_ui_widget_set_z_order(state->ui, state->card_panel, 30);
			state->card_dragging = true;
			ui_refresh_card_state(state);
		}
	}

	if (state->card_dragging && mouse_down) {
		const s_vec2 new_position = s_vec2(
			pointer_ndc->x - state->card_drag_pointer_offset.x,
			pointer_ndc->y - state->card_drag_pointer_offset.y);
		(void)se_ui_widget_set_position(state->ui, state->card_panel, &new_position);
	}

	if (!state->card_dragging || !mouse_released) {
		return;
	}

	state->card_dragging = false;

	se_box_2d left_bounds = {0};
	se_box_2d right_bounds = {0};
	const b8 has_left_bounds = se_ui_widget_get_bounds(state->ui, state->dock_left, &left_bounds);
	const b8 has_right_bounds = se_ui_widget_get_bounds(state->ui, state->dock_right, &right_bounds);
	const b8 drop_left = has_left_bounds && ui_point_inside_box(&left_bounds, pointer_ndc);
	const b8 drop_right = has_right_bounds && ui_point_inside_box(&right_bounds, pointer_ndc);

	if (drop_left || drop_right) {
		state->card_attached_left = drop_left;
		const se_ui_widget_handle target_dock = state->card_attached_left ? state->dock_left : state->dock_right;
		(void)se_ui_widget_attach(state->ui, target_dock, state->card_panel);
		(void)se_ui_widget_set_z_order(state->ui, state->card_panel, 0);
		state->card_detached = false;
	} else {
		state->card_detached = true;
		(void)se_ui_widget_set_z_order(state->ui, state->card_panel, 30);
	}

	ui_refresh_card_state(state);
}

static void ui_apply_alignment_state(ui_example_state* state) {
	if (!state) {
		return;
	}

	se_ui_layout container_layout = SE_UI_LAYOUT_DEFAULT;
	se_ui_layout sample_layout = SE_UI_LAYOUT_DEFAULT;
	if (!se_ui_widget_get_layout(state->ui, state->align_container, &container_layout)) {
		return;
	}
	if (!se_ui_widget_get_layout(state->ui, state->align_sample, &sample_layout)) {
		return;
	}

	const se_ui_layout_direction direction = g_direction_values[state->align_direction_index];
	const se_ui_alignment h = g_align_values[state->align_h_index];
	const se_ui_alignment v = g_align_values[state->align_v_index];

	container_layout.direction = direction;
	container_layout.spacing = 0.02f;
	if (direction == SE_UI_LAYOUT_VERTICAL) {
		container_layout.align_horizontal = SE_UI_ALIGN_STRETCH;
		container_layout.align_vertical = v;
		sample_layout.align_horizontal = h;
		sample_layout.align_vertical = SE_UI_ALIGN_START;
	} else {
		container_layout.align_horizontal = h;
		container_layout.align_vertical = SE_UI_ALIGN_STRETCH;
		sample_layout.align_horizontal = SE_UI_ALIGN_START;
		sample_layout.align_vertical = v;
	}

	(void)se_ui_widget_set_layout(state->ui, state->align_container, &container_layout);
	(void)se_ui_widget_set_layout(state->ui, state->align_sample, &sample_layout);
	ui_refresh_layout_state(state);
}

static void ui_add_scroll_item(ui_example_state* state) {
	if (!state) {
		return;
	}

	c8 line[96] = {0};
	const i32 index = (i32)s_array_get_size(&state->scroll_items) + 1;
	snprintf(line, sizeof(line), "Item %02d :: click to select", index);

	se_ui_widget_handle item = se_ui_scroll_item_add(state->ui, state->scrollbox, line);
	if (item != S_HANDLE_NULL) {
		(void)se_ui_widget_on_hover(state->ui, item, ui_on_hover_start, ui_on_hover_end, state);
		(void)se_ui_widget_on_press(state->ui, item, ui_on_scroll_item_press, ui_on_release, state);
		(void)se_ui_widget_on_click(state->ui, item, ui_on_scroll_item_click, state);
		s_array_add(&state->scroll_items, item);
		state->added_items_count++;
		if (se_ui_scrollbox_get_selected(state->ui, state->scrollbox) == S_HANDLE_NULL) {
			(void)se_ui_scrollbox_set_selected(state->ui, state->scrollbox, item);
		}
	}
	ui_refresh_scroll_item_selection(state);
	ui_refresh_scroll_state(state);
}

static void ui_remove_scroll_item(ui_example_state* state) {
	if (!state) {
		return;
	}
	const sz count = s_array_get_size(&state->scroll_items);
	if (count == 0) {
		ui_write_text(state, state->scroll_state_text, "scroll list already empty");
		return;
	}

	const s_handle entry = s_array_handle(&state->scroll_items, (u32)(count - 1));
	se_ui_widget_handle* item = s_array_get(&state->scroll_items, entry);
	const se_ui_widget_handle removed = item ? *item : S_HANDLE_NULL;
	if (item && *item != S_HANDLE_NULL) {
		(void)se_ui_widget_remove(state->ui, *item);
		state->removed_items_count++;
	}
	(void)s_array_remove(&state->scroll_items, entry);
	(void)removed;
	state->selected_scroll_item = se_ui_scrollbox_get_selected(state->ui, state->scrollbox);
	ui_refresh_scroll_item_selection(state);
	ui_refresh_scroll_state(state);
}

i32 main(void) {
	se_context* ctx = se_context_create();
	se_window_handle window = se_window_create("Syphax-Engine - UI Full Showcase", WIDTH, HEIGHT);
	if (window == S_HANDLE_NULL) {
		printf("6_ui :: window unavailable, skipping runtime\n");
		se_context_destroy(ctx);
		return 0;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.07f, 0.08f, 0.10f, 1.0f));

	se_ui_handle ui = se_ui_create(window, 512);
	if (ui == S_HANDLE_NULL) {
		printf("6_ui :: ui creation failed, skipping runtime\n");
		se_context_destroy(ctx);
		return 0;
	}

	ui_example_state state = {0};
	state.window = window;
	state.ui = ui;
	state.demo_button_visible = true;
	state.demo_button_enabled = true;
	state.demo_button_interactable = true;
	state.scroll_clipping = true;
	state.card_attached_left = true;
	state.floating_front = false;
	state.last_scroll_value = -1.0f;
	state.last_dirty = false;
	s_array_init(&state.scroll_items);

	se_ui_widget_handle root = se_ui_add_root(ui, {
		.size = s_vec2(1.0f, 1.0f)
	});
	(void)se_ui_vstack(ui, root, 0.012f, se_ui_edge_all(0.014f));
	(void)se_ui_widget_set_alignment(ui, root, SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);

	se_ui_widget_handle header = ui_add_panel(
		&state,
		root,
		s_vec2(0.97f, 0.10f),
		SE_UI_LAYOUT_HORIZONTAL,
		0.010f,
		(se_ui_edge){0.010f, 0.010f, 0.006f, 0.006f},
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_CENTER,
		false);

	state.title_text = ui_add_label(&state, header, "6_ui :: full widget showcase", s_vec2(0.50f, 0.040f), SE_UI_ALIGN_START, SE_UI_ALIGN_CENTER);
	state.summary_text = ui_add_label(
		&state,
		header,
		"events: hover=0 press=0 release=0 click=0 submit=0 scroll=0 item_click=0",
		s_vec2(0.36f, 0.030f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_CENTER);

	se_ui_widget_handle close_button = se_ui_add_button(header, {
		.text = "Exit",
		.size = s_vec2(0.10f, 0.042f),
		.on_click_fn = ui_on_exit,
		.on_click_data = &state,
		.on_hover_start_fn = ui_on_hover_start,
		.on_hover_start_data = &state,
		.on_hover_end_fn = ui_on_hover_end,
		.on_hover_end_data = &state,
		.on_press_fn = ui_on_press,
		.on_press_data = &state,
		.on_release_fn = ui_on_release,
		.on_release_data = &state
	});
	(void)se_ui_widget_set_alignment(ui, close_button, SE_UI_ALIGN_END, SE_UI_ALIGN_CENTER);
	(void)se_ui_widget_use_style_preset(ui, close_button, SE_UI_STYLE_PRESET_BUTTON_DANGER);
	se_ui_widget_handle status_panel = ui_add_panel(
		&state,
		root,
		s_vec2(0.97f, 0.18f),
		SE_UI_LAYOUT_VERTICAL,
		0.006f,
		se_ui_edge_all(0.010f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_START,
		false);

	state.hover_text = ui_add_label(&state, status_panel, "hover:", s_vec2(0.92f, 0.022f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	state.focus_text = ui_add_label(&state, status_panel, "focus:", s_vec2(0.92f, 0.022f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	state.textbox_state_text = ui_add_label(&state, status_panel, "textbox:", s_vec2(0.92f, 0.022f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	state.submit_state_text = ui_add_label(&state, status_panel, "submit:", s_vec2(0.92f, 0.022f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	state.scroll_state_text = ui_add_label(&state, status_panel, "scroll:", s_vec2(0.92f, 0.022f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	state.layout_state_text = ui_add_label(&state, status_panel, "layout:", s_vec2(0.92f, 0.022f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	state.clip_state_text = ui_add_label(&state, status_panel, "clip:", s_vec2(0.92f, 0.022f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	state.scene_state_text = ui_add_label(&state, status_panel, "scene:", s_vec2(0.92f, 0.022f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	state.card_state_text = ui_add_label(&state, status_panel, "card:", s_vec2(0.92f, 0.022f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);

	se_ui_widget_handle main = ui_add_panel(
		&state,
		root,
		s_vec2(0.97f, 0.67f),
		SE_UI_LAYOUT_HORIZONTAL,
		0.012f,
		se_ui_edge_all(0.010f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_STRETCH,
		false);

	se_ui_widget_handle controls = ui_add_panel(
		&state,
		main,
		s_vec2(0.45f, 0.63f),
		SE_UI_LAYOUT_VERTICAL,
		0.010f,
		se_ui_edge_all(0.010f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_START,
		false);

	(void)ui_add_label(&state, controls, "Callbacks + Mutations Controls", s_vec2(0.40f, 0.03f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	(void)ui_add_label(
		&state,
		controls,
		"Hotkeys: F1 overlay, F2 direction, F3 H-align, F4 V-align; drag card body",
		s_vec2(0.40f, 0.024f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_START);

	state.demo_button = se_ui_add_button(controls, {
		.text = "Demo Button (on_click)",
		.size = s_vec2(0.40f, 0.055f),
		.on_click_fn = ui_on_demo_button_click,
		.on_click_data = &state,
		.on_hover_start_fn = ui_on_hover_start,
		.on_hover_start_data = &state,
		.on_hover_end_fn = ui_on_hover_end,
		.on_hover_end_data = &state,
		.on_press_fn = ui_on_press,
		.on_press_data = &state,
		.on_release_fn = ui_on_release,
		.on_release_data = &state
	});
	(void)se_ui_widget_set_alignment(ui, state.demo_button, SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_CENTER);

	state.textbox = se_ui_add_textbox(controls, {
		.placeholder = "Type text, Enter to submit (supports selection + caret)",
		.size = s_vec2(0.42f, 0.055f),
		.on_hover_start_fn = ui_on_hover_start,
		.on_hover_start_data = &state,
		.on_hover_end_fn = ui_on_hover_end,
		.on_hover_end_data = &state,
		.on_focus_fn = ui_on_focus,
		.on_focus_data = &state,
		.on_blur_fn = ui_on_blur,
		.on_blur_data = &state,
		.on_press_fn = ui_on_press,
		.on_press_data = &state,
		.on_release_fn = ui_on_release,
		.on_release_data = &state,
		.on_change_fn = ui_on_change,
		.on_change_data = &state,
		.on_submit_fn = ui_on_submit,
		.on_submit_data = &state
	});
	(void)se_ui_widget_set_alignment(ui, state.textbox, SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_CENTER);

	se_ui_widget_handle row_a = ui_add_button_row(&state, controls);
	(void)ui_add_action_button(&state, row_a, "Focus Box", ui_on_focus_textbox_click);
	(void)ui_add_action_button(&state, row_a, "Clear Focus", ui_on_clear_focus_click);
	(void)ui_add_action_button(&state, row_a, "Toggle Overlay", ui_on_toggle_overlay_click);

	se_ui_widget_handle row_b = ui_add_button_row(&state, controls);
	(void)ui_add_action_button(&state, row_b, "Toggle Visible", ui_on_toggle_visible_click);
	(void)ui_add_action_button(&state, row_b, "Toggle Enabled", ui_on_toggle_enabled_click);
	(void)ui_add_action_button(&state, row_b, "Toggle HitTest", ui_on_toggle_interactable_click);

	se_ui_widget_handle row_c = ui_add_button_row(&state, controls);
	(void)ui_add_action_button(&state, row_c, "Toggle Style", ui_on_toggle_style_click);
	(void)ui_add_action_button(&state, row_c, "Force Dirty", ui_on_force_dirty_click);
	(void)ui_add_action_button(&state, row_c, "Swap Dock", ui_on_swap_dock_click);

	se_ui_widget_handle row_d = ui_add_button_row(&state, controls);
	(void)ui_add_action_button(&state, row_d, "Detach Card", ui_on_detach_attach_card_click);
	(void)ui_add_action_button(&state, row_d, "Float Z", ui_on_toggle_floating_z_click);
	(void)ui_add_action_button(&state, row_d, "Toggle Clip", ui_on_toggle_scroll_clip_click);

	se_ui_widget_handle row_e = ui_add_button_row(&state, controls);
	(void)ui_add_action_button(&state, row_e, "Add Item", ui_on_add_item_click);
	(void)ui_add_action_button(&state, row_e, "Remove Item", ui_on_remove_item_click);
	(void)ui_add_action_button(&state, row_e, "Scroll Top", ui_on_scroll_top_click);

	se_ui_widget_handle row_f = ui_add_button_row(&state, controls);
	(void)ui_add_action_button(&state, row_f, "Align H", ui_on_cycle_align_h_click);
	(void)ui_add_action_button(&state, row_f, "Align V", ui_on_cycle_align_v_click);
	(void)ui_add_action_button(&state, row_f, "Direction", ui_on_cycle_direction_click);

	se_ui_widget_handle row_g = ui_add_button_row(&state, controls);
	(void)ui_add_action_button(&state, row_g, "Scroll Bottom", ui_on_scroll_bottom_click);
	(void)ui_add_action_button(&state, row_g, "Add Item", ui_on_add_item_click);
	(void)ui_add_action_button(&state, row_g, "Remove Item", ui_on_remove_item_click);

	se_ui_widget_handle showcase = ui_add_panel(
		&state,
		main,
		s_vec2(0.49f, 0.63f),
		SE_UI_LAYOUT_VERTICAL,
		0.012f,
		se_ui_edge_all(0.010f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_START,
		false);

	(void)ui_add_label(&state, showcase, "Alignment sandbox (direction + H/V alignment)", s_vec2(0.46f, 0.028f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);

	state.align_container = ui_add_panel(
		&state,
		showcase,
		s_vec2(0.46f, 0.22f),
		SE_UI_LAYOUT_VERTICAL,
		0.02f,
		se_ui_edge_all(0.012f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_START,
		true);

	state.align_sample = se_ui_add_button(state.align_container, {
		.text = "Align Sample",
		.size = s_vec2(0.16f, 0.05f),
		.on_click_fn = ui_on_demo_button_click,
		.on_click_data = &state,
		.on_hover_start_fn = ui_on_hover_start,
		.on_hover_start_data = &state,
		.on_hover_end_fn = ui_on_hover_end,
		.on_hover_end_data = &state,
		.on_press_fn = ui_on_press,
		.on_press_data = &state,
		.on_release_fn = ui_on_release,
		.on_release_data = &state
	});
	(void)se_ui_widget_set_alignment(ui, state.align_sample, SE_UI_ALIGN_START, SE_UI_ALIGN_START);

	(void)ui_add_label(&state, showcase, "Drag card body and drop on LEFT/RIGHT dock", s_vec2(0.46f, 0.028f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);

	se_ui_widget_handle dock_row = ui_add_panel(
		&state,
		showcase,
		s_vec2(0.46f, 0.18f),
		SE_UI_LAYOUT_HORIZONTAL,
		0.010f,
		se_ui_edge_all(0.010f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_STRETCH,
		false);

	state.dock_left = ui_add_panel(
		&state,
		dock_row,
		s_vec2(0.22f, 0.14f),
		SE_UI_LAYOUT_VERTICAL,
		0.006f,
		se_ui_edge_all(0.006f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_START,
		false);
	state.dock_right = ui_add_panel(
		&state,
		dock_row,
		s_vec2(0.22f, 0.14f),
		SE_UI_LAYOUT_VERTICAL,
		0.006f,
		se_ui_edge_all(0.006f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_START,
		false);

	(void)ui_add_label(&state, state.dock_left, "LEFT DOCK", s_vec2(0.19f, 0.024f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	(void)ui_add_label(&state, state.dock_right, "RIGHT DOCK", s_vec2(0.19f, 0.024f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);

	state.card_panel = ui_add_panel(
		&state,
		state.dock_left,
		s_vec2(0.19f, 0.10f),
		SE_UI_LAYOUT_VERTICAL,
		0.004f,
		se_ui_edge_all(0.006f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_START,
		false);
	(void)se_ui_widget_set_interactable(ui, state.card_panel, true);
	(void)ui_add_label(&state, state.card_panel, "Drag this body", s_vec2(0.18f, 0.023f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);

	state.card_button = se_ui_add_button(state.card_panel, {
		.text = "Card Button",
		.size = s_vec2(0.17f, 0.042f),
		.on_click_fn = ui_on_card_button_click,
		.on_click_data = &state,
		.on_hover_start_fn = ui_on_hover_start,
		.on_hover_start_data = &state,
		.on_hover_end_fn = ui_on_hover_end,
		.on_hover_end_data = &state,
		.on_press_fn = ui_on_press,
		.on_press_data = &state,
		.on_release_fn = ui_on_release,
		.on_release_data = &state
	});
	(void)se_ui_widget_set_alignment(ui, state.card_button, SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_CENTER);

	(void)ui_add_label(&state, showcase, "Scrollbox (wheel + thumb drag + selectable items)", s_vec2(0.46f, 0.028f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);

	state.scrollbox = se_ui_add_scrollbox(showcase, {
		.size = s_vec2(0.46f, 0.22f),
		.on_hover_start_fn = ui_on_hover_start,
		.on_hover_start_data = &state,
		.on_hover_end_fn = ui_on_hover_end,
		.on_hover_end_data = &state,
		.on_focus_fn = ui_on_focus,
		.on_focus_data = &state,
		.on_blur_fn = ui_on_blur,
		.on_blur_data = &state,
		.on_press_fn = ui_on_press,
		.on_press_data = &state,
		.on_release_fn = ui_on_release,
		.on_release_data = &state,
		.on_scroll_fn = ui_on_scroll,
		.on_scroll_data = &state,
		.on_change_fn = ui_on_change,
		.on_change_data = &state
	});
	const se_ui_edge scroll_padding = (se_ui_edge){0.010f, 0.022f, 0.010f, 0.010f};
	(void)se_ui_vstack(ui, state.scrollbox, 0.006f, scroll_padding);
	(void)se_ui_widget_set_alignment(ui, state.scrollbox, SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	(void)se_ui_widget_set_clipping(ui, state.scrollbox, true);
	(void)se_ui_scrollbox_enable_single_select(ui, state.scrollbox, true);
	se_ui_style scroll_item_normal = SE_UI_STYLE_DEFAULT;
	se_ui_style scroll_item_selected = SE_UI_STYLE_DEFAULT;
	(void)se_ui_widget_use_style_preset(ui, state.demo_button, SE_UI_STYLE_PRESET_BUTTON_PRIMARY);
	(void)se_ui_widget_use_style_preset(ui, state.scrollbox, SE_UI_STYLE_PRESET_DEFAULT);
	(void)se_ui_widget_use_style_preset(ui, state.card_button, SE_UI_STYLE_PRESET_BUTTON_PRIMARY);
	scroll_item_selected.normal.background_color = s_vec4(0.94f, 0.76f, 0.16f, 0.98f);
	scroll_item_selected.hovered.background_color = s_vec4(0.99f, 0.82f, 0.24f, 1.0f);
	scroll_item_selected.pressed.background_color = s_vec4(0.82f, 0.64f, 0.10f, 1.0f);
	scroll_item_selected.normal.text_color = s_vec4(0.08f, 0.08f, 0.08f, 1.0f);
	scroll_item_selected.hovered.text_color = s_vec4(0.08f, 0.08f, 0.08f, 1.0f);
	scroll_item_selected.pressed.text_color = s_vec4(0.08f, 0.08f, 0.08f, 1.0f);
	scroll_item_selected.normal.border_color = s_vec4(0.98f, 0.95f, 0.70f, 1.0f);
	scroll_item_selected.hovered.border_color = s_vec4(1.0f, 1.0f, 0.85f, 1.0f);
	scroll_item_selected.pressed.border_color = s_vec4(0.98f, 0.95f, 0.70f, 1.0f);
	scroll_item_selected.focused.border_color = s_vec4(1.0f, 1.0f, 0.85f, 1.0f);
	scroll_item_selected.normal.border_width = 0.0045f;
	scroll_item_selected.hovered.border_width = 0.0045f;
	scroll_item_selected.pressed.border_width = 0.0045f;
	scroll_item_normal.normal.background_color = s_vec4(0.12f, 0.14f, 0.17f, 0.90f);
	scroll_item_normal.hovered.background_color = s_vec4(0.17f, 0.20f, 0.24f, 0.96f);
	scroll_item_normal.pressed.background_color = s_vec4(0.09f, 0.11f, 0.14f, 0.98f);
	scroll_item_normal.normal.border_color = s_vec4(0.24f, 0.28f, 0.34f, 1.0f);
	scroll_item_normal.hovered.border_color = s_vec4(0.34f, 0.40f, 0.48f, 1.0f);
	scroll_item_normal.pressed.border_color = s_vec4(0.22f, 0.26f, 0.31f, 1.0f);
	(void)se_ui_scrollbox_set_item_styles(ui, state.scrollbox, &scroll_item_normal, &scroll_item_selected);

	for (i32 i = 0; i < 16; ++i) {
		ui_add_scroll_item(&state);
	}

	state.floating_panel = ui_add_panel(
		&state,
		root,
		s_vec2(0.2f, 0.14f),
		SE_UI_LAYOUT_VERTICAL,
		0.004f,
		se_ui_edge_all(0.008f),
		SE_UI_ALIGN_STRETCH,
		SE_UI_ALIGN_START,
		true);
	(void)se_ui_widget_set_anchor_rect(ui, state.floating_panel, s_vec2(0.74f, 0.80f), s_vec2(0.98f, 0.97f));
	(void)se_ui_widget_set_z_order(ui, state.floating_panel, -20);

	(void)ui_add_label(&state, state.floating_panel, "Anchored panel", s_vec2(0.18f, 0.024f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);
	(void)ui_add_label(&state, state.floating_panel, "Toggle Float Z to move", s_vec2(0.18f, 0.024f), SE_UI_ALIGN_STRETCH, SE_UI_ALIGN_START);

	ui_refresh_summary(&state);
	ui_refresh_layout_state(&state);
	ui_refresh_clip_state(&state);
	ui_refresh_scene_state(&state);
	ui_refresh_scroll_state(&state);
	ui_refresh_card_state(&state);
	ui_write_text(&state, state.hover_text, "hover: interact with widgets");
	ui_write_text(&state, state.focus_text, "focus: click textbox or scrollbox");
	ui_write_text(&state, state.textbox_state_text, "textbox: type text and press Enter");
	ui_write_text(&state, state.submit_state_text, "submit: waiting");
	ui_apply_alignment_state(&state);

	se_ui_set_debug_overlay(false);
	se_window_set_target_fps(window, 60);

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_context_reload_changed_shaders();

		if (se_window_is_key_pressed(window, SE_KEY_F1)) {
			ui_on_toggle_overlay_click(NULL, &state);
		}
		if (se_window_is_key_pressed(window, SE_KEY_F2)) {
			ui_on_cycle_direction_click(NULL, &state);
		}
		if (se_window_is_key_pressed(window, SE_KEY_F3)) {
			ui_on_cycle_align_h_click(NULL, &state);
		}
		if (se_window_is_key_pressed(window, SE_KEY_F4)) {
			ui_on_cycle_align_v_click(NULL, &state);
		}

		se_ui_tick(ui);
		s_vec2 pointer_ndc = s_vec2(0.0f, 0.0f);
		const f32 mouse_x = se_window_get_mouse_position_x(window);
		const f32 mouse_y = se_window_get_mouse_position_y(window);
		if (!se_window_pixel_to_normalized(window, mouse_x, mouse_y, &pointer_ndc)) {
			se_window_get_mouse_position_normalized(window, &pointer_ndc);
		}
		ui_update_card_drag(&state, &pointer_ndc);

		const se_ui_widget_handle hovered = se_ui_get_hovered_widget(ui);
		if (hovered != state.last_hovered) {
			state.last_hovered = hovered;
			ui_refresh_scene_state(&state);
		}

		const se_ui_widget_handle focused = se_ui_get_focused_widget(ui);
		if (focused != state.last_focused) {
			state.last_focused = focused;
			ui_refresh_scene_state(&state);
		}

		const f32 scroll_value = se_ui_widget_get_value(ui, state.scrollbox);
		if (fabsf(scroll_value - state.last_scroll_value) > 0.0001f) {
			state.last_scroll_value = scroll_value;
			ui_refresh_scroll_state(&state);
		}
		if (state.card_dragging) {
			ui_refresh_card_state(&state);
		}

		const b8 dirty = se_ui_is_dirty(ui);
		if (dirty != state.last_dirty) {
			state.last_dirty = dirty;
			ui_refresh_scene_state(&state);
		}

		se_render_clear();
		se_ui_draw(ui);
		se_window_render_screen(window);
	}

	se_ui_destroy(ui);
	s_array_clear(&state.scroll_items);
	se_context_destroy(ctx);
	return 0;
}
