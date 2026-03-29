#include <stddef.h>
#include <stdio.h>

int main(void) {
    static int a[4096];
    for (int i = 0; i < 4096; ++i) {
        a[i] = (i % 97) - 33;
    }

    int* p0 = &a[0];
    int* p1 = &a[1024];
    int* p2 = &a[3072];
    int* p3 = &a[4095];
    int* pe = &a[4096];

    ptrdiff_t d01 = p1 - p0;
    ptrdiff_t d12 = p2 - p1;
    ptrdiff_t d23 = p3 - p2;
    ptrdiff_t d0e = pe - p0;

    long long reduce = 0;
    for (int i = 0; i < 4096; i += 257) {
        reduce += (long long)a[i] * (long long)(i + 3);
    }

    printf("%td %td %td %td %lld\n", d01, d12, d23, d0e, reduce);
    return 0;
}
