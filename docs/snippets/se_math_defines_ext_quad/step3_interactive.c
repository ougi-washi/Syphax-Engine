#include "se_math.h"
#include "se_defines.h"
#include "se_ext.h"
#include "se_quad.h"

int main(void) {
	se_box_2d box = {0};
	se_box_2d_make(&box, &s_mat3_identity);
	se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
	(void)se_result_str(se_get_last_error());
	se_ext_is_supported(SE_EXT_FEATURE_INSTANCING);
	se_quad quad = {0};
	se_quad_2d_create(&quad, 1);
	se_quad_render(&quad, 1);
	return 0;
}
