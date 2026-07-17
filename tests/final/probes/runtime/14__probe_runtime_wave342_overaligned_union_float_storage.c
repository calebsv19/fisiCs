#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

union Wave342Union {
    alignas(32) double real;
    uint64_t bits;
    unsigned char bytes[32];
};

static union Wave342Union values[2] = {
    {.real = 1.5},
    {.bits = UINT64_C(0x4004000000000000)},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %llx %.1f %zu\n",
           alignof(union Wave342Union),
           sizeof(union Wave342Union),
           (unsigned long long)values[0].bits,
           values[1].real,
           stride);
    return alignof(union Wave342Union) == 32 &&
                   sizeof(union Wave342Union) == 32 &&
                   values[0].bits == UINT64_C(0x3ff8000000000000) &&
                   values[1].real == 2.5 && stride == 32
               ? 0
               : 1;
}
