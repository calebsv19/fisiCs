#include "15__probe_axis19_wave1_runtime_clang_gcc16_tri_diff_multitu_subobject_pointer_walk_shared.h"

unsigned axis19_walk(const struct axis19_grid *grid, unsigned seed) {
    unsigned row;
    unsigned digest = seed;

    for (row = 0u; row < 3u; ++row) {
        const unsigned *cell = grid->rows[row];
        const unsigned *end = cell + 4u;
        while (cell != end) {
            digest = digest * 33u + *cell;
            ++cell;
        }
    }
    return digest;
}
