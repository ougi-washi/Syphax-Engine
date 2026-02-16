#include "se_vfx.h"

int main(void) {
	se_vfx_2d_handle vfx = se_vfx_2d_create(0);
	se_vfx_emitter_2d_handle emitter = se_vfx_2d_add_emitter(vfx, 0);
	se_vfx_2d_emitter_start(vfx, emitter);
	se_vfx_2d_emitter_set_spawn(vfx, emitter, 35.0f, 0, 0.3f, 1.2f);
	se_vfx_2d_emitter_burst(vfx, emitter, 24);
	se_vfx_2d_emitter_set_blend_mode(vfx, emitter, SE_VFX_BLEND_ALPHA);
	se_vfx_2d_emitter_stop(vfx, emitter);
	se_vfx_2d_destroy(vfx);
	return 0;
}
