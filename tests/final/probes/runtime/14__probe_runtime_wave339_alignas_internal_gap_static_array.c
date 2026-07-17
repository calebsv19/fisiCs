#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

struct Wave339Static {
    char head;
    alignas(16) int lane;
    char tail;
};

static struct Wave339Static values[2] = {{3, 17, 5}, {.lane = 19, .tail = 7}};

int main(void) {
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %zu %zu %zu %d %d %d %d %d\n",
           alignof(struct Wave339Static), sizeof(struct Wave339Static),
           offsetof(struct Wave339Static, lane), offsetof(struct Wave339Static, tail),
           stride, values[0].head, values[0].lane, values[0].tail,
           values[1].lane, values[1].tail);
    return alignof(struct Wave339Static) == 16 && sizeof(struct Wave339Static) == 32 &&
                   offsetof(struct Wave339Static, lane) == 16 &&
                   offsetof(struct Wave339Static, tail) == 20 && stride == 32 &&
                   values[0].head == 3 && values[0].lane == 17 && values[0].tail == 5 &&
                   values[1].lane == 19 && values[1].tail == 7 ? 0 : 1;
}
