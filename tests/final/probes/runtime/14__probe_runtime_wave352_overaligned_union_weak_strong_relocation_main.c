#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

extern int wave352_symbol;
extern int wave352_transform(int value);

struct Wave352Selected {
    int *symbol;
    int (*transform)(int);
    int tag;
};

struct Wave352Carrier {
    uint64_t lanes[3];
    int *tail;
};

union Wave352Union {
    alignas(64) struct Wave352Selected selected;
    struct Wave352Carrier carrier;
    unsigned char bytes[64];
};

static union Wave352Union values[2] = {
    {.selected = {&wave352_symbol, wave352_transform, 7}},
    {.carrier = {{11, 13, 17}, &wave352_symbol}},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %d %d %d %d %zu\n",
           alignof(union Wave352Union),
           sizeof(union Wave352Union),
           values[0].selected.symbol == &wave352_symbol,
           *values[0].selected.symbol,
           values[0].selected.transform(10),
           *values[1].carrier.tail,
           stride);
    return alignof(union Wave352Union) == 64 &&
                   sizeof(union Wave352Union) == 64 &&
                   values[0].selected.symbol == &wave352_symbol &&
                   *values[0].selected.symbol == 41 &&
                   values[0].selected.transform(10) == 19 &&
                   *values[1].carrier.tail == 41 && stride == 64
               ? 0
               : 1;
}
