#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

static long long consume_ints(int n, va_list ap) {
    long long acc = 0;
    for (int i = 0; i < n; ++i) {
        int v = va_arg(ap, int);
        acc += (long long)(i + 2) * (long long)v;
    }
    return acc;
}

static long long relay_once(int n, ...) {
    va_list ap;
    va_start(ap, n);
    long long out = consume_ints(n, ap);
    va_end(ap);
    return out;
}

static long long relay_twice(int n, ...) {
    va_list ap;
    va_list cp;
    va_start(ap, n);
    va_copy(cp, ap);
    long long a = consume_ints(n, ap);
    long long b = consume_ints(n, cp);
    va_end(cp);
    va_end(ap);
    return a + b * 2;
}

typedef long long (*RelayFn)(int, ...);

typedef struct Relay {
    RelayFn fn;
    int scale;
} Relay;

int main(void) {
    Relay lanes[2];
    lanes[0].fn = relay_once;
    lanes[0].scale = 3;
    lanes[1].fn = relay_twice;
    lanes[1].scale = 1;

    long long total = 0;
    for (int i = 0; i < 5; ++i) {
        Relay r = lanes[i & 1];
        long long v = r.fn(3, i + 2, i + 4, i + 6);
        total += v * (long long)r.scale;
    }

    printf("%lld\n", total);
    return 0;
}

