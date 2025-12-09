#include "builtin_array.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

bool compare(int *lhs, int *rhs) { return *rhs < *lhs; }

int main(void) {

    int_array *my_array = int_array_init(NULL);

    for (int i = 100; i > 0; i--) {
        int_array_append(my_array, &i);
    }
    for (int i = 200; i > 0; i--) {
        int_array_append(my_array, &i);
    }

    for (size_t i = 0; i < my_array->length; i++) {
        printf("Got value %i\n", my_array->data[i]);
    }
    int_array_sort(my_array, compare);
    for (size_t i = 0; i < my_array->length; i++) {
        printf("Got value %i\n", my_array->data[i]);
    }

    int_array_free(my_array);
    printf("Hello World!");
}
