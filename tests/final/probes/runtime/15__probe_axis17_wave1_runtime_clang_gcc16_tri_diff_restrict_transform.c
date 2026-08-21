#include <stddef.h>
#include <stdio.h>

static void axis17_transform(unsigned *restrict output,
                             const unsigned *restrict left,
                             const unsigned *restrict right, size_t count) {
    size_t i;

    for (i = 0u; i < count; ++i) {
        output[i] = (left[i] * 29u) ^ (right[i] + (unsigned)i * 11u);
    }
}

int main(void) {
    const unsigned left[6] = {3u, 7u, 11u, 19u, 23u, 31u};
    const unsigned right[6] = {2u, 5u, 13u, 17u, 29u, 37u};
    unsigned output[6] = {0u, 0u, 0u, 0u, 0u, 0u};
    unsigned digest = 0u;
    size_t i;

    axis17_transform(output, left, right, 6u);
    for (i = 0u; i < 6u; ++i) {
        digest = digest * 131u + output[i];
    }
    printf("axis17-restrict=%u,%u\n", digest, output[4]);
    return 0;
}
