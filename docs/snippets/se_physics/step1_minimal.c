#include "se_physics.h"

int main(void) {
	se_physics_world_params_2d world_params = SE_PHYSICS_WORLD_PARAMS_2D_DEFAULTS;
	se_physics_world_2d_handle world = se_physics_world_2d_create(&world_params);
	se_physics_world_2d_step(world, 1.0f / 60.0f);
	return 0;
}
