typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct W16Tuple {
    long double total;
    int count;
};

struct W16Tuple w16_collect(long double seed, int count, ...) {
    va_list ap;
    struct W16Tuple out;
    int i;
    out.total = seed;
    out.count = count;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        long double v = va_arg(ap, long double);
        out.total += v * (long double)(i + 1);
    }
    va_end(ap);
    return out;
}

long double w16_mix(struct W16Tuple t, long double scale) {
    return (t.total * scale) + (long double)t.count * 0.125L;
}
