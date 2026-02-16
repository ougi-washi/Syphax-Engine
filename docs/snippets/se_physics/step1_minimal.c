#include "se_physics.h"

int main(void) {
	se_physics_world_params_2d world_params = {0};
	se_physics_world_2d* world = se_physics_world_2d_create(&world_params);
	se_physics_world_2d_step(world, 1.0f / 60.0f);
	return 0;
}
