#include <stddef.h>
#include <stdio.h>

static long long fold_ptrdiff(int n, int a[n]) {
    int* begin = &a[0];
    int* end = &a[n];
    int* last = &a[n - 1];
    long long acc = 0;

    acc += (long long)(end - begin) * 10;
    acc += (long long)(begin - end);
    acc += (long long)(end - last);

    for (int i = 0; i < n; i += 3) {
        int* p = &a[i];
        int* q = &a[n - 1 - i];
        acc += (long long)(q - p);
        acc += (long long)(p - begin);
    }

    return acc;
}

int main(void) {
    int n = 24;
    int a[n];
    for (int i = 0; i < n; ++i) {
        a[i] = i * 5 + 1;
    }

    long long out = fold_ptrdiff(n, a);
    ptrdiff_t pspan = &a[23] - &a[0];
    ptrdiff_t edge1 = &a[24] - &a[23];
    ptrdiff_t edge2 = &a[24] - &a[0];
    printf("%lld %lld %lld %lld\n", out, (long long)pspan, (long long)edge1, (long long)edge2);
    return 0;
}
