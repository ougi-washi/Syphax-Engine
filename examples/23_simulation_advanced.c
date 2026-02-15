// Syphax-Engine - Ougi Washi

#include "se_simulation.h"
#include "se_debug.h"

#include "syphax/s_json.h"

#include <stdlib.h>
#include <string.h>

// Component ids used by this advanced scenario.
enum {
	SIM_COMPONENT_TRANSFORM = 1,
	SIM_COMPONENT_HEALTH = 2
};

// Event ids used by this advanced scenario.
enum {
	SIM_EVENT_IMPULSE = 1,
	SIM_EVENT_DAMAGE = 2
};

// Position, velocity, and optional scene linkage component.
typedef struct {
	s_vec3 position;
	s_vec3 velocity;
	se_scene_3d_handle scene;
} sim_transform;

// Health component.
typedef struct {
	f32 hp;
	f32 max_hp;
} sim_health;

// Event payload that adjusts entity velocity.
typedef struct {
	s_vec3 delta_velocity;
} sim_impulse_event;

// Event payload that subtracts health.
typedef struct {
	f32 amount;
} sim_damage_event;

// Compare transform values using s_types helpers.
static b8 sim_transform_equal(const sim_transform* a, const sim_transform* b, const f32 epsilon) {
	if (!a || !b) {
		return false;
	}
	return s_vec3_equal(&a->position, &b->position, epsilon) &&
		s_vec3_equal(&a->velocity, &b->velocity, epsilon);
}

// Print current simulation queue/capacity diagnostics.
static void sim_log_diagnostics(se_simulation_handle sim, const c8* label) {
	se_simulation_diagnostics diag = {0};
	se_simulation_get_diagnostics(sim, &diag);
	se_log(
		"23_simulation_advanced :: %s tick=%llu entities=%u ready_events=%u pending_events=%u payload=%u/%u",
		label ? label : "diagnostics",
		(unsigned long long)diag.current_tick,
		diag.entity_count,
		diag.ready_event_count,
		diag.pending_event_count,
		diag.used_event_payload_bytes,
		diag.max_event_payload_bytes);
}

// Print one entity's transform and health snapshot.
static void sim_log_entity_state(se_simulation_handle sim, const se_entity_id entity, const c8* label) {
	sim_transform transform = {0};
	sim_health health = {0};
	const b8 has_transform = se_simulation_component_get(sim, entity, SIM_COMPONENT_TRANSFORM, &transform, sizeof(transform));
	const b8 has_health = se_simulation_component_get(sim, entity, SIM_COMPONENT_HEALTH, &health, sizeof(health));
	if (!has_transform || !has_health) {
		se_log(
			"23_simulation_advanced :: %s entity=%llu state_unavailable",
			label ? label : "entity",
			(unsigned long long)entity);
		return;
	}
	se_log(
		"23_simulation_advanced :: %s entity=%llu pos=(%.3f %.3f %.3f) vel=(%.3f %.3f %.3f) hp=%.2f/%.2f",
		label ? label : "entity",
		(unsigned long long)entity,
		transform.position.x,
		transform.position.y,
		transform.position.z,
		transform.velocity.x,
		transform.velocity.y,
		transform.velocity.z,
		health.hp,
		health.max_hp);
}

// Apply all pending events for a single entity.
static void sim_apply_events_entity(se_simulation_handle sim, const se_entity_id entity, void* user_data) {
	(void)user_data;
	sim_transform* transform = (sim_transform*)se_simulation_component_get_ptr(sim, entity, SIM_COMPONENT_TRANSFORM);
	sim_health* health = (sim_health*)se_simulation_component_get_ptr(sim, entity, SIM_COMPONENT_HEALTH);
	if (!transform || !health) {
		return;
	}

	// Consume and apply all impulse events.
	sim_impulse_event impulse = {0};
	while (se_simulation_event_poll(sim, entity, SIM_EVENT_IMPULSE, &impulse, sizeof(impulse), NULL)) {
		transform->velocity.x += impulse.delta_velocity.x;
		transform->velocity.y += impulse.delta_velocity.y;
		transform->velocity.z += impulse.delta_velocity.z;
	}

	// Consume and apply all damage events.
	sim_damage_event damage = {0};
	while (se_simulation_event_poll(sim, entity, SIM_EVENT_DAMAGE, &damage, sizeof(damage), NULL)) {
		health->hp = s_max(0.0f, health->hp - damage.amount);
	}
}

// System wrapper: apply events for every alive entity.
static void sim_system_apply_events(se_simulation_handle sim, const se_sim_tick tick, void* user_data) {
	(void)tick;
	(void)user_data;
	se_simulation_for_each_entity(sim, sim_apply_events_entity, NULL);
}

// Integrate one entity transform using fixed dt.
static void sim_integrate_entity(se_simulation_handle sim, const se_entity_id entity, void* user_data) {
	const f32 dt = *(const f32*)user_data;
	sim_transform* transform = (sim_transform*)se_simulation_component_get_ptr(sim, entity, SIM_COMPONENT_TRANSFORM);
	if (!transform) {
		return;
	}
	transform->position.x += transform->velocity.x * dt;
	transform->position.y += transform->velocity.y * dt;
	transform->position.z += transform->velocity.z * dt;
}

// System wrapper: integrate all entity transforms.
static void sim_system_integrate(se_simulation_handle sim, const se_sim_tick tick, void* user_data) {
	(void)tick;
	(void)user_data;
	const f32 dt = se_simulation_get_fixed_dt(sim);
	se_simulation_for_each_entity(sim, sim_integrate_entity, (void*)&dt);
}

int main(void) {
	// Configure a constrained simulation for repeatable advanced tests.
	se_simulation_config config = SE_SIMULATION_CONFIG_DEFAULTS;
	config.max_entities = 64u;
	config.max_components_per_entity = 8u;
	config.max_events = 256u;
	config.max_event_payload_bytes = 16u * 1024u;
	config.fixed_dt = 1.0f / 30.0f;
	se_log(
		"23_simulation_advanced :: begin fixed_dt=%.5f max_entities=%u max_events=%u",
		config.fixed_dt,
		config.max_entities,
		config.max_events);

	// Create context and simulation instances.
	se_context* ctx = se_context_create();
	if (!ctx) {
		se_log("23_simulation_advanced :: failed to create context");
		return 1;
	}

	se_simulation_handle sim = se_simulation_create(&config);
	if (sim == SE_SIMULATION_HANDLE_NULL) {
		se_log("23_simulation_advanced :: failed to create simulation");
		se_context_destroy(ctx);
		return 1;
	}

	// Register all components, events, and systems used in this test.
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
		se_log("23_simulation_advanced :: setup failed, error=%s", se_result_str(se_get_last_error()));
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: setup complete");
	sim_log_diagnostics(sim, "after setup");

	// Create the primary entity and attach baseline components.
	const se_entity_id unit = se_simulation_entity_create(sim);
	if (unit == 0u) {
		se_log("23_simulation_advanced :: failed to create entity");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
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
		se_log("23_simulation_advanced :: failed to attach components");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_entity_state(sim, unit, "unit initial");

	// Define reusable event payloads for the primary entity.
	const sim_impulse_event impulse = {
		.delta_velocity = s_vec3(2.0f, 0.0f, 0.0f)
	};
	const sim_damage_event damage = {
		.amount = 12.5f
	};

	// Validate stale entity protection for event emit paths.
	const se_entity_id stale_unit = se_simulation_entity_create(sim);
	if (stale_unit == 0u || !se_simulation_entity_destroy(sim, stale_unit)) {
		se_log("23_simulation_advanced :: failed to create stale entity id for validation");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	if (se_simulation_event_emit(sim, stale_unit, SIM_EVENT_DAMAGE, &damage, sizeof(damage), 2u) ||
		se_get_last_error() != SE_RESULT_NOT_FOUND) {
		se_log("23_simulation_advanced :: stale target event emit should fail with not_found");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: stale entity event rejection OK");

	// Emit startup events and advance a few ticks.
	if (!se_simulation_event_emit(sim, unit, SIM_EVENT_IMPULSE, &impulse, sizeof(impulse), 1u) ||
		!se_simulation_event_emit(sim, unit, SIM_EVENT_DAMAGE, &damage, sizeof(damage), 2u)) {
		se_log("23_simulation_advanced :: failed to emit events");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "after initial event scheduling");

	if (!se_simulation_step(sim, 3u)) {
		se_log("23_simulation_advanced :: failed to step simulation");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}

	sim_transform after_step = {0};
	sim_health after_step_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &after_step, sizeof(after_step));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &after_step_health, sizeof(after_step_health));
	sim_log_diagnostics(sim, "after initial step");
	sim_log_entity_state(sim, unit, "unit after initial step");

	// Save initial checkpoint in memory and binary file.
	u8* snapshot_data = NULL;
	sz snapshot_size = 0u;
	if (!se_simulation_snapshot_save_memory(sim, &snapshot_data, &snapshot_size)) {
		se_log("23_simulation_advanced :: save memory snapshot failed");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: memory snapshot saved bytes=%zu", (size_t)snapshot_size);

	const char* snapshot_path = "23_simulation_advanced_snapshot.bin";
	if (!se_simulation_snapshot_save_file(sim, snapshot_path)) {
		se_log("23_simulation_advanced :: save file snapshot failed");
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: binary snapshot saved path=%s", snapshot_path);

	// Reject obviously invalid snapshot payloads.
	if (snapshot_size == 0u) {
		se_log("23_simulation_advanced :: snapshot size should not be zero");
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}

	u8* corrupted_snapshot_data = malloc(snapshot_size);
	if (!corrupted_snapshot_data) {
		se_log("23_simulation_advanced :: failed to allocate corrupted snapshot buffer");
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	memcpy(corrupted_snapshot_data, snapshot_data, snapshot_size);
	corrupted_snapshot_data[snapshot_size - 1u] ^= 0xFFu;
	if (se_simulation_snapshot_load_memory(sim, corrupted_snapshot_data, snapshot_size)) {
		se_log("23_simulation_advanced :: corrupted snapshot load should fail");
		free(corrupted_snapshot_data);
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	free(corrupted_snapshot_data);
	se_log("23_simulation_advanced :: corrupted snapshot rejection OK");

	// Ensure failed binary load did not mutate runtime state.
	sim_transform after_failed_load_transform = {0};
	sim_health after_failed_load_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &after_failed_load_transform, sizeof(after_failed_load_transform));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &after_failed_load_health, sizeof(after_failed_load_health));
	const b8 failed_load_preserved_state =
		sim_transform_equal(&after_failed_load_transform, &after_step, 0.0001f) &&
		s_f32_equal(after_failed_load_health.hp, after_step_health.hp, 0.0001f) &&
		se_simulation_get_tick(sim) == 3u;
	if (!failed_load_preserved_state) {
		se_log("23_simulation_advanced :: failed snapshot load should keep simulation state unchanged");
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}

	// Mutate runtime state, then restore from memory snapshot.
	after_step.position = s_vec3(100.0f, 100.0f, 100.0f);
	after_step.velocity = s_vec3(0.0f, 0.0f, 0.0f);
	after_step_health.hp = 1.0f;
	se_simulation_component_set(sim, unit, SIM_COMPONENT_TRANSFORM, &after_step, sizeof(after_step));
	se_simulation_component_set(sim, unit, SIM_COMPONENT_HEALTH, &after_step_health, sizeof(after_step_health));
	se_simulation_step(sim, 2u);

	if (!se_simulation_snapshot_load_memory(sim, snapshot_data, snapshot_size)) {
		se_log("23_simulation_advanced :: load memory snapshot failed");
		se_simulation_snapshot_free(snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "after memory snapshot reload");
	sim_log_entity_state(sim, unit, "unit after memory snapshot reload");

	// Validate restored state against expected deterministic values.
	se_simulation_snapshot_free(snapshot_data);

	sim_transform restored_transform = {0};
	sim_health restored_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &restored_transform, sizeof(restored_transform));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &restored_health, sizeof(restored_health));

	const b8 memory_roundtrip_ok =
		s_f32_equal(restored_transform.position.x, 0.2f, 0.0001f) &&
		s_f32_equal(restored_transform.velocity.x, 2.0f, 0.0001f) &&
		s_f32_equal(restored_health.hp, 87.5f, 0.0001f) &&
		se_simulation_get_tick(sim) == 3u;

	if (!memory_roundtrip_ok) {
		se_log("23_simulation_advanced :: memory snapshot roundtrip failed");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: memory snapshot roundtrip OK");

	// Save and validate JSON snapshot representation.
	s_json* snapshot_json = se_simulation_to_json(sim);
	if (!snapshot_json) {
		se_log("23_simulation_advanced :: to_json failed");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: json snapshot tree created");

	const char* snapshot_json_path = "23_simulation_advanced_snapshot.json";
	if (!se_simulation_json_save_file(sim, snapshot_json_path)) {
		se_log("23_simulation_advanced :: save file json failed");
		s_json_free(snapshot_json);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: json snapshot saved path=%s", snapshot_json_path);

	// Corrupt JSON version and validate strict rejection.
	s_json* corrupted_json = se_simulation_to_json(sim);
	if (!corrupted_json) {
		se_log("23_simulation_advanced :: to_json corrupted copy failed");
		s_json_free(snapshot_json);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	s_json* version_node = s_json_get(corrupted_json, "version");
	if (!version_node || version_node->type != S_JSON_NUMBER) {
		se_log("23_simulation_advanced :: corrupted json missing version node");
		s_json_free(corrupted_json);
		s_json_free(snapshot_json);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	version_node->as.number = 999.0;
	if (se_simulation_from_json(sim, corrupted_json)) {
		se_log("23_simulation_advanced :: corrupted json load should fail");
		s_json_free(corrupted_json);
		s_json_free(snapshot_json);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	s_json_free(corrupted_json);
	se_log("23_simulation_advanced :: corrupted json rejection OK");

	// Ensure failed JSON load did not mutate runtime state.
	sim_transform after_failed_json_load_transform = {0};
	sim_health after_failed_json_load_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &after_failed_json_load_transform, sizeof(after_failed_json_load_transform));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &after_failed_json_load_health, sizeof(after_failed_json_load_health));
	const b8 failed_json_load_preserved_state =
		sim_transform_equal(&after_failed_json_load_transform, &restored_transform, 0.0001f) &&
		s_f32_equal(after_failed_json_load_health.hp, restored_health.hp, 0.0001f) &&
		se_simulation_get_tick(sim) == 3u;
	if (!failed_json_load_preserved_state) {
		se_log("23_simulation_advanced :: failed json load should keep simulation state unchanged");
		s_json_free(snapshot_json);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}

	// Mutate runtime state, then restore from in-memory JSON.
	restored_transform.position = s_vec3(200.0f, 200.0f, 200.0f);
	restored_transform.velocity = s_vec3(0.0f, 0.0f, 0.0f);
	restored_health.hp = 2.0f;
	se_simulation_component_set(sim, unit, SIM_COMPONENT_TRANSFORM, &restored_transform, sizeof(restored_transform));
	se_simulation_component_set(sim, unit, SIM_COMPONENT_HEALTH, &restored_health, sizeof(restored_health));
	se_simulation_step(sim, 2u);

	if (!se_simulation_from_json(sim, snapshot_json)) {
		se_log("23_simulation_advanced :: from_json failed");
		s_json_free(snapshot_json);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "after in-memory json reload");
	sim_log_entity_state(sim, unit, "unit after in-memory json reload");

	// Validate in-memory JSON roundtrip values.
	sim_transform json_restored_transform = {0};
	sim_health json_restored_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &json_restored_transform, sizeof(json_restored_transform));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &json_restored_health, sizeof(json_restored_health));
	const b8 json_roundtrip_ok =
		s_f32_equal(json_restored_transform.position.x, 0.2f, 0.0001f) &&
		s_f32_equal(json_restored_transform.velocity.x, 2.0f, 0.0001f) &&
		s_f32_equal(json_restored_health.hp, 87.5f, 0.0001f) &&
		se_simulation_get_tick(sim) == 3u;
	if (!json_roundtrip_ok) {
		se_log("23_simulation_advanced :: json roundtrip failed");
		s_json_free(snapshot_json);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: json in-memory roundtrip OK");

	// Validate loading checkpoint from JSON file.
	se_simulation_step(sim, 4u);
	if (!se_simulation_json_load_file(sim, snapshot_json_path)) {
		se_log("23_simulation_advanced :: load file json failed");
		s_json_free(snapshot_json);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	if (se_simulation_get_tick(sim) != 3u) {
		se_log("23_simulation_advanced :: file json roundtrip failed");
		s_json_free(snapshot_json);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "after file json reload");

	// Validate loading checkpoint from binary file.
	s_json_free(snapshot_json);

	se_simulation_step(sim, 4u);
	if (!se_simulation_snapshot_load_file(sim, snapshot_path)) {
		se_log("23_simulation_advanced :: load file snapshot failed");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}

	if (se_simulation_get_tick(sim) != 3u) {
		se_log("23_simulation_advanced :: file snapshot roundtrip failed");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "after file snapshot reload");

	// Create a second entity for advanced multi-entity replay checks.
	const se_entity_id support = se_simulation_entity_create(sim);
	if (support == 0u) {
		se_log("23_simulation_advanced :: failed to create advanced support entity");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_transform support_transform = {
		.position = s_vec3(-1.0f, 0.0f, 0.0f),
		.velocity = s_vec3(1.0f, 0.0f, 0.0f),
		.scene = S_HANDLE_NULL
	};
	sim_health support_health = {
		.hp = 50.0f,
		.max_hp = 50.0f
	};
	if (!se_simulation_component_set(sim, support, SIM_COMPONENT_TRANSFORM, &support_transform, sizeof(support_transform)) ||
		!se_simulation_component_set(sim, support, SIM_COMPONENT_HEALTH, &support_health, sizeof(support_health))) {
		se_log("23_simulation_advanced :: failed to attach support components");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_entity_state(sim, support, "support initial");

	// Capture replay checkpoint tick and validate expected timeline start.
	const se_sim_tick advanced_start_tick = se_simulation_get_tick(sim);
	if (advanced_start_tick != 3u) {
		se_log("23_simulation_advanced :: advanced start tick mismatch");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: advanced checkpoint tick=%llu", (unsigned long long)advanced_start_tick);

	// Define queued and runtime replay events for both entities.
	const sim_impulse_event advanced_unit_ready_impulse = {
		.delta_velocity = s_vec3(3.0f, 0.0f, 0.0f)
	};
	const sim_impulse_event advanced_unit_tick4_impulse = {
		.delta_velocity = s_vec3(-1.0f, 0.0f, 0.0f)
	};
	const sim_damage_event advanced_unit_tick5_damage = {
		.amount = 5.0f
	};
	const sim_impulse_event advanced_support_tick4_impulse = {
		.delta_velocity = s_vec3(6.0f, 0.0f, 0.0f)
	};
	const sim_damage_event advanced_support_tick6_damage = {
		.amount = 8.0f
	};
	const sim_damage_event advanced_runtime_damage = {
		.amount = 1.5f
	};

	// Queue replay events at deterministic ticks and validate queue shape.
	if (!se_simulation_event_emit(sim, unit, SIM_EVENT_IMPULSE, &advanced_unit_ready_impulse, sizeof(advanced_unit_ready_impulse), advanced_start_tick) ||
		!se_simulation_event_emit(sim, unit, SIM_EVENT_IMPULSE, &advanced_unit_tick4_impulse, sizeof(advanced_unit_tick4_impulse), advanced_start_tick + 1u) ||
		!se_simulation_event_emit(sim, unit, SIM_EVENT_DAMAGE, &advanced_unit_tick5_damage, sizeof(advanced_unit_tick5_damage), advanced_start_tick + 2u) ||
		!se_simulation_event_emit(sim, support, SIM_EVENT_IMPULSE, &advanced_support_tick4_impulse, sizeof(advanced_support_tick4_impulse), advanced_start_tick + 1u) ||
		!se_simulation_event_emit(sim, support, SIM_EVENT_DAMAGE, &advanced_support_tick6_damage, sizeof(advanced_support_tick6_damage), advanced_start_tick + 3u)) {
		se_log("23_simulation_advanced :: failed to emit advanced events");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "advanced queued events");

	se_simulation_diagnostics advanced_diag = {0};
	se_simulation_get_diagnostics(sim, &advanced_diag);
	if (advanced_diag.ready_event_count != 1u || advanced_diag.pending_event_count != 4u) {
		se_log("23_simulation_advanced :: advanced queue setup mismatch");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}

	// Save advanced replay checkpoint in binary and JSON.
	u8* advanced_snapshot_data = NULL;
	sz advanced_snapshot_size = 0u;
	if (!se_simulation_snapshot_save_memory(sim, &advanced_snapshot_data, &advanced_snapshot_size) || advanced_snapshot_size == 0u) {
		se_log("23_simulation_advanced :: save advanced snapshot failed");
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: advanced snapshot saved bytes=%zu", (size_t)advanced_snapshot_size);
	s_json* advanced_snapshot_json = se_simulation_to_json(sim);
	if (!advanced_snapshot_json) {
		se_log("23_simulation_advanced :: save advanced json failed");
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: advanced json snapshot created");

	// Run baseline continuation and inject a runtime event mid-stream.
	if (!se_simulation_step(sim, 3u)) {
		se_log("23_simulation_advanced :: advanced baseline step failed");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "advanced baseline after +3 ticks");
	sim_log_entity_state(sim, unit, "advanced baseline unit before runtime emit");
	sim_log_entity_state(sim, support, "advanced baseline support before runtime emit");
	if (!se_simulation_event_emit(sim, unit, SIM_EVENT_DAMAGE, &advanced_runtime_damage, sizeof(advanced_runtime_damage), se_simulation_get_tick(sim))) {
		se_log("23_simulation_advanced :: runtime event emit during advanced baseline failed");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log(
		"23_simulation_advanced :: advanced baseline runtime event scheduled at tick=%llu",
		(unsigned long long)se_simulation_get_tick(sim));
	se_simulation_get_diagnostics(sim, &advanced_diag);
	if (advanced_diag.ready_event_count != 1u || advanced_diag.pending_event_count != 0u) {
		se_log("23_simulation_advanced :: advanced baseline runtime queue mismatch");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	if (!se_simulation_step(sim, 1u)) {
		se_log("23_simulation_advanced :: advanced baseline final step failed");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "advanced baseline final");
	se_simulation_get_diagnostics(sim, &advanced_diag);
	if (advanced_diag.ready_event_count != 0u || advanced_diag.pending_event_count != 0u) {
		se_log("23_simulation_advanced :: advanced baseline queue should be empty");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}

	// Capture baseline terminal state used for replay equivalence checks.
	const se_sim_tick advanced_baseline_tick = se_simulation_get_tick(sim);
	sim_transform advanced_baseline_unit_transform = {0};
	sim_health advanced_baseline_unit_health = {0};
	sim_transform advanced_baseline_support_transform = {0};
	sim_health advanced_baseline_support_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &advanced_baseline_unit_transform, sizeof(advanced_baseline_unit_transform));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &advanced_baseline_unit_health, sizeof(advanced_baseline_unit_health));
	se_simulation_component_get(sim, support, SIM_COMPONENT_TRANSFORM, &advanced_baseline_support_transform, sizeof(advanced_baseline_support_transform));
	se_simulation_component_get(sim, support, SIM_COMPONENT_HEALTH, &advanced_baseline_support_health, sizeof(advanced_baseline_support_health));
	sim_log_entity_state(sim, unit, "advanced baseline unit final");
	sim_log_entity_state(sim, support, "advanced baseline support final");

	const b8 advanced_baseline_expected =
		advanced_baseline_tick == 7u &&
		s_f32_equal(advanced_baseline_unit_transform.velocity.x, 4.0f, 0.0001f) &&
		s_f32_equal(advanced_baseline_unit_transform.position.x, 0.733333f, 0.0002f) &&
		s_f32_equal(advanced_baseline_unit_health.hp, 81.0f, 0.0001f) &&
		s_f32_equal(advanced_baseline_support_transform.velocity.x, 7.0f, 0.0001f) &&
		s_f32_equal(advanced_baseline_support_transform.position.x, -0.066666f, 0.0002f) &&
		s_f32_equal(advanced_baseline_support_health.hp, 42.0f, 0.0001f);
	if (!advanced_baseline_expected) {
		se_log("23_simulation_advanced :: advanced baseline expected values mismatch");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: advanced baseline reference captured");

	// Restore binary checkpoint, replay continuation, and compare with baseline.
	if (!se_simulation_snapshot_load_memory(sim, advanced_snapshot_data, advanced_snapshot_size)) {
		se_log("23_simulation_advanced :: advanced binary load failed");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "advanced binary restored checkpoint");
	se_simulation_get_diagnostics(sim, &advanced_diag);
	if (se_simulation_get_tick(sim) != advanced_start_tick || advanced_diag.ready_event_count != 1u || advanced_diag.pending_event_count != 4u) {
		se_log("23_simulation_advanced :: advanced binary state restore mismatch");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	if (!se_simulation_step(sim, 3u) ||
		!se_simulation_event_emit(sim, unit, SIM_EVENT_DAMAGE, &advanced_runtime_damage, sizeof(advanced_runtime_damage), se_simulation_get_tick(sim)) ||
		!se_simulation_step(sim, 1u)) {
		se_log("23_simulation_advanced :: advanced binary continuation failed");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "advanced binary final");

	sim_transform advanced_binary_unit_transform = {0};
	sim_health advanced_binary_unit_health = {0};
	sim_transform advanced_binary_support_transform = {0};
	sim_health advanced_binary_support_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &advanced_binary_unit_transform, sizeof(advanced_binary_unit_transform));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &advanced_binary_unit_health, sizeof(advanced_binary_unit_health));
	se_simulation_component_get(sim, support, SIM_COMPONENT_TRANSFORM, &advanced_binary_support_transform, sizeof(advanced_binary_support_transform));
	se_simulation_component_get(sim, support, SIM_COMPONENT_HEALTH, &advanced_binary_support_health, sizeof(advanced_binary_support_health));
	sim_log_entity_state(sim, unit, "advanced binary unit final");
	sim_log_entity_state(sim, support, "advanced binary support final");

	const b8 advanced_binary_matches_baseline =
		se_simulation_get_tick(sim) == advanced_baseline_tick &&
		sim_transform_equal(&advanced_binary_unit_transform, &advanced_baseline_unit_transform, 0.0001f) &&
		s_f32_equal(advanced_binary_unit_health.hp, advanced_baseline_unit_health.hp, 0.0001f) &&
		sim_transform_equal(&advanced_binary_support_transform, &advanced_baseline_support_transform, 0.0001f) &&
		s_f32_equal(advanced_binary_support_health.hp, advanced_baseline_support_health.hp, 0.0001f);
	if (!advanced_binary_matches_baseline) {
		se_log("23_simulation_advanced :: advanced binary mismatch vs baseline");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: advanced binary matches baseline");

	// Restore JSON checkpoint, replay continuation, and compare with baseline.
	if (!se_simulation_from_json(sim, advanced_snapshot_json)) {
		se_log("23_simulation_advanced :: advanced json load failed");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "advanced json restored checkpoint");
	se_simulation_get_diagnostics(sim, &advanced_diag);
	if (se_simulation_get_tick(sim) != advanced_start_tick || advanced_diag.ready_event_count != 1u || advanced_diag.pending_event_count != 4u) {
		se_log("23_simulation_advanced :: advanced json state restore mismatch");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	if (!se_simulation_step(sim, 3u) ||
		!se_simulation_event_emit(sim, unit, SIM_EVENT_DAMAGE, &advanced_runtime_damage, sizeof(advanced_runtime_damage), se_simulation_get_tick(sim)) ||
		!se_simulation_step(sim, 1u)) {
		se_log("23_simulation_advanced :: advanced json continuation failed");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	sim_log_diagnostics(sim, "advanced json final");

	sim_transform advanced_json_unit_transform = {0};
	sim_health advanced_json_unit_health = {0};
	sim_transform advanced_json_support_transform = {0};
	sim_health advanced_json_support_health = {0};
	se_simulation_component_get(sim, unit, SIM_COMPONENT_TRANSFORM, &advanced_json_unit_transform, sizeof(advanced_json_unit_transform));
	se_simulation_component_get(sim, unit, SIM_COMPONENT_HEALTH, &advanced_json_unit_health, sizeof(advanced_json_unit_health));
	se_simulation_component_get(sim, support, SIM_COMPONENT_TRANSFORM, &advanced_json_support_transform, sizeof(advanced_json_support_transform));
	se_simulation_component_get(sim, support, SIM_COMPONENT_HEALTH, &advanced_json_support_health, sizeof(advanced_json_support_health));
	sim_log_entity_state(sim, unit, "advanced json unit final");
	sim_log_entity_state(sim, support, "advanced json support final");

	const b8 advanced_json_matches_baseline =
		se_simulation_get_tick(sim) == advanced_baseline_tick &&
		sim_transform_equal(&advanced_json_unit_transform, &advanced_baseline_unit_transform, 0.0001f) &&
		s_f32_equal(advanced_json_unit_health.hp, advanced_baseline_unit_health.hp, 0.0001f) &&
		sim_transform_equal(&advanced_json_support_transform, &advanced_baseline_support_transform, 0.0001f) &&
		s_f32_equal(advanced_json_support_health.hp, advanced_baseline_support_health.hp, 0.0001f);
	if (!advanced_json_matches_baseline) {
		se_log("23_simulation_advanced :: advanced json mismatch vs baseline");
		s_json_free(advanced_snapshot_json);
		se_simulation_snapshot_free(advanced_snapshot_data);
		se_simulation_destroy(sim);
		se_context_destroy(ctx);
		return 1;
	}
	se_log("23_simulation_advanced :: advanced json matches baseline");

	// Release replay snapshots and finish.
	s_json_free(advanced_snapshot_json);
	se_simulation_snapshot_free(advanced_snapshot_data);

	se_log("23_simulation_advanced :: snapshot/json advanced restore OK");
	se_simulation_destroy(sim);
	se_context_destroy(ctx);
	return 0;
}
