// Syphax-Engine - Ougi Washi

#include "se_simulation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct se_simulation se_simulation;

typedef s_array(u8, se_sim_bytes);
typedef s_array(u32, se_sim_u32s);

typedef struct {
	se_sim_component_type type;
	u32 size;
	u32 alignment;
	c8 name[SE_MAX_NAME_LENGTH];
} se_sim_component_meta;
typedef s_array(se_sim_component_meta, se_sim_component_metas);

typedef struct {
	se_sim_component_type type;
	se_sim_bytes data;
} se_sim_component_blob;
typedef s_array(se_sim_component_blob, se_sim_component_blobs);

typedef struct {
	u32 generation;
	b8 alive : 1;
	se_sim_component_blobs components;
} se_sim_entity_slot;
typedef s_array(se_sim_entity_slot, se_sim_entity_slots);

typedef struct {
	se_sim_event_type type;
	u32 payload_size;
	c8 name[SE_MAX_NAME_LENGTH];
} se_sim_event_meta;
typedef s_array(se_sim_event_meta, se_sim_event_metas);

typedef struct {
	se_entity_id target;
	se_sim_event_type type;
	se_sim_tick deliver_at_tick;
	u64 sequence;
	se_sim_bytes payload;
} se_sim_event_record;
typedef s_array(se_sim_event_record, se_sim_event_records);

typedef struct {
	se_sim_system_fn fn;
	void* user_data;
	i32 order;
	u64 registration_index;
} se_sim_system_entry;
typedef s_array(se_sim_system_entry, se_sim_system_entries);

struct se_simulation {
	se_simulation_handle self_handle;
	se_simulation_config config;
	se_sim_tick current_tick;
	u64 next_sequence;
	u64 next_system_registration_index;
	u32 alive_entities;
	u32 used_event_payload_bytes;
	se_sim_entity_slots entity_slots;
	se_sim_u32s free_slots;
	se_sim_component_metas component_registry;
	se_sim_event_metas event_registry;
	se_sim_event_records pending_events;
	se_sim_event_records ready_events;
	se_sim_system_entries systems;
};

static se_simulation* se_simulation_from_handle_mut(const se_simulation_handle handle) {
	if (handle == SE_SIMULATION_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_context* ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_simulation* sim = s_array_get(&ctx->simulations, handle);
	if (!sim) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	return sim;
}

static const se_simulation* se_simulation_from_handle_const(const se_simulation_handle handle) {
	if (handle == SE_SIMULATION_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_context* ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	const se_simulation* sim = s_array_get(&ctx->simulations, handle);
	if (!sim) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	return sim;
}

typedef struct {
	const u8* data;
	sz size;
	sz cursor;
} se_sim_reader;

typedef struct {
	u32 type;
	se_sim_bytes bytes;
} se_sim_snapshot_section_data;
typedef s_array(se_sim_snapshot_section_data, se_sim_snapshot_sections);

typedef struct {
	const u8* data;
	sz size;
	b8 found;
} se_sim_snapshot_section_view;

enum {
	SE_SIM_SNAPSHOT_VERSION = 1u,
	SE_SIM_SNAPSHOT_ENDIANNESS_LITTLE = 1u,
	SE_SIM_SNAPSHOT_HEADER_SIZE = 48u,
	SE_SIM_SNAPSHOT_SECTION_ENTRY_SIZE = 24u
};

enum {
	SE_SIM_SNAPSHOT_SECTION_CONFIG = 1u,
	SE_SIM_SNAPSHOT_SECTION_TICK = 2u,
	SE_SIM_SNAPSHOT_SECTION_ENTITY_TABLE = 3u,
	SE_SIM_SNAPSHOT_SECTION_COMPONENT_REGISTRY = 4u,
	SE_SIM_SNAPSHOT_SECTION_COMPONENT_PAYLOADS = 5u,
	SE_SIM_SNAPSHOT_SECTION_EVENT_REGISTRY = 6u,
	SE_SIM_SNAPSHOT_SECTION_EVENT_QUEUES = 7u
};

#define SE_SIM_ENTITY_ID_INVALID ((se_entity_id)0)

static se_entity_id se_sim_entity_id_make(const u32 slot, const u32 generation) {
	return ((u64)generation << 32) | (u64)slot;
}

static u32 se_sim_entity_id_slot(const se_entity_id id) {
	return (u32)(id & 0xFFFFFFFFu);
}

static u32 se_sim_entity_id_generation(const se_entity_id id) {
	return (u32)(id >> 32);
}

static b8 se_simulation_config_validate(const se_simulation_config* config) {
	if (!config) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (config->max_entities == 0u ||
		config->max_components_per_entity == 0u ||
		config->max_events == 0u ||
		config->max_event_payload_bytes == 0u ||
		config->fixed_dt <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	return true;
}

static void se_sim_bytes_copy_from_internal(se_sim_bytes* out, const void* data, const u32 size) {
	s_array_init(out);
	if (size == 0u) {
		return;
	}
	s_array_reserve(out, size);
	const u8* src = (const u8*)data;
	for (u32 i = 0; i < size; ++i) {
		u8 value = src[i];
		s_array_add(out, value);
	}
}

static b8 se_sim_bytes_set(se_sim_bytes* out, const void* data, const u32 size) {
	if (!out) {
		return false;
	}
	s_array_clear(out);
	s_array_init(out);
	if (size == 0u) {
		return true;
	}
	if (!data) {
		return false;
	}
	s_array_reserve(out, size);
	const u8* src = (const u8*)data;
	for (u32 i = 0; i < size; ++i) {
		u8 value = src[i];
		s_array_add(out, value);
	}
	return true;
}

static b8 se_sim_bytes_append(se_sim_bytes* out, const void* data, const sz size) {
	if (!out) {
		return false;
	}
	if (size == 0) {
		return true;
	}
	if (!data) {
		return false;
	}
	const sz current_size = s_array_get_size(out);
	s_array_reserve(out, current_size + size);
	const u8* src = (const u8*)data;
	for (sz i = 0; i < size; ++i) {
		u8 value = src[i];
		s_array_add(out, value);
	}
	return true;
}

static b8 se_sim_bytes_append_u8(se_sim_bytes* out, const u8 value) {
	return se_sim_bytes_append(out, &value, sizeof(value));
}

static b8 se_sim_bytes_append_u16(se_sim_bytes* out, const u16 value) {
	u8 bytes[2] = {
		(u8)(value & 0xFFu),
		(u8)((value >> 8) & 0xFFu)
	};
	return se_sim_bytes_append(out, bytes, sizeof(bytes));
}

static b8 se_sim_bytes_append_u32(se_sim_bytes* out, const u32 value) {
	u8 bytes[4] = {
		(u8)(value & 0xFFu),
		(u8)((value >> 8) & 0xFFu),
		(u8)((value >> 16) & 0xFFu),
		(u8)((value >> 24) & 0xFFu)
	};
	return se_sim_bytes_append(out, bytes, sizeof(bytes));
}

static b8 se_sim_bytes_append_u64(se_sim_bytes* out, const u64 value) {
	u8 bytes[8] = {
		(u8)(value & 0xFFu),
		(u8)((value >> 8) & 0xFFu),
		(u8)((value >> 16) & 0xFFu),
		(u8)((value >> 24) & 0xFFu),
		(u8)((value >> 32) & 0xFFu),
		(u8)((value >> 40) & 0xFFu),
		(u8)((value >> 48) & 0xFFu),
		(u8)((value >> 56) & 0xFFu)
	};
	return se_sim_bytes_append(out, bytes, sizeof(bytes));
}

static u32 se_sim_f32_to_u32(const f32 value) {
	union {
		f32 f;
		u32 u;
	} cast = {0};
	cast.f = value;
	return cast.u;
}

static f32 se_sim_u32_to_f32(const u32 value) {
	union {
		f32 f;
		u32 u;
	} cast = {0};
	cast.u = value;
	return cast.f;
}

static b8 se_sim_reader_read(se_sim_reader* reader, void* out_data, const sz size) {
	if (!reader || !out_data) {
		return false;
	}
	if (reader->cursor + size > reader->size) {
		return false;
	}
	memcpy(out_data, reader->data + reader->cursor, size);
	reader->cursor += size;
	return true;
}

static b8 se_sim_reader_skip(se_sim_reader* reader, const sz size) {
	if (!reader) {
		return false;
	}
	if (reader->cursor + size > reader->size) {
		return false;
	}
	reader->cursor += size;
	return true;
}

static b8 se_sim_reader_read_u8(se_sim_reader* reader, u8* out_value) {
	return se_sim_reader_read(reader, out_value, sizeof(*out_value));
}

static b8 se_sim_reader_read_u16(se_sim_reader* reader, u16* out_value) {
	u8 bytes[2] = {0};
	if (!se_sim_reader_read(reader, bytes, sizeof(bytes))) {
		return false;
	}
	*out_value = (u16)((u16)bytes[0] | ((u16)bytes[1] << 8));
	return true;
}

static b8 se_sim_reader_read_u32(se_sim_reader* reader, u32* out_value) {
	u8 bytes[4] = {0};
	if (!se_sim_reader_read(reader, bytes, sizeof(bytes))) {
		return false;
	}
	*out_value =
		((u32)bytes[0]) |
		((u32)bytes[1] << 8) |
		((u32)bytes[2] << 16) |
		((u32)bytes[3] << 24);
	return true;
}

static b8 se_sim_reader_read_u64(se_sim_reader* reader, u64* out_value) {
	u8 bytes[8] = {0};
	if (!se_sim_reader_read(reader, bytes, sizeof(bytes))) {
		return false;
	}
	*out_value =
		((u64)bytes[0]) |
		((u64)bytes[1] << 8) |
		((u64)bytes[2] << 16) |
		((u64)bytes[3] << 24) |
		((u64)bytes[4] << 32) |
		((u64)bytes[5] << 40) |
		((u64)bytes[6] << 48) |
		((u64)bytes[7] << 56);
	return true;
}

static b8 se_sim_reader_read_bytes(se_sim_reader* reader, const u8** out_data, const u32 size) {
	if (!reader || !out_data) {
		return false;
	}
	if ((sz)size > reader->size - reader->cursor) {
		return false;
	}
	*out_data = reader->data + reader->cursor;
	reader->cursor += size;
	return true;
}

static b8 se_simulation_component_desc_valid(const se_sim_component_desc* desc) {
	return desc && desc->type != 0u && desc->size > 0u;
}

static b8 se_simulation_event_desc_valid(const se_sim_event_desc* desc) {
	return desc && desc->type != 0u;
}

static se_sim_component_meta* se_simulation_find_component_meta(se_simulation* sim, const se_sim_component_type type) {
	if (!sim || type == 0u) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size(&sim->component_registry); ++i) {
		se_sim_component_meta* meta = s_array_get(&sim->component_registry, s_array_handle(&sim->component_registry, (u32)i));
		if (meta && meta->type == type) {
			return meta;
		}
	}
	return NULL;
}

static const se_sim_component_meta* se_simulation_find_component_meta_const(const se_simulation* sim, const se_sim_component_type type) {
	if (!sim || type == 0u) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size((se_sim_component_metas*)&sim->component_registry); ++i) {
		const se_sim_component_meta* meta = s_array_get((se_sim_component_metas*)&sim->component_registry, s_array_handle((se_sim_component_metas*)&sim->component_registry, (u32)i));
		if (meta && meta->type == type) {
			return meta;
		}
	}
	return NULL;
}

static se_sim_event_meta* se_simulation_find_event_meta(se_simulation* sim, const se_sim_event_type type) {
	if (!sim || type == 0u) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size(&sim->event_registry); ++i) {
		se_sim_event_meta* meta = s_array_get(&sim->event_registry, s_array_handle(&sim->event_registry, (u32)i));
		if (meta && meta->type == type) {
			return meta;
		}
	}
	return NULL;
}

static const se_sim_event_meta* se_simulation_find_event_meta_const(const se_simulation* sim, const se_sim_event_type type) {
	if (!sim || type == 0u) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size((se_sim_event_metas*)&sim->event_registry); ++i) {
		const se_sim_event_meta* meta = s_array_get((se_sim_event_metas*)&sim->event_registry, s_array_handle((se_sim_event_metas*)&sim->event_registry, (u32)i));
		if (meta && meta->type == type) {
			return meta;
		}
	}
	return NULL;
}

static void se_sim_component_blob_clear(se_sim_component_blob* blob) {
	if (!blob) {
		return;
	}
	s_array_clear(&blob->data);
	memset(blob, 0, sizeof(*blob));
}

static void se_sim_entity_slot_clear_components(se_sim_entity_slot* slot) {
	if (!slot) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&slot->components); ++i) {
		se_sim_component_blob* blob = s_array_get(&slot->components, s_array_handle(&slot->components, (u32)i));
		if (!blob) {
			continue;
		}
		se_sim_component_blob_clear(blob);
	}
	s_array_clear(&slot->components);
	s_array_init(&slot->components);
}

static void se_sim_event_record_clear(se_sim_event_record* event_record) {
	if (!event_record) {
		return;
	}
	s_array_clear(&event_record->payload);
	memset(event_record, 0, sizeof(*event_record));
}

static void se_sim_event_queue_clear(se_sim_event_records* queue) {
	if (!queue) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(queue); ++i) {
		se_sim_event_record* event_record = s_array_get(queue, s_array_handle(queue, (u32)i));
		if (!event_record) {
			continue;
		}
		se_sim_event_record_clear(event_record);
	}
	s_array_clear(queue);
	s_array_init(queue);
}

static void se_simulation_entities_clear(se_simulation* sim) {
	if (!sim) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&sim->entity_slots); ++i) {
		se_sim_entity_slot* slot = s_array_get(&sim->entity_slots, s_array_handle(&sim->entity_slots, (u32)i));
		if (!slot) {
			continue;
		}
		se_sim_entity_slot_clear_components(slot);
	}
	s_array_clear(&sim->entity_slots);
	s_array_init(&sim->entity_slots);
	s_array_clear(&sim->free_slots);
	s_array_init(&sim->free_slots);
	sim->alive_entities = 0u;
}

static void se_simulation_events_clear(se_simulation* sim) {
	if (!sim) {
		return;
	}
	se_sim_event_queue_clear(&sim->pending_events);
	se_sim_event_queue_clear(&sim->ready_events);
	sim->used_event_payload_bytes = 0u;
}

static void se_simulation_registries_clear(se_simulation* sim) {
	if (!sim) {
		return;
	}
	s_array_clear(&sim->component_registry);
	s_array_init(&sim->component_registry);
	s_array_clear(&sim->event_registry);
	s_array_init(&sim->event_registry);
}

static void se_simulation_clear_runtime_state_for_load(se_simulation* sim) {
	if (!sim) {
		return;
	}
	se_simulation_entities_clear(sim);
	se_simulation_events_clear(sim);
	se_simulation_registries_clear(sim);
	sim->current_tick = 0u;
	sim->next_sequence = 1u;
}

static se_sim_entity_slot* se_simulation_entity_slot_from_id(se_simulation* sim, const se_entity_id entity, u32* out_slot_index) {
	if (!sim || entity == SE_SIM_ENTITY_ID_INVALID) {
		return NULL;
	}
	const u32 slot_index = se_sim_entity_id_slot(entity);
	const u32 generation = se_sim_entity_id_generation(entity);
	if ((sz)slot_index >= s_array_get_size(&sim->entity_slots)) {
		return NULL;
	}
	se_sim_entity_slot* slot = s_array_get(&sim->entity_slots, s_array_handle(&sim->entity_slots, slot_index));
	if (!slot || !slot->alive || slot->generation != generation) {
		return NULL;
	}
	if (out_slot_index) {
		*out_slot_index = slot_index;
	}
	return slot;
}

static const se_sim_entity_slot* se_simulation_entity_slot_from_id_const(const se_simulation* sim, const se_entity_id entity, u32* out_slot_index) {
	if (!sim || entity == SE_SIM_ENTITY_ID_INVALID) {
		return NULL;
	}
	const u32 slot_index = se_sim_entity_id_slot(entity);
	const u32 generation = se_sim_entity_id_generation(entity);
	if ((sz)slot_index >= s_array_get_size((se_sim_entity_slots*)&sim->entity_slots)) {
		return NULL;
	}
	const se_sim_entity_slot* slot = s_array_get((se_sim_entity_slots*)&sim->entity_slots, s_array_handle((se_sim_entity_slots*)&sim->entity_slots, slot_index));
	if (!slot || !slot->alive || slot->generation != generation) {
		return NULL;
	}
	if (out_slot_index) {
		*out_slot_index = slot_index;
	}
	return slot;
}

static se_sim_component_blob* se_simulation_find_component_blob(se_sim_entity_slot* slot, const se_sim_component_type type) {
	if (!slot || type == 0u) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size(&slot->components); ++i) {
		se_sim_component_blob* blob = s_array_get(&slot->components, s_array_handle(&slot->components, (u32)i));
		if (blob && blob->type == type) {
			return blob;
		}
	}
	return NULL;
}

static const se_sim_component_blob* se_simulation_find_component_blob_const(const se_sim_entity_slot* slot, const se_sim_component_type type) {
	if (!slot || type == 0u) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size((se_sim_component_blobs*)&slot->components); ++i) {
		const se_sim_component_blob* blob = s_array_get((se_sim_component_blobs*)&slot->components, s_array_handle((se_sim_component_blobs*)&slot->components, (u32)i));
		if (blob && blob->type == type) {
			return blob;
		}
	}
	return NULL;
}

static i32 se_simulation_event_compare(const se_sim_event_record* a, const se_sim_event_record* b) {
	if (a->deliver_at_tick < b->deliver_at_tick) {
		return -1;
	}
	if (a->deliver_at_tick > b->deliver_at_tick) {
		return 1;
	}
	if (a->sequence < b->sequence) {
		return -1;
	}
	if (a->sequence > b->sequence) {
		return 1;
	}
	return 0;
}

static void se_simulation_pending_insert_sorted(se_simulation* sim, const se_sim_event_record* event_record) {
	s_array_add(&sim->pending_events, *event_record);
	if (s_array_get_size(&sim->pending_events) <= 1) {
		return;
	}
	se_sim_event_record* data = s_array_get_data(&sim->pending_events);
	sz index = s_array_get_size(&sim->pending_events) - 1;
	while (index > 0) {
		if (se_simulation_event_compare(&data[index - 1], &data[index]) <= 0) {
			break;
		}
		se_sim_event_record tmp = data[index - 1];
		data[index - 1] = data[index];
		data[index] = tmp;
		index--;
	}
}

static void se_simulation_sort_systems(se_simulation* sim) {
	if (!sim || s_array_get_size(&sim->systems) < 2) {
		return;
	}
	se_sim_system_entry* systems_data = s_array_get_data(&sim->systems);
	for (sz i = 1; i < s_array_get_size(&sim->systems); ++i) {
		sz j = i;
		while (j > 0) {
			const se_sim_system_entry* lhs = &systems_data[j - 1];
			const se_sim_system_entry* rhs = &systems_data[j];
			b8 keep_order = false;
			if (lhs->order < rhs->order) {
				keep_order = true;
			} else if (lhs->order == rhs->order && lhs->registration_index <= rhs->registration_index) {
				keep_order = true;
			}
			if (keep_order) {
				break;
			}
			se_sim_system_entry tmp = systems_data[j - 1];
			systems_data[j - 1] = systems_data[j];
			systems_data[j] = tmp;
			j--;
		}
	}
}

static void se_simulation_move_due_events_to_ready(se_simulation* sim) {
	if (!sim) {
		return;
	}
	while (s_array_get_size(&sim->pending_events) > 0) {
		s_handle handle = s_array_handle(&sim->pending_events, 0u);
		se_sim_event_record* first = s_array_get(&sim->pending_events, handle);
		if (!first) {
			s_array_remove_ordered(&sim->pending_events, handle);
			continue;
		}
		if (first->deliver_at_tick > sim->current_tick) {
			break;
		}
		se_sim_event_record moved = *first;
		s_array_add(&sim->ready_events, moved);
		s_array_remove_ordered(&sim->pending_events, handle);
	}
}

static b8 se_simulation_event_record_init(
	se_sim_event_record* out_record,
	const se_entity_id target,
	const se_sim_event_type type,
	const se_sim_tick deliver_at_tick,
	const u64 sequence,
	const void* payload,
	const u32 payload_size) {
	if (!out_record) {
		return false;
	}
	memset(out_record, 0, sizeof(*out_record));
	out_record->target = target;
	out_record->type = type;
	out_record->deliver_at_tick = deliver_at_tick;
	out_record->sequence = sequence;
	s_array_init(&out_record->payload);
	if (payload_size > 0u) {
		if (!payload) {
			return false;
		}
		s_array_reserve(&out_record->payload, payload_size);
		const u8* bytes = (const u8*)payload;
		for (u32 i = 0; i < payload_size; ++i) {
			u8 value = bytes[i];
			s_array_add(&out_record->payload, value);
		}
	}
	return true;
}

static u32 se_simulation_event_queue_payload_bytes(const se_sim_event_records* queue) {
	if (!queue) {
		return 0u;
	}
	u32 bytes = 0u;
	for (sz i = 0; i < s_array_get_size((se_sim_event_records*)queue); ++i) {
		const se_sim_event_record* event_record = s_array_get((se_sim_event_records*)queue, s_array_handle((se_sim_event_records*)queue, (u32)i));
		if (!event_record) {
			continue;
		}
		bytes += (u32)s_array_get_size((se_sim_bytes*)&event_record->payload);
	}
	return bytes;
}

static b8 se_simulation_snapshot_build_config_section(const se_simulation* sim, se_sim_bytes* out) {
	s_array_init(out);
	return se_sim_bytes_append_u32(out, sim->config.max_entities) &&
		se_sim_bytes_append_u32(out, sim->config.max_components_per_entity) &&
		se_sim_bytes_append_u32(out, sim->config.max_events) &&
		se_sim_bytes_append_u32(out, sim->config.max_event_payload_bytes) &&
		se_sim_bytes_append_u32(out, se_sim_f32_to_u32(sim->config.fixed_dt));
}

static b8 se_simulation_snapshot_build_tick_section(const se_simulation* sim, se_sim_bytes* out) {
	s_array_init(out);
	return se_sim_bytes_append_u64(out, sim->current_tick) &&
		se_sim_bytes_append_u64(out, sim->next_sequence);
}

static b8 se_simulation_snapshot_build_entity_table_section(const se_simulation* sim, se_sim_bytes* out) {
	s_array_init(out);
	if (!se_sim_bytes_append_u32(out, (u32)s_array_get_size((se_sim_entity_slots*)&sim->entity_slots))) {
		return false;
	}
	if (!se_sim_bytes_append_u32(out, sim->alive_entities)) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size((se_sim_entity_slots*)&sim->entity_slots); ++i) {
		const se_sim_entity_slot* slot = s_array_get((se_sim_entity_slots*)&sim->entity_slots, s_array_handle((se_sim_entity_slots*)&sim->entity_slots, (u32)i));
		if (!slot) {
			if (!se_sim_bytes_append_u32(out, 1u) ||
				!se_sim_bytes_append_u8(out, 0u) ||
				!se_sim_bytes_append_u8(out, 0u) ||
				!se_sim_bytes_append_u8(out, 0u) ||
				!se_sim_bytes_append_u8(out, 0u) ||
				!se_sim_bytes_append_u32(out, 0u)) {
				return false;
			}
			continue;
		}
		const u32 component_count = slot->alive ? (u32)s_array_get_size((se_sim_component_blobs*)&slot->components) : 0u;
		if (!se_sim_bytes_append_u32(out, slot->generation) ||
			!se_sim_bytes_append_u8(out, slot->alive ? 1u : 0u) ||
			!se_sim_bytes_append_u8(out, 0u) ||
			!se_sim_bytes_append_u8(out, 0u) ||
			!se_sim_bytes_append_u8(out, 0u) ||
			!se_sim_bytes_append_u32(out, component_count)) {
			return false;
		}
	}
	return true;
}

static b8 se_simulation_snapshot_build_component_registry_section(const se_simulation* sim, se_sim_bytes* out) {
	s_array_init(out);
	if (!se_sim_bytes_append_u32(out, (u32)s_array_get_size((se_sim_component_metas*)&sim->component_registry))) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size((se_sim_component_metas*)&sim->component_registry); ++i) {
		const se_sim_component_meta* meta = s_array_get((se_sim_component_metas*)&sim->component_registry, s_array_handle((se_sim_component_metas*)&sim->component_registry, (u32)i));
		if (!meta) {
			continue;
		}
		if (!se_sim_bytes_append_u32(out, meta->type) ||
			!se_sim_bytes_append_u32(out, meta->size) ||
			!se_sim_bytes_append_u32(out, meta->alignment) ||
			!se_sim_bytes_append(out, meta->name, sizeof(meta->name))) {
			return false;
		}
	}
	return true;
}

static b8 se_simulation_snapshot_build_component_payloads_section(const se_simulation* sim, se_sim_bytes* out) {
	s_array_init(out);
	u32 record_count = 0u;
	for (sz slot_index = 0; slot_index < s_array_get_size((se_sim_entity_slots*)&sim->entity_slots); ++slot_index) {
		const se_sim_entity_slot* slot = s_array_get((se_sim_entity_slots*)&sim->entity_slots, s_array_handle((se_sim_entity_slots*)&sim->entity_slots, (u32)slot_index));
		if (!slot || !slot->alive) {
			continue;
		}
		record_count += (u32)s_array_get_size((se_sim_component_blobs*)&slot->components);
	}
	if (!se_sim_bytes_append_u32(out, record_count)) {
		return false;
	}
	for (sz slot_index = 0; slot_index < s_array_get_size((se_sim_entity_slots*)&sim->entity_slots); ++slot_index) {
		const se_sim_entity_slot* slot = s_array_get((se_sim_entity_slots*)&sim->entity_slots, s_array_handle((se_sim_entity_slots*)&sim->entity_slots, (u32)slot_index));
		if (!slot || !slot->alive) {
			continue;
		}
		for (sz j = 0; j < s_array_get_size((se_sim_component_blobs*)&slot->components); ++j) {
			const se_sim_component_blob* blob = s_array_get((se_sim_component_blobs*)&slot->components, s_array_handle((se_sim_component_blobs*)&slot->components, (u32)j));
			if (!blob) {
				continue;
			}
			const u32 blob_size = (u32)s_array_get_size((se_sim_bytes*)&blob->data);
			const u8* blob_data = s_array_get_data((se_sim_bytes*)&blob->data);
			if (!se_sim_bytes_append_u32(out, (u32)slot_index) ||
				!se_sim_bytes_append_u32(out, blob->type) ||
				!se_sim_bytes_append_u32(out, blob_size) ||
				!se_sim_bytes_append(out, blob_data, blob_size)) {
				return false;
			}
		}
	}
	return true;
}

static b8 se_simulation_snapshot_build_event_registry_section(const se_simulation* sim, se_sim_bytes* out) {
	s_array_init(out);
	if (!se_sim_bytes_append_u32(out, (u32)s_array_get_size((se_sim_event_metas*)&sim->event_registry))) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size((se_sim_event_metas*)&sim->event_registry); ++i) {
		const se_sim_event_meta* meta = s_array_get((se_sim_event_metas*)&sim->event_registry, s_array_handle((se_sim_event_metas*)&sim->event_registry, (u32)i));
		if (!meta) {
			continue;
		}
		if (!se_sim_bytes_append_u32(out, meta->type) ||
			!se_sim_bytes_append_u32(out, meta->payload_size) ||
			!se_sim_bytes_append(out, meta->name, sizeof(meta->name))) {
			return false;
		}
	}
	return true;
}

static b8 se_simulation_snapshot_write_event_records(se_sim_bytes* out, const se_sim_event_records* queue) {
	for (sz i = 0; i < s_array_get_size((se_sim_event_records*)queue); ++i) {
		const se_sim_event_record* event_record = s_array_get((se_sim_event_records*)queue, s_array_handle((se_sim_event_records*)queue, (u32)i));
		if (!event_record) {
			continue;
		}
		const u32 payload_size = (u32)s_array_get_size((se_sim_bytes*)&event_record->payload);
		const u8* payload = s_array_get_data((se_sim_bytes*)&event_record->payload);
		if (!se_sim_bytes_append_u64(out, event_record->target) ||
			!se_sim_bytes_append_u32(out, event_record->type) ||
			!se_sim_bytes_append_u64(out, event_record->deliver_at_tick) ||
			!se_sim_bytes_append_u64(out, event_record->sequence) ||
			!se_sim_bytes_append_u32(out, payload_size) ||
			!se_sim_bytes_append(out, payload, payload_size)) {
			return false;
		}
	}
	return true;
}

static b8 se_simulation_snapshot_build_event_queues_section(const se_simulation* sim, se_sim_bytes* out) {
	s_array_init(out);
	if (!se_sim_bytes_append_u32(out, (u32)s_array_get_size((se_sim_event_records*)&sim->pending_events)) ||
		!se_sim_bytes_append_u32(out, (u32)s_array_get_size((se_sim_event_records*)&sim->ready_events))) {
		return false;
	}
	if (!se_simulation_snapshot_write_event_records(out, &sim->pending_events)) {
		return false;
	}
	if (!se_simulation_snapshot_write_event_records(out, &sim->ready_events)) {
		return false;
	}
	return true;
}

static u32 se_simulation_snapshot_checksum(const u8* data, const sz size) {
	u32 hash = 2166136261u;
	for (sz i = 0; i < size; ++i) {
		hash ^= (u32)data[i];
		hash *= 16777619u;
	}
	return hash;
}

static b8 se_simulation_snapshot_patch_u32(se_sim_bytes* out, const sz offset, const u32 value) {
	u8* data = s_array_get_data(out);
	if (!data || offset + 4 > s_array_get_size(out)) {
		return false;
	}
	data[offset + 0] = (u8)(value & 0xFFu);
	data[offset + 1] = (u8)((value >> 8) & 0xFFu);
	data[offset + 2] = (u8)((value >> 16) & 0xFFu);
	data[offset + 3] = (u8)((value >> 24) & 0xFFu);
	return true;
}

static void se_simulation_snapshot_sections_clear(se_sim_snapshot_sections* sections) {
	if (!sections) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(sections); ++i) {
		se_sim_snapshot_section_data* section = s_array_get(sections, s_array_handle(sections, (u32)i));
		if (!section) {
			continue;
		}
		s_array_clear(&section->bytes);
		memset(section, 0, sizeof(*section));
	}
	s_array_clear(sections);
	s_array_init(sections);
}

static se_simulation* se_simulation_create_internal(const se_simulation_config* config) {
	if (!se_simulation_config_validate(config)) {
		return NULL;
	}

	se_context* ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_simulation_handle sim_handle = s_array_increment(&ctx->simulations);
	se_simulation* sim = s_array_get(&ctx->simulations, sim_handle);
	if (!sim) {
		if (sim_handle != SE_SIMULATION_HANDLE_NULL) {
			s_array_remove(&ctx->simulations, sim_handle);
		}
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}

	memset(sim, 0, sizeof(*sim));
	sim->self_handle = sim_handle;
	sim->config = *config;
	sim->current_tick = 0u;
	sim->next_sequence = 1u;
	sim->next_system_registration_index = 1u;
	sim->alive_entities = 0u;
	sim->used_event_payload_bytes = 0u;

	s_array_init(&sim->entity_slots);
	s_array_init(&sim->free_slots);
	s_array_init(&sim->component_registry);
	s_array_init(&sim->event_registry);
	s_array_init(&sim->pending_events);
	s_array_init(&sim->ready_events);
	s_array_init(&sim->systems);

	s_array_reserve(&sim->entity_slots, config->max_entities);
	s_array_reserve(&sim->free_slots, config->max_entities);
	s_array_reserve(&sim->pending_events, config->max_events);
	s_array_reserve(&sim->ready_events, config->max_events);

	se_set_last_error(SE_RESULT_OK);
	return sim;
}

static void se_simulation_destroy_internal(se_simulation* sim) {
	if (!sim) {
		return;
	}

	se_context* ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	const se_simulation_handle handle = sim->self_handle;

	se_simulation_entities_clear(sim);
	se_simulation_events_clear(sim);
	se_simulation_registries_clear(sim);
	s_array_clear(&sim->systems);
	memset(sim, 0, sizeof(*sim));
	if (handle != SE_SIMULATION_HANDLE_NULL) {
		s_array_remove(&ctx->simulations, handle);
	}
	se_set_last_error(SE_RESULT_OK);
}

static void se_simulation_reset_internal(se_simulation* sim) {
	if (!sim) {
		return;
	}
	se_simulation_entities_clear(sim);
	se_simulation_events_clear(sim);
	sim->current_tick = 0u;
	sim->next_sequence = 1u;
	se_set_last_error(SE_RESULT_OK);
}

static se_entity_id se_simulation_entity_create_internal(se_simulation* sim) {
	if (!sim) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return SE_SIM_ENTITY_ID_INVALID;
	}

	u32 slot_index = 0u;
	se_sim_entity_slot* slot = NULL;

	if (s_array_get_size(&sim->free_slots) > 0) {
		s_handle free_handle = s_array_handle(&sim->free_slots, (u32)(s_array_get_size(&sim->free_slots) - 1));
		u32* free_slot_ptr = s_array_get(&sim->free_slots, free_handle);
		if (!free_slot_ptr) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return SE_SIM_ENTITY_ID_INVALID;
		}
		slot_index = *free_slot_ptr;
		s_array_remove(&sim->free_slots, free_handle);
		slot = s_array_get(&sim->entity_slots, s_array_handle(&sim->entity_slots, slot_index));
		if (!slot) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return SE_SIM_ENTITY_ID_INVALID;
		}
	} else {
		if (s_array_get_size(&sim->entity_slots) >= sim->config.max_entities) {
			se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
			return SE_SIM_ENTITY_ID_INVALID;
		}
		s_handle slot_handle = s_array_increment(&sim->entity_slots);
		slot = s_array_get(&sim->entity_slots, slot_handle);
		if (!slot) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return SE_SIM_ENTITY_ID_INVALID;
		}
		slot_index = (u32)(s_array_get_size(&sim->entity_slots) - 1u);
		memset(slot, 0, sizeof(*slot));
		s_array_init(&slot->components);
		s_array_reserve(&slot->components, sim->config.max_components_per_entity);
		slot->generation = 1u;
	}

	se_sim_entity_slot_clear_components(slot);
	slot->alive = true;
	if (slot->generation == 0u) {
		slot->generation = 1u;
	}
	sim->alive_entities++;
	se_set_last_error(SE_RESULT_OK);
	return se_sim_entity_id_make(slot_index, slot->generation);
}

static b8 se_simulation_entity_destroy_internal(se_simulation* sim, const se_entity_id entity) {
	if (!sim || entity == SE_SIM_ENTITY_ID_INVALID) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	u32 slot_index = 0u;
	se_sim_entity_slot* slot = se_simulation_entity_slot_from_id(sim, entity, &slot_index);
	if (!slot) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	se_sim_entity_slot_clear_components(slot);
	slot->alive = false;
	slot->generation++;
	if (slot->generation == 0u) {
		slot->generation = 1u;
	}
	s_array_add(&sim->free_slots, slot_index);
	if (sim->alive_entities > 0u) {
		sim->alive_entities--;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static b8 se_simulation_entity_alive_internal(const se_simulation* sim, const se_entity_id entity) {
	if (!sim || entity == SE_SIM_ENTITY_ID_INVALID) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const se_sim_entity_slot* slot = se_simulation_entity_slot_from_id_const(sim, entity, NULL);
	if (!slot) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static u32 se_simulation_entity_count_internal(const se_simulation* sim) {
	if (!sim) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0u;
	}
	se_set_last_error(SE_RESULT_OK);
	return sim->alive_entities;
}

static void se_simulation_for_each_entity_internal(se_simulation* sim, se_sim_entity_iterator_fn fn, void* user_data) {
	if (!sim || !fn) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&sim->entity_slots); ++i) {
		se_sim_entity_slot* slot = s_array_get(&sim->entity_slots, s_array_handle(&sim->entity_slots, (u32)i));
		if (!slot || !slot->alive) {
			continue;
		}
		fn(sim->self_handle, se_sim_entity_id_make((u32)i, slot->generation), user_data);
	}
}

static b8 se_simulation_component_register_internal(se_simulation* sim, const se_sim_component_desc* desc) {
	if (!sim || !se_simulation_component_desc_valid(desc)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (se_simulation_find_component_meta(sim, desc->type) != NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_sim_component_meta meta = {0};
	meta.type = desc->type;
	meta.size = desc->size;
	meta.alignment = desc->alignment == 0u ? 1u : desc->alignment;
	strncpy(meta.name, desc->name, sizeof(meta.name) - 1);
	s_array_add(&sim->component_registry, meta);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static b8 se_simulation_component_set_internal(se_simulation* sim, const se_entity_id entity, const se_sim_component_type type, const void* data, const sz size) {
	if (!sim || !data || size == 0u || size > UINT32_MAX) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_sim_entity_slot* slot = se_simulation_entity_slot_from_id(sim, entity, NULL);
	if (!slot) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	se_sim_component_meta* meta = se_simulation_find_component_meta(sim, type);
	if (!meta || meta->size != (u32)size) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_sim_component_blob* existing = se_simulation_find_component_blob(slot, type);
	if (existing) {
		if (!se_sim_bytes_set(&existing->data, data, (u32)size)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	if (s_array_get_size(&slot->components) >= sim->config.max_components_per_entity) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}
	se_sim_component_blob blob = {0};
	blob.type = type;
	s_array_init(&blob.data);
	if (!se_sim_bytes_set(&blob.data, data, (u32)size)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	s_array_add(&slot->components, blob);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static b8 se_simulation_component_get_internal(const se_simulation* sim, const se_entity_id entity, const se_sim_component_type type, void* out_data, const sz out_size) {
	if (!sim || !out_data || out_size == 0u || out_size > UINT32_MAX) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const se_sim_entity_slot* slot = se_simulation_entity_slot_from_id_const(sim, entity, NULL);
	if (!slot) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const se_sim_component_meta* meta = se_simulation_find_component_meta_const(sim, type);
	if (!meta || out_size < meta->size) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const se_sim_component_blob* blob = se_simulation_find_component_blob_const(slot, type);
	if (!blob) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const u8* src = s_array_get_data((se_sim_bytes*)&blob->data);
	if (!src || s_array_get_size((se_sim_bytes*)&blob->data) != meta->size) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	memcpy(out_data, src, meta->size);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static void* se_simulation_component_get_ptr_internal(se_simulation* sim, const se_entity_id entity, const se_sim_component_type type) {
	if (!sim) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_sim_entity_slot* slot = se_simulation_entity_slot_from_id(sim, entity, NULL);
	if (!slot) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	if (!se_simulation_find_component_meta(sim, type)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	se_sim_component_blob* blob = se_simulation_find_component_blob(slot, type);
	if (!blob) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	if (s_array_get_size(&blob->data) == 0u) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	se_set_last_error(SE_RESULT_OK);
	return s_array_get_data(&blob->data);
}

static const void* se_simulation_component_get_const_ptr_internal(const se_simulation* sim, const se_entity_id entity, const se_sim_component_type type) {
	if (!sim) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	const se_sim_entity_slot* slot = se_simulation_entity_slot_from_id_const(sim, entity, NULL);
	if (!slot) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	if (!se_simulation_find_component_meta_const(sim, type)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	const se_sim_component_blob* blob = se_simulation_find_component_blob_const(slot, type);
	if (!blob || s_array_get_size((se_sim_bytes*)&blob->data) == 0u) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	se_set_last_error(SE_RESULT_OK);
	return s_array_get_data((se_sim_bytes*)&blob->data);
}

static b8 se_simulation_component_remove_internal(se_simulation* sim, const se_entity_id entity, const se_sim_component_type type) {
	if (!sim || type == 0u) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_sim_entity_slot* slot = se_simulation_entity_slot_from_id(sim, entity, NULL);
	if (!slot) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&slot->components); ++i) {
		s_handle handle = s_array_handle(&slot->components, (u32)i);
		se_sim_component_blob* blob = s_array_get(&slot->components, handle);
		if (!blob || blob->type != type) {
			continue;
		}
		se_sim_component_blob_clear(blob);
		s_array_remove_ordered(&slot->components, handle);
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

static b8 se_simulation_event_register_internal(se_simulation* sim, const se_sim_event_desc* desc) {
	if (!sim || !se_simulation_event_desc_valid(desc)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (se_simulation_find_event_meta(sim, desc->type) != NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_sim_event_meta meta = {0};
	meta.type = desc->type;
	meta.payload_size = desc->payload_size;
	strncpy(meta.name, desc->name, sizeof(meta.name) - 1);
	s_array_add(&sim->event_registry, meta);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static b8 se_simulation_event_emit_internal(
	se_simulation* sim,
	const se_entity_id target,
	const se_sim_event_type type,
	const void* payload,
	const sz size,
	const se_sim_tick deliver_at_tick) {
	if (!sim || size > UINT32_MAX) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_simulation_entity_slot_from_id_const(sim, target, NULL)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const se_sim_event_meta* meta = se_simulation_find_event_meta_const(sim, type);
	if (!meta) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if ((u32)size != meta->payload_size || (size > 0u && !payload)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const sz total_events = s_array_get_size(&sim->pending_events) + s_array_get_size(&sim->ready_events);
	if (total_events >= sim->config.max_events) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}
	if ((u64)sim->used_event_payload_bytes + (u64)size > (u64)sim->config.max_event_payload_bytes) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}

	se_sim_event_record event_record = {0};
	if (!se_simulation_event_record_init(&event_record, target, type, deliver_at_tick, sim->next_sequence++, payload, (u32)size)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	sim->used_event_payload_bytes += (u32)size;
	if (deliver_at_tick <= sim->current_tick) {
		s_array_add(&sim->ready_events, event_record);
	} else {
		se_simulation_pending_insert_sorted(sim, &event_record);
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static b8 se_simulation_event_poll_internal(
	se_simulation* sim,
	const se_entity_id target,
	const se_sim_event_type type,
	void* out_payload,
	const sz out_size,
	se_sim_tick* out_tick) {
	if (!sim || type == 0u) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const se_sim_event_meta* meta = se_simulation_find_event_meta_const(sim, type);
	if (!meta) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if ((meta->payload_size > 0u) && (!out_payload || out_size < meta->payload_size)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	for (sz i = 0; i < s_array_get_size(&sim->ready_events); ++i) {
		s_handle handle = s_array_handle(&sim->ready_events, (u32)i);
		se_sim_event_record* event_record = s_array_get(&sim->ready_events, handle);
		if (!event_record) {
			continue;
		}
		if (event_record->target != target || event_record->type != type) {
			continue;
		}
		const u32 payload_size = (u32)s_array_get_size(&event_record->payload);
		if (payload_size > 0u) {
			const u8* payload_data = s_array_get_data(&event_record->payload);
			if (!payload_data || out_size < payload_size) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				return false;
			}
			memcpy(out_payload, payload_data, payload_size);
		}
		if (out_tick) {
			*out_tick = event_record->deliver_at_tick;
		}
		if (sim->used_event_payload_bytes >= payload_size) {
			sim->used_event_payload_bytes -= payload_size;
		} else {
			sim->used_event_payload_bytes = 0u;
		}
		se_sim_event_record_clear(event_record);
		s_array_remove_ordered(&sim->ready_events, handle);
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

static b8 se_simulation_register_system_internal(se_simulation* sim, se_sim_system_fn fn, void* user_data, const i32 order) {
	if (!sim || !fn) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_sim_system_entry entry = {0};
	entry.fn = fn;
	entry.user_data = user_data;
	entry.order = order;
	entry.registration_index = sim->next_system_registration_index++;
	s_array_add(&sim->systems, entry);
	se_simulation_sort_systems(sim);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static b8 se_simulation_step_internal(se_simulation* sim, const u32 tick_count) {
	if (!sim) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (tick_count == 0u) {
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	for (u32 i = 0; i < tick_count; ++i) {
		sim->current_tick++;
		se_simulation_move_due_events_to_ready(sim);
		for (sz j = 0; j < s_array_get_size(&sim->systems); ++j) {
			se_sim_system_entry* system_entry = s_array_get(&sim->systems, s_array_handle(&sim->systems, (u32)j));
			if (!system_entry || !system_entry->fn) {
				continue;
			}
			system_entry->fn(sim->self_handle, sim->current_tick, system_entry->user_data);
		}
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static se_sim_tick se_simulation_get_tick_internal(const se_simulation* sim) {
	if (!sim) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0u;
	}
	se_set_last_error(SE_RESULT_OK);
	return sim->current_tick;
}

static f32 se_simulation_get_fixed_dt_internal(const se_simulation* sim) {
	if (!sim) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0.0f;
	}
	se_set_last_error(SE_RESULT_OK);
	return sim->config.fixed_dt;
}

static void se_simulation_get_diagnostics_internal(const se_simulation* sim, se_simulation_diagnostics* out_diag) {
	if (!sim || !out_diag) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	memset(out_diag, 0, sizeof(*out_diag));
	out_diag->entity_capacity = sim->config.max_entities;
	out_diag->entity_count = sim->alive_entities;
	out_diag->component_registry_count = (u32)s_array_get_size((se_sim_component_metas*)&sim->component_registry);
	out_diag->event_registry_count = (u32)s_array_get_size((se_sim_event_metas*)&sim->event_registry);
	out_diag->pending_event_count = (u32)s_array_get_size((se_sim_event_records*)&sim->pending_events);
	out_diag->ready_event_count = (u32)s_array_get_size((se_sim_event_records*)&sim->ready_events);
	out_diag->max_events = sim->config.max_events;
	out_diag->used_event_payload_bytes = sim->used_event_payload_bytes;
	out_diag->max_event_payload_bytes = sim->config.max_event_payload_bytes;
	out_diag->current_tick = sim->current_tick;
	out_diag->fixed_dt = sim->config.fixed_dt;
	se_set_last_error(SE_RESULT_OK);
}

static b8 se_simulation_snapshot_save_memory_internal(const se_simulation* sim, u8** out_data, sz* out_size) {
	if (!sim || !out_data || !out_size) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_sim_snapshot_sections sections = {0};
	s_array_init(&sections);

	se_sim_snapshot_section_data section = {0};

	section.type = SE_SIM_SNAPSHOT_SECTION_CONFIG;
	if (!se_simulation_snapshot_build_config_section(sim, &section.bytes)) {
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	s_array_add(&sections, section);

	section.type = SE_SIM_SNAPSHOT_SECTION_TICK;
	if (!se_simulation_snapshot_build_tick_section(sim, &section.bytes)) {
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	s_array_add(&sections, section);

	section.type = SE_SIM_SNAPSHOT_SECTION_ENTITY_TABLE;
	if (!se_simulation_snapshot_build_entity_table_section(sim, &section.bytes)) {
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	s_array_add(&sections, section);

	section.type = SE_SIM_SNAPSHOT_SECTION_COMPONENT_REGISTRY;
	if (!se_simulation_snapshot_build_component_registry_section(sim, &section.bytes)) {
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	s_array_add(&sections, section);

	section.type = SE_SIM_SNAPSHOT_SECTION_COMPONENT_PAYLOADS;
	if (!se_simulation_snapshot_build_component_payloads_section(sim, &section.bytes)) {
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	s_array_add(&sections, section);

	section.type = SE_SIM_SNAPSHOT_SECTION_EVENT_REGISTRY;
	if (!se_simulation_snapshot_build_event_registry_section(sim, &section.bytes)) {
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	s_array_add(&sections, section);

	section.type = SE_SIM_SNAPSHOT_SECTION_EVENT_QUEUES;
	if (!se_simulation_snapshot_build_event_queues_section(sim, &section.bytes)) {
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	s_array_add(&sections, section);

	const u32 section_count = (u32)s_array_get_size(&sections);
	const u64 section_table_offset = SE_SIM_SNAPSHOT_HEADER_SIZE;
	const u64 payload_offset = section_table_offset + (u64)section_count * SE_SIM_SNAPSHOT_SECTION_ENTRY_SIZE;
	u64 payload_size = 0u;
	for (sz i = 0; i < s_array_get_size(&sections); ++i) {
		se_sim_snapshot_section_data* curr = s_array_get(&sections, s_array_handle(&sections, (u32)i));
		if (!curr) {
			continue;
		}
		payload_size += (u64)s_array_get_size(&curr->bytes);
	}
	const u64 total_size_u64 = payload_offset + payload_size;
	if (total_size_u64 > SIZE_MAX) {
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}

	se_sim_bytes file_bytes = {0};
	s_array_init(&file_bytes);
	s_array_reserve(&file_bytes, (sz)total_size_u64);

	const u8 magic[6] = {'S', 'E', 'S', 'I', 'M', '\0'};
	if (!se_sim_bytes_append(&file_bytes, magic, sizeof(magic)) ||
		!se_sim_bytes_append_u16(&file_bytes, SE_SIM_SNAPSHOT_VERSION) ||
		!se_sim_bytes_append_u8(&file_bytes, SE_SIM_SNAPSHOT_ENDIANNESS_LITTLE) ||
		!se_sim_bytes_append_u8(&file_bytes, 0u) ||
		!se_sim_bytes_append_u8(&file_bytes, 0u) ||
		!se_sim_bytes_append_u8(&file_bytes, 0u) ||
		!se_sim_bytes_append_u32(&file_bytes, section_count) ||
		!se_sim_bytes_append_u64(&file_bytes, section_table_offset) ||
		!se_sim_bytes_append_u64(&file_bytes, payload_offset) ||
		!se_sim_bytes_append_u64(&file_bytes, total_size_u64) ||
		!se_sim_bytes_append_u32(&file_bytes, 0u) ||
		!se_sim_bytes_append_u32(&file_bytes, 0u)) {
		s_array_clear(&file_bytes);
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}

	u64 cursor = payload_offset;
	for (sz i = 0; i < s_array_get_size(&sections); ++i) {
		se_sim_snapshot_section_data* curr = s_array_get(&sections, s_array_handle(&sections, (u32)i));
		if (!curr) {
			continue;
		}
		const u64 section_size = (u64)s_array_get_size(&curr->bytes);
		if (!se_sim_bytes_append_u32(&file_bytes, curr->type) ||
			!se_sim_bytes_append_u32(&file_bytes, 0u) ||
			!se_sim_bytes_append_u64(&file_bytes, cursor) ||
			!se_sim_bytes_append_u64(&file_bytes, section_size)) {
			s_array_clear(&file_bytes);
			se_simulation_snapshot_sections_clear(&sections);
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		cursor += section_size;
	}

	for (sz i = 0; i < s_array_get_size(&sections); ++i) {
		se_sim_snapshot_section_data* curr = s_array_get(&sections, s_array_handle(&sections, (u32)i));
		if (!curr) {
			continue;
		}
		const u8* bytes = s_array_get_data(&curr->bytes);
		if (!se_sim_bytes_append(&file_bytes, bytes, s_array_get_size(&curr->bytes))) {
			s_array_clear(&file_bytes);
			se_simulation_snapshot_sections_clear(&sections);
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
	}

	u8* file_data = s_array_get_data(&file_bytes);
	if (!file_data || s_array_get_size(&file_bytes) != (sz)total_size_u64) {
		s_array_clear(&file_bytes);
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	const u32 checksum = se_simulation_snapshot_checksum(file_data + payload_offset, (sz)(total_size_u64 - payload_offset));
	if (!se_simulation_snapshot_patch_u32(&file_bytes, 40u, checksum)) {
		s_array_clear(&file_bytes);
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}

	*out_data = malloc((sz)total_size_u64);
	if (!*out_data) {
		s_array_clear(&file_bytes);
		se_simulation_snapshot_sections_clear(&sections);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return false;
	}
	memcpy(*out_data, s_array_get_data(&file_bytes), (sz)total_size_u64);
	*out_size = (sz)total_size_u64;

	s_array_clear(&file_bytes);
	se_simulation_snapshot_sections_clear(&sections);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_simulation_snapshot_free(void* data) {
	free(data);
}

static b8 se_simulation_snapshot_save_file_internal(const se_simulation* sim, const c8* path) {
	if (!sim || !path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	u8* memory = NULL;
	sz size = 0u;
	if (!se_simulation_snapshot_save_memory_internal(sim, &memory, &size)) {
		return false;
	}
	FILE* file = fopen(path, "wb");
	if (!file) {
		se_simulation_snapshot_free(memory);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	const sz written = fwrite(memory, 1, size, file);
	fclose(file);
	se_simulation_snapshot_free(memory);
	if (written != size) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static b8 se_simulation_snapshot_parse_config_section(se_simulation* sim, const se_sim_snapshot_section_view* section) {
	se_sim_reader reader = {section->data, section->size, 0u};
	u32 max_entities = 0u;
	u32 max_components_per_entity = 0u;
	u32 max_events = 0u;
	u32 max_event_payload_bytes = 0u;
	u32 fixed_dt_bits = 0u;
	if (!se_sim_reader_read_u32(&reader, &max_entities) ||
		!se_sim_reader_read_u32(&reader, &max_components_per_entity) ||
		!se_sim_reader_read_u32(&reader, &max_events) ||
		!se_sim_reader_read_u32(&reader, &max_event_payload_bytes) ||
		!se_sim_reader_read_u32(&reader, &fixed_dt_bits)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	if (reader.cursor != reader.size) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	if (max_entities > sim->config.max_entities ||
		max_components_per_entity > sim->config.max_components_per_entity ||
		max_events > sim->config.max_events ||
		max_event_payload_bytes > sim->config.max_event_payload_bytes) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}
	const f32 snapshot_fixed_dt = se_sim_u32_to_f32(fixed_dt_bits);
	if (snapshot_fixed_dt <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	sim->config.fixed_dt = snapshot_fixed_dt;
	return true;
}

static b8 se_simulation_snapshot_parse_component_registry_section(se_simulation* sim, const se_sim_snapshot_section_view* section) {
	se_sim_reader reader = {section->data, section->size, 0u};
	u32 count = 0u;
	if (!se_sim_reader_read_u32(&reader, &count)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	for (u32 i = 0; i < count; ++i) {
		se_sim_component_desc desc = {0};
		if (!se_sim_reader_read_u32(&reader, &desc.type) ||
			!se_sim_reader_read_u32(&reader, &desc.size) ||
			!se_sim_reader_read_u32(&reader, &desc.alignment) ||
			!se_sim_reader_read(&reader, desc.name, sizeof(desc.name))) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		desc.name[SE_MAX_NAME_LENGTH - 1] = '\0';
		if (!se_simulation_component_register_internal(sim, &desc)) {
			return false;
		}
	}
	if (reader.cursor != reader.size) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	return true;
}

static b8 se_simulation_snapshot_parse_event_registry_section(se_simulation* sim, const se_sim_snapshot_section_view* section) {
	se_sim_reader reader = {section->data, section->size, 0u};
	u32 count = 0u;
	if (!se_sim_reader_read_u32(&reader, &count)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	for (u32 i = 0; i < count; ++i) {
		se_sim_event_desc desc = {0};
		if (!se_sim_reader_read_u32(&reader, &desc.type) ||
			!se_sim_reader_read_u32(&reader, &desc.payload_size) ||
			!se_sim_reader_read(&reader, desc.name, sizeof(desc.name))) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		desc.name[SE_MAX_NAME_LENGTH - 1] = '\0';
		if (!se_simulation_event_register_internal(sim, &desc)) {
			return false;
		}
	}
	if (reader.cursor != reader.size) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	return true;
}

static b8 se_simulation_snapshot_parse_entity_table_section(
	se_simulation* sim,
	const se_sim_snapshot_section_view* section,
	se_sim_u32s* out_expected_component_counts) {
	se_sim_reader reader = {section->data, section->size, 0u};
	u32 slot_count = 0u;
	u32 alive_count = 0u;
	if (!se_sim_reader_read_u32(&reader, &slot_count) ||
		!se_sim_reader_read_u32(&reader, &alive_count)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	if (slot_count > sim->config.max_entities) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}
	s_array_reserve(&sim->entity_slots, slot_count);
	s_array_reserve(&sim->free_slots, slot_count);
	s_array_reserve(out_expected_component_counts, slot_count);
	u32 computed_alive = 0u;
	for (u32 i = 0; i < slot_count; ++i) {
		u32 generation = 0u;
		u8 alive = 0u;
		u32 expected_component_count = 0u;
		if (!se_sim_reader_read_u32(&reader, &generation) ||
			!se_sim_reader_read_u8(&reader, &alive) ||
			!se_sim_reader_skip(&reader, 3u) ||
			!se_sim_reader_read_u32(&reader, &expected_component_count)) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		s_handle slot_handle = s_array_increment(&sim->entity_slots);
		se_sim_entity_slot* slot = s_array_get(&sim->entity_slots, slot_handle);
		if (!slot) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		memset(slot, 0, sizeof(*slot));
		s_array_init(&slot->components);
		s_array_reserve(&slot->components, sim->config.max_components_per_entity);
		slot->generation = generation == 0u ? 1u : generation;
		slot->alive = (alive != 0u);
		s_array_add(out_expected_component_counts, expected_component_count);
		if (slot->alive) {
			computed_alive++;
		} else {
			u32 free_slot_index = i;
			s_array_add(&sim->free_slots, free_slot_index);
		}
	}
	if (reader.cursor != reader.size || computed_alive != alive_count) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	sim->alive_entities = computed_alive;
	return true;
}

static b8 se_simulation_snapshot_parse_component_payloads_section(
	se_simulation* sim,
	const se_sim_snapshot_section_view* section,
	const se_sim_u32s* expected_component_counts) {
	se_sim_reader reader = {section->data, section->size, 0u};
	u32 record_count = 0u;
	if (!se_sim_reader_read_u32(&reader, &record_count)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	for (u32 i = 0; i < record_count; ++i) {
		u32 slot_index = 0u;
		u32 type = 0u;
		u32 data_size = 0u;
		if (!se_sim_reader_read_u32(&reader, &slot_index) ||
			!se_sim_reader_read_u32(&reader, &type) ||
			!se_sim_reader_read_u32(&reader, &data_size)) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		const u8* data_ptr = NULL;
		if (!se_sim_reader_read_bytes(&reader, &data_ptr, data_size)) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		if ((sz)slot_index >= s_array_get_size(&sim->entity_slots)) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		se_sim_entity_slot* slot = s_array_get(&sim->entity_slots, s_array_handle(&sim->entity_slots, slot_index));
		if (!slot || !slot->alive) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		const se_sim_component_meta* meta = se_simulation_find_component_meta_const(sim, type);
		if (!meta || meta->size != data_size) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		if (se_simulation_find_component_blob(slot, type) != NULL) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		if (s_array_get_size(&slot->components) >= sim->config.max_components_per_entity) {
			se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
			return false;
		}
		se_sim_component_blob blob = {0};
		blob.type = type;
		se_sim_bytes_copy_from_internal(&blob.data, data_ptr, data_size);
		s_array_add(&slot->components, blob);
	}
	if (reader.cursor != reader.size) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&sim->entity_slots); ++i) {
		const se_sim_entity_slot* slot = s_array_get(&sim->entity_slots, s_array_handle(&sim->entity_slots, (u32)i));
		if (!slot || !slot->alive) {
			continue;
		}
		u32* expected_count_ptr = s_array_get((se_sim_u32s*)expected_component_counts, s_array_handle((se_sim_u32s*)expected_component_counts, (u32)i));
		const u32 expected_count = expected_count_ptr ? *expected_count_ptr : 0u;
		if ((u32)s_array_get_size((se_sim_component_blobs*)&slot->components) != expected_count) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
	}
	return true;
}

static b8 se_simulation_snapshot_parse_event_record(
	se_simulation* sim,
	se_sim_reader* reader,
	se_sim_event_record* out_event,
	u64* in_out_max_sequence) {
	u64 target = 0u;
	u32 type = 0u;
	u64 deliver_at_tick = 0u;
	u64 sequence = 0u;
	u32 payload_size = 0u;
	if (!se_sim_reader_read_u64(reader, &target) ||
		!se_sim_reader_read_u32(reader, &type) ||
		!se_sim_reader_read_u64(reader, &deliver_at_tick) ||
		!se_sim_reader_read_u64(reader, &sequence) ||
		!se_sim_reader_read_u32(reader, &payload_size)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	const se_sim_event_meta* meta = se_simulation_find_event_meta_const(sim, type);
	if (!meta || meta->payload_size != payload_size) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	if ((u64)sim->used_event_payload_bytes + (u64)payload_size > sim->config.max_event_payload_bytes) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}
	const u8* payload_ptr = NULL;
	if (!se_sim_reader_read_bytes(reader, &payload_ptr, payload_size)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	if (!se_simulation_event_record_init(out_event, target, type, deliver_at_tick, sequence, payload_ptr, payload_size)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	sim->used_event_payload_bytes += payload_size;
	if (in_out_max_sequence && sequence > *in_out_max_sequence) {
		*in_out_max_sequence = sequence;
	}
	return true;
}

static b8 se_simulation_snapshot_parse_event_queues_section(se_simulation* sim, const se_sim_snapshot_section_view* section) {
	se_sim_reader reader = {section->data, section->size, 0u};
	u32 pending_count = 0u;
	u32 ready_count = 0u;
	if (!se_sim_reader_read_u32(&reader, &pending_count) ||
		!se_sim_reader_read_u32(&reader, &ready_count)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	if ((u64)pending_count + (u64)ready_count > sim->config.max_events) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}
	s_array_reserve(&sim->pending_events, pending_count);
	s_array_reserve(&sim->ready_events, ready_count);

	u64 max_sequence = 0u;
	for (u32 i = 0; i < pending_count; ++i) {
		se_sim_event_record event_record = {0};
		if (!se_simulation_snapshot_parse_event_record(sim, &reader, &event_record, &max_sequence)) {
			return false;
		}
		se_simulation_pending_insert_sorted(sim, &event_record);
	}
	for (u32 i = 0; i < ready_count; ++i) {
		se_sim_event_record event_record = {0};
		if (!se_simulation_snapshot_parse_event_record(sim, &reader, &event_record, &max_sequence)) {
			return false;
		}
		s_array_add(&sim->ready_events, event_record);
	}
	if (reader.cursor != reader.size) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	if (sim->next_sequence <= max_sequence) {
		sim->next_sequence = max_sequence + 1u;
	}
	return true;
}

static b8 se_simulation_snapshot_parse_tick_section(se_simulation* sim, const se_sim_snapshot_section_view* section) {
	se_sim_reader reader = {section->data, section->size, 0u};
	u64 tick = 0u;
	u64 next_sequence = 0u;
	if (!se_sim_reader_read_u64(&reader, &tick) ||
		!se_sim_reader_read_u64(&reader, &next_sequence) ||
		reader.cursor != reader.size) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	sim->current_tick = tick;
	sim->next_sequence = next_sequence == 0u ? 1u : next_sequence;
	return true;
}

static void se_simulation_snapshot_load_staging_init(se_simulation* staging, const se_simulation* source) {
	memset(staging, 0, sizeof(*staging));
	staging->self_handle = source->self_handle;
	staging->config = source->config;
	staging->current_tick = 0u;
	staging->next_sequence = 1u;
	staging->alive_entities = 0u;
	staging->used_event_payload_bytes = 0u;
	s_array_init(&staging->entity_slots);
	s_array_init(&staging->free_slots);
	s_array_init(&staging->component_registry);
	s_array_init(&staging->event_registry);
	s_array_init(&staging->pending_events);
	s_array_init(&staging->ready_events);
	s_array_reserve(&staging->entity_slots, source->config.max_entities);
	s_array_reserve(&staging->free_slots, source->config.max_entities);
	s_array_reserve(&staging->pending_events, source->config.max_events);
	s_array_reserve(&staging->ready_events, source->config.max_events);
}

static void se_simulation_snapshot_load_staging_clear(se_simulation* staging) {
	if (!staging) {
		return;
	}
	se_simulation_entities_clear(staging);
	se_simulation_events_clear(staging);
	se_simulation_registries_clear(staging);
}

static void se_simulation_snapshot_load_commit(se_simulation* sim, se_simulation* staging) {
	se_simulation_clear_runtime_state_for_load(sim);
	sim->config.fixed_dt = staging->config.fixed_dt;
	sim->current_tick = staging->current_tick;
	sim->next_sequence = staging->next_sequence;
	sim->alive_entities = staging->alive_entities;
	sim->used_event_payload_bytes = staging->used_event_payload_bytes;
	sim->entity_slots = staging->entity_slots;
	sim->free_slots = staging->free_slots;
	sim->component_registry = staging->component_registry;
	sim->event_registry = staging->event_registry;
	sim->pending_events = staging->pending_events;
	sim->ready_events = staging->ready_events;
	s_array_init(&staging->entity_slots);
	s_array_init(&staging->free_slots);
	s_array_init(&staging->component_registry);
	s_array_init(&staging->event_registry);
	s_array_init(&staging->pending_events);
	s_array_init(&staging->ready_events);
}

static b8 se_simulation_snapshot_load_memory_internal(se_simulation* sim, const u8* data, const sz size) {
	if (!sim || !data || size < SE_SIM_SNAPSHOT_HEADER_SIZE) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_sim_reader reader = {data, size, 0u};
	u8 magic[6] = {0};
	u16 version = 0u;
	u8 endianness = 0u;
	u32 section_count = 0u;
	u64 section_table_offset = 0u;
	u64 payload_offset = 0u;
	u64 total_size = 0u;
	u32 checksum = 0u;

	if (!se_sim_reader_read(&reader, magic, sizeof(magic)) ||
		!se_sim_reader_read_u16(&reader, &version) ||
		!se_sim_reader_read_u8(&reader, &endianness) ||
		!se_sim_reader_skip(&reader, 3u) ||
		!se_sim_reader_read_u32(&reader, &section_count) ||
		!se_sim_reader_read_u64(&reader, &section_table_offset) ||
		!se_sim_reader_read_u64(&reader, &payload_offset) ||
		!se_sim_reader_read_u64(&reader, &total_size) ||
		!se_sim_reader_read_u32(&reader, &checksum) ||
		!se_sim_reader_skip(&reader, 4u)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}

	const u8 expected_magic[6] = {'S', 'E', 'S', 'I', 'M', '\0'};
	if (memcmp(magic, expected_magic, sizeof(expected_magic)) != 0) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}
	if (version != SE_SIM_SNAPSHOT_VERSION || endianness != SE_SIM_SNAPSHOT_ENDIANNESS_LITTLE) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}
	if (total_size != (u64)size || section_table_offset != SE_SIM_SNAPSHOT_HEADER_SIZE) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	if (section_count == 0u) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	const u64 expected_payload_offset = section_table_offset + (u64)section_count * SE_SIM_SNAPSHOT_SECTION_ENTRY_SIZE;
	if (payload_offset != expected_payload_offset || payload_offset > (u64)size) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	const u32 computed_checksum = se_simulation_snapshot_checksum(data + payload_offset, size - (sz)payload_offset);
	if (computed_checksum != checksum) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}

	se_sim_snapshot_section_view section_config = {0};
	se_sim_snapshot_section_view section_tick = {0};
	se_sim_snapshot_section_view section_entity_table = {0};
	se_sim_snapshot_section_view section_component_registry = {0};
	se_sim_snapshot_section_view section_component_payloads = {0};
	se_sim_snapshot_section_view section_event_registry = {0};
	se_sim_snapshot_section_view section_event_queues = {0};

	for (u32 i = 0; i < section_count; ++i) {
		const sz entry_offset = (sz)section_table_offset + (sz)i * SE_SIM_SNAPSHOT_SECTION_ENTRY_SIZE;
		se_sim_reader entry_reader = {data + entry_offset, SE_SIM_SNAPSHOT_SECTION_ENTRY_SIZE, 0u};
		u32 section_type = 0u;
		u32 section_reserved = 0u;
		u64 section_offset = 0u;
		u64 section_size = 0u;
		if (!se_sim_reader_read_u32(&entry_reader, &section_type) ||
			!se_sim_reader_read_u32(&entry_reader, &section_reserved) ||
			!se_sim_reader_read_u64(&entry_reader, &section_offset) ||
			!se_sim_reader_read_u64(&entry_reader, &section_size)) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		(void)section_reserved;
		if (section_offset < payload_offset ||
			section_offset > (u64)size ||
			section_size > (u64)size - section_offset) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		se_sim_snapshot_section_view* target = NULL;
		switch (section_type) {
			case SE_SIM_SNAPSHOT_SECTION_CONFIG:
				target = &section_config;
				break;
			case SE_SIM_SNAPSHOT_SECTION_TICK:
				target = &section_tick;
				break;
			case SE_SIM_SNAPSHOT_SECTION_ENTITY_TABLE:
				target = &section_entity_table;
				break;
			case SE_SIM_SNAPSHOT_SECTION_COMPONENT_REGISTRY:
				target = &section_component_registry;
				break;
			case SE_SIM_SNAPSHOT_SECTION_COMPONENT_PAYLOADS:
				target = &section_component_payloads;
				break;
			case SE_SIM_SNAPSHOT_SECTION_EVENT_REGISTRY:
				target = &section_event_registry;
				break;
			case SE_SIM_SNAPSHOT_SECTION_EVENT_QUEUES:
				target = &section_event_queues;
				break;
			default:
				break;
		}
		if (!target || target->found) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		target->data = data + section_offset;
		target->size = (sz)section_size;
		target->found = true;
	}

	if (!section_config.found ||
		!section_tick.found ||
		!section_entity_table.found ||
		!section_component_registry.found ||
		!section_component_payloads.found ||
		!section_event_registry.found ||
		!section_event_queues.found) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}

	se_simulation staged = {0};
	se_simulation_snapshot_load_staging_init(&staged, sim);

	if (!se_simulation_snapshot_parse_config_section(&staged, &section_config)) {
		se_simulation_snapshot_load_staging_clear(&staged);
		return false;
	}
	if (!se_simulation_snapshot_parse_component_registry_section(&staged, &section_component_registry)) {
		se_simulation_snapshot_load_staging_clear(&staged);
		return false;
	}
	if (!se_simulation_snapshot_parse_event_registry_section(&staged, &section_event_registry)) {
		se_simulation_snapshot_load_staging_clear(&staged);
		return false;
	}

	se_sim_u32s expected_component_counts = {0};
	s_array_init(&expected_component_counts);
	if (!se_simulation_snapshot_parse_entity_table_section(&staged, &section_entity_table, &expected_component_counts)) {
		s_array_clear(&expected_component_counts);
		se_simulation_snapshot_load_staging_clear(&staged);
		return false;
	}
	if (!se_simulation_snapshot_parse_component_payloads_section(&staged, &section_component_payloads, &expected_component_counts)) {
		s_array_clear(&expected_component_counts);
		se_simulation_snapshot_load_staging_clear(&staged);
		return false;
	}
	s_array_clear(&expected_component_counts);

	if (!se_simulation_snapshot_parse_tick_section(&staged, &section_tick)) {
		se_simulation_snapshot_load_staging_clear(&staged);
		return false;
	}
	if (!se_simulation_snapshot_parse_event_queues_section(&staged, &section_event_queues)) {
		se_simulation_snapshot_load_staging_clear(&staged);
		return false;
	}

	const u32 queue_payload_bytes =
		se_simulation_event_queue_payload_bytes(&staged.pending_events) +
		se_simulation_event_queue_payload_bytes(&staged.ready_events);
	staged.used_event_payload_bytes = queue_payload_bytes;
	if (staged.used_event_payload_bytes > staged.config.max_event_payload_bytes) {
		se_simulation_snapshot_load_staging_clear(&staged);
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}

	se_simulation_snapshot_load_commit(sim, &staged);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static b8 se_simulation_snapshot_load_file_internal(se_simulation* sim, const c8* path) {
	if (!sim || !path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	FILE* file = fopen(path, "rb");
	if (!file) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	const long file_size = ftell(file);
	if (file_size <= 0) {
		fclose(file);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	u8* bytes = malloc((sz)file_size);
	if (!bytes) {
		fclose(file);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return false;
	}
	const sz read_size = fread(bytes, 1, (sz)file_size, file);
	fclose(file);
	if (read_size != (sz)file_size) {
		free(bytes);
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	const b8 ok = se_simulation_snapshot_load_memory_internal(sim, bytes, (sz)file_size);
	free(bytes);
	return ok;
}

se_simulation_handle se_simulation_create(const se_simulation_config* config) {
	se_simulation* sim = se_simulation_create_internal(config);
	if (!sim) {
		return SE_SIMULATION_HANDLE_NULL;
	}
	return sim->self_handle;
}

void se_simulation_destroy(const se_simulation_handle sim) {
	if (sim == SE_SIMULATION_HANDLE_NULL) {
		return;
	}
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return;
	}
	se_simulation_destroy_internal(sim_ptr);
}

void se_simulation_reset(const se_simulation_handle sim) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return;
	}
	se_simulation_reset_internal(sim_ptr);
}

se_entity_id se_simulation_entity_create(const se_simulation_handle sim) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return SE_SIM_ENTITY_ID_INVALID;
	}
	return se_simulation_entity_create_internal(sim_ptr);
}

b8 se_simulation_entity_destroy(const se_simulation_handle sim, const se_entity_id entity) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_entity_destroy_internal(sim_ptr, entity);
}

b8 se_simulation_entity_alive(const se_simulation_handle sim, const se_entity_id entity) {
	const se_simulation* sim_ptr = se_simulation_from_handle_const(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_entity_alive_internal(sim_ptr, entity);
}

u32 se_simulation_entity_count(const se_simulation_handle sim) {
	const se_simulation* sim_ptr = se_simulation_from_handle_const(sim);
	if (!sim_ptr) {
		return 0u;
	}
	return se_simulation_entity_count_internal(sim_ptr);
}

void se_simulation_for_each_entity(const se_simulation_handle sim, se_sim_entity_iterator_fn fn, void* user_data) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return;
	}
	se_simulation_for_each_entity_internal(sim_ptr, fn, user_data);
}

b8 se_simulation_component_register(const se_simulation_handle sim, const se_sim_component_desc* desc) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_component_register_internal(sim_ptr, desc);
}

b8 se_simulation_component_set(
	const se_simulation_handle sim,
	const se_entity_id entity,
	const se_sim_component_type type,
	const void* data,
	const sz size) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_component_set_internal(sim_ptr, entity, type, data, size);
}

b8 se_simulation_component_get(
	const se_simulation_handle sim,
	const se_entity_id entity,
	const se_sim_component_type type,
	void* out_data,
	const sz out_size) {
	const se_simulation* sim_ptr = se_simulation_from_handle_const(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_component_get_internal(sim_ptr, entity, type, out_data, out_size);
}

void* se_simulation_component_get_ptr(
	const se_simulation_handle sim,
	const se_entity_id entity,
	const se_sim_component_type type) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return NULL;
	}
	return se_simulation_component_get_ptr_internal(sim_ptr, entity, type);
}

const void* se_simulation_component_get_const_ptr(
	const se_simulation_handle sim,
	const se_entity_id entity,
	const se_sim_component_type type) {
	const se_simulation* sim_ptr = se_simulation_from_handle_const(sim);
	if (!sim_ptr) {
		return NULL;
	}
	return se_simulation_component_get_const_ptr_internal(sim_ptr, entity, type);
}

b8 se_simulation_component_remove(
	const se_simulation_handle sim,
	const se_entity_id entity,
	const se_sim_component_type type) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_component_remove_internal(sim_ptr, entity, type);
}

b8 se_simulation_event_register(const se_simulation_handle sim, const se_sim_event_desc* desc) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_event_register_internal(sim_ptr, desc);
}

b8 se_simulation_event_emit(
	const se_simulation_handle sim,
	const se_entity_id target,
	const se_sim_event_type type,
	const void* payload,
	const sz size,
	const se_sim_tick deliver_at_tick) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_event_emit_internal(sim_ptr, target, type, payload, size, deliver_at_tick);
}

b8 se_simulation_event_poll(
	const se_simulation_handle sim,
	const se_entity_id target,
	const se_sim_event_type type,
	void* out_payload,
	const sz out_size,
	se_sim_tick* out_tick) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_event_poll_internal(sim_ptr, target, type, out_payload, out_size, out_tick);
}

b8 se_simulation_register_system(
	const se_simulation_handle sim,
	se_sim_system_fn fn,
	void* user_data,
	const i32 order) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_register_system_internal(sim_ptr, fn, user_data, order);
}

b8 se_simulation_step(const se_simulation_handle sim, const u32 tick_count) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_step_internal(sim_ptr, tick_count);
}

se_sim_tick se_simulation_get_tick(const se_simulation_handle sim) {
	const se_simulation* sim_ptr = se_simulation_from_handle_const(sim);
	if (!sim_ptr) {
		return 0u;
	}
	return se_simulation_get_tick_internal(sim_ptr);
}

f32 se_simulation_get_fixed_dt(const se_simulation_handle sim) {
	const se_simulation* sim_ptr = se_simulation_from_handle_const(sim);
	if (!sim_ptr) {
		return 0.0f;
	}
	return se_simulation_get_fixed_dt_internal(sim_ptr);
}

void se_simulation_get_diagnostics(const se_simulation_handle sim, se_simulation_diagnostics* out_diag) {
	const se_simulation* sim_ptr = se_simulation_from_handle_const(sim);
	if (!sim_ptr) {
		return;
	}
	se_simulation_get_diagnostics_internal(sim_ptr, out_diag);
}

b8 se_simulation_snapshot_save_file(const se_simulation_handle sim, const c8* path) {
	const se_simulation* sim_ptr = se_simulation_from_handle_const(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_snapshot_save_file_internal(sim_ptr, path);
}

b8 se_simulation_snapshot_load_file(const se_simulation_handle sim, const c8* path) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_snapshot_load_file_internal(sim_ptr, path);
}

b8 se_simulation_snapshot_save_memory(const se_simulation_handle sim, u8** out_data, sz* out_size) {
	const se_simulation* sim_ptr = se_simulation_from_handle_const(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_snapshot_save_memory_internal(sim_ptr, out_data, out_size);
}

b8 se_simulation_snapshot_load_memory(const se_simulation_handle sim, const u8* data, const sz size) {
	se_simulation* sim_ptr = se_simulation_from_handle_mut(sim);
	if (!sim_ptr) {
		return false;
	}
	return se_simulation_snapshot_load_memory_internal(sim_ptr, data, size);
}
