#include "se_curve.h"

int main(void) {
	se_curve_float_keys curve = {0};
	se_curve_float_init(&curve);
	se_curve_float_add_key(&curve, 0.0f, 0.0f);
	se_curve_float_add_key(&curve, 1.0f, 1.0f);
	se_curve_float_sort(&curve);
	f32 value = 0.0f;
	se_curve_float_eval(&curve, SE_CURVE_LINEAR, 0.5f, &value);
	(void)value;
	return 0;
}
