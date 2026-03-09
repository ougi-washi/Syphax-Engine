#include "se_sdf.h"

int main(void) {
	se_sdf_desc sdf_desc = SE_SDF_DESC_DEFAULTS;
	se_sdf_handle sdf = se_sdf_create(&sdf_desc);
	se_sdf_clear(sdf);
	return 0;
}
