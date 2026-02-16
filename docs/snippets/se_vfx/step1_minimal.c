#include "se_vfx.h"

int main(void) {
	se_vfx_2d_handle vfx = se_vfx_2d_create(0);
	se_vfx_emitter_2d_handle emitter = se_vfx_2d_add_emitter(vfx, 0);
	return 0;
}
