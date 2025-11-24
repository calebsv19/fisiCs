#include <stddef.h>

int main(void) {
    int buffer[3] = {1, 2, 3};
    int *p = buffer;
    int *q = buffer + 1;

    // disallow pointer + pointer and pointer + float
    int sum = p + q;
    int other = (float)3.14f + p;

    return sum + other;
}
*** End Patch to File: tests/codegen/pointer_arith.c Code:PATCH_COMMAND***
