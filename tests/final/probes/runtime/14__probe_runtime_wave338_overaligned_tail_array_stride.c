#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

struct Wave338Tail {
    alignas(32) int value;
};

int main(void) {
    struct Wave338Tail values[2] = {{11}, {29}};
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%llu %llu %llu %d\n",
           (unsigned long long)alignof(struct Wave338Tail),
           (unsigned long long)sizeof(struct Wave338Tail),
           (unsigned long long)stride,
           values[0].value + values[1].value);
    return alignof(struct Wave338Tail) == 32 &&
                   sizeof(struct Wave338Tail) == 32 &&
                   stride == 32 &&
                   values[0].value == 11 &&
                   values[1].value == 29
               ? 0
               : 1;
}
