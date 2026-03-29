#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

static unsigned fold_bytes(const unsigned char *bytes, int n) {
    unsigned hash = 2166136261u;
    for (int i = 0; i < n; ++i) {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    return hash;
}

static unsigned hash_ld_args(int n, ...) {
    va_list ap;
    unsigned combined = 0u;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        long double value = va_arg(ap, long double);
        unsigned part = fold_bytes((const unsigned char *)&value, (int)sizeof(value));
        combined ^= part + (unsigned)(i * 97);
    }
    va_end(ap);
    return combined;
}

int main(void) {
    long double a = 1.0L;
    long double b = -2.0L;
    long double c = 3.5L;
    unsigned hash = hash_ld_args(3, a, b, c);
    printf("%zu %u\n", sizeof(long double), hash);
    return 0;
}
