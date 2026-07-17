#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

static int first_target = 37;
static int second_target = 53;

struct Wave345Small {
    int *pointer;
    int tag;
};

struct Wave345Large {
    int *pointer;
    uint64_t words[3];
};

union Wave345Union {
    alignas(32) struct Wave345Small small;
    struct Wave345Large large;
    unsigned char bytes[32];
};

static union Wave345Union values[2] = {
    {.small = {&first_target, 7}},
    {.large = {&second_target, {11, 13, 17}}},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %d %d %d %d %d %llu %zu\n",
           alignof(union Wave345Union),
           sizeof(union Wave345Union),
           values[0].small.pointer == &first_target,
           *values[0].small.pointer,
           values[0].small.tag,
           values[1].large.pointer == &second_target,
           *values[1].large.pointer,
           (unsigned long long)values[1].large.words[2],
           stride);
    return alignof(union Wave345Union) == 32 &&
                   sizeof(union Wave345Union) == 32 &&
                   values[0].small.pointer == &first_target &&
                   *values[0].small.pointer == 37 && values[0].small.tag == 7 &&
                   values[1].large.pointer == &second_target &&
                   *values[1].large.pointer == 53 &&
                   values[1].large.words[2] == 17 && stride == 32
               ? 0
               : 1;
}
