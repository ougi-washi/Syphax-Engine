#include "se_input.h"

int main(void) {
	se_input_handle* input = se_input_create(S_HANDLE_NULL, 48);
	se_input_tick(input);
	return 0;
}
