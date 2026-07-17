#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

struct Wave338Inner {
    alignas(16) int value;
};

struct Wave338Outer {
    struct Wave338Inner items[2];
    unsigned char tail;
};

int main(void) {
    struct Wave338Outer value = {{{7}, {13}}, 5};
    size_t stride = (size_t)((uintptr_t)&value.items[1] -
                             (uintptr_t)&value.items[0]);
    printf("%llu %llu %llu %llu %d %d %u\n",
           (unsigned long long)alignof(struct Wave338Inner),
           (unsigned long long)sizeof(struct Wave338Inner),
           (unsigned long long)sizeof(struct Wave338Outer),
           (unsigned long long)offsetof(struct Wave338Outer, tail),
           value.items[0].value,
           value.items[1].value,
           (unsigned)value.tail);
    return alignof(struct Wave338Inner) == 16 &&
                   sizeof(struct Wave338Inner) == 16 &&
                   sizeof(struct Wave338Outer) == 48 &&
                   stride == 16 &&
                   offsetof(struct Wave338Outer, tail) == 32 &&
                   value.items[0].value == 7 &&
                   value.items[1].value == 13 &&
                   value.tail == 5
               ? 0
               : 1;
}
