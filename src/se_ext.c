// Syphax-Engine - Ougi Washi

#include "se_ext.h"
#include "se_debug.h"

const char *se_ext_feature_name(const se_ext_feature feature) {
	switch (feature) {
		case SE_EXT_FEATURE_INSTANCING:
			return "instancing";
		case SE_EXT_FEATURE_MULTI_RENDER_TARGET:
			return "multi_render_target";
		case SE_EXT_FEATURE_FLOAT_RENDER_TARGET:
			return "float_render_target";
		case SE_EXT_FEATURE_COMPUTE:
			return "compute";
		default:
			return "unknown";
	}
}

b8 se_ext_is_supported(const se_ext_feature feature) {
	const se_capabilities caps = se_capabilities_get();
	if (!caps.valid) {
		return false;
	}

	switch (feature) {
		case SE_EXT_FEATURE_INSTANCING:
			return caps.instancing;
		case SE_EXT_FEATURE_MULTI_RENDER_TARGET:
			return caps.max_mrt_count > 1;
		case SE_EXT_FEATURE_FLOAT_RENDER_TARGET:
			return caps.float_render_targets;
		case SE_EXT_FEATURE_COMPUTE:
			return caps.compute_available;
		default:
			return false;
	}
}

b8 se_ext_require(const se_ext_feature feature) {
	if (se_ext_is_supported(feature)) {
		return true;
	}
	se_set_last_error(SE_RESULT_UNSUPPORTED);
	se_debug_log(
		SE_DEBUG_LEVEL_WARN,
		SE_DEBUG_CATEGORY_RENDER,
		"se_ext_require :: extension '%s' is not supported by the active backend/profile",
		se_ext_feature_name(feature));
	return false;
}
