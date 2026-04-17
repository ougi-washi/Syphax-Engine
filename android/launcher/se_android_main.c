// Syphax-Engine - Ougi Washi

#include "window/se_android_internal.h"

#include <android/log.h>
#include <android_native_app_glue.h>

extern int se_android_example_main(void);

void android_main(struct android_app* app) {
	se_android_set_app(app);
	const int exit_code = se_android_example_main();
	__android_log_print(ANDROID_LOG_INFO, "syphax-engine", "android_main :: example exited with code %d", exit_code);
	if (app && app->activity) {
		ANativeActivity_finish(app->activity);
	}
	se_android_clear_app();
}
