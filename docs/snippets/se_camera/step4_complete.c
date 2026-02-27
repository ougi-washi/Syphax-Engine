#include "se_camera.h"

int main(void) {
	se_camera_handle camera = se_camera_create();
	se_camera_set_aspect(camera, 1280.0f, 720.0f);
	se_camera_set_perspective(camera, 60.0f, 0.1f, 200.0f);
	s_vec3 pivot = s_vec3(0.0f, 0.0f, 0.0f);
	se_camera_set_target_mode(camera, true);
	se_camera_set_target(camera, &pivot);
	s_vec3 rotation = s_vec3(0.34f, 0.62f, 0.0f);
	se_camera_set_rotation(camera, &rotation);
	se_camera_add_rotation(camera, &s_vec3(-0.01f, 0.02f, 0.0f));
	f32 camera_distance = 10.0f - 0.25f;
	if (camera_distance < 2.0f) {
		camera_distance = 2.0f;
	}
	if (camera_distance > 30.0f) {
		camera_distance = 30.0f;
	}
	s_vec3 forward = se_camera_get_forward_vector(camera);
	s_vec3 position = s_vec3_sub(&pivot, &s_vec3_muls(&forward, camera_distance));
	se_camera_set_location(camera, &position);
	s_mat4 view_projection = se_camera_get_view_projection_matrix(camera);
	(void)view_projection;
	se_camera_destroy(camera);
	return 0;
}
