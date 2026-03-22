#include <stddef.h>
#include <stdio.h>

int main(void) {
    long long arr[12];
    for (int i = 0; i < 12; ++i) {
        arr[i] = (long long)i * 1000LL + 7LL;
    }

    long long *p = &arr[2];
    long long *q = &arr[10];
    ptrdiff_t typed = q - p;

    char *cb = (char*)p;
    char *cq = (char*)q;
    ptrdiff_t bytes = cq - cb;
    ptrdiff_t roundtrip_ll = bytes / (ptrdiff_t)sizeof(long long);
    ptrdiff_t roundtrip_i = bytes / (ptrdiff_t)sizeof(int);

    printf("%lld %lld %lld %lld %lld\n",
           (long long)typed,
           (long long)roundtrip_ll,
           (long long)bytes,
           (long long)roundtrip_i,
           arr[5]);
    return 0;
}

