typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Wave83Tuple {
    long double total;
    int count;
};

struct Wave83Tuple wave83_collect(long double seed, int count, ...) {
    va_list ap;
    struct Wave83Tuple out;
    out.total = seed;
    out.count = count;

    va_start(ap, count);
    for (int i = 0; i < count; ++i) {
        long double v = va_arg(ap, long double);
        out.total += v * (long double)(i + 1);
    }
    va_end(ap);
    return out;
}

long double wave83_mix(struct Wave83Tuple t, long double scale) {
    return (t.total * scale) + (long double)t.count * 0.125L;
}
