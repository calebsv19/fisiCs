#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

static int targets[4] = {3, 5, 7, 11};

struct Wave348Node {
    int pad;
    int value;
};

static struct Wave348Node nodes[2] = {
    {13, 17},
    {19, 23},
};

struct Wave348Selected {
    int tag;
    int *array_item;
    int *field;
};

struct Wave348Carrier {
    uint64_t lanes[3];
    int *tail;
};

union Wave348Union {
    alignas(64) struct Wave348Selected selected;
    struct Wave348Carrier carrier;
    unsigned char bytes[64];
};

static union Wave348Union values[2] = {
    {.selected = {9, &targets[2], &nodes[1].value}},
    {.carrier = {{31, 37, 41}, &targets[3]}},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %d %d %d %d %d %d %zu\n",
           alignof(union Wave348Union),
           sizeof(union Wave348Union),
           values[0].selected.tag,
           values[0].selected.array_item == &targets[2],
           *values[0].selected.array_item,
           values[0].selected.field == &nodes[1].value,
           *values[0].selected.field,
           *values[1].carrier.tail,
           stride);
    return alignof(union Wave348Union) == 64 &&
                   sizeof(union Wave348Union) == 64 &&
                   values[0].selected.tag == 9 &&
                   values[0].selected.array_item == &targets[2] &&
                   *values[0].selected.array_item == 7 &&
                   values[0].selected.field == &nodes[1].value &&
                   *values[0].selected.field == 23 &&
                   *values[1].carrier.tail == 11 && stride == 64
               ? 0
               : 1;
}
