#include <stdint.h>
#include <stdalign.h>

extern int wave351_targets[4];
extern int wave351_add_nine(int value);

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

union Wave351Union wave351_values[2] = {
    {.selected = {11, &wave351_targets[1], wave351_add_nine}},
    {.carrier = {{17, 19, 23}, &wave351_targets[3]}},
};
