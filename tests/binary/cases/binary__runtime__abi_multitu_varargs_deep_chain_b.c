typedef __builtin_va_list va_list;
#define va_arg(ap, type) __builtin_va_arg(ap, type)

long long w18_deep_leaf(long double ld, int i, unsigned u);

long long w18_deep_mid(int rounds, va_list ap) {
    long long acc = 0;
    int r;
    for (r = 0; r < rounds; ++r) {
        long double ld = va_arg(ap, long double);
        int i = va_arg(ap, int);
        unsigned u = va_arg(ap, unsigned);
        acc += w18_deep_leaf(ld, i, u);
    }
    return acc;
}
