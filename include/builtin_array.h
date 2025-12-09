#ifndef BUILTIN_ARRAY
#define BUILTIN_ARRAY
#include "array_template.h"
#include <stdint.h>
#include <stddef.h>

ARRAY_DECLARE(int, int_array)
ARRAY_DECLARE(char, char_array)
ARRAY_DECLARE(double, double_array)
ARRAY_DECLARE(int64_t, int64_array)
#endif // !BUILTIN_ARRAY

