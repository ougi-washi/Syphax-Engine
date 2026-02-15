// Syphax-Engine - Ougi Washi

#include "se_simulation.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	SIM_COMPONENT_TRANSFORM = 1,
	SIM_COMPONENT_HEALTH = 2
};

enum {
	SIM_EVENT_IMPULSE = 1,
	SIM_EVENT_DAMAGE = 2
};

typedef struct {
	s_vec3 position;
	s_vec3 velocity;
	se_scene_3d_handle scene;
} sim_transform;

typedef struct {
	f32 hp;
	f32 max_hp;
} sim_health;

typedef struct {
	s_vec3 delta_velocity;
} sim_impulse_event;

typedef struct {
	f32 amount;
} sim_damage_event;

static b8 sim_near_equal_f32(const f32 a, const f32 b, const f32 epsilon) {
	return fabsf(a - b) <= epsilon;
}

static b8 sim_transform_near_equal(const sim_transform* a, const sim_transform* b, const f32 epsilon) {
	if (!a || !b) {
		return false;
	}
	return sim_near_equal_f32(a->position.x, b->position.x, epsilon) &&
		sim_near_equal_f32(a->position.y, b->position.y, epsilon) &&
		sim_near_equal_f32(a->position.z, b->position.z, epsilon) &&
		sim_near_equal_f32(a->velocity.x, b->velocity.x, epsilon) &&
		sim_near_equal_f32(a->velocity.y, b->velocity.y, epsilon) &&
		sim_near_equal_f32(a->velocity.z, b->velocity.z, epsilon);
}

static void sim_apply_events_entity(se_simulation* sim, const se_entity_id entity, void* user_data) {
	(void)user_data;
	sim_transform* transform = (sim_transform*)se_simulation_component_get_ptr(sim, entity, SIM_COMPONENT_TRANSFORM);
	sim_health* health = (sim_health*)se_simulation_component_get_ptr(sim, entity, SIM_COMPONENT_HEALTH);
	if (!transform || !health) {
		return;
	}

	sim_impulse_event impulse = {0};
	while (se_simulation_event_poll(sim, entity, SIM_EVENT_IMPULSE, &impulse, sizeof(impulse), NULL)) {
		transform->velocity.x += impulse.delta_velocity.x;
		transform->velocity.y += impulse.delta_velocity.y;
		transform->velocity.z += impulse.delta_velocity.z;
	}

	sim_damage_event damage = {0};
	while (se_simulation_event_poll(sim, entity, SIM_EVENT_DAMAGE, &damage, sizeof(damage), NULL)) {
		health->hp = s_max(0.0f, health->hp - damage.amount);
	}
}

static void sim_system_apply_events(se_simulation* sim, const se_sim_tick tick, void* user_data) {
	(void)tick;
	(void)user_data;
	se_simulation_for_each_entity(sim, sim_apply_events_entity, NULL);
}

static void sim_integrate_entity(se_simulation* sim, const se_entity_id entity, void* user_data) {
	const f32 dt = *(const f32*)user_data;
	sim_transform* transform = (sim_transform*)se_simulation_component_get_ptr(sim, entity, SIM_COMPONENT_TRANSFORM);
	if (!transform) {
		return;
	}
	transform->position.x += transform->velocity.x * dt;
	transform->position.y += transform->velocity.y * dt;
	transform->position.z += transform->velocity.z * dt;
}

static void sim_system_integrate(se_simulation* sim, const se_sim_tick tick, void* user_data) {
	(void)tick;
	(void)user_data;
	const f32 dt = se_simulation_get_fixed_dt(sim);
	se_simulation_for_each_entity(sim, sim_integrate_entity, (void*)&dt);
}

int main(void) {
	se_simulation_config config = SE_SIMULATION_CONFIG_DEFAULTS;
	config.max_entities = 64u;
	config.max_components_per_entity = 8u;
	config.max_events = 256u;
	config.max_event_payload_bytes = 16u * 1024u;
	config.fixed_dt = 1.0f / 30.0f;

	se_simulation* sim = se_simulation_create(&config);
	if (!sim) {
		printf("22_simulation :: failed to create simulation\n");
		return 1;
	}

	if (!se_simulation_component_register(sim, &(se_sim_component_desc){
		.type = SIM_COMPONENT_TRANSFORM,
		.size = sizeof(sim_transform),
		.alignment = 4u,
		.name = "transform"
	}) ||
		!se_simulation_component_register(sim, &(se_sim_component_desc){
		.type = SIM_COMPONENT_HEALTH,
		.size = sizeof(sim_health),
		.alignment = 4u,
		.name = "health"
	}) ||
		!se_simulation_event_register(sim, &(se_sim_event_desc){
		.type = SIM_EVENT_IMPULSE,
		.payload_size = sizeof(sim_impulse_event),
		.name = "impulse"
	}) ||
		!se_simulation_event_register(sim, &(se_sim_event_desc){
		.type = SIM_EVENT_DAMAGE,
		.payload_size = sizeof(sim_damage_event),
		.name = "damage"
	}) ||
		!se_simulation_register_system(sim, sim_system_apply_events, NULL, 0) ||
		!se_simulation_register_system(sim, sim_system_integrate, NULL, 10)) {
		printf("22_simulation :: setup failed, error=%s\n", se_result_str(se_get_last_error()));
		se_simulation_destroy(sim);
		return 1;
	}

	const se_entity_id unit = se_simulation_entity_create(sim);
	if (unit == 0u) {
		printf("22_simulation :: failed to create entity\n");
		se_simulation_destroy(sim);
		return 1;
	}

	sim_transform transform = {
		.position = s_vec3(0.0f, 0.0f, 0.0f),
		.velocity = s_vec3(0.0f, 0.0f, 0.0f),
		.scene = S_HANDLE_NULL
	};
	sim_health health = {
		.hp = 100.0f,
		.max_hp = 100.0f
	};
	if (!se_simulation_component_set(sim, unit, SIM_COMPONENT_TRANSFORM, &transform, sizeof(transform)) ||
		!se_simulation_component_set(sim, unit, SIM_COMPONENT_HEALTH, &health, sizeof(health))) {
		printf("22_simulation :: failed to attach components\n");
		se_simulation_destroy(sim);
		return 1;
	}

	const sim_impulse_event impulse = {
		.delta_velocity = s_vec3(2.0f, 0.0f, 0.0f)
	};
	const sim_damage_event damage = {
		.amount = 12.5f
	};

	const se_entity_id stale_unit = se_simulation_entity_create(sim);
	if (stale_unit == 0u || !se_simulation_entity_destroy(sim, stale_unit)) {
		printf("22_simulation :: failed to create stale entity id for validation\n");
		se_simulation_destroy(sim);
		return 1;
	}
	if (se_simulation_event_emit(sim, stale_unit, SIM_EVENT_DAMAGE, &damage, sizeof(damage), 2u) ||
		se_get_last_error() != SE_RESULT_NOT_FOUND) {
		printf("22_simulation :: stale target event emit should fail with not_found\n");
		se_simulation_destroy(sim);
		return 1;
	}

	if (!se_simulation_event_emit(sim, unit, SIM_EVENT_IMPULSE, &impulse, sizeof(impulse), 1u) ||
		!se_simulation_event_emit(sim, unit, SIM_EVENT_DAMAGE, &damage, sizeof(damage), 2u)) {
		printf("22_simulation :: failed to emit events\n");
		se_simulation_destroy(sim);
		return 1;
	}

	if (!se_simulation_step(sim, 3u)) {
		printf("22_simulation :: failed to step simulation\n");
		se_simulation_destroy(sim);
		return 1;
	}

	sim_transform after_step = {0};
	sim_health after_step_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &after_step, sizeof(after_step));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &after_step_health, sizeof(after_step_health));

	printf(
		"22_simulation :: tick=%llu pos=(%.3f %.3f %.3f) vel=(%.3f %.3f %.3f) hp=%.2f\n",
		(unsigned long long)se_simulation_get_tick(sim),
		after_step.position.x,
		after_step.position.y,
		after_step.position.z,
		after_step.velocity.x,
		after_step.velocity.y,
		after_step.velocity.z,
		after_step_health.hp);

	u8* snapshot_data = NULL;
	sz snapshot_size = 0u;
	if (!se_simulation_snapshot_save_memory(sim, &snapshot_data, &snapshot_size)) {
		printf("22_simulation :: save memory snapshot failed\n");
		se_simulation_destroy(sim);
		return 1;
	}

	const char* snapshot_path = "/tmp/syphax_simulation_snapshot.bin";
	if (!se_simulation_snapshot_save_file(sim, snapshot_path)) {
		printf("22_simulation :: save file snapshot failed\n");
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		return 1;
	}

	if (snapshot_size == 0u) {
		printf("22_simulation :: snapshot size should not be zero\n");
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		return 1;
	}

	u8* corrupted_snapshot_data = malloc(snapshot_size);
	if (!corrupted_snapshot_data) {
		printf("22_simulation :: failed to allocate corrupted snapshot buffer\n");
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		return 1;
	}
	memcpy(corrupted_snapshot_data, snapshot_data, snapshot_size);
	corrupted_snapshot_data[snapshot_size - 1u] ^= 0xFFu;
	if (se_simulation_snapshot_load_memory(sim, corrupted_snapshot_data, snapshot_size)) {
		printf("22_simulation :: corrupted snapshot load should fail\n");
		free(corrupted_snapshot_data);
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		return 1;
	}
	free(corrupted_snapshot_data);

	sim_transform after_failed_load_transform = {0};
	sim_health after_failed_load_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &after_failed_load_transform, sizeof(after_failed_load_transform));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &after_failed_load_health, sizeof(after_failed_load_health));
	const b8 failed_load_preserved_state =
		sim_transform_near_equal(&after_failed_load_transform, &after_step, 0.0001f) &&
		sim_near_equal_f32(after_failed_load_health.hp, after_step_health.hp, 0.0001f) &&
		se_simulation_get_tick(sim) == 3u;
	if (!failed_load_preserved_state) {
		printf("22_simulation :: failed snapshot load should keep simulation state unchanged\n");
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		return 1;
	}

	after_step.position = s_vec3(100.0f, 100.0f, 100.0f);
	after_step.velocity = s_vec3(0.0f, 0.0f, 0.0f);
	after_step_health.hp = 1.0f;
	se_simulation_component_set(sim, unit, SIM_COMPONENT_TRANSFORM, &after_step, sizeof(after_step));
	se_simulation_component_set(sim, unit, SIM_COMPONENT_HEALTH, &after_step_health, sizeof(after_step_health));
	se_simulation_step(sim, 2u);

	if (!se_simulation_snapshot_load_memory(sim, snapshot_data, snapshot_size)) {
		printf("22_simulation :: load memory snapshot failed\n");
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		return 1;
	}

	se_simulation_snapshot_free(snapshot_data);

	sim_transform restored_transform = {0};
	sim_health restored_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &restored_transform, sizeof(restored_transform));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &restored_health, sizeof(restored_health));

	const b8 memory_roundtrip_ok =
		sim_near_equal_f32(restored_transform.position.x, 0.2f, 0.0001f) &&
		sim_near_equal_f32(restored_transform.velocity.x, 2.0f, 0.0001f) &&
		sim_near_equal_f32(restored_health.hp, 87.5f, 0.0001f) &&
		se_simulation_get_tick(sim) == 3u;

	if (!memory_roundtrip_ok) {
		printf("22_simulation :: memory snapshot roundtrip failed\n");
		se_simulation_destroy(sim);
		return 1;
	}

	se_simulation_step(sim, 4u);
	if (!se_simulation_snapshot_load_file(sim, snapshot_path)) {
		printf("22_simulation :: load file snapshot failed\n");
		se_simulation_destroy(sim);
		return 1;
	}

	if (se_simulation_get_tick(sim) != 3u) {
		printf("22_simulation :: file snapshot roundtrip failed\n");
		se_simulation_destroy(sim);
		return 1;
	}

	printf("22_simulation :: snapshot restore OK\n");
	se_simulation_destroy(sim);
	return 0;
}
