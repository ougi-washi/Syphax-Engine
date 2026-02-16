#include "se_math.h"
#include "se_defines.h"
#include "se_ext.h"
#include "se_quad.h"

int main(void) {
	se_box_2d box = {0};
	se_box_2d_make(&box, &s_mat3_identity);
	return 0;
}
