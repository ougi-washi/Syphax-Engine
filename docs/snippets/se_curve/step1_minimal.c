#include "se_curve.h"

int main(void) {
	se_curve_float_keys curve = {0};
	se_curve_float_init(&curve);
	se_curve_float_add_key(&curve, 0.0f, 0.0f);
	return 0;
}
