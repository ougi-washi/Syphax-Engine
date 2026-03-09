// Syphax-Engine - Ougi Washi

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "se_camera.h"
#include "se_debug.h"
#include "se_graphics.h"
#include "se_scene.h"
#include "se_sdf.h"
#include "se_window.h"

typedef s_array(se_sdf_handle, sdf_unique_handles);
typedef s_array(f32, sdf_unique_frame_samples);

typedef struct {
	u32 entity_count;
	f32 run_seconds;
	b8 perf_mode;
	b8 perf_no_assert;
} sdf_unique_config;

typedef struct {
	u32 entity_count;
	u32 cluster_count;
	u32 cluster_columns;
	u32 cluster_rows;
	f32 entity_spacing;
	f32 cluster_spacing;
	f32 field_width;
	f32 field_depth;
	f32 field_radius;
	s_vec3 center;
} sdf_unique_layout;

typedef struct {
	f32 avg_fps;
	f32 avg_frame_ms;
	f32 p95_frame_ms;
	f32 max_frame_ms;
	f32 avg_visible_refs;
	f32 avg_resident_bricks;
	u32 max_visible_refs;
	u32 frame_count;
} sdf_unique_perf_summary;

typedef struct {
	se_window_handle window;
	se_camera_handle camera;
	se_sdf_renderer_handle renderer;
	b8 render_failed;
} sdf_unique_present_state;

enum {
	SDF_UNIQUE_DEFAULT_COUNT = 1000u,
	SDF_UNIQUE_CLUSTER_ENTITY_COLUMNS = 10u,
	SDF_UNIQUE_CLUSTER_ENTITY_ROWS = 10u,
	SDF_UNIQUE_CLUSTER_ENTITY_CAPACITY = SDF_UNIQUE_CLUSTER_ENTITY_COLUMNS * SDF_UNIQUE_CLUSTER_ENTITY_ROWS
};

static i32 sdf_unique_compare_f32(const void* lhs, const void* rhs) {
	const f32 a = lhs ? *(const f32*)lhs : 0.0f;
	const f32 b = rhs ? *(const f32*)rhs : 0.0f;
	if (a < b) {
		return -1;
	}
	if (a > b) {
		return 1;
	}
	return 0;
}

static u32 sdf_unique_hash(u32 value) {
	value ^= value >> 16u;
	value *= 0x7feb352du;
	value ^= value >> 15u;
	value *= 0x846ca68bu;
	value ^= value >> 16u;
	return value;
}

static f32 sdf_unique_rand01(const u32 index, const u32 salt) {
	const u32 hash = sdf_unique_hash(index * 0x45d9f3bu + salt * 0x9e3779b9u);
	return (f32)(hash & 0x00ffffffu) / 16777215.0f;
}

static f64 sdf_unique_now_seconds(void) {
	struct timespec ts = {0};
	timespec_get(&ts, TIME_UTC);
	return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1000000000.0;
}

static s_vec3 sdf_unique_palette(const u32 index, const f32 phase) {
	const f32 t = (f32)index * 0.137f + phase;
	return s_vec3(
		0.50f + 0.30f * sinf(t),
		0.48f + 0.28f * sinf(t + 2.1f),
		0.52f + 0.26f * sinf(t + 4.2f)
	);
}

static sdf_unique_config sdf_unique_parse_config(const i32 argc, char** argv) {
	sdf_unique_config config = {
		.entity_count = SDF_UNIQUE_DEFAULT_COUNT,
		.run_seconds = 8.0f,
		.perf_mode = false,
		.perf_no_assert = false
	};
	for (i32 i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--perf") == 0) {
			config.perf_mode = true;
			continue;
		}
		if (strcmp(argv[i], "--perf-no-assert") == 0) {
			config.perf_mode = true;
			config.perf_no_assert = true;
			continue;
		}
		if (strncmp(argv[i], "--seconds=", 10) == 0) {
			const f32 parsed = (f32)atof(argv[i] + 10);
			if (parsed > 0.5f) {
				config.run_seconds = parsed;
			}
			continue;
		}
		if (strncmp(argv[i], "--entities=", 11) == 0) {
			const i32 parsed = atoi(argv[i] + 11);
			if (parsed > 0) {
				config.entity_count = (u32)parsed;
			}
			continue;
		}
	}
	return config;
}

static sdf_unique_layout sdf_unique_make_layout(const u32 entity_count) {
	sdf_unique_layout layout = {0};
	layout.entity_count = entity_count;
	layout.cluster_count = (entity_count + SDF_UNIQUE_CLUSTER_ENTITY_CAPACITY - 1u) / SDF_UNIQUE_CLUSTER_ENTITY_CAPACITY;
	layout.cluster_columns = (u32)ceilf(sqrtf((f32)layout.cluster_count));
	if (layout.cluster_columns == 0u) {
		layout.cluster_columns = 1u;
	}
	layout.cluster_rows = (layout.cluster_count + layout.cluster_columns - 1u) / layout.cluster_columns;
	layout.entity_spacing = 3.5f;
	layout.cluster_spacing = layout.entity_spacing * (f32)SDF_UNIQUE_CLUSTER_ENTITY_COLUMNS + 6.0f;
	layout.field_width = ((f32)layout.cluster_columns - 1.0f) * layout.cluster_spacing;
	layout.field_depth = ((f32)layout.cluster_rows - 1.0f) * layout.cluster_spacing;
	layout.field_radius = 0.5f * sqrtf(layout.field_width * layout.field_width + layout.field_depth * layout.field_depth);
	layout.center = s_vec3(0.0f, 3.4f, 0.0f);
	return layout;
}

static s_vec3 sdf_unique_cluster_entity_position(const sdf_unique_layout* layout, const u32 local_index) {
	if (!layout) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	const u32 column = local_index % SDF_UNIQUE_CLUSTER_ENTITY_COLUMNS;
	const u32 row = local_index / SDF_UNIQUE_CLUSTER_ENTITY_COLUMNS;
	const f32 center_x = ((f32)SDF_UNIQUE_CLUSTER_ENTITY_COLUMNS - 1.0f) * 0.5f;
	const f32 center_z = ((f32)SDF_UNIQUE_CLUSTER_ENTITY_ROWS - 1.0f) * 0.5f;
	return s_vec3(
		((f32)column - center_x) * layout->entity_spacing,
		0.0f,
		((f32)row - center_z) * layout->entity_spacing
	);
}

static void sdf_unique_debug_dump_samples(const se_sdf_handle sdf, const sdf_unique_layout* layout) {
	const c8* enabled = getenv("SE_SDF_DEBUG_SAMPLES");
	if (!enabled || !layout || sdf == SE_SDF_NULL) {
		return;
	}
	se_sdf_bounds bounds = {0};
	if (se_sdf_calculate_bounds(sdf, &bounds) && bounds.valid) {
		fprintf(
			stderr,
			"sdf_unique_entities :: bounds min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f)\n",
			bounds.min.x, bounds.min.y, bounds.min.z,
			bounds.max.x, bounds.max.y, bounds.max.z
		);
	}
	const s_vec3 sample_points[] = {
		sdf_unique_cluster_entity_position(layout, 0u),
		sdf_unique_cluster_entity_position(layout, s_min(1u, layout->entity_count > 1u ? layout->entity_count - 1u : 0u)),
		sdf_unique_cluster_entity_position(layout, s_min(11u, layout->entity_count > 11u ? layout->entity_count - 1u : 0u)),
		s_vec3(0.0f, 1.5f, 0.0f)
	};
	for (u32 i = 0u; i < sizeof(sample_points) / sizeof(sample_points[0]); ++i) {
		s_vec3 point = sample_points[i];
		if (i < 3u) {
			point.y += 0.85f;
		}
		f32 distance = 0.0f;
		if (se_sdf_sample_distance(sdf, &point, &distance, NULL)) {
			fprintf(
				stderr,
				"sdf_unique_entities :: sample[%u] point=(%.2f, %.2f, %.2f) distance=%.4f\n",
				i,
				point.x, point.y, point.z,
				distance
			);
		}
	}
}

static s_vec3 sdf_unique_cluster_world_position(const sdf_unique_layout* layout, const u32 cluster_index) {
	if (!layout || layout->cluster_columns == 0u) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	const u32 column = cluster_index % layout->cluster_columns;
	const u32 row = cluster_index / layout->cluster_columns;
	const f32 center_x = ((f32)layout->cluster_columns - 1.0f) * 0.5f;
	const f32 center_z = ((f32)layout->cluster_rows - 1.0f) * 0.5f;
	return s_vec3(
		((f32)column - center_x) * layout->cluster_spacing,
		0.0f,
		((f32)row - center_z) * layout->cluster_spacing
	);
}

static b8 sdf_unique_add_primitive(
	const se_sdf_handle sdf,
	const se_sdf_node_handle root,
	const se_sdf_node_primitive_desc* desc,
	const s_vec3* color,
	const se_sdf_noise* noise
) {
	if (sdf == SE_SDF_NULL || root == SE_SDF_NODE_NULL || !desc) {
		return false;
	}
	const se_sdf_node_handle node = se_sdf_node_create_primitive(sdf, desc);
	if (node == SE_SDF_NODE_NULL) {
		return false;
	}
	if (color) {
		(void)se_sdf_node_set_color(sdf, node, color);
	}
	if (noise) {
		(void)se_sdf_node_set_noise(sdf, node, noise);
	}
	return se_sdf_node_add_child(sdf, root, node);
}

static b8 sdf_unique_add_entity(
	const se_sdf_handle sdf,
	const se_sdf_node_handle cluster_root,
	const u32 global_index,
	const s_vec3* local_position
) {
	if (sdf == SE_SDF_NULL || cluster_root == SE_SDF_NODE_NULL || !local_position) {
		return false;
	}
	const f32 seed_a = sdf_unique_rand01(global_index, 11u);
	const f32 seed_b = sdf_unique_rand01(global_index, 23u);
	const f32 seed_c = sdf_unique_rand01(global_index, 37u);
	const f32 seed_d = sdf_unique_rand01(global_index, 53u);
	const s_vec3 primary = sdf_unique_palette(global_index, 0.0f);
	const s_vec3 secondary = sdf_unique_palette(global_index, 1.7f);
	const s_vec3 accent = sdf_unique_palette(global_index, 3.4f);

	se_sdf_node_group_desc entity_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	entity_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	entity_desc.operation_amount = 0.10f + 0.12f * seed_a;
	entity_desc.transform = se_sdf_transform(
		*local_position,
		s_vec3(0.0f, seed_d * 1.2f, 0.0f),
		s_vec3(1.0f, 1.0f + seed_b * 0.08f, 1.0f)
	);
	const se_sdf_node_handle entity_root = se_sdf_node_create_group(sdf, &entity_desc);
	if (entity_root == SE_SDF_NODE_NULL || !se_sdf_node_add_child(sdf, cluster_root, entity_root)) {
		return false;
	}

	se_sdf_node_primitive_desc pedestal = {0};
	pedestal.operation = SE_SDF_OP_UNION;
	pedestal.primitive.type = SE_SDF_PRIMITIVE_ROUND_BOX;
	pedestal.primitive.round_box.size = s_vec3(0.72f + seed_a * 0.34f, 0.18f + seed_b * 0.10f, 0.72f + seed_c * 0.34f);
	pedestal.primitive.round_box.roundness = 0.12f + seed_d * 0.08f;
	pedestal.transform = se_sdf_transform(
		s_vec3(0.0f, pedestal.primitive.round_box.size.y, 0.0f),
		s_vec3(0.0f, seed_b * 0.4f, 0.0f),
		s_vec3(1.0f, 1.0f, 1.0f)
	);
	if (!sdf_unique_add_primitive(sdf, entity_root, &pedestal, &primary, NULL)) {
		return false;
	}

	se_sdf_node_primitive_desc spine = {0};
	spine.operation = SE_SDF_OP_SMOOTH_UNION;
	spine.operation_amount = 0.12f + seed_c * 0.10f;
	spine.primitive.type = SE_SDF_PRIMITIVE_VERTICAL_CAPSULE;
	spine.primitive.vertical_capsule.height = 0.60f + seed_b * 0.60f;
	spine.primitive.vertical_capsule.radius = 0.20f + seed_a * 0.14f;
	spine.transform = se_sdf_transform(
		s_vec3(0.0f, 0.74f + spine.primitive.vertical_capsule.height * 0.62f, 0.0f),
		s_vec3(0.0f, seed_b * 0.45f, 0.0f),
		s_vec3(1.0f, 1.0f, 1.0f)
	);
	se_sdf_noise spine_noise = SE_SDF_NOISE_DEFAULTS;
	spine_noise.active = ((global_index % 4u) == 0u);
	spine_noise.scale = 0.02f + seed_d * 0.03f;
	spine_noise.frequency = 1.4f + seed_a * 1.8f;
	if (!sdf_unique_add_primitive(sdf, entity_root, &spine, &secondary, &spine_noise)) {
		return false;
	}

	se_sdf_node_primitive_desc ring = {0};
	ring.operation = SE_SDF_OP_SMOOTH_UNION;
	ring.operation_amount = 0.08f + seed_a * 0.06f;
	ring.primitive.type = SE_SDF_PRIMITIVE_TORUS;
	ring.primitive.torus.radii = s_vec2(0.46f + seed_b * 0.22f, 0.08f + seed_c * 0.06f);
	ring.transform = se_sdf_transform(
		s_vec3(0.0f, 0.96f + seed_a * 0.55f, 0.0f),
		s_vec3(1.5707963f, seed_d * 0.8f, 0.0f),
		s_vec3(1.0f, 1.0f, 1.0f)
	);
	if (!sdf_unique_add_primitive(sdf, entity_root, &ring, &accent, NULL)) {
		return false;
	}

	se_sdf_node_primitive_desc crown = {0};
	crown.operation = SE_SDF_OP_SMOOTH_UNION;
	crown.operation_amount = 0.12f + seed_d * 0.10f;
	switch (global_index % 4u) {
		case 0u:
			crown.primitive.type = SE_SDF_PRIMITIVE_SPHERE;
			crown.primitive.sphere.radius = 0.30f + seed_a * 0.26f;
			break;
		case 1u:
			crown.primitive.type = SE_SDF_PRIMITIVE_ROUNDED_CYLINDER;
			crown.primitive.rounded_cylinder.outer_radius = 0.24f + seed_a * 0.16f;
			crown.primitive.rounded_cylinder.corner_radius = 0.08f + seed_b * 0.05f;
			crown.primitive.rounded_cylinder.half_height = 0.20f + seed_c * 0.16f;
			break;
		case 2u:
			crown.primitive.type = SE_SDF_PRIMITIVE_OCTAHEDRON;
			crown.primitive.octahedron.size = 0.36f + seed_b * 0.24f;
			break;
		default:
			crown.primitive.type = SE_SDF_PRIMITIVE_BOX_FRAME;
			crown.primitive.box_frame.size = s_vec3(0.30f + seed_a * 0.20f, 0.30f + seed_c * 0.16f, 0.30f + seed_d * 0.20f);
			crown.primitive.box_frame.thickness = 0.06f + seed_b * 0.05f;
			break;
	}
	crown.transform = se_sdf_transform(
		s_vec3(0.0f, 1.48f + seed_a * 0.90f, 0.0f),
		s_vec3(seed_b * 0.4f, seed_c * 1.0f, seed_d * 0.3f),
		s_vec3(1.0f, 1.0f, 1.0f)
	);
	if (!sdf_unique_add_primitive(sdf, entity_root, &crown, &secondary, NULL)) {
		return false;
	}

	return true;
}

static b8 sdf_unique_populate_cluster(
	const se_sdf_handle cluster_sdf,
	const sdf_unique_layout* layout,
	const u32 first_entity_index,
	const u32 entity_count
) {
	if (cluster_sdf == SE_SDF_NULL || !layout || entity_count == 0u) {
		return false;
	}
	se_sdf_clear(cluster_sdf);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	const se_sdf_node_handle root = se_sdf_node_create_group(cluster_sdf, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(cluster_sdf, root)) {
		return false;
	}

	for (u32 local_index = 0u; local_index < entity_count; ++local_index) {
		const s_vec3 local_position = sdf_unique_cluster_entity_position(layout, local_index);
		if (!sdf_unique_add_entity(cluster_sdf, root, first_entity_index + local_index, &local_position)) {
			return false;
		}
	}
	return true;
}

static b8 sdf_unique_build_root(
	const se_sdf_handle root_sdf,
	const sdf_unique_handles* clusters,
	const sdf_unique_layout* layout,
	const se_sdf_prepare_desc* prepare_desc
) {
	if (root_sdf == SE_SDF_NULL || !clusters || !layout) {
		return false;
	}
	se_sdf_clear(root_sdf);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	const se_sdf_node_handle root = se_sdf_node_create_group(root_sdf, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(root_sdf, root)) {
		return false;
	}

	for (u32 i = 0u; i < layout->cluster_count; ++i) {
		se_sdf_handle* cluster = s_array_get((sdf_unique_handles*)clusters, s_array_handle((sdf_unique_handles*)clusters, i));
		if (!cluster || *cluster == SE_SDF_NULL) {
			return false;
		}
		se_sdf_node_ref_desc ref_desc = SE_SDF_NODE_REF_DESC_DEFAULTS;
		ref_desc.source = *cluster;
		ref_desc.transform = se_sdf_transform(
			sdf_unique_cluster_world_position(layout, i),
			s_vec3(0.0f, sdf_unique_rand01(i, 109u) * 0.2f, 0.0f),
			s_vec3(1.0f, 1.0f, 1.0f)
		);
		const se_sdf_node_handle ref = se_sdf_node_create_ref(root_sdf, &ref_desc);
		if (ref == SE_SDF_NODE_NULL || !se_sdf_node_add_child(root_sdf, root, ref)) {
			return false;
		}
	}
	return se_sdf_prepare(root_sdf, prepare_desc);
}

static void sdf_unique_perf_summary_build(
	const sdf_unique_frame_samples* samples,
	const f64 visible_sum,
	const f64 resident_sum,
	const u32 max_visible_refs,
	sdf_unique_perf_summary* out_summary
) {
	if (!samples || !out_summary) {
		return;
	}
	memset(out_summary, 0, sizeof(*out_summary));
	const u32 frame_count = (u32)s_array_get_size((sdf_unique_frame_samples*)samples);
	out_summary->frame_count = frame_count;
	out_summary->max_visible_refs = max_visible_refs;
	if (frame_count == 0u) {
		return;
	}

	f64 frame_sum = 0.0;
	f32 max_frame_ms = 0.0f;
	f32* sorted = malloc(sizeof(*sorted) * frame_count);
	if (!sorted) {
		return;
	}
	for (u32 i = 0u; i < frame_count; ++i) {
		f32* sample = s_array_get((sdf_unique_frame_samples*)samples, s_array_handle((sdf_unique_frame_samples*)samples, i));
		const f32 frame_ms = sample ? *sample : 0.0f;
		frame_sum += frame_ms;
		if (frame_ms > max_frame_ms) {
			max_frame_ms = frame_ms;
		}
		sorted[i] = frame_ms;
	}
	qsort(sorted, frame_count, sizeof(*sorted), sdf_unique_compare_f32);
	out_summary->avg_frame_ms = (f32)(frame_sum / (f64)frame_count);
	out_summary->avg_fps = out_summary->avg_frame_ms > 0.0f ? 1000.0f / out_summary->avg_frame_ms : 0.0f;
	out_summary->max_frame_ms = max_frame_ms;
	out_summary->p95_frame_ms = sorted[(frame_count - 1u) * 95u / 100u];
	out_summary->avg_visible_refs = (f32)(visible_sum / (f64)frame_count);
	out_summary->avg_resident_bricks = (f32)(resident_sum / (f64)frame_count);
	free(sorted);
}

static void sdf_unique_render_scene(se_scene_3d_handle scene, void* user_data) {
	(void)scene;
	sdf_unique_present_state* present = user_data;
	if (!present || present->window == S_HANDLE_NULL || present->camera == S_HANDLE_NULL || present->renderer == SE_SDF_RENDERER_NULL) {
		return;
	}
	u32 width = 0u;
	u32 height = 0u;
	se_window_get_framebuffer_size(present->window, &width, &height);
	se_sdf_frame_desc frame = SE_SDF_FRAME_DESC_DEFAULTS;
	frame.camera = present->camera;
	frame.resolution = s_vec2((f32)s_max(width, 1u), (f32)s_max(height, 1u));
	frame.time_seconds = (f32)se_window_get_time(present->window);
	if (!se_sdf_renderer_render(present->renderer, &frame)) {
		const se_sdf_build_diagnostics diagnostics = se_sdf_renderer_get_build_diagnostics(present->renderer);
		fprintf(stderr, "sdf_unique_entities :: render failed at stage '%s': %s\n", diagnostics.stage, diagnostics.message);
		present->render_failed = true;
		se_window_set_should_close(present->window, true);
	}
}

int main(const i32 argc, char** argv) {
	const sdf_unique_config config = sdf_unique_parse_config(argc, argv);
	const sdf_unique_layout layout = sdf_unique_make_layout(config.entity_count);
	const f32 perf_warmup_seconds = config.perf_mode ? fminf(1.0f, config.run_seconds * 0.2f) : 0.0f;
	const u32 window_width = config.perf_mode ? 640u : 1280u;
	const u32 window_height = config.perf_mode ? 360u : 720u;
	const s_vec3 base_camera_position = config.perf_mode ?
		s_vec3(layout.field_width * 0.16f, layout.cluster_spacing * 4.9f + 88.0f, layout.cluster_spacing * 6.4f + 188.0f) :
		s_vec3(layout.field_width * 0.55f, layout.cluster_spacing * 2.6f + 48.0f, layout.field_depth * 1.3f + layout.cluster_spacing * 2.8f + 52.0f);
	se_sdf_prepare_desc preview_prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	se_sdf_prepare_desc final_prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	se_context* context = se_context_create();
	se_window_handle window = S_HANDLE_NULL;
	se_scene_3d_handle scene = S_HANDLE_NULL;
	se_camera_handle camera = S_HANDLE_NULL;
	se_sdf_renderer_handle renderer = SE_SDF_RENDERER_NULL;
	se_sdf_handle root_sdf = SE_SDF_NULL;
	sdf_unique_present_state present_state = {0};
	sdf_unique_handles clusters;
	sdf_unique_frame_samples frame_samples;
	u8* cluster_final_ready = NULL;
	u8* cluster_final_active = NULL;
	b8 setup_ok = true;
	b8 perf_failed = false;
	f64 visible_sum = 0.0;
	f64 resident_sum = 0.0;
	u32 max_visible_refs = 0u;
	u32 final_clusters_ready = 0u;
	u32 active_background_bakes = 0u;
	f64 load_start_time = 0.0;
	f64 preview_ready_time = -1.0;
	f64 final_ready_time = -1.0;
	f32 last_stats_print = -1000.0f;
	f32 run_start_time = -1.0f;
	sdf_unique_perf_summary perf_summary = {0};
	const u32 max_background_bakes = 2u;

	s_array_init(&clusters);
	s_array_init(&frame_samples);

	if (!context) {
		return 1;
	}
	window = se_window_create("Syphax-Engine - sdf_unique_entities", (i32)window_width, (i32)window_height);
	if (window == S_HANDLE_NULL) {
		fprintf(stderr, "sdf_unique_entities :: failed to create window\n");
		setup_ok = false;
		goto cleanup;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_vsync(window, false);
	se_window_set_target_fps(window, 240);
	se_render_set_background(s_vec4(0.09f, 0.11f, 0.15f, 1.0f));
	load_start_time = sdf_unique_now_seconds();

	scene = se_scene_3d_create_for_window(window, 1);
	camera = scene != S_HANDLE_NULL ? se_scene_3d_get_camera(scene) : S_HANDLE_NULL;
	if (scene == S_HANDLE_NULL || camera == S_HANDLE_NULL) {
		setup_ok = false;
		goto cleanup;
	}
	se_camera_set_target_mode(camera, true);
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, config.perf_mode ? 44.0f : 40.0f, 0.1f, layout.field_radius * 10.0f + layout.cluster_spacing * 8.0f + 320.0f);
	se_camera_set_location(camera, &base_camera_position);
	se_camera_set_target(camera, &layout.center);

	root_sdf = se_sdf_create(NULL);
	if (root_sdf == SE_SDF_NULL) {
		setup_ok = false;
		goto cleanup;
	}

	preview_prepare_desc.lod_count = 1u;
	preview_prepare_desc.lod_resolutions[0] = 4u;
	preview_prepare_desc.lod_resolutions[1] = 4u;
	preview_prepare_desc.lod_resolutions[2] = 4u;
	preview_prepare_desc.lod_resolutions[3] = 4u;
	preview_prepare_desc.brick_size = 4u;
	preview_prepare_desc.brick_border = 1u;

	final_prepare_desc.lod_count = config.perf_mode ? 2u : 3u;
	final_prepare_desc.lod_resolutions[0] = config.perf_mode ? 6u : 20u;
	final_prepare_desc.lod_resolutions[1] = config.perf_mode ? 4u : 10u;
	final_prepare_desc.lod_resolutions[2] = config.perf_mode ? 4u : 5u;
	final_prepare_desc.lod_resolutions[3] = 2u;
	final_prepare_desc.brick_size = 8u;
	final_prepare_desc.brick_border = 1u;

	cluster_final_ready = calloc(layout.cluster_count > 0u ? layout.cluster_count : 1u, sizeof(*cluster_final_ready));
	if (!cluster_final_ready) {
		fprintf(stderr, "sdf_unique_entities :: failed to allocate cluster ready state\n");
		setup_ok = false;
		goto cleanup;
	}
	cluster_final_active = calloc(layout.cluster_count > 0u ? layout.cluster_count : 1u, sizeof(*cluster_final_active));
	if (!cluster_final_active) {
		fprintf(stderr, "sdf_unique_entities :: failed to allocate cluster active state\n");
		setup_ok = false;
		goto cleanup;
	}

	s_array_reserve(&clusters, layout.cluster_count);
	for (u32 cluster_index = 0u; cluster_index < layout.cluster_count; ++cluster_index) {
		const u32 first_entity = cluster_index * SDF_UNIQUE_CLUSTER_ENTITY_CAPACITY;
		const u32 remaining = config.entity_count > first_entity ? config.entity_count - first_entity : 0u;
		const u32 cluster_entity_count = s_min(remaining, (u32)SDF_UNIQUE_CLUSTER_ENTITY_CAPACITY);
		const se_sdf_handle cluster_sdf = se_sdf_create(NULL);
		if (cluster_sdf == SE_SDF_NULL ||
			!sdf_unique_populate_cluster(cluster_sdf, &layout, first_entity, cluster_entity_count) ||
			!se_sdf_prepare(cluster_sdf, &preview_prepare_desc)) {
			setup_ok = false;
			break;
		}
		s_array_add(&clusters, cluster_sdf);
		fprintf(stderr, "sdf_unique_entities :: preview cluster %u/%u (%u entities)\n", cluster_index + 1u, layout.cluster_count, cluster_entity_count);
	}
	if (!setup_ok || s_array_get_size(&clusters) != layout.cluster_count) {
		fprintf(stderr, "sdf_unique_entities :: failed while preparing clusters\n");
		goto cleanup;
	}
	if (layout.cluster_count == 1u) {
		se_sdf_handle* only_cluster = s_array_get(&clusters, s_array_handle(&clusters, 0u));
		if (!only_cluster || *only_cluster == SE_SDF_NULL) {
			fprintf(stderr, "sdf_unique_entities :: failed to select single cluster root\n");
			setup_ok = false;
			goto cleanup;
		}
		se_sdf_destroy(root_sdf);
		root_sdf = *only_cluster;
	} else if (!sdf_unique_build_root(root_sdf, &clusters, &layout, &preview_prepare_desc)) {
		fprintf(stderr, "sdf_unique_entities :: failed while preparing root SDF\n");
		setup_ok = false;
		goto cleanup;
	}

	se_sdf_renderer_desc renderer_desc = SE_SDF_RENDERER_DESC_DEFAULTS;
	renderer_desc.max_visible_instances = s_max(layout.cluster_count + 16u, 64u);
	renderer_desc.brick_budget = 1024u;
	renderer_desc.brick_upload_budget = 256u;
	renderer_desc.lod_fade_ratio = 0.10f;
	renderer = se_sdf_renderer_create(&renderer_desc);
	if (renderer == SE_SDF_RENDERER_NULL) {
		setup_ok = false;
		goto cleanup;
	}
	{
		se_sdf_raymarch_quality quality = SE_SDF_RAYMARCH_QUALITY_DEFAULTS;
		quality.max_steps = config.perf_mode ? 15 : 56;
		quality.hit_epsilon = config.perf_mode ? 0.0060f : 0.0022f;
		quality.max_distance = layout.field_radius * 4.0f + layout.cluster_spacing * 4.0f + 160.0f;
		quality.enable_shadows = 0;
		quality.shadow_steps = 0;
		quality.shadow_strength = 0.0f;
		(void)se_sdf_renderer_set_quality(renderer, &quality);
	}
	(void)se_sdf_renderer_set_light_rig(
		renderer,
		&s_vec3(-0.44f, -0.86f, -0.24f),
		&s_vec3(1.18f, 1.12f, 1.04f),
		&s_vec3(0.18f, 0.22f, 0.30f),
		0.010f
	);
	(void)se_sdf_renderer_set_sdf(renderer, root_sdf);
	present_state.window = window;
	present_state.camera = camera;
	present_state.renderer = renderer;
	(void)se_scene_3d_register_custom_render(scene, sdf_unique_render_scene, &present_state);
	for (u32 cluster_index = 0u; cluster_index < (u32)s_array_get_size(&clusters); ++cluster_index) {
		if (active_background_bakes >= max_background_bakes) {
			break;
		}
		se_sdf_handle* cluster = s_array_get(&clusters, s_array_handle(&clusters, cluster_index));
		if (!cluster || *cluster == SE_SDF_NULL || !se_sdf_prepare_async(*cluster, &final_prepare_desc)) {
			fprintf(stderr, "sdf_unique_entities :: failed to start background bake for cluster %u\n", cluster_index);
			setup_ok = false;
			goto cleanup;
		}
		cluster_final_active[cluster_index] = 1u;
		active_background_bakes++;
	}

	fprintf(
		stderr,
		"sdf_unique_entities :: preview ready for %u unique entities across %u clusters in %.2fs; baking finals in background\n",
		config.entity_count,
		layout.cluster_count,
		sdf_unique_now_seconds() - load_start_time
	);
	sdf_unique_debug_dump_samples(root_sdf, &layout);
	preview_ready_time = sdf_unique_now_seconds();
	se_window_loop(window,
		const f32 t = (f32)se_window_get_time(window);
		for (u32 cluster_index = 0u; cluster_index < (u32)s_array_get_size(&clusters); ++cluster_index) {
			se_sdf_handle* cluster = s_array_get(&clusters, s_array_handle(&clusters, cluster_index));
			if (!cluster || *cluster == SE_SDF_NULL || cluster_final_ready[cluster_index] || !cluster_final_active[cluster_index]) {
				continue;
			}
			se_sdf_prepare_status status = {0};
			if (!se_sdf_prepare_poll(*cluster, &status)) {
				fprintf(stderr, "sdf_unique_entities :: background bake failed for cluster %u (result=%d)\n", cluster_index, (i32)se_get_last_error());
				se_window_set_should_close(window, true);
				setup_ok = false;
				break;
			}
			if (status.failed) {
				fprintf(stderr, "sdf_unique_entities :: background bake failed for cluster %u (result=%d)\n", cluster_index, (i32)status.result);
				se_window_set_should_close(window, true);
				setup_ok = false;
				break;
			}
			if (status.applied) {
				cluster_final_active[cluster_index] = 0u;
				if (active_background_bakes > 0u) {
					active_background_bakes--;
				}
				cluster_final_ready[cluster_index] = 1u;
				final_clusters_ready++;
				fprintf(stderr, "sdf_unique_entities :: swapped final cluster %u/%u\n", final_clusters_ready, layout.cluster_count);
			}
		}
		for (u32 cluster_index = 0u;
			setup_ok &&
			active_background_bakes < max_background_bakes &&
			cluster_index < (u32)s_array_get_size(&clusters);
			++cluster_index) {
			se_sdf_handle* cluster = s_array_get(&clusters, s_array_handle(&clusters, cluster_index));
			if (!cluster || *cluster == SE_SDF_NULL || cluster_final_ready[cluster_index] || cluster_final_active[cluster_index]) {
				continue;
			}
			if (!se_sdf_prepare_async(*cluster, &final_prepare_desc)) {
				fprintf(stderr, "sdf_unique_entities :: failed to start background bake for cluster %u\n", cluster_index);
				se_window_set_should_close(window, true);
				setup_ok = false;
				break;
			}
			cluster_final_active[cluster_index] = 1u;
			active_background_bakes++;
		}
		const b8 final_scene_ready = final_clusters_ready == layout.cluster_count;
		if (final_scene_ready && final_ready_time < 0.0f) {
			final_ready_time = sdf_unique_now_seconds();
			fprintf(
				stderr,
				"sdf_unique_entities :: final bake resident in %.2fs (preview-to-final %.2fs)\n",
				final_ready_time - load_start_time,
				preview_ready_time >= 0.0 ? final_ready_time - preview_ready_time : 0.0
			);
		}
		if ((!config.perf_mode || final_scene_ready) && run_start_time < 0.0f) {
			run_start_time = t;
			if (config.perf_mode) {
				fprintf(stderr, "sdf_unique_entities :: final bake ready, starting perf window\n");
			}
		}
		const f32 elapsed = run_start_time >= 0.0f ? (t - run_start_time) : 0.0f;
		const f32 orbit_x = base_camera_position.x + sinf(t * 0.08f) * layout.field_width * (config.perf_mode ? 0.025f : 0.08f);
		const f32 orbit_y = base_camera_position.y + sinf(t * 0.07f) * (config.perf_mode ? 5.0f : 5.0f);
		const f32 orbit_z = base_camera_position.z + cosf(t * 0.09f) * layout.field_depth * (config.perf_mode ? 0.07f : 0.18f);
		const s_vec3 camera_position = s_vec3(orbit_x, orbit_y, orbit_z);
		const s_vec3 camera_target = s_vec3(0.0f, 3.4f + sinf(t * 0.11f) * 1.0f, 0.0f);
		u32 width = 1u;
		u32 height = 1u;
		se_window_get_framebuffer_size(window, &width, &height);
		se_camera_set_location(camera, &camera_position);
		se_camera_set_target(camera, &camera_target);
		se_camera_set_aspect(camera, (f32)width, (f32)height);
		se_debug_trace_begin("sdf_unique_entities");
		se_scene_3d_draw(scene, window);
		se_debug_trace_end("sdf_unique_entities");

		{
			const se_sdf_renderer_stats stats = se_sdf_renderer_get_stats(renderer);
			if (stats.visible_refs > max_visible_refs) {
				max_visible_refs = stats.visible_refs;
			}
			if ((!config.perf_mode || final_scene_ready) && elapsed >= perf_warmup_seconds) {
				const f32 frame_ms = (f32)(se_window_get_delta_time(window) * 1000.0);
				s_array_add(&frame_samples, frame_ms);
				visible_sum += (f64)stats.visible_refs;
				resident_sum += (f64)stats.resident_bricks;
			}
			if (!config.perf_mode && t - last_stats_print >= 1.0f) {
				fprintf(
					stderr,
					"sdf_unique_entities :: visible_clusters=%u resident=%u misses=%u lods=[%u,%u,%u,%u] fps=%.1f ready=%u/%u\n",
					stats.visible_refs,
					stats.resident_bricks,
					stats.page_misses,
					stats.selected_lod_counts[0],
					stats.selected_lod_counts[1],
					stats.selected_lod_counts[2],
					stats.selected_lod_counts[3],
					(f32)se_window_get_fps(window),
					final_clusters_ready,
					layout.cluster_count
				);
				last_stats_print = t;
			}
		}

		if (config.perf_mode && final_scene_ready && elapsed >= config.run_seconds) {
			se_window_set_should_close(window, true);
		}
	);

	if (config.perf_mode) {
		sdf_unique_perf_summary_build(&frame_samples, visible_sum, resident_sum, max_visible_refs, &perf_summary);
		fprintf(
			stderr,
			"sdf_unique_entities :: perf avg_fps=%.2f avg_ms=%.3f p95_ms=%.3f max_ms=%.3f avg_visible_clusters=%.1f max_visible_clusters=%u avg_resident=%.1f frames=%u entities=%u\n",
			perf_summary.avg_fps,
			perf_summary.avg_frame_ms,
			perf_summary.p95_frame_ms,
			perf_summary.max_frame_ms,
			perf_summary.avg_visible_refs,
			perf_summary.max_visible_refs,
			perf_summary.avg_resident_bricks,
			perf_summary.frame_count,
			config.entity_count
		);
		if (!config.perf_no_assert && (perf_summary.avg_fps < 120.0f || perf_summary.max_visible_refs < layout.cluster_count)) {
			fprintf(
				stderr,
				"sdf_unique_entities :: perf gate failed (required avg_fps >= 120 and max_visible_clusters >= %u)\n",
				layout.cluster_count
			);
			perf_failed = true;
		}
	}

cleanup:
	if (renderer != SE_SDF_RENDERER_NULL) {
		se_sdf_renderer_destroy(renderer);
	}
	if (scene != S_HANDLE_NULL) {
		se_scene_3d_destroy(scene);
		camera = S_HANDLE_NULL;
	}
	if (root_sdf != SE_SDF_NULL && (layout.cluster_count != 1u || s_array_get_size(&clusters) == 0u)) {
		se_sdf_destroy(root_sdf);
	}
	for (u32 i = 0u; i < (u32)s_array_get_size(&clusters); ++i) {
		se_sdf_handle* cluster = s_array_get(&clusters, s_array_handle(&clusters, i));
		if (cluster && *cluster != SE_SDF_NULL) {
			se_sdf_destroy(*cluster);
		}
	}
	s_array_clear(&clusters);
	s_array_clear(&frame_samples);
	free(cluster_final_ready);
	free(cluster_final_active);
	if (camera != S_HANDLE_NULL) {
		se_camera_destroy(camera);
	}
	se_sdf_shutdown();
	se_context_destroy(context);
	if (!setup_ok) {
		return 1;
	}
	if (perf_failed) {
		return 2;
	}
	return 0;
}
