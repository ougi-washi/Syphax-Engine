#include "se_camera.h"

int main(void) {
	se_camera_handle camera = se_camera_create();
	se_camera_set_aspect(camera, 1280.0f, 720.0f);
	se_camera_set_perspective(camera, 60.0f, 0.1f, 200.0f);
	s_mat4 view_projection = se_camera_get_view_projection_matrix(camera);
	(void)view_projection;
	s_vec3 pivot = s_vec3(0.0f, 0.0f, 0.0f);
	se_camera_orbit(camera, &pivot, 0.01f, -0.01f, -1.3f, 1.3f);
	se_camera_dolly(camera, &pivot, -0.25f, 2.0f, 30.0f);
	return 0;
}
