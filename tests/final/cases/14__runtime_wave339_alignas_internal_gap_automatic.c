#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

struct Wave339Auto {
    char head;
    alignas(16) int lane;
    char tail;
};

int main(void) {
    struct Wave339Auto positional = {3, 17, 5};
    struct Wave339Auto designated = {.lane = 19, .tail = 7};
    size_t lane_delta = (size_t)((uintptr_t)&positional.lane - (uintptr_t)&positional);
    size_t tail_delta = (size_t)((uintptr_t)&positional.tail - (uintptr_t)&positional);

    printf("%zu %zu %zu %zu %zu %zu %d %d %d %d %d\n",
           alignof(struct Wave339Auto),
           sizeof(struct Wave339Auto),
           offsetof(struct Wave339Auto, lane),
           offsetof(struct Wave339Auto, tail),
           lane_delta,
           tail_delta,
           positional.head,
           positional.lane,
           positional.tail,
           designated.lane,
           designated.tail);

    return alignof(struct Wave339Auto) == 16 &&
                   sizeof(struct Wave339Auto) == 32 &&
                   offsetof(struct Wave339Auto, lane) == 16 &&
                   offsetof(struct Wave339Auto, tail) == 20 &&
                   lane_delta == 16 && tail_delta == 20 &&
                   positional.head == 3 && positional.lane == 17 &&
                   positional.tail == 5 && designated.lane == 19 &&
                   designated.tail == 7
               ? 0
               : 1;
}
