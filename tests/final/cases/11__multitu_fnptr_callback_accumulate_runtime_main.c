#include <stdio.h>

typedef int (*BridgeCallback)(int, int);

int fnptr_callback_accumulate(int seed, int count, BridgeCallback cb);

static int callback_scale_mix(int seed, int step) {
    return seed + step * 3;
}

int main(void) {
    int total = fnptr_callback_accumulate(5, 4, callback_scale_mix);
    if (total != 38) return 1;
    printf("%d\n", total);
    return 0;
}
