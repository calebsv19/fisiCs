typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

long long w17_mix_variadic(const char *fmt, ...) {
    va_list ap;
    const char *p = fmt;
    long long acc = 0;
    va_start(ap, fmt);
    while (*p) {
        if (*p == 'L') {
            long double v = va_arg(ap, long double);
            acc += (long long)(v * 32.0L);
        } else if (*p == 'i') {
            acc += (long long)va_arg(ap, int) * 10ll;
        } else if (*p == 'u') {
            acc += (long long)va_arg(ap, unsigned) * 7ll;
        }
        ++p;
    }
    va_end(ap);
    return acc;
}
