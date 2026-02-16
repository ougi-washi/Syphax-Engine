#include "se_ui.h"

int main(void) {
	se_ui_handle ui = se_ui_create(S_HANDLE_NULL, 128);
	se_ui_tick(ui);
	se_ui_draw(ui);
	se_ui_mark_layout_dirty(ui);
	se_ui_mark_visual_dirty(ui);
	return 0;
}
