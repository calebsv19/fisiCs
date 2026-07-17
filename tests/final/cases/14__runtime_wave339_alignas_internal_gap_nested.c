#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

struct Wave339Inner {
    char head;
    alignas(16) int lane;
    char tail;
};

struct Wave339Outer {
    char prefix;
    struct Wave339Inner item;
    int end;
};

static struct Wave339Outer values[2] = {
    {1, {2, 3, 4}, 5},
    {6, {7, 8, 9}, 10},
};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %zu %zu %zu %zu %zu %zu %zu %d %d %d\n",
           alignof(struct Wave339Inner),
           sizeof(struct Wave339Inner),
           offsetof(struct Wave339Inner, lane),
           offsetof(struct Wave339Inner, tail),
           alignof(struct Wave339Outer),
           sizeof(struct Wave339Outer),
           offsetof(struct Wave339Outer, item),
           offsetof(struct Wave339Outer, end),
           stride,
           values[0].item.lane,
           values[1].item.lane,
           values[1].end);

    return alignof(struct Wave339Inner) == 16 &&
                   sizeof(struct Wave339Inner) == 32 &&
                   offsetof(struct Wave339Inner, lane) == 16 &&
                   offsetof(struct Wave339Inner, tail) == 20 &&
                   alignof(struct Wave339Outer) == 16 &&
                   sizeof(struct Wave339Outer) == 64 &&
                   offsetof(struct Wave339Outer, item) == 16 &&
                   offsetof(struct Wave339Outer, end) == 48 &&
                   stride == 64 && values[0].prefix == 1 &&
                   values[0].item.lane == 3 && values[0].end == 5 &&
                   values[1].item.lane == 8 && values[1].end == 10
               ? 0
               : 1;
}
