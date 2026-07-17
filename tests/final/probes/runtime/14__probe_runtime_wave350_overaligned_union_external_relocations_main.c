#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

extern int wave350_targets[4];
extern int wave350_add_five(int value);

struct Wave350Selected {
    int tag;
    int *item;
    int (*transform)(int);
};

struct Wave350Carrier {
    uint64_t lanes[3];
    int *tail;
};

union Wave350Union {
    alignas(64) struct Wave350Selected selected;
    struct Wave350Carrier carrier;
    unsigned char bytes[64];
};

static union Wave350Union values[2] = {
    {.selected = {7, &wave350_targets[2], wave350_add_five}},
    {.carrier = {{11, 13, 17}, &wave350_targets[3]}},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %d %d %d %d %d %zu\n",
           alignof(union Wave350Union),
           sizeof(union Wave350Union),
           values[0].selected.item == &wave350_targets[2],
           *values[0].selected.item,
           values[0].selected.transform(10),
           (int)values[1].carrier.lanes[2],
           *values[1].carrier.tail,
           stride);
    return alignof(union Wave350Union) == 64 &&
                   sizeof(union Wave350Union) == 64 &&
                   values[0].selected.item == &wave350_targets[2] &&
                   *values[0].selected.item == 29 &&
                   values[0].selected.transform(10) == 15 &&
                   values[1].carrier.lanes[2] == 17 &&
                   *values[1].carrier.tail == 31 && stride == 64
               ? 0
               : 1;
}
