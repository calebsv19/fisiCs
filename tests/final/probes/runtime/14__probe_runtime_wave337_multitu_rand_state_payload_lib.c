#include <stdlib.h>

typedef union SharedBytes {
    unsigned char bytes[7];
    unsigned short halves[3];
} SharedBytes;

typedef struct SharedFrame {
    SharedBytes payload;
    unsigned guard;
} SharedFrame;

SharedFrame wave337_seed_and_build(unsigned seed, unsigned salt) {
    SharedFrame frame;
    unsigned i;

    srand(seed);
    frame.guard = 0x6200u + salt;
    for (i = 0u; i < 7u; ++i) {
        unsigned value = (unsigned)rand();
        frame.payload.bytes[i] = (unsigned char)(value ^ (salt + i * 29u));
        frame.guard = frame.guard * 65u + frame.payload.bytes[i];
    }
    return frame;
}

unsigned wave337_take_next(void) {
    return (unsigned)rand();
}
