#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

static int target = 29;

struct Wave344Small {
    int *pointer;
    int tag;
};

struct Wave344Big {
    uint64_t words[4];
};

union Wave344Union {
    alignas(32) struct Wave344Small small;
    struct Wave344Big big;
    unsigned char bytes[32];
};

static union Wave344Union values[2] = {
    {.small = {&target, 7}},
    {.big = {{11, 13, 17, 19}}},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %d %d %d %llu %zu\n",
           alignof(union Wave344Union),
           sizeof(union Wave344Union),
           values[0].small.pointer == &target,
           *values[0].small.pointer,
           values[0].small.tag,
           (unsigned long long)values[1].big.words[2],
           stride);
    return alignof(union Wave344Union) == 32 &&
                   sizeof(union Wave344Union) == 32 &&
                   values[0].small.pointer == &target &&
                   *values[0].small.pointer == 29 && values[0].small.tag == 7 &&
                   values[1].big.words[2] == 17 && stride == 32
               ? 0
               : 1;
}
