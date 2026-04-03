typedef __builtin_va_list va_list;
#define va_arg(ap, type) __builtin_va_arg(ap, type)

long long w18_fold_tail(const char *fmt, int start, va_list ap) {
    long long acc = 0;
    int i;
    for (i = start; fmt[i]; ++i) {
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
    return acc;
}
