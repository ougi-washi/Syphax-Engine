#include "se_backend.h"

int main(void) {
	se_backend_info backend = se_get_backend_info();
	(void)backend;
	return 0;
}
