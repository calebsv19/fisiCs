#include <stdio.h>
#include <stdlib.h>

typedef union ExitBytes {
    unsigned char bytes[4];
    unsigned short half[2];
} ExitBytes;

typedef struct ExitState {
    ExitBytes payload;
    unsigned short guard;
    unsigned calls;
} ExitState;

static ExitState state = {{{0u, 0u, 0u, 0u}}, 0x1200u, 0u};

static void commit_second(void) {
    state.payload.bytes[3] = (unsigned char)(state.payload.bytes[3] + state.payload.bytes[0]);
    state.guard = (unsigned short)(state.guard + state.payload.half[0]);
    ++state.calls;
    printf("exit-second %u %u %u %u\n", (unsigned)state.payload.bytes[3],
           (unsigned)state.guard, state.calls, (unsigned)state.payload.half[0]);
}

static void commit_first(void) {
    state.payload.bytes[1] = (unsigned char)(state.payload.bytes[1] ^ state.payload.bytes[2]);
    state.guard = (unsigned short)(state.guard + state.payload.bytes[1]);
    ++state.calls;
    printf("exit-first %u %u %u %u\n", (unsigned)state.payload.bytes[1],
           (unsigned)state.guard, state.calls, (unsigned)state.payload.half[0]);
}

int main(void) {
    state.payload.bytes[0] = 9u;
    state.payload.bytes[1] = 21u;
    state.payload.bytes[2] = 0x34u;
    state.payload.bytes[3] = 7u;

    if (atexit(commit_first) != 0 || atexit(commit_second) != 0) {
        return 1;
    }
    printf("exit-main %u %u %u\n", (unsigned)state.payload.half[0],
           (unsigned)state.guard, state.calls);
    return 0;
}
