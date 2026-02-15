// Syphax-Engine - Ougi Washi

#ifndef SE_SIMULATION_H
#define SE_SIMULATION_H

#include "se.h"
#include "se_defines.h"

typedef u64 se_entity_id;
typedef u64 se_sim_tick;
typedef u32 se_sim_component_type;
typedef u32 se_sim_event_type;

#define SE_SIMULATION_HANDLE_NULL S_HANDLE_NULL

typedef struct se_entity {
	se_entity_id id;
} se_entity;

typedef struct {
	u32 max_entities;
	u32 max_components_per_entity;
	u32 max_events;
	u32 max_event_payload_bytes;
	f32 fixed_dt;
} se_simulation_config;

#define SE_SIMULATION_CONFIG_DEFAULTS ((se_simulation_config){ \
	.max_entities = 4096u, \
	.max_components_per_entity = 16u, \
	.max_events = 4096u, \
	.max_event_payload_bytes = (1024u * 1024u), \
	.fixed_dt = (1.0f / 60.0f) \
})

typedef struct {
	se_sim_component_type type;
	u32 size;
	u32 alignment;
	c8 name[SE_MAX_NAME_LENGTH];
} se_sim_component_desc;

typedef struct {
	se_sim_event_type type;
	u32 payload_size;
	c8 name[SE_MAX_NAME_LENGTH];
} se_sim_event_desc;

typedef struct {
	u32 entity_capacity;
	u32 entity_count;
	u32 component_registry_count;
	u32 event_registry_count;
	u32 pending_event_count;
	u32 ready_event_count;
	u32 max_events;
	u32 used_event_payload_bytes;
	u32 max_event_payload_bytes;
	se_sim_tick current_tick;
	f32 fixed_dt;
} se_simulation_diagnostics;

typedef void (*se_sim_system_fn)(se_simulation_handle sim, se_sim_tick tick, void* user_data);
typedef void (*se_sim_entity_iterator_fn)(se_simulation_handle sim, se_entity_id entity, void* user_data);

extern se_simulation_handle se_simulation_create(const se_simulation_config* config);
extern void se_simulation_destroy(se_simulation_handle sim);
extern void se_simulation_reset(se_simulation_handle sim);

extern se_entity_id se_simulation_entity_create(se_simulation_handle sim);
extern b8 se_simulation_entity_destroy(se_simulation_handle sim, se_entity_id entity);
extern b8 se_simulation_entity_alive(se_simulation_handle sim, se_entity_id entity);
extern u32 se_simulation_entity_count(se_simulation_handle sim);
extern void se_simulation_for_each_entity(se_simulation_handle sim, se_sim_entity_iterator_fn fn, void* user_data);

extern b8 se_simulation_component_register(se_simulation_handle sim, const se_sim_component_desc* desc);
extern b8 se_simulation_component_set(se_simulation_handle sim, se_entity_id entity, se_sim_component_type type, const void* data, sz size);
extern b8 se_simulation_component_get(se_simulation_handle sim, se_entity_id entity, se_sim_component_type type, void* out_data, sz out_size);
extern void* se_simulation_component_get_ptr(se_simulation_handle sim, se_entity_id entity, se_sim_component_type type);
extern const void* se_simulation_component_get_const_ptr(se_simulation_handle sim, se_entity_id entity, se_sim_component_type type);
extern b8 se_simulation_component_remove(se_simulation_handle sim, se_entity_id entity, se_sim_component_type type);

extern b8 se_simulation_event_register(se_simulation_handle sim, const se_sim_event_desc* desc);
extern b8 se_simulation_event_emit(se_simulation_handle sim, se_entity_id target, se_sim_event_type type, const void* payload, sz size, se_sim_tick deliver_at_tick);
extern b8 se_simulation_event_poll(se_simulation_handle sim, se_entity_id target, se_sim_event_type type, void* out_payload, sz out_size, se_sim_tick* out_tick);

extern b8 se_simulation_register_system(se_simulation_handle sim, se_sim_system_fn fn, void* user_data, i32 order);

extern b8 se_simulation_step(se_simulation_handle sim, u32 tick_count);
extern se_sim_tick se_simulation_get_tick(se_simulation_handle sim);
extern f32 se_simulation_get_fixed_dt(se_simulation_handle sim);
extern void se_simulation_get_diagnostics(se_simulation_handle sim, se_simulation_diagnostics* out_diag);

extern b8 se_simulation_snapshot_save_file(se_simulation_handle sim, const c8* path);
extern b8 se_simulation_snapshot_load_file(se_simulation_handle sim, const c8* path);
extern b8 se_simulation_snapshot_save_memory(se_simulation_handle sim, u8** out_data, sz* out_size);
extern b8 se_simulation_snapshot_load_memory(se_simulation_handle sim, const u8* data, sz size);
extern void se_simulation_snapshot_free(void* data);

#endif // SE_SIMULATION_H
