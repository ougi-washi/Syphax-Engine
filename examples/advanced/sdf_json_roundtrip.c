// Syphax-Engine - Ougi Washi

#include "se.h"
#include "se_sdf.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static b8 sdf_json_write(const c8* path, const c8* text) {
	if (!path || !text) {
		return 0;
	}
	if (!s_file_write(path, text, strlen(text))) {
		se_set_last_error(SE_RESULT_IO);
		return 0;
	}
	return 1;
}

static c8* sdf_json_stringify_scene(const se_sdf_handle sdf) {
	s_json* root = se_sdf_to_json(sdf);
	if (!root) {
		return NULL;
	}
	c8* text = s_json_stringify(root);
	s_json_free(root);
	if (!text) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
	}
	return text;
}

static s_vec3 sdf_json_color_from_index(const u32 index) {
	const f32 t = (f32)(index % 11u) / 10.0f;
	return s_vec3(
		0.25f + 0.65f * t,
		0.20f + 0.55f * (1.0f - t),
		0.35f + 0.45f * (f32)((index * 3u) % 7u) / 6.0f
	);
}

static se_sdf_primitive_desc sdf_json_make_primitive_desc(const se_sdf_primitive_type type) {
	se_sdf_primitive_desc primitive = {0};
	primitive.type = type;
	switch (type) {
		case SE_SDF_PRIMITIVE_SPHERE:
			primitive.sphere.radius = 0.55f;
			break;
		case SE_SDF_PRIMITIVE_BOX:
			primitive.box.size = s_vec3(0.55f, 0.35f, 0.45f);
			break;
		case SE_SDF_PRIMITIVE_ROUND_BOX:
			primitive.round_box.size = s_vec3(0.45f, 0.30f, 0.35f);
			primitive.round_box.roundness = 0.12f;
			break;
		case SE_SDF_PRIMITIVE_BOX_FRAME:
			primitive.box_frame.size = s_vec3(0.55f, 0.40f, 0.55f);
			primitive.box_frame.thickness = 0.08f;
			break;
		case SE_SDF_PRIMITIVE_TORUS:
			primitive.torus.radii = s_vec2(0.48f, 0.16f);
			break;
		case SE_SDF_PRIMITIVE_CAPPED_TORUS:
			primitive.capped_torus.axis_sin_cos = s_vec2(0.50f, 0.8660254f);
			primitive.capped_torus.major_radius = 0.52f;
			primitive.capped_torus.minor_radius = 0.14f;
			break;
		case SE_SDF_PRIMITIVE_LINK:
			primitive.link.half_length = 0.32f;
			primitive.link.outer_radius = 0.44f;
			primitive.link.inner_radius = 0.12f;
			break;
		case SE_SDF_PRIMITIVE_CYLINDER:
			primitive.cylinder.axis_and_radius = s_vec3(0.0f, 0.0f, 0.34f);
			break;
		case SE_SDF_PRIMITIVE_CONE:
			primitive.cone.angle_sin_cos = s_vec2(0.50f, 0.8660254f);
			primitive.cone.height = 0.82f;
			break;
		case SE_SDF_PRIMITIVE_CONE_INFINITE:
			primitive.cone_infinite.angle_sin_cos = s_vec2(0.42f, 0.9075241f);
			break;
		case SE_SDF_PRIMITIVE_PLANE:
			primitive.plane.normal = s_vec3(0.0f, 1.0f, 0.0f);
			primitive.plane.offset = 0.12f;
			break;
		case SE_SDF_PRIMITIVE_HEX_PRISM:
			primitive.hex_prism.half_size = s_vec2(0.32f, 0.58f);
			break;
		case SE_SDF_PRIMITIVE_CAPSULE:
			primitive.capsule.a = s_vec3(-0.24f, -0.34f, 0.0f);
			primitive.capsule.b = s_vec3(0.24f, 0.34f, 0.0f);
			primitive.capsule.radius = 0.16f;
			break;
		case SE_SDF_PRIMITIVE_VERTICAL_CAPSULE:
			primitive.vertical_capsule.height = 0.78f;
			primitive.vertical_capsule.radius = 0.18f;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER:
			primitive.capped_cylinder.radius = 0.28f;
			primitive.capped_cylinder.half_height = 0.45f;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY:
			primitive.capped_cylinder_arbitrary.a = s_vec3(-0.36f, -0.18f, 0.0f);
			primitive.capped_cylinder_arbitrary.b = s_vec3(0.36f, 0.18f, 0.0f);
			primitive.capped_cylinder_arbitrary.radius = 0.14f;
			break;
		case SE_SDF_PRIMITIVE_ROUNDED_CYLINDER:
			primitive.rounded_cylinder.outer_radius = 0.42f;
			primitive.rounded_cylinder.corner_radius = 0.10f;
			primitive.rounded_cylinder.half_height = 0.34f;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CONE:
			primitive.capped_cone.height = 0.82f;
			primitive.capped_cone.radius_base = 0.42f;
			primitive.capped_cone.radius_top = 0.16f;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY:
			primitive.capped_cone_arbitrary.a = s_vec3(-0.32f, -0.34f, 0.0f);
			primitive.capped_cone_arbitrary.b = s_vec3(0.32f, 0.34f, 0.0f);
			primitive.capped_cone_arbitrary.radius_a = 0.20f;
			primitive.capped_cone_arbitrary.radius_b = 0.08f;
			break;
		case SE_SDF_PRIMITIVE_SOLID_ANGLE:
			primitive.solid_angle.angle_sin_cos = s_vec2(0.42f, 0.9075241f);
			primitive.solid_angle.radius = 0.76f;
			break;
		case SE_SDF_PRIMITIVE_CUT_SPHERE:
			primitive.cut_sphere.radius = 0.58f;
			primitive.cut_sphere.cut_height = 0.18f;
			break;
		case SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE:
			primitive.cut_hollow_sphere.radius = 0.60f;
			primitive.cut_hollow_sphere.cut_height = 0.16f;
			primitive.cut_hollow_sphere.thickness = 0.07f;
			break;
		case SE_SDF_PRIMITIVE_DEATH_STAR:
			primitive.death_star.radius_a = 0.62f;
			primitive.death_star.radius_b = 0.42f;
			primitive.death_star.separation = 0.28f;
			break;
		case SE_SDF_PRIMITIVE_ROUND_CONE:
			primitive.round_cone.radius_base = 0.28f;
			primitive.round_cone.radius_top = 0.08f;
			primitive.round_cone.height = 0.92f;
			break;
		case SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY:
			primitive.round_cone_arbitrary.a = s_vec3(-0.30f, -0.36f, 0.0f);
			primitive.round_cone_arbitrary.b = s_vec3(0.30f, 0.36f, 0.0f);
			primitive.round_cone_arbitrary.radius_a = 0.18f;
			primitive.round_cone_arbitrary.radius_b = 0.06f;
			break;
		case SE_SDF_PRIMITIVE_VESICA_SEGMENT:
			primitive.vesica_segment.a = s_vec3(-0.28f, 0.0f, 0.0f);
			primitive.vesica_segment.b = s_vec3(0.28f, 0.0f, 0.0f);
			primitive.vesica_segment.width = 0.24f;
			break;
		case SE_SDF_PRIMITIVE_RHOMBUS:
			primitive.rhombus.length_a = 0.42f;
			primitive.rhombus.length_b = 0.24f;
			primitive.rhombus.height = 0.58f;
			primitive.rhombus.corner_radius = 0.08f;
			break;
		case SE_SDF_PRIMITIVE_OCTAHEDRON:
			primitive.octahedron.size = 0.52f;
			break;
		case SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND:
			primitive.octahedron_bound.size = 0.52f;
			break;
		case SE_SDF_PRIMITIVE_PYRAMID:
			primitive.pyramid.height = 0.74f;
			break;
		case SE_SDF_PRIMITIVE_TRIANGLE:
			primitive.triangle.a = s_vec3(-0.42f, -0.28f, 0.00f);
			primitive.triangle.b = s_vec3(0.44f, -0.24f, 0.02f);
			primitive.triangle.c = s_vec3(0.02f, 0.46f, 0.12f);
			break;
		case SE_SDF_PRIMITIVE_QUAD:
			primitive.quad.a = s_vec3(-0.40f, -0.28f, -0.04f);
			primitive.quad.b = s_vec3(0.38f, -0.30f, 0.03f);
			primitive.quad.c = s_vec3(0.42f, 0.30f, 0.08f);
			primitive.quad.d = s_vec3(-0.36f, 0.34f, -0.06f);
			break;
		default:
			break;
	}
	return primitive;
}

static b8 sdf_json_add_primitive(
	const se_sdf_handle sdf,
	const se_sdf_node_handle parent,
	const se_sdf_primitive_type type,
	const s_vec3 translation,
	const s_vec3 rotation,
	const u32 color_index,
	const b8 noisy
) {
	se_sdf_node_primitive_desc desc = {0};
	const s_vec3 color = sdf_json_color_from_index(color_index);
	desc.transform = se_sdf_transform(translation, rotation, s_vec3(1.0f, 1.0f, 1.0f));
	desc.operation = SE_SDF_OP_UNION;
	desc.operation_amount = 1.0f;
	desc.primitive = sdf_json_make_primitive_desc(type);
	if (noisy) {
		desc.noise.active = 1;
		desc.noise.scale = 0.035f;
		desc.noise.offset = 0.0f;
		desc.noise.frequency = 1.75f;
	}
	se_sdf_node_handle node = se_sdf_node_create_primitive(sdf, &desc);
	if (node == SE_SDF_NODE_NULL ||
		!se_sdf_node_add_child(sdf, parent, node) ||
		!se_sdf_node_set_color(sdf, node, &color)) {
		return 0;
	}
	return 1;
}

static se_sdf_handle sdf_json_build_detail_scene(void) {
	se_sdf_handle sdf = se_sdf_create(NULL);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	root_desc.operation_amount = 0.22f;
	se_sdf_node_handle root = se_sdf_node_create_group(sdf, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(sdf, root)) {
		return SE_SDF_NULL;
	}
	if (!sdf_json_add_primitive(sdf, root, SE_SDF_PRIMITIVE_TORUS, s_vec3(0.0f, 0.0f, 0.0f), s_vec3(0.35f, 0.2f, 0.0f), 90u, 0) ||
		!sdf_json_add_primitive(sdf, root, SE_SDF_PRIMITIVE_BOX, s_vec3(0.0f, 0.0f, 0.0f), s_vec3(0.0f, 0.6f, 0.0f), 91u, 1)) {
		return SE_SDF_NULL;
	}
	return sdf;
}

static se_sdf_handle sdf_json_build_cluster_scene(
	const u32 first_type,
	const u32 last_type,
	const se_sdf_handle nested_ref
) {
	se_sdf_handle sdf = se_sdf_create(NULL);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.operation_amount = 1.0f;
	se_sdf_node_handle root = se_sdf_node_create_group(sdf, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(sdf, root)) {
		return SE_SDF_NULL;
	}

	se_sdf_node_group_desc blend_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	blend_desc.transform = s_mat4_identity;
	blend_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	blend_desc.operation_amount = 0.18f;
	se_sdf_node_handle blend_group = se_sdf_node_create_group(sdf, &blend_desc);
	if (blend_group == SE_SDF_NODE_NULL || !se_sdf_node_add_child(sdf, root, blend_group)) {
		return SE_SDF_NULL;
	}

	u32 index = 0u;
	for (u32 type_value = first_type; type_value <= last_type; ++type_value, ++index) {
		const i32 column = (i32)(index % 4u);
		const i32 row = (i32)(index / 4u);
		const s_vec3 translation = s_vec3(
			((f32)column - 1.5f) * 2.2f,
			((type_value == (u32)SE_SDF_PRIMITIVE_PLANE) ? -1.1f : 0.35f + 0.10f * (f32)(index % 3u)),
			((f32)row - 1.5f) * 2.0f
		);
		const s_vec3 rotation = s_vec3(
			0.15f * (f32)(index % 3u),
			0.30f * (f32)((index + 1u) % 4u),
			0.08f * (f32)(index % 5u)
		);
		if (!sdf_json_add_primitive(
				sdf,
				blend_group,
				(se_sdf_primitive_type)type_value,
				translation,
				rotation,
				type_value,
				(type_value % 4u) == 0u && type_value != (u32)SE_SDF_PRIMITIVE_PLANE)) {
			return SE_SDF_NULL;
		}
	}

	if (nested_ref != SE_SDF_NULL) {
		se_sdf_node_ref_desc ref_desc = SE_SDF_NODE_REF_DESC_DEFAULTS;
		ref_desc.source = nested_ref;
		ref_desc.transform = se_sdf_transform(s_vec3(0.0f, 0.8f, -4.0f), s_vec3(0.0f, 0.6f, 0.0f), s_vec3(1.0f, 1.0f, 1.0f));
		se_sdf_node_handle ref_node = se_sdf_node_create_ref(sdf, &ref_desc);
		if (ref_node == SE_SDF_NODE_NULL || !se_sdf_node_add_child(sdf, root, ref_node)) {
			return SE_SDF_NULL;
		}
	}

	return sdf;
}

static b8 sdf_json_build_root_scene(
	const se_sdf_handle root_sdf,
	const se_sdf_handle cluster_a,
	const se_sdf_handle cluster_b
) {
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.operation_amount = 1.0f;
	se_sdf_node_handle root = se_sdf_node_create_group(root_sdf, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(root_sdf, root)) {
		return 0;
	}

	se_sdf_node_group_desc smooth_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	smooth_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	smooth_desc.operation_amount = 0.28f;
	se_sdf_node_handle smooth_group = se_sdf_node_create_group(root_sdf, &smooth_desc);
	if (smooth_group == SE_SDF_NODE_NULL || !se_sdf_node_add_child(root_sdf, root, smooth_group)) {
		return 0;
	}
	if (!sdf_json_add_primitive(root_sdf, smooth_group, SE_SDF_PRIMITIVE_ROUND_BOX, s_vec3(-0.65f, 0.6f, 0.0f), s_vec3(0.0f, 0.2f, 0.0f), 200u, 1) ||
		!sdf_json_add_primitive(root_sdf, smooth_group, SE_SDF_PRIMITIVE_SPHERE, s_vec3(0.55f, 0.75f, 0.10f), s_vec3(0.0f, 0.0f, 0.0f), 201u, 0)) {
		return 0;
	}

	se_sdf_node_group_desc subtract_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	subtract_desc.transform = s_mat4_identity;
	subtract_desc.operation = SE_SDF_OP_SUBTRACTION;
	subtract_desc.operation_amount = 1.0f;
	se_sdf_node_handle subtract_group = se_sdf_node_create_group(root_sdf, &subtract_desc);
	if (subtract_group == SE_SDF_NODE_NULL || !se_sdf_node_add_child(root_sdf, root, subtract_group)) {
		return 0;
	}
	if (!sdf_json_add_primitive(root_sdf, subtract_group, SE_SDF_PRIMITIVE_BOX, s_vec3(0.0f, -0.15f, 0.0f), s_vec3(0.0f, 0.5f, 0.0f), 202u, 0) ||
		!sdf_json_add_primitive(root_sdf, subtract_group, SE_SDF_PRIMITIVE_SPHERE, s_vec3(0.0f, -0.05f, 0.0f), s_vec3(0.0f, 0.0f, 0.0f), 203u, 0)) {
		return 0;
	}

	se_sdf_node_ref_desc ref_a_desc = SE_SDF_NODE_REF_DESC_DEFAULTS;
	ref_a_desc.source = cluster_a;
	ref_a_desc.transform = se_sdf_transform(s_vec3(-5.5f, 0.0f, -4.5f), s_vec3(0.0f, 0.25f, 0.0f), s_vec3(1.0f, 1.0f, 1.0f));
	se_sdf_node_handle ref_a = se_sdf_node_create_ref(root_sdf, &ref_a_desc);
	se_sdf_node_ref_desc ref_b_desc = SE_SDF_NODE_REF_DESC_DEFAULTS;
	ref_b_desc.source = cluster_b;
	ref_b_desc.transform = se_sdf_transform(s_vec3(4.6f, 0.0f, 3.6f), s_vec3(0.0f, -0.20f, 0.0f), s_vec3(1.0f, 1.0f, 1.0f));
	se_sdf_node_handle ref_b = se_sdf_node_create_ref(root_sdf, &ref_b_desc);
	se_sdf_node_ref_desc ref_a_repeat_desc = SE_SDF_NODE_REF_DESC_DEFAULTS;
	ref_a_repeat_desc.source = cluster_a;
	ref_a_repeat_desc.transform = se_sdf_transform(s_vec3(5.4f, 0.0f, -5.1f), s_vec3(0.0f, 0.85f, 0.0f), s_vec3(0.9f, 0.9f, 0.9f));
	se_sdf_node_handle ref_a_repeat = se_sdf_node_create_ref(root_sdf, &ref_a_repeat_desc);
	return ref_a != SE_SDF_NODE_NULL &&
		ref_b != SE_SDF_NODE_NULL &&
		ref_a_repeat != SE_SDF_NODE_NULL &&
		se_sdf_node_add_child(root_sdf, root, ref_a) &&
		se_sdf_node_add_child(root_sdf, root, ref_b) &&
		se_sdf_node_add_child(root_sdf, root, ref_a_repeat);
}

static b8 sdf_json_compare_bounds(
	const se_sdf_handle a,
	const se_sdf_handle b,
	const c8* label
) {
	const f32 epsilon = 0.0005f;
	se_sdf_bounds bounds_a = SE_SDF_BOUNDS_DEFAULTS;
	se_sdf_bounds bounds_b = SE_SDF_BOUNDS_DEFAULTS;
	if (!se_sdf_calculate_bounds(a, &bounds_a) || !se_sdf_calculate_bounds(b, &bounds_b)) {
		printf("sdf_json_roundtrip :: bounds compare failed for %s\n", label);
		return 0;
	}
	if (bounds_a.valid != bounds_b.valid || bounds_a.has_unbounded_primitives != bounds_b.has_unbounded_primitives) {
		printf("sdf_json_roundtrip :: bounds flags mismatch for %s\n", label);
		return 0;
	}
	if (!bounds_a.valid) {
		return 1;
	}
	if (fabsf(bounds_a.center.x - bounds_b.center.x) > epsilon ||
		fabsf(bounds_a.center.y - bounds_b.center.y) > epsilon ||
		fabsf(bounds_a.center.z - bounds_b.center.z) > epsilon ||
		fabsf(bounds_a.radius - bounds_b.radius) > epsilon) {
		printf("sdf_json_roundtrip :: bounds center/radius mismatch for %s\n", label);
		return 0;
	}
	if (!bounds_a.has_unbounded_primitives &&
		(fabsf(bounds_a.min.x - bounds_b.min.x) > epsilon ||
		 fabsf(bounds_a.min.y - bounds_b.min.y) > epsilon ||
		 fabsf(bounds_a.min.z - bounds_b.min.z) > epsilon ||
		 fabsf(bounds_a.max.x - bounds_b.max.x) > epsilon ||
		 fabsf(bounds_a.max.y - bounds_b.max.y) > epsilon ||
		 fabsf(bounds_a.max.z - bounds_b.max.z) > epsilon)) {
		printf("sdf_json_roundtrip :: bounds extents mismatch for %s\n", label);
		return 0;
	}
	return 1;
}

static b8 sdf_json_compare_samples(
	const se_sdf_handle a,
	const se_sdf_handle b,
	const c8* label
) {
	const f32 epsilon = 0.0025f;
	for (i32 zi = 0; zi < 11; ++zi) {
		for (i32 yi = 0; yi < 7; ++yi) {
			for (i32 xi = 0; xi < 11; ++xi) {
				const s_vec3 point = s_vec3(
					-10.0f + (f32)xi * 2.0f,
					-3.0f + (f32)yi * 1.5f,
					-10.0f + (f32)zi * 2.0f
				);
				f32 da = 0.0f;
				f32 db = 0.0f;
				if (!se_sdf_sample_distance(a, &point, &da, NULL) ||
					!se_sdf_sample_distance(b, &point, &db, NULL) ||
					fabsf(da - db) > epsilon) {
					printf(
						"sdf_json_roundtrip :: sample mismatch for %s at (%.2f %.2f %.2f): %.6f vs %.6f\n",
						label,
						point.x, point.y, point.z,
						(double)da, (double)db
					);
					return 0;
				}
			}
		}
	}
	return 1;
}

static b8 sdf_json_compare_rays(
	const se_sdf_handle a,
	const se_sdf_handle b,
	const c8* label
) {
	const s_vec3 origins[] = {
		s_vec3(0.0f, 6.0f, 0.0f),
		s_vec3(-5.0f, 4.0f, -5.0f),
		s_vec3(5.0f, 5.0f, 5.0f),
		s_vec3(0.0f, 3.0f, -8.0f),
		s_vec3(8.0f, 2.0f, 0.0f)
	};
	const s_vec3 directions[] = {
		s_vec3(0.0f, -1.0f, 0.0f),
		s_vec3(0.4f, -0.8f, 0.4f),
		s_vec3(-0.5f, -0.7f, -0.3f),
		s_vec3(0.0f, -0.4f, 1.0f),
		s_vec3(-1.0f, -0.2f, 0.0f)
	};
	const f32 epsilon = 0.0035f;
	for (u32 i = 0u; i < sizeof(origins) / sizeof(origins[0]); ++i) {
		se_sdf_surface_hit hit_a = {0};
		se_sdf_surface_hit hit_b = {0};
		const b8 result_a = se_sdf_raycast(a, &origins[i], &directions[i], 64.0f, &hit_a);
		const se_result error_a = se_get_last_error();
		const b8 result_b = se_sdf_raycast(b, &origins[i], &directions[i], 64.0f, &hit_b);
		const se_result error_b = se_get_last_error();
		if ((!result_a && error_a != SE_RESULT_NOT_FOUND) ||
			(!result_b && error_b != SE_RESULT_NOT_FOUND) ||
			hit_a.hit != hit_b.hit) {
			printf(
				"sdf_json_roundtrip :: ray hit mismatch for %s at ray %u (a: result=%d error=%s hit=%d, b: result=%d error=%s hit=%d)\n",
				label,
				i,
				result_a,
				se_result_str(error_a),
				hit_a.hit,
				result_b,
				se_result_str(error_b),
				hit_b.hit
			);
			return 0;
		}
		if (hit_a.hit &&
			(fabsf(hit_a.distance - hit_b.distance) > epsilon ||
			 fabsf(hit_a.position.x - hit_b.position.x) > epsilon ||
			 fabsf(hit_a.position.y - hit_b.position.y) > epsilon ||
			 fabsf(hit_a.position.z - hit_b.position.z) > epsilon)) {
			printf("sdf_json_roundtrip :: ray distance mismatch for %s at ray %u\n", label, i);
			return 0;
		}
	}
	return 1;
}

static b8 sdf_json_build_v1_reference_scene(const se_sdf_handle sdf) {
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	se_sdf_node_handle root = se_sdf_node_create_group(sdf, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(sdf, root)) {
		return 0;
	}
	if (!sdf_json_add_primitive(sdf, root, SE_SDF_PRIMITIVE_BOX, s_vec3(-0.25f, 0.0f, 0.0f), s_vec3(0.0f, 0.0f, 0.0f), 300u, 0) ||
		!sdf_json_add_primitive(sdf, root, SE_SDF_PRIMITIVE_SPHERE, s_vec3(0.35f, 0.15f, 0.0f), s_vec3(0.0f, 0.0f, 0.0f), 301u, 1)) {
		return 0;
	}
	return 1;
}

static s_json* sdf_json_clone_node(const s_json* node) {
	if (!node) {
		return NULL;
	}
	s_json* copy = NULL;
	switch (node->type) {
		case S_JSON_NULL:
			copy = s_json_null(node->name);
			break;
		case S_JSON_BOOL:
			copy = s_json_bool(node->name, node->as.boolean);
			break;
		case S_JSON_NUMBER:
			copy = s_json_num(node->name, node->as.number);
			break;
		case S_JSON_STRING:
			copy = s_json_str(node->name, node->as.string);
			break;
		case S_JSON_ARRAY:
			copy = s_json_array_empty(node->name);
			break;
		case S_JSON_OBJECT:
			copy = s_json_object_empty(node->name);
			break;
		default:
			return NULL;
	}
	if (!copy) {
		return NULL;
	}
	if (node->type == S_JSON_ARRAY || node->type == S_JSON_OBJECT) {
		for (sz i = 0; i < node->as.children.count; ++i) {
			s_json* child_copy = sdf_json_clone_node(node->as.children.items[i]);
			if (!child_copy || !s_json_add(copy, child_copy)) {
				if (child_copy) {
					s_json_free(child_copy);
				}
				s_json_free(copy);
				return NULL;
			}
		}
	}
	return copy;
}

static s_json* sdf_json_make_v1_json(const se_sdf_handle sdf) {
	s_json* v2_root = se_sdf_to_json(sdf);
	s_json* v1_root = NULL;
	if (!v2_root) {
		return NULL;
	}
	const s_json* sdfs_json = s_json_get(v2_root, "sdfs");
	const s_json* scene_json = sdfs_json ? s_json_at(sdfs_json, 0u) : NULL;
	const s_json* root_index_json = scene_json ? s_json_get(scene_json, "root_index") : NULL;
	const s_json* nodes_json = scene_json ? s_json_get(scene_json, "nodes") : NULL;
	if (!scene_json || !root_index_json || !nodes_json) {
		s_json_free(v2_root);
		return NULL;
	}

	v1_root = s_json_object_empty(NULL);
	if (!v1_root ||
		!s_json_add(v1_root, s_json_str("format", "se_sdf_json")) ||
		!s_json_add(v1_root, s_json_num("version", 1.0)) ||
		!s_json_add(v1_root, sdf_json_clone_node(root_index_json)) ||
		!s_json_add(v1_root, sdf_json_clone_node(nodes_json))) {
		if (v1_root) {
			s_json_free(v1_root);
		}
		s_json_free(v2_root);
		return NULL;
	}
	s_json_free(v2_root);
	return v1_root;
}

static b8 sdf_json_expect_load_failure(
	const c8* label,
	const c8* json_text,
	const se_result expected_error
) {
	b8 ok = 0;
	se_sdf_handle sdf = se_sdf_create(NULL);
	s_json* json = NULL;
	if (sdf == SE_SDF_NULL) {
		printf("sdf_json_roundtrip :: failure test setup failed for %s (%s)\n", label, se_result_str(se_get_last_error()));
		return 0;
	}
	json = s_json_parse(json_text);
	if (!json) {
		printf("sdf_json_roundtrip :: failure test parse failed for %s\n", label);
		goto cleanup;
	}
	if (se_sdf_from_json(sdf, json)) {
		printf("sdf_json_roundtrip :: invalid json unexpectedly loaded for %s\n", label);
		goto cleanup;
	}
	if (se_get_last_error() != expected_error) {
		printf(
			"sdf_json_roundtrip :: wrong error for %s (%s, expected %s)\n",
			label,
			se_result_str(se_get_last_error()),
			se_result_str(expected_error)
		);
		goto cleanup;
	}
	ok = 1;

cleanup:
	if (json) {
		s_json_free(json);
	}
	if (sdf != SE_SDF_NULL) {
		se_sdf_destroy(sdf);
	}
	return ok;
}

static b8 sdf_json_expect_failure_preserves_scene(
	const c8* label,
	const c8* invalid_json_text
) {
	b8 ok = 0;
	c8* before_text = NULL;
	c8* after_text = NULL;
	s_json* invalid_json = NULL;
	se_sdf_handle target = se_sdf_create(NULL);
	if (target == SE_SDF_NULL) {
		printf("sdf_json_roundtrip :: preserve test setup failed for %s (%s)\n", label, se_result_str(se_get_last_error()));
		return 0;
	}
	if (!sdf_json_build_v1_reference_scene(target)) {
		printf("sdf_json_roundtrip :: preserve test seed build failed for %s (%s)\n", label, se_result_str(se_get_last_error()));
		goto cleanup;
	}
	before_text = sdf_json_stringify_scene(target);
	invalid_json = s_json_parse(invalid_json_text);
	if (!before_text || !invalid_json) {
		printf("sdf_json_roundtrip :: preserve test setup parse/stringify failed for %s\n", label);
		goto cleanup;
	}
	if (se_sdf_from_json(target, invalid_json)) {
		printf("sdf_json_roundtrip :: preserve test unexpectedly loaded invalid json for %s\n", label);
		goto cleanup;
	}
	after_text = sdf_json_stringify_scene(target);
	if (!after_text || strcmp(before_text, after_text) != 0) {
		printf("sdf_json_roundtrip :: failed load mutated scene for %s\n", label);
		goto cleanup;
	}
	ok = 1;

cleanup:
	if (before_text) {
		free(before_text);
	}
	if (after_text) {
		free(after_text);
	}
	if (invalid_json) {
		s_json_free(invalid_json);
	}
	if (target != SE_SDF_NULL) {
		se_sdf_destroy(target);
	}
	return ok;
}

static b8 sdf_json_compare_legacy_resource_roundtrip(void) {
	const c8* path = SE_RESOURCE_EXAMPLE("civilization/landscape.json");
	b8 ok = 0;
	c8* source_text = NULL;
	c8* roundtrip_text = NULL;
	s_json* source_json = NULL;
	se_sdf_handle source = se_sdf_create(NULL);
	se_sdf_handle roundtrip = se_sdf_create(NULL);
	char validation_error[256] = {0};
	if (source == SE_SDF_NULL || roundtrip == SE_SDF_NULL) {
		printf("sdf_json_roundtrip :: legacy file setup failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}
	if (!se_sdf_from_json_file(source, path) ||
		!se_sdf_validate(source, validation_error, sizeof(validation_error))) {
		printf("sdf_json_roundtrip :: legacy file load failed for %s (%s)\n", path, se_result_str(se_get_last_error()));
		goto cleanup;
	}
	source_text = sdf_json_stringify_scene(source);
	if (!source_text) {
		printf("sdf_json_roundtrip :: legacy file stringify failed\n");
		goto cleanup;
	}
	source_json = s_json_parse(source_text);
	if (!source_json || !se_sdf_from_json(roundtrip, source_json) ||
		!se_sdf_validate(roundtrip, validation_error, sizeof(validation_error)) ||
		!sdf_json_compare_bounds(source, roundtrip, "legacy_file") ||
		!sdf_json_compare_samples(source, roundtrip, "legacy_file") ||
		!sdf_json_compare_rays(source, roundtrip, "legacy_file")) {
		printf("sdf_json_roundtrip :: legacy file roundtrip failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}
	roundtrip_text = sdf_json_stringify_scene(roundtrip);
	if (!roundtrip_text || strcmp(source_text, roundtrip_text) != 0) {
		printf("sdf_json_roundtrip :: legacy file text mismatch\n");
		goto cleanup;
	}
	ok = 1;

cleanup:
	if (source_text) {
		free(source_text);
	}
	if (roundtrip_text) {
		free(roundtrip_text);
	}
	if (source_json) {
		s_json_free(source_json);
	}
	if (source != SE_SDF_NULL) {
		se_sdf_destroy(source);
	}
	if (roundtrip != SE_SDF_NULL) {
		se_sdf_destroy(roundtrip);
	}
	return ok;
}

static b8 sdf_json_compare_overwrite_loads(
	const se_sdf_handle source,
	const se_sdf_handle v1_reference
) {
	b8 ok = 0;
	s_json* source_json_a = NULL;
	s_json* source_json_b = NULL;
	s_json* v1_json = NULL;
	c8* final_text = NULL;
	c8* expected_text = NULL;
	se_sdf_handle target = se_sdf_create(NULL);
	char validation_error[256] = {0};
	if (target == SE_SDF_NULL) {
		printf("sdf_json_roundtrip :: overwrite setup failed (%s)\n", se_result_str(se_get_last_error()));
		return 0;
	}

	source_json_a = se_sdf_to_json(source);
	source_json_b = se_sdf_to_json(source);
	v1_json = sdf_json_make_v1_json(v1_reference);
	if (!source_json_a || !source_json_b || !v1_json) {
		printf("sdf_json_roundtrip :: overwrite source json build failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}

	if (!se_sdf_from_json(target, source_json_a) ||
		!se_sdf_validate(target, validation_error, sizeof(validation_error)) ||
		!sdf_json_compare_bounds(source, target, "overwrite_first") ||
		!sdf_json_compare_samples(source, target, "overwrite_first") ||
		!sdf_json_compare_rays(source, target, "overwrite_first")) {
		printf("sdf_json_roundtrip :: overwrite first load failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}

	if (!se_sdf_from_json(target, source_json_b) ||
		!se_sdf_validate(target, validation_error, sizeof(validation_error)) ||
		!sdf_json_compare_bounds(source, target, "overwrite_second") ||
		!sdf_json_compare_samples(source, target, "overwrite_second") ||
		!sdf_json_compare_rays(source, target, "overwrite_second")) {
		printf("sdf_json_roundtrip :: overwrite second load failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}

	if (!se_sdf_from_json(target, v1_json) ||
		!se_sdf_validate(target, validation_error, sizeof(validation_error)) ||
		!sdf_json_compare_bounds(v1_reference, target, "overwrite_v1") ||
		!sdf_json_compare_samples(v1_reference, target, "overwrite_v1") ||
		!sdf_json_compare_rays(v1_reference, target, "overwrite_v1")) {
		printf("sdf_json_roundtrip :: overwrite replacement load failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}

	expected_text = sdf_json_stringify_scene(v1_reference);
	final_text = sdf_json_stringify_scene(target);
	if (!expected_text || !final_text || strcmp(expected_text, final_text) != 0) {
		printf("sdf_json_roundtrip :: overwrite final scene text mismatch\n");
		goto cleanup;
	}

	ok = 1;

cleanup:
	if (source_json_a) {
		s_json_free(source_json_a);
	}
	if (source_json_b) {
		s_json_free(source_json_b);
	}
	if (v1_json) {
		s_json_free(v1_json);
	}
	if (final_text) {
		free(final_text);
	}
	if (expected_text) {
		free(expected_text);
	}
	if (target != SE_SDF_NULL) {
		se_sdf_destroy(target);
	}
	return ok;
}

int main(void) {
	const c8* json_path = "sdf_json_roundtrip_snapshot.json";
	const c8* invalid_missing_format =
		"{\"version\":2,\"entry_sdf_index\":0,\"sdfs\":[{\"root_index\":-1,\"nodes\":[]}]}";
	const c8* invalid_unsupported_version =
		"{\"format\":\"se_sdf_json\",\"version\":99}";
	const c8* invalid_ref_index =
		"{"
			"\"format\":\"se_sdf_json\","
			"\"version\":2,"
			"\"entry_sdf_index\":0,"
			"\"sdfs\":["
				"{"
					"\"root_index\":0,"
					"\"nodes\":["
						"{"
							"\"children\":[],"
							"\"is_group\":false,"
							"\"node_type\":2,"
							"\"transform\":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"
							"\"operation\":1,"
							"\"operation_amount\":1,"
							"\"noise\":{\"active\":false,\"scale\":0,\"offset\":0,\"frequency\":1},"
							"\"color\":[0,0,0],"
							"\"has_color\":false,"
							"\"ref_sdf_index\":7"
						"}"
					"]"
				"}"
			"]"
		"}";
	const c8* invalid_self_cycle =
		"{"
			"\"format\":\"se_sdf_json\","
			"\"version\":2,"
			"\"entry_sdf_index\":0,"
			"\"sdfs\":["
				"{"
					"\"root_index\":0,"
					"\"nodes\":["
						"{"
							"\"children\":[],"
							"\"is_group\":false,"
							"\"node_type\":2,"
							"\"transform\":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"
							"\"operation\":1,"
							"\"operation_amount\":1,"
							"\"noise\":{\"active\":false,\"scale\":0,\"offset\":0,\"frequency\":1},"
							"\"color\":[0,0,0],"
							"\"has_color\":false,"
							"\"ref_sdf_index\":0"
						"}"
					"]"
				"}"
			"]"
		"}";
	const c8* invalid_indirect_cycle =
		"{"
			"\"format\":\"se_sdf_json\","
			"\"version\":2,"
			"\"entry_sdf_index\":0,"
			"\"sdfs\":["
				"{"
					"\"root_index\":0,"
					"\"nodes\":["
						"{"
							"\"children\":[],"
							"\"is_group\":false,"
							"\"node_type\":2,"
							"\"transform\":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"
							"\"operation\":1,"
							"\"operation_amount\":1,"
							"\"noise\":{\"active\":false,\"scale\":0,\"offset\":0,\"frequency\":1},"
							"\"color\":[0,0,0],"
							"\"has_color\":false,"
							"\"ref_sdf_index\":1"
						"}"
					"]"
				"},"
				"{"
					"\"root_index\":0,"
					"\"nodes\":["
						"{"
							"\"children\":[],"
							"\"is_group\":false,"
							"\"node_type\":2,"
							"\"transform\":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"
							"\"operation\":1,"
							"\"operation_amount\":1,"
							"\"noise\":{\"active\":false,\"scale\":0,\"offset\":0,\"frequency\":1},"
							"\"color\":[0,0,0],"
							"\"has_color\":false,"
							"\"ref_sdf_index\":0"
						"}"
					"]"
				"}"
			"]"
		"}";
	const c8* invalid_v1_ref = 
		"{"
			"\"format\":\"se_sdf_json\","
			"\"version\":1,"
			"\"root_index\":0,"
			"\"nodes\":["
				"{"
					"\"children\":[],"
					"\"is_group\":false,"
					"\"node_type\":2,"
					"\"transform\":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"
					"\"operation\":1,"
					"\"operation_amount\":1,"
					"\"noise\":{\"active\":false,\"scale\":0,\"offset\":0,\"frequency\":1},"
					"\"color\":[0,0,0],"
					"\"has_color\":false"
				"}"
			"]"
		"}";
	c8* source_text = NULL;
	c8* memory_text = NULL;
	c8* file_text = NULL;
	c8* v1_reference_text = NULL;
	c8* v1_text = NULL;
	b8 ok = 0;

	se_context* context = se_context_create();
	se_sdf_handle detail = sdf_json_build_detail_scene();
	se_sdf_handle cluster_a = sdf_json_build_cluster_scene((u32)SE_SDF_PRIMITIVE_SPHERE, (u32)SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY, SE_SDF_NULL);
	se_sdf_handle cluster_b = sdf_json_build_cluster_scene((u32)SE_SDF_PRIMITIVE_ROUNDED_CYLINDER, (u32)SE_SDF_PRIMITIVE_QUAD, detail);
	se_sdf_handle source = se_sdf_create(NULL);
	se_sdf_handle memory_loaded = se_sdf_create(NULL);
	se_sdf_handle file_loaded = se_sdf_create(NULL);
	se_sdf_handle v1_loaded = se_sdf_create(NULL);
	se_sdf_handle v1_reference = se_sdf_create(NULL);
	if (detail == SE_SDF_NULL || cluster_a == SE_SDF_NULL || cluster_b == SE_SDF_NULL ||
		source == SE_SDF_NULL || memory_loaded == SE_SDF_NULL || file_loaded == SE_SDF_NULL ||
		v1_loaded == SE_SDF_NULL || v1_reference == SE_SDF_NULL) {
		printf("sdf_json_roundtrip :: setup failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}

	if (!sdf_json_build_root_scene(source, cluster_a, cluster_b) ||
		!sdf_json_build_v1_reference_scene(v1_reference)) {
		printf("sdf_json_roundtrip :: graph build failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}

	char validation_error[256] = {0};
	if (!se_sdf_validate(source, validation_error, sizeof(validation_error))) {
		printf("sdf_json_roundtrip :: source validate failed: %s\n", validation_error);
		goto cleanup;
	}

	source_text = sdf_json_stringify_scene(source);
	if (!source_text || !sdf_json_write(json_path, source_text)) {
		printf("sdf_json_roundtrip :: save failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}

	s_json* source_json = s_json_parse(source_text);
	if (!source_json || !se_sdf_from_json(memory_loaded, source_json) || !se_sdf_from_json_file(file_loaded, json_path)) {
		if (source_json) {
			s_json_free(source_json);
		}
		printf("sdf_json_roundtrip :: load failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}
	s_json_free(source_json);

	memory_text = sdf_json_stringify_scene(memory_loaded);
	file_text = sdf_json_stringify_scene(file_loaded);
	if (!memory_text || !file_text ||
		strcmp(source_text, memory_text) != 0 ||
		strcmp(source_text, file_text) != 0) {
		printf("sdf_json_roundtrip :: roundtrip json text mismatch\n");
		goto cleanup;
	}

	if (!se_sdf_validate(memory_loaded, validation_error, sizeof(validation_error)) ||
		!se_sdf_validate(file_loaded, validation_error, sizeof(validation_error)) ||
		!sdf_json_compare_bounds(source, memory_loaded, "memory") ||
		!sdf_json_compare_bounds(source, file_loaded, "file") ||
		!sdf_json_compare_samples(source, memory_loaded, "memory") ||
		!sdf_json_compare_samples(source, file_loaded, "file") ||
		!sdf_json_compare_rays(source, memory_loaded, "memory") ||
		!sdf_json_compare_rays(source, file_loaded, "file")) {
		goto cleanup;
	}

	s_json* v1_json = sdf_json_make_v1_json(v1_reference);
	if (!v1_json || !se_sdf_from_json(v1_loaded, v1_json) || !se_sdf_validate(v1_loaded, validation_error, sizeof(validation_error))) {
		if (v1_json) {
			s_json_free(v1_json);
		}
		printf("sdf_json_roundtrip :: version 1 compatibility failed (%s)\n", se_result_str(se_get_last_error()));
		goto cleanup;
	}
	s_json_free(v1_json);
	v1_reference_text = sdf_json_stringify_scene(v1_reference);
	v1_text = sdf_json_stringify_scene(v1_loaded);
	if (!v1_text ||
		!v1_reference_text ||
		strcmp(v1_reference_text, v1_text) != 0 ||
		!sdf_json_compare_bounds(v1_reference, v1_loaded, "v1") ||
		!sdf_json_compare_samples(v1_reference, v1_loaded, "v1") ||
		!sdf_json_compare_rays(v1_reference, v1_loaded, "v1")) {
		if (v1_reference_text && v1_text && strcmp(v1_reference_text, v1_text) != 0) {
			printf("sdf_json_roundtrip :: v1 text mismatch after load\n");
		}
		goto cleanup;
	}

	if (!sdf_json_expect_load_failure("missing_format", invalid_missing_format, SE_RESULT_INVALID_ARGUMENT) ||
		!sdf_json_expect_load_failure("unsupported_version", invalid_unsupported_version, SE_RESULT_UNSUPPORTED) ||
		!sdf_json_expect_load_failure("ref_index", invalid_ref_index, SE_RESULT_INVALID_ARGUMENT) ||
		!sdf_json_expect_load_failure("self_cycle", invalid_self_cycle, SE_RESULT_INVALID_ARGUMENT) ||
		!sdf_json_expect_load_failure("indirect_cycle", invalid_indirect_cycle, SE_RESULT_INVALID_ARGUMENT) ||
		!sdf_json_expect_load_failure("v1_ref_unsupported", invalid_v1_ref, SE_RESULT_UNSUPPORTED) ||
		!sdf_json_expect_failure_preserves_scene("invalid_keeps_target", invalid_self_cycle) ||
		!sdf_json_compare_legacy_resource_roundtrip() ||
		!sdf_json_compare_overwrite_loads(source, v1_reference)) {
		goto cleanup;
	}

	printf("sdf_json_roundtrip :: exact graph/file roundtrip OK\n");
	printf("sdf_json_roundtrip :: version 1 compatibility OK\n");
	printf("sdf_json_roundtrip :: invalid input rejection OK\n");
	printf("sdf_json_roundtrip :: legacy file roundtrip OK\n");
	ok = 1;

cleanup:
	if (source_text) {
		free(source_text);
	}
	if (memory_text) {
		free(memory_text);
	}
	if (file_text) {
		free(file_text);
	}
	if (v1_text) {
		free(v1_text);
	}
	if (v1_reference_text) {
		free(v1_reference_text);
	}
	(void)remove(json_path);
	if (source != SE_SDF_NULL) {
		se_sdf_destroy(source);
	}
	if (cluster_b != SE_SDF_NULL) {
		se_sdf_destroy(cluster_b);
	}
	if (cluster_a != SE_SDF_NULL) {
		se_sdf_destroy(cluster_a);
	}
	if (detail != SE_SDF_NULL) {
		se_sdf_destroy(detail);
	}
	if (memory_loaded != SE_SDF_NULL) {
		se_sdf_destroy(memory_loaded);
	}
	if (file_loaded != SE_SDF_NULL) {
		se_sdf_destroy(file_loaded);
	}
	if (v1_loaded != SE_SDF_NULL) {
		se_sdf_destroy(v1_loaded);
	}
	if (v1_reference != SE_SDF_NULL) {
		se_sdf_destroy(v1_reference);
	}
	se_sdf_shutdown();
	se_context_destroy(context);
	return ok ? 0 : 1;
}
