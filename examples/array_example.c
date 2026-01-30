// Syphax-Engine - Ougi Washi

#include "syphax/s_array.h"

#define ARRAY_SIZE 8

typedef s_array(int, ints);

void display_array(ints* array) {
	printf("Current size: %zu | Elements: ", s_array_get_size(array));
	s_foreach(array, i) {
		printf("%d, ", *s_array_get(array, i));
	}
	printf("\n");
}

i32 main() {
	ints my_ints = {0};
	s_array_init(&my_ints, ARRAY_SIZE);

	printf("Adding elements to array\n");
	for (sz i = 0; i < ARRAY_SIZE; i++) {
		s_array_add(&my_ints, i);
	}
	display_array(&my_ints);

	printf("Removing element at index 5\n");
	s_array_remove_at(&my_ints, 5);
	display_array(&my_ints);

	printf("Removing element at index 0\n");
	s_array_remove_at(&my_ints, 0);
	display_array(&my_ints);

	printf("Add element with value 10\n");
	s_array_add(&my_ints, 10);
	display_array(&my_ints);

	printf("Clearing array\n");
	s_array_clear(&my_ints);
	display_array(&my_ints);

	return 0;
}
