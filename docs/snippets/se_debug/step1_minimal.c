#include "se_debug.h"

int main(void) {
	se_debug_set_level(SE_DEBUG_LEVEL_TRACE);
	se_debug_frame_begin();
	se_debug_trace_begin("physics_step");
	return 0;
}
