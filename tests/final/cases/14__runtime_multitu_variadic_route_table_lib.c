typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

static long long fold_once(int tag, int lanes, va_list ap) {
    long long acc = (long long)tag * 13LL + 9LL;
    for (int i = 0; i < lanes; ++i) {
        int kind = va_arg(ap, int);
        long long v = 0;
        if (kind == 0) {
            v = (long long)va_arg(ap, int);
        } else if (kind == 1) {
            v = (long long)va_arg(ap, unsigned);
        } else if (kind == 2) {
            v = (long long)(va_arg(ap, double) * 100.0);
        } else {
            v = va_arg(ap, long long);
        }
        acc = acc * 31LL + (long long)(i + 1) * v;
    }
    return acc;
}

long long abi_variadic_route(int tag, int lanes, ...) {
    va_list ap;
    va_list cp;
    va_start(ap, lanes);
    va_copy(cp, ap);
    long long a = fold_once(tag, lanes, ap);
    long long b = fold_once(tag + 1, lanes, cp);
    va_end(cp);
    va_end(ap);
    return a + b;
}

long long abi_variadic_driver(int seed) {
    long long x = abi_variadic_route(1 + seed, 4, 0, seed + 3, 1, 7u + (unsigned)seed, 2, 0.75, 3, 1000LL + (long long)seed);
    long long y = abi_variadic_route(2 + seed, 3, 1, 99u, 0, -8, 2, -1.25);
    return x - y;
}
