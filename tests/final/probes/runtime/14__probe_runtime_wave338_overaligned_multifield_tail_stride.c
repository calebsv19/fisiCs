#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

struct Wave338Box {
    alignas(16) unsigned char bytes[16];
    int tail;
};

int main(void) {
    struct Wave338Box values[2] = {{{1}, 7}, {{2}, 9}};
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%llu %llu %llu %llu %d %d\n",
           (unsigned long long)alignof(struct Wave338Box),
           (unsigned long long)sizeof(struct Wave338Box),
           (unsigned long long)stride,
           (unsigned long long)offsetof(struct Wave338Box, tail),
           values[0].tail,
           values[1].tail);
    return alignof(struct Wave338Box) == 16 &&
                   sizeof(struct Wave338Box) == 32 &&
                   stride == 32 &&
                   offsetof(struct Wave338Box, tail) == 16 &&
                   values[0].bytes[0] == 1 &&
                   values[1].bytes[0] == 2 &&
                   values[0].tail == 7 &&
                   values[1].tail == 9
               ? 0
               : 1;
}
