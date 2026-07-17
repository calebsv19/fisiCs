#include <stdarg.h>

struct wave49_pair {
    int a;
    int b;
};

struct wave49_big {
    int lane[6];
    long total;
};

typedef int (*wave49_fold_fn)(struct wave49_pair item, int scale);

static int wave49_alt_fold(struct wave49_pair item, int scale) {
    return item.a + item.b * scale;
}

static wave49_fold_fn wave49_choose_fold(int selector, wave49_fold_fn fallback) {
    if ((selector & 1) != 0) {
        return fallback;
    }
    return wave49_alt_fold;
}

struct wave49_big wave49_variadic_fnptr_sret_matrix(int seed, int selector, wave49_fold_fn fallback, int count, ...) {
    struct wave49_big out;
    va_list ap;
    wave49_fold_fn fold;
    long total = seed + selector + count;
    int i;

    for (i = 0; i < 6; i++) {
        out.lane[i] = seed - i;
    }

    va_start(ap, count);
    fold = wave49_choose_fold(selector, fallback);
    for (i = 0; i < count && i < 5; i++) {
        int value = va_arg(ap, int);
        struct wave49_pair item;
        item.a = value + seed;
        item.b = selector + i;
        out.lane[i] = fold(item, i + 2) + value;
        total += out.lane[i] * (i + 3);
    }
    va_end(ap);

    out.lane[5] = out.lane[0] + out.lane[4] - seed;
    out.total = total + out.lane[5];
    return out;
}
