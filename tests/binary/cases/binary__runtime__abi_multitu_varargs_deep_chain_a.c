typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)

long long w18_deep_mid(int rounds, va_list ap);

long long w18_deep_entry(int rounds, ...) {
    va_list ap;
    long long acc;
    va_start(ap, rounds);
    acc = w18_deep_mid(rounds, ap);
    va_end(ap);
    return acc;
}
