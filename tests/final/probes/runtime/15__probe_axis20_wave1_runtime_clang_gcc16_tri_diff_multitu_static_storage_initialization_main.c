#include <stdio.h>

#include "15__probe_axis20_wave1_runtime_clang_gcc16_tri_diff_multitu_static_storage_initialization_shared.h"

int main(void) {
    unsigned initial_zero = axis20_zero;
    unsigned digest = axis20_step(5u);

    digest = digest * 97u + axis20_step(11u);
    digest = digest * 97u + axis20_step(23u);
    printf("axis20-storage=%u,%u,%u\n", initial_zero, digest, axis20_explicit[2]);
    return 0;
}
