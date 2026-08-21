#include <stdio.h>

#include "15__probe_axis16_wave1_runtime_clang_gcc16_tri_diff_multitu_volatile_access_order_shared.h"

int main(void) {
    volatile unsigned slots[3] = {0u, 0u, 0u};
    unsigned digest;

    axis16_store(&slots[0], 17u);
    axis16_store(&slots[1], axis16_load(&slots[0]) * 3u + 1u);
    axis16_store(&slots[2], axis16_load(&slots[1]) ^ 0x55u);
    axis16_store(&slots[0], axis16_load(&slots[2]) + axis16_load(&slots[0]));
    digest = axis16_load(&slots[0]) * 97u + axis16_load(&slots[1]) * 7u +
             axis16_load(&slots[2]);
    printf("axis16-volatile=%u,%u\n", digest, axis16_load(&slots[0]));
    return 0;
}
