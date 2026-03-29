#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Wave83Item {
    long double value;
    int weight;
};

static long long quant16(long double x) {
    if (x >= 0.0L) {
        return (long long)(x * 16.0L + 0.5L);
    }
    return (long long)(x * 16.0L - 0.5L);
}

static struct Wave83Item fold_items(int count, int base_weight, ...) {
    va_list ap;
    struct Wave83Item out;
    out.value = 0.0L;
    out.weight = base_weight;

    va_start(ap, base_weight);
    for (int i = 0; i < count; ++i) {
        long double v = va_arg(ap, long double);
        out.value += v;
        out.weight += 1;
    }
    va_end(ap);
    return out;
}

int main(void) {
    struct Wave83Item out = fold_items(4, 10, 1.25L, -2.5L, 0.75L, 3.0L);
    long long q = quant16(out.value);
    printf("%lld %d\n", q, out.weight);
    return 0;
}
