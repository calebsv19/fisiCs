#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

static int target = 41;

union Wave343Union {
    alignas(32) int *pointer;
    uintptr_t bits;
    unsigned char bytes[32];
};

static union Wave343Union values[2] = {
    {.pointer = &target},
    {.pointer = &target},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %d %d %zu\n",
           alignof(union Wave343Union),
           sizeof(union Wave343Union),
           values[0].pointer == &target,
           *values[1].pointer,
           stride);
    return alignof(union Wave343Union) == 32 &&
                   sizeof(union Wave343Union) == 32 &&
                   values[0].pointer == &target &&
                   values[1].pointer == &target && *values[1].pointer == 41 &&
                   stride == 32
               ? 0
               : 1;
}
