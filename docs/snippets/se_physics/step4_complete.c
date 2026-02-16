#include "se_physics.h"

int main(void) {
	se_physics_world_params_2d world_params = {0};
	se_physics_world_2d* world = se_physics_world_2d_create(&world_params);
	se_physics_world_2d_step(world, 1.0f / 60.0f);
	se_physics_body_params_2d dynamic_params = {0};
	dynamic_params.type = SE_PHYSICS_BODY_DYNAMIC;
	se_physics_body_2d* body = se_physics_body_2d_create(world, &dynamic_params);
	s_vec2 zero = s_vec2(0.0f, 0.0f);
	s_vec2 half = s_vec2(0.5f, 0.5f);
	se_physics_body_2d_add_aabb(body, &zero, &half, 0);
	s_vec2 impulse = s_vec2(0.0f, 4.0f);
	se_physics_body_2d_apply_impulse(body, &impulse, &zero);
	for (u32 i = 0; i < 6; ++i) {
		se_physics_world_2d_step(world, 1.0f / 60.0f);
	}
	se_physics_body_2d_destroy(world, body);
	se_physics_world_2d_destroy(world);
	return 0;
}
