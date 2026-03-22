#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

typedef long long (*VarFn)(int, ...);

typedef struct Entry {
    VarFn fn;
    int bias;
} Entry;

static long long sum_weighted(int lanes, ...) {
    va_list ap;
    va_start(ap, lanes);
    long long acc = 0;
    for (int i = 0; i < lanes; ++i) {
        int v = va_arg(ap, int);
        acc += (long long)(i + 1) * (long long)v;
    }
    va_end(ap);
    return acc;
}

static long long sum_zigzag(int lanes, ...) {
    va_list ap;
    va_start(ap, lanes);
    long long acc = 0;
    for (int i = 0; i < lanes; ++i) {
        int v = va_arg(ap, int);
        int sign = (i & 1) ? -1 : 1;
        acc += (long long)sign * (long long)(i + 3) * (long long)v;
    }
    va_end(ap);
    return acc;
}

static Entry make_entry(int k) {
    Entry e;
    e.fn = (k & 1) ? sum_zigzag : sum_weighted;
    e.bias = k * 5;
    return e;
}

int main(void) {
    Entry table[3];
    for (int i = 0; i < 3; ++i) {
        table[i] = make_entry(i + 2);
    }

    long long total = 0;
    for (int i = 0; i < 6; ++i) {
        Entry e = (i & 1) ? make_entry(i + 3) : table[i % 3];
        total += e.fn(4, i + 1, i + 2, i + 3, i + 4);
        total += (long long)e.bias;
    }

    Entry tail = (1 ? table[2] : make_entry(9));
    total += tail.fn(3, 7, 8, 9);
    total += (long long)tail.bias;

    printf("%lld\n", total);
    return 0;
}

