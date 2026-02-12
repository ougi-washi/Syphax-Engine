// Syphax-Engine - Ougi Washi

#ifndef SE_SDF_H
#define SE_SDF_H

#include "syphax/s_types.h"
#include "syphax/s_array.h"

#define SE_SDF_MAX_STRING_LENGTH 4096

typedef enum {
	SE_SDF_NONE,  // Container/group node with no shape
	SE_SDF_SPHERE,
	SE_SDF_BOX,
	SE_SDF_ROUND_BOX,
	SE_SDF_BOX_FRAME,
	SE_SDF_TORUS,
	SE_SDF_CAPPED_TORUS,
	SE_SDF_LINK,
	SE_SDF_CYLINDER,
	SE_SDF_CONE,
	SE_SDF_CONE_INFINITE,
	SE_SDF_PLANE,
	SE_SDF_HEX_PRISM,
	SE_SDF_CAPSULE,
	SE_SDF_VERTICAL_CAPSULE,
	SE_SDF_CAPPED_CYLINDER,
	SE_SDF_CAPPED_CYLINDER_ARBITRARY,
	SE_SDF_ROUNDED_CYLINDER,
	SE_SDF_CAPPED_CONE,
	SE_SDF_CAPPED_CONE_ARBITRARY,
	SE_SDF_SOLID_ANGLE,
	SE_SDF_CUT_SPHERE,
	SE_SDF_CUT_HOLLOW_SPHERE,
	SE_SDF_DEATH_STAR,
	SE_SDF_ROUND_CONE,
	SE_SDF_ROUND_CONE_ARBITRARY,
	SE_SDF_VESICA_SEGMENT,
	SE_SDF_RHOMBUS,
	SE_SDF_OCTAHEDRON,
	SE_SDF_OCTAHEDRON_BOUND,
	SE_SDF_PYRAMID,
	SE_SDF_TRIANGLE,
	SE_SDF_QUAD
} se_sdf_object_type;

typedef struct {
	b8 active;
	f32 scale;
	f32 offset;
	f32 frequency;
} se_sdf_noise;

typedef enum {
	SE_SDF_OP_NONE,
	SE_SDF_OP_UNION,
	SE_SDF_OP_INTERSECTION,
	SE_SDF_OP_SUBTRACTION
} se_sdf_operation;

typedef struct se_sdf_object {
	s_mat4 transform;
	se_sdf_object_type type;
	union {
		struct { f32 radius; } sphere;
		struct { s_vec3 size; } box;
		struct { s_vec3 size; f32 roundness; } round_box;
		struct { s_vec3 size; f32 thickness; } box_frame;
		struct { s_vec2 radii; } torus;
		struct { s_vec2 axis_sin_cos; f32 major_radius; f32 minor_radius; } capped_torus;
		struct { f32 half_length; f32 outer_radius; f32 inner_radius; } link;
		struct { s_vec3 axis_and_radius; } cylinder;
		struct { s_vec2 angle_sin_cos; f32 height; } cone;
		struct { s_vec2 angle_sin_cos; } cone_infinite;
		struct { s_vec3 normal; f32 offset; } plane;
		struct { s_vec2 half_size; } hex_prism;
		struct { s_vec3 a; s_vec3 b; f32 radius; } capsule;
		struct { f32 height; f32 radius; } vertical_capsule;
		struct { f32 radius; f32 half_height; } capped_cylinder;
		struct { s_vec3 a; s_vec3 b; f32 radius; } capped_cylinder_arbitrary;
		struct { f32 outer_radius; f32 corner_radius; f32 half_height; } rounded_cylinder;
		struct { f32 height; f32 radius_base; f32 radius_top; } capped_cone;
		struct { s_vec3 a; s_vec3 b; f32 radius_a; f32 radius_b; } capped_cone_arbitrary;
		struct { s_vec2 angle_sin_cos; f32 radius; } solid_angle;
		struct { f32 radius; f32 cut_height; } cut_sphere;
		struct { f32 radius; f32 cut_height; f32 thickness; } cut_hollow_sphere;
		struct { f32 radius_a; f32 radius_b; f32 separation; } death_star;
		struct { f32 radius_base; f32 radius_top; f32 height; } round_cone;
		struct { s_vec3 a; s_vec3 b; f32 radius_a; f32 radius_b; } round_cone_arbitrary;
		struct { s_vec3 a; s_vec3 b; f32 width; } vesica_segment;
		struct { f32 length_a; f32 length_b; f32 height; f32 corner_radius; } rhombus;
		struct { f32 size; } octahedron;
		struct { f32 size; } octahedron_bound;
		struct { f32 height; } pyramid;
		struct { s_vec3 a; s_vec3 b; s_vec3 c; } triangle;
		struct { s_vec3 a; s_vec3 b; s_vec3 c; s_vec3 d; } quad;
	};			
	se_sdf_operation operation;
	se_sdf_noise noise;
	s_array(struct se_sdf_object, children);
} se_sdf_object;

static inline void se_sdf_add_children(se_sdf_object* parent, sz count, se_sdf_object* children) {
	for (sz i = 0; i < count; ++i) s_array_add(&parent->children, children[i]);
}

static inline se_sdf_object se_sdf_make(se_sdf_object obj) {
	s_array_init(&obj.children);
	return obj;
}

// Shape macros: se_sdf_sphere(transform, .radius = value, ...)
#define se_sdf_sphere(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_SPHERE, .transform = (xform), __VA_ARGS__})
#define se_sdf_box(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_BOX, .transform = (xform), __VA_ARGS__})
#define se_sdf_round_box(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_ROUND_BOX, .transform = (xform), __VA_ARGS__})
#define se_sdf_box_frame(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_BOX_FRAME, .transform = (xform), __VA_ARGS__})
#define se_sdf_torus(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_TORUS, .transform = (xform), __VA_ARGS__})
#define se_sdf_capped_torus(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPPED_TORUS, .transform = (xform), __VA_ARGS__})
#define se_sdf_link(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_LINK, .transform = (xform), __VA_ARGS__})
#define se_sdf_cylinder(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CYLINDER, .transform = (xform), __VA_ARGS__})
#define se_sdf_cone(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CONE, .transform = (xform), __VA_ARGS__})
#define se_sdf_cone_inf(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CONE_INFINITE, .transform = (xform), __VA_ARGS__})
#define se_sdf_plane(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_PLANE, .transform = (xform), __VA_ARGS__})
#define se_sdf_hex_prism(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_HEX_PRISM, .transform = (xform), __VA_ARGS__})
#define se_sdf_capsule(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPSULE, .transform = (xform), __VA_ARGS__})
#define se_sdf_vert_capsule(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_VERTICAL_CAPSULE, .transform = (xform), __VA_ARGS__})
#define se_sdf_capped_cyl(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPPED_CYLINDER, .transform = (xform), __VA_ARGS__})
#define se_sdf_capped_cyl_arb(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPPED_CYLINDER_ARBITRARY, .transform = (xform), __VA_ARGS__})
#define se_sdf_rounded_cyl(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_ROUNDED_CYLINDER, .transform = (xform), __VA_ARGS__})
#define se_sdf_capped_cone(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPPED_CONE, .transform = (xform), __VA_ARGS__})
#define se_sdf_capped_cone_arb(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPPED_CONE_ARBITRARY, .transform = (xform), __VA_ARGS__})
#define se_sdf_solid_angle(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_SOLID_ANGLE, .transform = (xform), __VA_ARGS__})
#define se_sdf_cut_sphere(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CUT_SPHERE, .transform = (xform), __VA_ARGS__})
#define se_sdf_cut_hollow_sphere(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CUT_HOLLOW_SPHERE, .transform = (xform), __VA_ARGS__})
#define se_sdf_death_star(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_DEATH_STAR, .transform = (xform), __VA_ARGS__})
#define se_sdf_round_cone(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_ROUND_CONE, .transform = (xform), __VA_ARGS__})
#define se_sdf_round_cone_arb(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_ROUND_CONE_ARBITRARY, .transform = (xform), __VA_ARGS__})
#define se_sdf_vesica(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_VESICA_SEGMENT, .transform = (xform), __VA_ARGS__})
#define se_sdf_rhombus(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_RHOMBUS, .transform = (xform), __VA_ARGS__})
#define se_sdf_octa(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_OCTAHEDRON, .transform = (xform), __VA_ARGS__})
#define se_sdf_octa_bound(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_OCTAHEDRON_BOUND, .transform = (xform), __VA_ARGS__})
#define se_sdf_pyramid(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_PYRAMID, .transform = (xform), __VA_ARGS__})
#define se_sdf_tri(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_TRIANGLE, .transform = (xform), __VA_ARGS__})
#define se_sdf_quad(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_QUAD, .transform = (xform), __VA_ARGS__})

// Container/group node (no shape, just holds children with operation)
#define se_sdf_group(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_NONE, .transform = (xform), __VA_ARGS__})

// Add nodes with operation: se_sdf_nodes(parent, SE_SDF_OP_UNION, child1, child2)
// Alternative names considered: subshapes, operands, elements, branches
// 'nodes' is used as it fits scene graph terminology
#define se_sdf_nodes(parent, op, ...) do { \
	se_sdf_object _n[] = { __VA_ARGS__ }; \
	(parent).operation = (op); \
	se_sdf_add_children(&(parent), sizeof(_n)/sizeof(_n[0]), _n); \
} while(0)

// Convenience constants
#define se_sdf_union SE_SDF_OP_UNION
#define se_sdf_intersection SE_SDF_OP_INTERSECTION
#define se_sdf_subtraction SE_SDF_OP_SUBTRACTION
#define se_sdf_noop SE_SDF_OP_NONE

// Default transform (identity)
#define se_sdf_default s_mat4_identity

// ============================================================================
// Shader String Generation
// ============================================================================

typedef struct {
    char* data;
    sz capacity;
    sz size;
} se_sdf_string;

void se_sdf_string_init(se_sdf_string* str, sz capacity);
void se_sdf_string_free(se_sdf_string* str);
void se_sdf_string_append(se_sdf_string* str, const char* fmt, ...);
void se_sdf_string_append_vec2(se_sdf_string* str, s_vec2 v);
void se_sdf_string_append_vec3(se_sdf_string* str, s_vec3 v);

// Shape generators - each returns GLSL distance function code
void se_sdf_gen_sphere(se_sdf_string* out, const char* p, f32 radius);
void se_sdf_gen_box(se_sdf_string* out, const char* p, s_vec3 size);
void se_sdf_gen_round_box(se_sdf_string* out, const char* p, s_vec3 size, f32 roundness);
void se_sdf_gen_box_frame(se_sdf_string* out, const char* p, s_vec3 size, f32 thickness);
void se_sdf_gen_torus(se_sdf_string* out, const char* p, f32 major_r, f32 minor_r);
void se_sdf_gen_capped_torus(se_sdf_string* out, const char* p, s_vec2 sc, f32 ra, f32 rb);
void se_sdf_gen_link(se_sdf_string* out, const char* p, f32 le, f32 r1, f32 r2);
void se_sdf_gen_cylinder(se_sdf_string* out, const char* p, s_vec3 c);
void se_sdf_gen_cone(se_sdf_string* out, const char* p, s_vec2 sc, f32 h);
void se_sdf_gen_cone_inf(se_sdf_string* out, const char* p, s_vec2 sc);
void se_sdf_gen_plane(se_sdf_string* out, const char* p, s_vec3 n, f32 h);
void se_sdf_gen_hex_prism(se_sdf_string* out, const char* p, s_vec2 h);
void se_sdf_gen_capsule(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r);
void se_sdf_gen_vert_capsule(se_sdf_string* out, const char* p, f32 h, f32 r);
void se_sdf_gen_capped_cyl(se_sdf_string* out, const char* p, f32 r, f32 hh);
void se_sdf_gen_capped_cyl_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r);
void se_sdf_gen_rounded_cyl(se_sdf_string* out, const char* p, f32 ra, f32 rb, f32 hh);
void se_sdf_gen_capped_cone(se_sdf_string* out, const char* p, f32 h, f32 r1, f32 r2);
void se_sdf_gen_capped_cone_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 ra, f32 rb);
void se_sdf_gen_solid_angle(se_sdf_string* out, const char* p, s_vec2 sc, f32 r);
void se_sdf_gen_cut_sphere(se_sdf_string* out, const char* p, f32 r, f32 ch);
void se_sdf_gen_cut_hollow_sphere(se_sdf_string* out, const char* p, f32 r, f32 ch, f32 t);
void se_sdf_gen_death_star(se_sdf_string* out, const char* p, f32 ra, f32 rb, f32 d);
void se_sdf_gen_round_cone(se_sdf_string* out, const char* p, f32 r1, f32 r2, f32 h);
void se_sdf_gen_round_cone_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r1, f32 r2);
void se_sdf_gen_vesica(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 w);
void se_sdf_gen_rhombus(se_sdf_string* out, const char* p, f32 la, f32 lb, f32 h, f32 r);
void se_sdf_gen_octa(se_sdf_string* out, const char* p, f32 s);
void se_sdf_gen_octa_bound(se_sdf_string* out, const char* p, f32 s);
void se_sdf_gen_pyramid(se_sdf_string* out, const char* p, f32 h);
void se_sdf_gen_tri(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, s_vec3 c);
void se_sdf_gen_quad(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, s_vec3 c, s_vec3 d);

// Generate transform application: (inverse(mat) * vec4(p, 1.0)).xyz
void se_sdf_gen_transform(se_sdf_string* out, const char* p_var, s_mat4 transform, const char* result_var);

// Generate operation combining two distances
void se_sdf_gen_operation(se_sdf_string* out, se_sdf_operation op, const char* d1, const char* d2);

// Main recursive generator
void se_sdf_object_to_string(se_sdf_string* out, se_sdf_object* obj, const char* p_var);

// Helper to generate full distance function
void se_sdf_generate_distance_function(se_sdf_string* out, se_sdf_object* root, const char* func_name);

// Example:
static inline void test_sdf(void) {
	se_sdf_object root = se_sdf_sphere(s_mat4_identity, .sphere.radius = 1.0f);
	
	s_vec3 a = {0, 0, 0}, b = {1, 0, 0}, c = {0, 1, 0};
	se_sdf_object child1 = se_sdf_box(s_mat4_identity, .box.size = (s_vec3){1, 1, 1});
	se_sdf_object child2 = se_sdf_tri(s_mat4_identity, .triangle.a = a, .triangle.b = b, .triangle.c = c);
	
	se_sdf_nodes(root, se_sdf_union, child1, child2);
	
	// Example using a group/container node:
	se_sdf_object scene = se_sdf_group(s_mat4_identity);  // No shape, just a container
	se_sdf_object sphere_a = se_sdf_sphere(s_mat4_identity, .sphere.radius = 1.0f);
	se_sdf_object sphere_b = se_sdf_sphere(s_mat4_identity, .sphere.radius = 0.5f);
	se_sdf_nodes(scene, se_sdf_union, sphere_a, sphere_b);
}

// ============================================================================
// Shader String Generation Example
// ============================================================================

static inline void test_sdf_shader_generation(void) {
	// Build scene
	se_sdf_object root = se_sdf_group(s_mat4_identity);
	se_sdf_object sphere = se_sdf_sphere(s_mat4_identity, .sphere.radius = 1.0f);
	se_sdf_object box = se_sdf_box(s_mat4_identity, .box.size = (s_vec3){0.5f, 0.5f, 0.5f});
	se_sdf_nodes(root, se_sdf_union, sphere, box);
	
	// Generate shader
	se_sdf_string shader;
	se_sdf_string_init(&shader, 8192);
	se_sdf_generate_distance_function(&shader, &root, "map");
	
	// shader.data now contains the GLSL distance function
	// printf("%s\n", shader.data);
	
	se_sdf_string_free(&shader);
}

#endif // SE_SDF_H
