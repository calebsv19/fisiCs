#include <stdio.h>

int bucket10_wave60_current_shared = 11;

static int bump_file_scope_current(int step) {
    bucket10_wave60_current_shared += step;
    return bucket10_wave60_current_shared;
}

static int bump_static_current_shadow(int step) {
    static int bucket10_wave60_current_static_shadow = 3;
    int snapshot;

    bucket10_wave60_current_static_shadow += step;
    snapshot = bucket10_wave60_current_static_shadow;
    bucket10_wave60_current_shared += snapshot;
    return snapshot;
}

int main(void) {
    int a = bump_file_scope_current(4);
    int b = bump_static_current_shadow(5);
    int c = bump_static_current_shadow(2);
    int d = bucket10_wave60_current_shared;

    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
