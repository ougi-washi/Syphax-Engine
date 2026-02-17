#include "se_model.h"
#include "se_window.h"

int main(void) {
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("se_model_core", 960, 540);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_model_handle model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/sphere.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl")
	);
	if (model == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	s_vec3 scale = s_vec3(1.2f, 1.2f, 1.2f);
	s_vec3 rotation = s_vec3(0.0f, 0.01f, 0.0f);
	se_model_scale(model, &scale);
	se_model_rotate(model, &rotation);

	se_model_destroy(model);
	se_context_destroy(context);
	return 0;
}
