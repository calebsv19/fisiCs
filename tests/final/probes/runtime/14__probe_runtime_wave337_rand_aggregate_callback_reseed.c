#include <stdio.h>
#include <stdlib.h>

typedef union CallbackBytes {
    unsigned char bytes[6];
    unsigned short halves[3];
} CallbackBytes;

typedef struct CallbackFrame {
    CallbackBytes payload;
    unsigned short guard;
} CallbackFrame;

typedef CallbackFrame (*FrameBuilder)(unsigned);

static CallbackFrame build_frame(unsigned salt) {
    CallbackFrame frame;
    unsigned i;

    frame.guard = (unsigned short)(0x5100u + salt);
    for (i = 0u; i < 6u; ++i) {
        unsigned value = (unsigned)rand();
        frame.payload.bytes[i] = (unsigned char)(value + salt + i * 23u);
        frame.guard = (unsigned short)(frame.guard + frame.payload.bytes[i] + i);
    }
    return frame;
}

static unsigned fold_frame(CallbackFrame frame) {
    unsigned acc = frame.guard;
    unsigned i;

    for (i = 0u; i < 6u; ++i) {
        acc = acc * 97u + frame.payload.bytes[i];
    }
    return acc;
}

int main(void) {
    FrameBuilder builder = build_frame;
    CallbackFrame a;
    CallbackFrame b;
    CallbackFrame repeat;

    srand(0x1357u);
    a = builder(3u);
    b = builder(9u);
    srand(0x1357u);
    repeat = builder(3u);

    printf("rand-callback %u %u %u %u %u\n", fold_frame(a), fold_frame(b),
           fold_frame(repeat), (unsigned)b.payload.halves[2],
           (unsigned)(a.guard == repeat.guard));
    return 0;
}
