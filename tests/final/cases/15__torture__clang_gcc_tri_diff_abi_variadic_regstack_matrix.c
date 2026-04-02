#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)

static unsigned fold32(unsigned acc, unsigned v) {
    acc ^= v + 0x9e3779b9u + (acc << 6u) + (acc >> 2u);
    return acc;
}

static unsigned sum_ints(int n, ...) {
    va_list ap;
    unsigned acc = 0x811c9dc5u;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        int v = va_arg(ap, int);
        acc = fold32(acc, (unsigned)v ^ (unsigned)(i * 17 + 3));
    }
    va_end(ap);
    return acc;
}

static unsigned sum_doubles_scaled(int n, ...) {
    va_list ap;
    unsigned acc = 0x1234567u;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        double d = va_arg(ap, double);
        unsigned scaled = (unsigned)(d * 1024.0 + 0.5);
        acc = fold32(acc, scaled ^ (unsigned)(i * 29 + 7));
    }
    va_end(ap);
    return acc;
}

int main(void) {
    unsigned a = sum_ints(
        20,
        3, 5, 8, 13, 21, 34, 55, 89, 144, 233,
        377, 610, 987, 1597, 2584, 4181, 6765, 10946, 17711, 28657
    );
    unsigned b = sum_ints(
        18,
        17, 19, 23, 29, 31, 37, 41, 43, 47,
        53, 59, 61, 67, 71, 73, 79, 83, 89
    );

    unsigned c = sum_doubles_scaled(
        14,
        0.5, 1.25, 2.75, 3.5, 5.125, 8.5, 13.75,
        21.0, 34.25, 55.5, 89.75, 144.0, 233.25, 377.5
    );
    unsigned d = sum_doubles_scaled(
        12,
        1.5, 2.0, 4.25, 8.75, 16.0, 32.5,
        64.25, 128.75, 256.0, 512.5, 1024.25, 2048.75
    );

    printf("%u %u\n", a ^ c, b ^ (d + c * 3u));
    return 0;
}
