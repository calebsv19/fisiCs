typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

long long w16_mix_variadic(const char *fmt, ...) {
    va_list ap;
    const char *p = fmt;
    long long acc = 0;
    va_start(ap, fmt);
    while (*p) {
        if (*p == 'i') {
            acc += (long long)va_arg(ap, int) * 100ll;
        } else if (*p == 'u') {
            acc += (long long)va_arg(ap, unsigned) * 10ll;
        } else if (*p == 'l') {
            acc += va_arg(ap, long long);
        } else if (*p == 'U') {
            unsigned long long uv = va_arg(ap, unsigned long long);
            acc += (long long)(uv % 1000ull);
        } else if (*p == 'd') {
            double dv = va_arg(ap, double);
            acc += (long long)(dv * 8.0);
        }
        ++p;
    }
    va_end(ap);
    return acc;
}
