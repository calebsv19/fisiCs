#include <stddef.h>
#include <stdio.h>

int main(void) {
    int values[20];
    for (int i = 0; i < 20; ++i) {
        values[i] = i * 9 + 1;
    }

    int *base = &values[3];
    int *tail = &values[17];
    ptrdiff_t typed_delta = tail - base;

    char *base_bytes = (char*)base;
    char *tail_bytes = (char*)tail;
    ptrdiff_t byte_delta = tail_bytes - base_bytes;
    ptrdiff_t roundtrip_delta = byte_delta / (ptrdiff_t)sizeof(int);

    int *pivot = (int*)(base_bytes + (ptrdiff_t)(sizeof(int) * 5));
    ptrdiff_t pivot_delta = pivot - base;

    printf("%lld %lld %lld %d\n",
           (long long)typed_delta,
           (long long)roundtrip_delta,
           (long long)pivot_delta,
           *pivot);
    return 0;
}
