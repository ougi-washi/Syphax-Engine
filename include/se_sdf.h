// Syphax-Engine - Ougi Washi

#ifndef SE_SDF_H
#define SE_SDF_H

#include "syphax/s_types.h"
#include "syphax/s_array.h"

typedef enum {
	SE_SDF_OBJECT_TYPE_SPHERE,
	SE_SDF_OBJECT_TYPE_BOX,
	SE_SDF_OBJECT_TYPE_ROUND_BOX
} se_sdf_object_type;

typedef struct {
	b8 active;
	f32 scale;
	f32 offset;
	f32 frequency;
} se_sdf_noise;

typedef enum {
	SE_SDF_OPERATION_NONE,
	SE_SDF_OPERATION_UNION,
	SE_SDF_OPERATION_INTERSECTION,
	SE_SDF_OPERATION_SUBTRACTION
} se_sdf_operation;

typedef struct se_sdf_object {
	s_mat4 transform;
	union {
		struct { f32 radius; } sphere;
		struct { s_vec3 size; } box;
		struct { s_vec3 size; f32 roundness; } round_box;
	};			
	s_array(struct se_sdf_object, children);
} se_sdf_object;

#endif // SE_SDF_H
