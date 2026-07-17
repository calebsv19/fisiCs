#include <stdio.h>

struct wave49_pair {
    int a;
    int b;
};

struct wave49_big {
    int lane[6];
    long total;
};

typedef int (*wave49_fold_fn)(struct wave49_pair item, int scale);

struct wave49_big wave49_variadic_fnptr_sret_matrix(int seed, int selector, wave49_fold_fn fallback, int count, ...);

static int wave49_fold_local(struct wave49_pair item, int scale) {
    return item.a * scale + item.b;
}

int main(void) {
    struct wave49_big got = wave49_variadic_fnptr_sret_matrix(5, 1, wave49_fold_local, 5, 4, 7, 2, 9, 6);
    printf("%d %d %d %d %ld\n", got.lane[0], got.lane[2], got.lane[4], got.lane[5], got.total);
    return 0;
}
