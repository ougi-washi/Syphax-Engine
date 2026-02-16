#include "se_texture.h"

int main(void) {
	se_texture_handle texture = se_texture_load(SE_RESOURCE_PUBLIC("textures/default.png"), SE_REPEAT);
	se_texture_handle texture2 = se_texture_load_from_memory((const u8*)"abc", 3, SE_CLAMP);
	(void)texture;
	(void)texture2;
	se_texture_destroy(texture);
	se_texture_destroy(texture2);
	return 0;
}
