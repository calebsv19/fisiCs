#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

struct Wave340VLA {
    alignas(32) int value;
};

static int run(int count) {
    struct Wave340VLA values[count];
    values[0].value = 7;
    values[count - 1].value = 11;
    size_t base_mod = (size_t)((uintptr_t)&values[0] % 32u);
    size_t stride = (size_t)((uintptr_t)&values[1] - (uintptr_t)&values[0]);
    printf("%zu %zu %zu %zu %d %d\n",
           alignof(struct Wave340VLA),
           sizeof(struct Wave340VLA),
           base_mod,
           stride,
           values[0].value,
           values[count - 1].value);
    return alignof(struct Wave340VLA) == 32 &&
                   sizeof(struct Wave340VLA) == 32 &&
                   base_mod == 0 && stride == 32 &&
                   values[0].value == 7 && values[count - 1].value == 11;
}

int main(void) {
    return run(3) ? 0 : 1;
}
