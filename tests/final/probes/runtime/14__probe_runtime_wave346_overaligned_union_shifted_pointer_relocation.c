#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

static int shifted_target = 61;
static int carrier_target = 73;

struct Wave346Shifted {
    int tag;
    int *pointer;
};

struct Wave346Carrier {
    int *pointer;
    uint64_t words[3];
};

union Wave346Union {
    alignas(32) struct Wave346Shifted shifted;
    struct Wave346Carrier carrier;
    unsigned char bytes[32];
};

static union Wave346Union values[2] = {
    {.shifted = {9, &shifted_target}},
    {.carrier = {&carrier_target, {11, 13, 17}}},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %d %d %d %d %d %llu %zu\n",
           alignof(union Wave346Union),
           sizeof(union Wave346Union),
           values[0].shifted.tag,
           values[0].shifted.pointer == &shifted_target,
           *values[0].shifted.pointer,
           values[1].carrier.pointer == &carrier_target,
           *values[1].carrier.pointer,
           (unsigned long long)values[1].carrier.words[2],
           stride);
    return alignof(union Wave346Union) == 32 &&
                   sizeof(union Wave346Union) == 32 &&
                   values[0].shifted.tag == 9 &&
                   values[0].shifted.pointer == &shifted_target &&
                   *values[0].shifted.pointer == 61 &&
                   values[1].carrier.pointer == &carrier_target &&
                   *values[1].carrier.pointer == 73 &&
                   values[1].carrier.words[2] == 17 && stride == 32
               ? 0
               : 1;
}
