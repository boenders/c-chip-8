#ifndef ARRAY_TEMPLATE
#define ARRAY_TEMPLATE
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define ARRAY_DECLARE(T, Name) \
    typedef void (*Name##_destructor)(T*); \
    typedef struct { \
        T *data; \
        size_t length; \
        size_t size; \
        Name##_destructor destructor; \
    } Name; \
\
    Name *Name##_init(Name##_destructor destructor); \
    void Name##_free(Name *array); \
    int Name##_append(Name *array, T *value); \
    void Name##_remove(Name *array, int index); \
    void Name##_sort(Name *array, bool compare(T*, T*)); \

#define ARRAY_DEFINE(T, Name) \
    void Name##_partition(T *start, T *end, bool compare(T *, T *), size_t step_size); \
\
    Name *Name##_init(Name##_destructor destructor) { \
        Name *result = (Name *)malloc(sizeof *result); \
        result->length = 0; \
        result->size = 8; \
        result->destructor = destructor; \
        result->data = (T *)calloc(result->size, sizeof(T)); \
        return result; \
    } \
\
    void Name##_free(Name *array) { \
        if (array->destructor) { \
            for (size_t i = 0; i < array->length; i++) { \
                array->destructor(array->data + (i * sizeof *array->data)); \
            } \
        } \
        free(array->data); \
        free(array); \
    } \
\
    int Name##_append(Name *array, T *value) { \
        fflush(stdout); \
        if (array->length == array->size) { \
            T *new_pointer = realloc(array->data, array->size * sizeof *array->data * 2);    \
            if (!new_pointer) { \
                return -1; \
            } \
            array->data = new_pointer; \
            array->data[array->length] = *value; \
            array->size *= 2; \
        } else { \
            array->data[array->length] = *value; \
        } \
        array->length++; \
        return 0; \
    } \
\
    void Name##_remove(Name *array, int index) { \
        for (size_t i = index; i < array->length - 1; i++) { \
            array->data[i] = array->data[i+1]; \
        } \
        array->length--; \
    } \
\
    void Name##_sort(Name *array, bool compare(T*, T*)) { \
        Name##_partition(array->data, \
                         array->data + array->length, \
                         compare, \
                         sizeof(T) \
                        ); \
    } \
\
    void Name##_partition(T *start, T *end, bool compare(T *, T *), size_t step_size) { \
        if (start + 1 >= end) { \
            return; \
        } \
        printf("Step size is: %li", step_size); \
        T *pivot = start; \
        T *last = end - 1; \
        T *buffer = malloc(step_size); \
\
        while (pivot != last) { \
            if (compare(pivot, last)) { \
                last -= 1; \
                continue; \
            } else { \
                *buffer = *last; \
                *last = *(pivot + 1); \
                *(pivot + 1) = *pivot; \
                *pivot = *buffer; \
                pivot += 1; \
            } \
        } \
        free(buffer); \
        Name##_partition(start, pivot, compare, step_size); \
        Name##_partition(pivot + 1, end, compare, step_size); \
    }
#endif // ARRAY_TEMPLATE
