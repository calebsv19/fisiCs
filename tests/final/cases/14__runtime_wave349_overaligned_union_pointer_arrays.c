#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

static int data[3] = {29, 31, 37};

static int add_one(int value) {
    return value + 1;
}

static int add_two(int value) {
    return value + 2;
}

struct Wave349Selected {
    int *data_pointers[2];
    int (*transforms[2])(int);
    int tag;
};

struct Wave349Carrier {
    uint64_t lanes[5];
    int *tail;
};

union Wave349Union {
    alignas(64) struct Wave349Selected selected;
    struct Wave349Carrier carrier;
    unsigned char bytes[64];
};

static union Wave349Union values[2] = {
    {.selected = {{&data[0], &data[2]}, {add_one, add_two}, 11}},
    {.carrier = {{13, 17, 19, 23, 29}, &data[1]}},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %d %d %d %d %d %d %d %d %zu\n",
           alignof(union Wave349Union),
           sizeof(union Wave349Union),
           values[0].selected.data_pointers[0] == &data[0],
           *values[0].selected.data_pointers[0],
           values[0].selected.data_pointers[1] == &data[2],
           *values[0].selected.data_pointers[1],
           values[0].selected.transforms[0](10),
           values[0].selected.transforms[1](10),
           values[0].selected.tag,
           *values[1].carrier.tail,
           stride);
    return alignof(union Wave349Union) == 64 &&
                   sizeof(union Wave349Union) == 64 &&
                   values[0].selected.data_pointers[0] == &data[0] &&
                   *values[0].selected.data_pointers[0] == 29 &&
                   values[0].selected.data_pointers[1] == &data[2] &&
                   *values[0].selected.data_pointers[1] == 37 &&
                   values[0].selected.transforms[0](10) == 11 &&
                   values[0].selected.transforms[1](10) == 12 &&
                   values[0].selected.tag == 11 &&
                   *values[1].carrier.tail == 31 && stride == 64
               ? 0
               : 1;
}
