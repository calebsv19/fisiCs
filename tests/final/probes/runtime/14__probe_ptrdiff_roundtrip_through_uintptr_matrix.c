#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    int lane[28];
    int* left;
    int* right;
    ptrdiff_t typed;
    uintptr_t left_u;
    uintptr_t right_u;
    uintptr_t delta_u;
    int* back;
    ptrdiff_t roundtrip;
    unsigned digest = 41u;
    int i;

    for (i = 0; i < 28; ++i) {
        lane[i] = i * 13 + 5;
    }

    left = &lane[4];
    right = &lane[22];
    typed = right - left;

    left_u = (uintptr_t)left;
    right_u = (uintptr_t)right;
    delta_u = right_u - left_u;
    back = (int*)(left_u + delta_u);
    roundtrip = back - left;

    digest = digest * 173u + (unsigned)typed;
    digest = digest * 173u + (unsigned)roundtrip;
    digest = digest * 173u + (unsigned)((delta_u / sizeof(int)) & 0xffffffffu);
    digest = digest * 173u + (unsigned)lane[4];
    digest = digest * 173u + (unsigned)lane[22];

    printf("%lld %lld %u\n", (long long)typed, (long long)roundtrip, digest);
    return 0;
}
