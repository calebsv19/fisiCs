#include <stdio.h>
#include <stdlib.h>

typedef union RandomBytes {
    unsigned char bytes[8];
    unsigned short halves[4];
} RandomBytes;

typedef struct RandomFrame {
    RandomBytes payload;
    unsigned guard;
} RandomFrame;

static RandomFrame sample_frame(unsigned count) {
    RandomFrame frame;
    unsigned i;

    frame.guard = 0x4100u + count;
    for (i = 0u; i < 8u; ++i) {
        unsigned value = (unsigned)rand();
        frame.payload.bytes[i] = (unsigned char)(value ^ (value >> 9) ^ i * 17u);
        frame.guard = frame.guard * 33u + frame.payload.bytes[i];
    }
    return frame;
}

static unsigned fold_frame(RandomFrame frame) {
    unsigned acc = frame.guard;
    unsigned i;

    for (i = 0u; i < 8u; ++i) {
        acc = acc * 131u + frame.payload.bytes[i] + i;
    }
    return acc;
}

int main(void) {
    RandomFrame first;
    RandomFrame repeat;
    RandomFrame next;

    srand(0x2468u);
    first = sample_frame(1u);
    next = sample_frame(2u);
    srand(0x2468u);
    repeat = sample_frame(1u);

    printf("rand-state %u %u %u %u %u\n", fold_frame(first), fold_frame(next),
           fold_frame(repeat), (unsigned)first.payload.halves[1],
           (unsigned)next.payload.bytes[6]);
    return 0;
}
