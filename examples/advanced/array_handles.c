// Syphax-Engine - Ougi Washi

#include "syphax/s_array.h"
#include <stdio.h>

#define ARRAY_SIZE 8

typedef s_array(int, ints);

void display_array(ints* array) {
	printf("Current size: %zu | Elements: ", s_array_get_size(array));
	int *value = NULL;
	s_foreach(array, value) {
		printf("%d, ", *value);
	}
	printf("\n");
}

i32 main(void) {
	printf("advanced/array_handles :: Advanced example (reference)\n");
	ints my_ints = {0};
	s_array_init(&my_ints);
	s_array_reserve(&my_ints, ARRAY_SIZE);

	printf("Adding elements to array\n");
	for (sz i = 0; i < ARRAY_SIZE; i++) {
		int value = (int)i;
		s_array_add(&my_ints, value);
	}
	display_array(&my_ints);

	printf("Removing element at index 5\n");
	{
		s_handle handle = s_array_handle(&my_ints, 5);
		s_array_remove(&my_ints, handle);
	}
	display_array(&my_ints);

	printf("Removing element at index 0\n");
	{
		s_handle handle = s_array_handle(&my_ints, 0);
		s_array_remove(&my_ints, handle);
	}
	display_array(&my_ints);

	printf("Add element with value 10\n");
	{
		int value = 10;
		s_array_add(&my_ints, value);
	}
	display_array(&my_ints);

	printf("Clearing array\n");
	s_array_clear(&my_ints);
	display_array(&my_ints);

	return 0;
}
