#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

static int first_target = 17;
static int second_target = 23;
static int carrier_target = 31;

static int add_five(int value) {
    return value + 5;
}

static int add_nine(int value) {
    return value + 9;
}

struct Wave347Selected {
    int *first;
    int (*transform)(int);
    int *second;
    int tag;
};

struct Wave347Carrier {
    uint64_t first_bits;
    int (*transform)(int);
    uint64_t second_bits;
    uint64_t tag_bits;
    int *tail_pointer;
};

union Wave347Union {
    alignas(64) struct Wave347Selected selected;
    struct Wave347Carrier carrier;
    unsigned char bytes[64];
};

static union Wave347Union values[2] = {
    {.selected = {&first_target, add_five, &second_target, 7}},
    {.carrier = {11, add_nine, 13, 17, &carrier_target}},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %d %d %d %d %d %d %llu %d %d %zu\n",
           alignof(union Wave347Union),
           sizeof(union Wave347Union),
           values[0].selected.first == &first_target,
           *values[0].selected.first,
           values[0].selected.transform(10),
           values[0].selected.second == &second_target,
           *values[0].selected.second,
           values[0].selected.tag,
           (unsigned long long)values[1].carrier.second_bits,
           values[1].carrier.transform(10),
           *values[1].carrier.tail_pointer,
           stride);
    return alignof(union Wave347Union) == 64 &&
                   sizeof(union Wave347Union) == 64 &&
                   values[0].selected.first == &first_target &&
                   *values[0].selected.first == 17 &&
                   values[0].selected.transform(10) == 15 &&
                   values[0].selected.second == &second_target &&
                   *values[0].selected.second == 23 &&
                   values[0].selected.tag == 7 &&
                   values[1].carrier.second_bits == 13 &&
                   values[1].carrier.transform(10) == 19 &&
                   *values[1].carrier.tail_pointer == 31 && stride == 64
               ? 0
               : 1;
}
