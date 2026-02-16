#include "se_backend.h"

int main(void) {
	se_backend_info backend = se_get_backend_info();
	(void)backend;
	se_portability_profile profile = se_get_portability_profile();
	(void)profile;
	return 0;
}
