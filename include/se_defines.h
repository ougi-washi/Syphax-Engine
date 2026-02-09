// Syphax-Engine - Ougi Washi

#ifndef SE_DEFINES_H
#define SE_DEFINES_H

#define SE_MAX_PATH_LENGTH 256
#ifndef RESOURCES_DIR
#	error "RESOURCES_DIR not defined!"
#endif

typedef enum {
	SE_RESULT_OK = 0,
	SE_RESULT_INVALID_ARGUMENT,
	SE_RESULT_OUT_OF_MEMORY,
	SE_RESULT_NOT_FOUND,
	SE_RESULT_IO,
	SE_RESULT_BACKEND_FAILURE,
	SE_RESULT_UNSUPPORTED,
	SE_RESULT_CAPACITY_EXCEEDED
} se_result;

extern const char* se_result_str(se_result result);
extern se_result se_get_last_error(void);
extern void se_set_last_error(se_result result);

#endif // SE_DEFINES_H
