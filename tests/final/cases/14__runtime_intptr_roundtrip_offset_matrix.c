#include <stdint.h>
#include <stdio.h>

static long long fold_offsets(int n, int a[n]) {
    static const int offs[] = {0, 3, 7, 11};
    intptr_t base = (intptr_t)(void*)&a[0];
    long long acc = 0;

    for (int i = 0; i < 4; ++i) {
        intptr_t pi = base + (intptr_t)offs[i] * (intptr_t)sizeof(int);
        int* p = (int*)(void*)pi;
        intptr_t back = (intptr_t)(void*)p - (intptr_t)offs[i] * (intptr_t)sizeof(int);
        acc += (long long)(*p) * (long long)(i + 1);
        acc += (long long)((pi - base) / (intptr_t)sizeof(int));
        acc += (back == base) ? 5 : -1000;
    }
    return acc;
}

int main(void) {
    int n = 16;
    int a[n];
    for (int i = 0; i < n; ++i) {
        a[i] = i * 13 + 9;
    }

    long long out = fold_offsets(n, a);
    intptr_t raw = (intptr_t)(void*)&a[11] - (intptr_t)(void*)&a[0];
    printf("%lld %lld %lld\n", out, (long long)(&a[11] - &a[0]), (long long)(raw / (intptr_t)sizeof(int)));
    return 0;
}
