#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

int wave351_targets[4] = {37, 41, 43, 47};

int wave351_add_nine(int value) {
    return value + 9;
}

struct Wave351Selected {
    int tag;
    int *item;
    int (*transform)(int);
};

struct Wave351Carrier {
    uint64_t lanes[3];
    int *tail;
};

union Wave351Union {
    alignas(64) struct Wave351Selected selected;
    struct Wave351Carrier carrier;
    unsigned char bytes[64];
};

extern union Wave351Union wave351_values[2];

int main(void) {
    size_t stride = (size_t)((uintptr_t)&wave351_values[1] -
                             (uintptr_t)&wave351_values[0]);
    printf("%zu %zu %d %d %d %d %d %zu\n",
           alignof(union Wave351Union),
           sizeof(union Wave351Union),
           wave351_values[0].selected.item == &wave351_targets[1],
           *wave351_values[0].selected.item,
           wave351_values[0].selected.transform(10),
           (int)wave351_values[1].carrier.lanes[2],
           *wave351_values[1].carrier.tail,
           stride);
    return alignof(union Wave351Union) == 64 &&
                   sizeof(union Wave351Union) == 64 &&
                   wave351_values[0].selected.item == &wave351_targets[1] &&
                   *wave351_values[0].selected.item == 41 &&
                   wave351_values[0].selected.transform(10) == 19 &&
                   wave351_values[1].carrier.lanes[2] == 23 &&
                   *wave351_values[1].carrier.tail == 47 && stride == 64
               ? 0
               : 1;
}
