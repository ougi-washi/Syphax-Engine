#include "se_texture.h"

int main(void) {
	se_texture_handle texture = se_texture_load(SE_RESOURCE_PUBLIC("textures/default.png"), SE_REPEAT);
	return 0;
}
