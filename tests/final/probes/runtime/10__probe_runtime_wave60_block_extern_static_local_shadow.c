#include <stdio.h>

int bucket10_wave60_shared = 11;

static int bump_file_scope(int step) {
    extern int bucket10_wave60_shared;
    bucket10_wave60_shared += step;
    return bucket10_wave60_shared;
}

static int bump_static_shadow(int step) {
    static int bucket10_wave60_shared = 3;
    int snapshot;

    bucket10_wave60_shared += step;
    snapshot = bucket10_wave60_shared;
    {
        extern int bucket10_wave60_shared;
        bucket10_wave60_shared += snapshot;
    }
    return snapshot;
}

int main(void) {
    int a = bump_file_scope(4);
    int b = bump_static_shadow(5);
    int c = bump_static_shadow(2);
    int d = bucket10_wave60_shared;

    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
