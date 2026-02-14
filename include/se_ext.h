// Syphax-Engine - Ougi Washi

#ifndef SE_EXT_H
#define SE_EXT_H

#include "se_backend.h"

typedef enum {
	SE_EXT_FEATURE_INSTANCING = 1 << 0,
	SE_EXT_FEATURE_MULTI_RENDER_TARGET = 1 << 1,
	SE_EXT_FEATURE_FLOAT_RENDER_TARGET = 1 << 2,
	SE_EXT_FEATURE_COMPUTE = 1 << 3
} se_ext_feature;

extern const char *se_ext_feature_name(const se_ext_feature feature);
extern b8 se_ext_is_supported(const se_ext_feature feature);
extern b8 se_ext_require(const se_ext_feature feature);

#endif // SE_EXT_H
