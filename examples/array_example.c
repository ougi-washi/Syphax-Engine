// Syphax-Engine - Ougi Washi

#include "syphax/s_array.h"
#include <stdio.h>

#define ARRAY_SIZE 8

S_DEFINE_ARRAY(i32, ints, ARRAY_SIZE);

void display_array(ints* array) {
    printf("Current size: %zu | Elements: ", ints_get_size(array));
    s_foreach(array, i) {
        printf("%d, ", *ints_get(array, i));
    }
    printf("\n");
}

i32 main() {
    
    ints my_ints = {0};

    printf("Adding elements to array\n");
    for (sz i = 0; i < ARRAY_SIZE; i++) {
        ints_add(&my_ints, i);
    }
    display_array(&my_ints);

    printf("Removing element at index 5\n");
    ints_remove_at(&my_ints, 5);
    display_array(&my_ints);

    printf("Removing element at index 0\n");
    ints_remove_at(&my_ints, 0);
    display_array(&my_ints);

    printf("Add element with value 10\n");
    ints_add(&my_ints, 10);
    display_array(&my_ints);

    printf("Clearing array\n");
    ints_clear(&my_ints);
    display_array(&my_ints);

    return 0;
}
