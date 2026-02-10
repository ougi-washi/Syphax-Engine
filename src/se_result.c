// Syphax-Engine - Ougi Washi

#include "se_defines.h"

static _Thread_local se_result g_se_last_error = SE_RESULT_OK;

const char* se_result_str(se_result result) {
	switch (result) {
		case SE_RESULT_OK:
			return "ok";
		case SE_RESULT_INVALID_ARGUMENT:
			return "invalid_argument";
		case SE_RESULT_OUT_OF_MEMORY:
			return "out_of_memory";
		case SE_RESULT_NOT_FOUND:
			return "not_found";
		case SE_RESULT_IO:
			return "io";
		case SE_RESULT_BACKEND_FAILURE:
			return "backend_failure";
		case SE_RESULT_UNSUPPORTED:
			return "unsupported";
		case SE_RESULT_CAPACITY_EXCEEDED:
			return "capacity_exceeded";
		default:
			return "unknown";
	}
}

se_result se_get_last_error(void) {
	return g_se_last_error;
}

void se_set_last_error(se_result result) {
	g_se_last_error = result;
}
