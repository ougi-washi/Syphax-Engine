// Syphax-Engine - Ougi Washi

#if defined(SE_RENDER_BACKEND_VULKAN)
#include "se_render.h"
#include <stdio.h>

b8 se_render_init(void) {
	printf("se_render_init :: Vulkan backend not implemented\n");
	return false;
}
#endif // SE_RENDER_BACKEND_VULKAN
