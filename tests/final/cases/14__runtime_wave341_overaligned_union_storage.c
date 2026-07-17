#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

union Wave341Union {
    alignas(32) int value;
    unsigned char bytes[32];
    long long scalar;
};

struct Wave341Outer {
    char prefix;
    union Wave341Union payload;
    int suffix;
};

static union Wave341Union static_values[2] = {
    {.value = 7},
    {.bytes = {3}},
};

int main(void) {
    union Wave341Union automatic = {.value = 11};
    struct Wave341Outer nested = {1, {.value = 13}, 17};
    size_t static_stride =
        (size_t)((uintptr_t)&static_values[1] - (uintptr_t)&static_values[0]);

    printf("%zu %zu %zu %zu %zu %zu %zu %zu %d %u %d %d\n",
           alignof(union Wave341Union),
           sizeof(union Wave341Union),
           alignof(struct Wave341Outer),
           sizeof(struct Wave341Outer),
           offsetof(struct Wave341Outer, payload),
           offsetof(struct Wave341Outer, suffix),
           static_stride,
           (size_t)((uintptr_t)&automatic % 32u),
           static_values[0].value,
           (unsigned)static_values[1].bytes[0],
           automatic.value,
           nested.payload.value + nested.suffix);

    return alignof(union Wave341Union) == 32 &&
                   sizeof(union Wave341Union) == 32 &&
                   alignof(struct Wave341Outer) == 32 &&
                   sizeof(struct Wave341Outer) == 96 &&
                   offsetof(struct Wave341Outer, payload) == 32 &&
                   offsetof(struct Wave341Outer, suffix) == 64 &&
                   static_stride == 32 &&
                   ((uintptr_t)&automatic % 32u) == 0 &&
                   static_values[0].value == 7 &&
                   static_values[1].bytes[0] == 3 &&
                   automatic.value == 11 &&
                   nested.prefix == 1 && nested.payload.value == 13 &&
                   nested.suffix == 17
               ? 0
               : 1;
}
