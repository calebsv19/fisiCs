typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

long long abi_sum_tagged(const char* tag, int count, ...) {
    va_list ap;
    va_start(ap, count);

    long long sum = 0;
    for (int i = 0; i < count; ++i) {
        if ((i & 1) == 0) {
            sum += (long long)va_arg(ap, int);
        } else {
            sum += va_arg(ap, long long);
        }
    }

    va_end(ap);

    if (tag && tag[0] == 'o' && tag[1] == 'k' && tag[2] == '\0') {
        sum += 7;
    }
    return sum;
}
