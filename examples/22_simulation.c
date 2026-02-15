// Syphax-Engine - Ougi Washi

#include "se_simulation.h"
#include "se_debug.h"

// Component ids used by this example.
enum {
	SIM_COMPONENT_TRANSFORM = 1,
	SIM_COMPONENT_HEALTH = 2
};

// Event ids used by this example.
enum {
	SIM_EVENT_IMPULSE = 1,
	SIM_EVENT_DAMAGE = 2
};

// Position and velocity state for one entity.
typedef struct {
	s_vec3 position;
	s_vec3 velocity;
} sim_transform;

// Health state for one entity.
typedef struct {
	f32 hp;
	f32 max_hp;
} sim_health;

// Event payload that pushes velocity.
typedef struct {
	s_vec3 delta_velocity;
} sim_impulse_event;

// Event payload that removes health.
typedef struct {
	f32 amount;
} sim_damage_event;

// Compact snapshot of one entity used in logs and equality checks.
typedef struct {
	sim_transform transform;
	sim_health health;
	se_sim_tick tick;
} sim_state;

// Read current entity components and tick into a single state struct.
static b8 sim_read_state(se_simulation_handle sim, const se_entity_id entity, sim_state* out_state) {
	if (!out_state) {
		return false;
	}
	if (!se_simulation_component_get(sim, entity, SIM_COMPONENT_TRANSFORM, &out_state->transform, sizeof(out_state->transform)) ||
		!se_simulation_component_get(sim, entity, SIM_COMPONENT_HEALTH, &out_state->health, sizeof(out_state->health))) {
		return false;
	}
	out_state->tick = se_simulation_get_tick(sim);
	return true;
}

// Print one state in a single log line.
static void sim_log_state(const c8* label, const sim_state* state) {
	if (!state) {
		return;
	}
	se_log(
		"22_simulation :: %s tick=%llu pos=(%.3f %.3f %.3f) vel=(%.3f %.3f %.3f) hp=%.2f/%.2f",
		label ? label : "state",
		(unsigned long long)state->tick,
		state->transform.position.x,
		state->transform.position.y,
		state->transform.position.z,
		state->transform.velocity.x,
		state->transform.velocity.y,
		state->transform.velocity.z,
		state->health.hp,
		state->health.max_hp);
}

// Compare two captured states using shared s_types equality helpers.
static b8 sim_state_equal(const sim_state* a, const sim_state* b, const f32 epsilon) {
	if (!a || !b) {
		return false;
	}
	return a->tick == b->tick &&
		s_vec3_equal(&a->transform.position, &b->transform.position, epsilon) &&
		s_vec3_equal(&a->transform.velocity, &b->transform.velocity, epsilon) &&
		s_f32_equal(a->health.hp, b->health.hp, epsilon) &&
		s_f32_equal(a->health.max_hp, b->health.max_hp, epsilon);
}

// Apply queued events for one entity.
static void sim_apply_events_entity(se_simulation_handle sim, const se_entity_id entity, void* user_data) {
	(void)user_data;
	sim_transform* transform = (sim_transform*)se_simulation_component_get_ptr(sim, entity, SIM_COMPONENT_TRANSFORM);
	sim_health* health = (sim_health*)se_simulation_component_get_ptr(sim, entity, SIM_COMPONENT_HEALTH);
	if (!transform || !health) {
		return;
	}

	// Consume all impulse events due for this entity.
	sim_impulse_event impulse = {0};
	while (se_simulation_event_poll(sim, entity, SIM_EVENT_IMPULSE, &impulse, sizeof(impulse), NULL)) {
		transform->velocity.x += impulse.delta_velocity.x;
		transform->velocity.y += impulse.delta_velocity.y;
		transform->velocity.z += impulse.delta_velocity.z;
	}

	// Consume all damage events due for this entity.
	sim_damage_event damage = {0};
	while (se_simulation_event_poll(sim, entity, SIM_EVENT_DAMAGE, &damage, sizeof(damage), NULL)) {
		health->hp = s_max(0.0f, health->hp - damage.amount);
	}
}

// System wrapper that applies events to every entity.
static void sim_system_apply_events(se_simulation_handle sim, const se_sim_tick tick, void* user_data) {
	(void)tick;
	(void)user_data;
	se_simulation_for_each_entity(sim, sim_apply_events_entity, NULL);
}

// Integrate position for one entity using fixed dt.
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

// System wrapper that integrates all entities.
static void sim_system_integrate(se_simulation_handle sim, const se_sim_tick tick, void* user_data) {
	(void)tick;
	(void)user_data;
	const f32 dt = se_simulation_get_fixed_dt(sim);
	se_simulation_for_each_entity(sim, sim_integrate_entity, (void*)&dt);
}

// Register components, events, and systems used by this example.
static b8 sim_setup(se_simulation_handle sim) {
	return se_simulation_component_register(sim, &(se_sim_component_desc){
		.type = SIM_COMPONENT_TRANSFORM,
		.size = sizeof(sim_transform),
		.alignment = 4u,
		.name = "transform"
	}) &&
		se_simulation_component_register(sim, &(se_sim_component_desc){
			.type = SIM_COMPONENT_HEALTH,
			.size = sizeof(sim_health),
			.alignment = 4u,
			.name = "health"
		}) &&
		se_simulation_event_register(sim, &(se_sim_event_desc){
			.type = SIM_EVENT_IMPULSE,
			.payload_size = sizeof(sim_impulse_event),
			.name = "impulse"
		}) &&
		se_simulation_event_register(sim, &(se_sim_event_desc){
			.type = SIM_EVENT_DAMAGE,
			.payload_size = sizeof(sim_damage_event),
			.name = "damage"
		}) &&
		se_simulation_register_system(sim, sim_system_apply_events, NULL, 0) &&
		se_simulation_register_system(sim, sim_system_integrate, NULL, 10);
}

// Emit the same replay input sequence and advance two ticks.
static b8 sim_emit_replay_inputs(se_simulation_handle sim, const se_entity_id player, const se_sim_tick start_tick) {
	const sim_impulse_event replay_impulse = {
		.delta_velocity = s_vec3(1.0f, 0.0f, 0.0f)
	};
	const sim_damage_event replay_damage = {
		.amount = 4.0f
	};

	return se_simulation_event_emit(sim, player, SIM_EVENT_IMPULSE, &replay_impulse, sizeof(replay_impulse), start_tick) &&
		se_simulation_event_emit(sim, player, SIM_EVENT_DAMAGE, &replay_damage, sizeof(replay_damage), start_tick + 1u) &&
		se_simulation_step(sim, 2u);
}

int main(void) {
	// Declare and initialize resources.
	se_context* ctx = se_context_create();
	u8* replay_snapshot_data = NULL;

	// Configure a small deterministic simulation.
	se_simulation_config config = SE_SIMULATION_CONFIG_DEFAULTS;
	config.max_entities = 64u;
	config.max_components_per_entity = 8u;
	config.max_events = 256u;
	config.max_event_payload_bytes = 16u * 1024u;
	config.fixed_dt = 1.0f / 30.0f;

	// Save snapshots next to the executable.
	const c8* snapshot_path = "22_simulation_snapshot.bin";
	const c8* snapshot_json_path = "22_simulation_snapshot.json";

	se_log("22_simulation :: begin (simple flow + replay)");

	// Create simulation, then register simulation schema/systems.
	se_simulation_handle sim = se_simulation_create(&config);
	sim_setup(sim);

	// Create a single gameplay entity used across the walkthrough.
	const se_entity_id player = se_simulation_entity_create(sim);

	// Attach starting transform + health components.
	sim_transform transform = {
		.position = s_vec3(0.0f, 0.0f, 0.0f),
		.velocity = s_vec3(0.0f, 0.0f, 0.0f)
	};
	sim_health health = {
		.hp = 100.0f,
		.max_hp = 100.0f
	};
	se_simulation_component_set(sim, player, SIM_COMPONENT_TRANSFORM, &transform, sizeof(transform));
	se_simulation_component_set(sim, player, SIM_COMPONENT_HEALTH, &health, sizeof(health));
	se_log("22_simulation :: entity created and components attached");

	// Run a short warmup with one impulse and one damage event.
	const se_sim_tick warmup_start_tick = se_simulation_get_tick(sim);
	const sim_impulse_event warmup_impulse = {
		.delta_velocity = s_vec3(2.0f, 0.0f, 0.0f)
	};
	const sim_damage_event warmup_damage = {
		.amount = 10.0f
	};
	
	// Triggering events
	se_simulation_event_emit(sim, player, SIM_EVENT_IMPULSE, &warmup_impulse, sizeof(warmup_impulse), warmup_start_tick);
	se_simulation_event_emit(sim, player, SIM_EVENT_DAMAGE, &warmup_damage, sizeof(warmup_damage), warmup_start_tick + 1u);
	se_simulation_step(sim, 2u);

	// Capture and log the post-warmup state.
	sim_state warmup_state = {0};
	sim_read_state(sim, player, &warmup_state);
	sim_log_state("after warmup", &warmup_state);

	// Save a replay checkpoint in memory and to binary/json files.
	sz replay_snapshot_size = 0u;
	se_simulation_snapshot_save_memory(sim, &replay_snapshot_data, &replay_snapshot_size);
	se_simulation_snapshot_save_file(sim, snapshot_path);
	se_simulation_json_save_file(sim, snapshot_json_path);

	// Replay run A from the checkpoint.
	const se_sim_tick replay_start_tick = se_simulation_get_tick(sim);
	se_log("22_simulation :: replay checkpoint saved at tick=%llu", (unsigned long long)replay_start_tick);

	sim_emit_replay_inputs(sim, player, replay_start_tick);
	
	sim_state replay_run_a = {0};
	sim_read_state(sim, player, &replay_run_a);
	sim_log_state("after replay run A", &replay_run_a);

	// Restore checkpoint and replay the same inputs again (run B).
	se_simulation_snapshot_load_memory(sim, replay_snapshot_data, replay_snapshot_size);
	se_log("22_simulation :: replay checkpoint restored");

	sim_emit_replay_inputs(sim, player, replay_start_tick);
	
	sim_state replay_run_b = {0};
	sim_read_state(sim, player, &replay_run_b);
	sim_log_state("after replay run B", &replay_run_b);
	se_log(
		"22_simulation :: replay run A/B equal=%d",
		sim_state_equal(&replay_run_a, &replay_run_b, 0.0001f) ? 1 : 0);

	// Restore checkpoint from JSON and compare with warmup state.
	se_simulation_json_load_file(sim, snapshot_json_path);

	sim_state json_restored_state = {0};
	sim_read_state(sim, player, &json_restored_state);
	sim_log_state("after json checkpoint load", &json_restored_state);
	se_log(
		"22_simulation :: json restored equals checkpoint=%d",
		sim_state_equal(&json_restored_state, &warmup_state, 0.0001f) ? 1 : 0);

	// Mark successful completion.
	se_log("22_simulation :: simple replay + snapshot flow OK");

	// Release resources in reverse ownership order.
	if (replay_snapshot_data) {
		se_simulation_snapshot_free(replay_snapshot_data);
	}
	if (sim != SE_SIMULATION_HANDLE_NULL) {
		se_simulation_destroy(sim);
	}
	if (ctx) {
		se_context_destroy(ctx);
	}
	return 0;
}
