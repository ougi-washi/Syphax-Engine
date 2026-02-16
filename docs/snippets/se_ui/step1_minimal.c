#include "se_ui.h"

int main(void) {
	se_ui_handle ui = se_ui_create(S_HANDLE_NULL, 128);
	se_ui_tick(ui);
	return 0;
}
