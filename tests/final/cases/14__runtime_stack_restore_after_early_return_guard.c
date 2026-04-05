#include <stdio.h>

static int early_return_probe(int x) {
    volatile int frame_pad[64];
    frame_pad[0] = x;
    frame_pad[63] = x + 1;

#if 1
    return frame_pad[0] + 3;
#endif

    int acc = 0;
    for (int i = 0; i < 64; ++i) {
        acc += frame_pad[i];
    }
    return acc;
}

int main(void) {
    long long sum = 0;
    for (int i = 0; i < 200000; ++i) {
        sum += early_return_probe(i & 15);
    }
    printf("STACK_OK %lld\n", sum);
    return 0;
}
