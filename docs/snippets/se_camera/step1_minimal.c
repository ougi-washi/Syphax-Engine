#include "se_camera.h"

int main(void) {
	se_camera_handle camera = se_camera_create();
	se_camera_set_aspect(camera, 1280.0f, 720.0f);
	return 0;
}
