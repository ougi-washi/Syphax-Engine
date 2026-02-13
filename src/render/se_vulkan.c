// Syphax-Engine - Ougi Washi

#if defined(SE_RENDER_BACKEND_VULKAN)
#include "se_render.h"
#include <stdio.h>

b8 se_render_init(void) {
	printf("se_render_init :: Vulkan backend not implemented\n");
	return false;
}

void se_render_shutdown(void) {}

b8 se_render_has_context(void) {
	return false;
}

u64 se_render_get_generation(void) {
	return 0;
}
#endif // SE_RENDER_BACKEND_VULKAN
