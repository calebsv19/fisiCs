typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

long long w18_fold_tail(const char *fmt, int start, va_list ap);

long long w18_entry(const char *fmt, ...) {
    va_list ap;
    long long acc = 0;
    int i = 0;

    va_start(ap, fmt);
    for (i = 0; fmt[i] && i < 2; ++i) {
        if (fmt[i] == 'i') {
            acc += (long long)va_arg(ap, int) * 100ll;
        } else if (fmt[i] == 'u') {
            acc += (long long)va_arg(ap, unsigned) * 10ll;
        } else if (fmt[i] == 'L') {
            long double v = va_arg(ap, long double);
            acc += (long long)(v * 16.0L);
        } else if (fmt[i] == 'd') {
            double v = va_arg(ap, double);
            acc += (long long)(v * 8.0);
        }
    }
    acc += w18_fold_tail(fmt, i, ap);
    va_end(ap);
    return acc;
}
