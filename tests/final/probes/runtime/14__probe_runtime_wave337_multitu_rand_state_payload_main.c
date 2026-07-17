#include <stdio.h>
#include <stdlib.h>

typedef union SharedBytes {
    unsigned char bytes[7];
    unsigned short halves[3];
} SharedBytes;

typedef struct SharedFrame {
    SharedBytes payload;
    unsigned guard;
} SharedFrame;

SharedFrame wave337_seed_and_build(unsigned seed, unsigned salt);
unsigned wave337_take_next(void);

static unsigned fold_frame(SharedFrame frame) {
    unsigned acc = frame.guard;
    unsigned i;

    for (i = 0u; i < 7u; ++i) {
        acc = acc * 151u + frame.payload.bytes[i] + i;
    }
    return acc;
}

int main(void) {
    SharedFrame frame = wave337_seed_and_build(0x5a5au, 11u);
    unsigned from_lib = wave337_take_next();
    unsigned from_main = (unsigned)rand();

    srand(0x5a5au);
    (void)wave337_take_next();
    printf("rand-multitu %u %u %u %u\n", fold_frame(frame), from_lib, from_main,
           (unsigned)frame.payload.halves[1]);
    return 0;
}
