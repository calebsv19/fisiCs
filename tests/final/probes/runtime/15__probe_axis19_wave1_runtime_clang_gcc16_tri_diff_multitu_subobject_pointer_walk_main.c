#include <stdio.h>

#include "15__probe_axis19_wave1_runtime_clang_gcc16_tri_diff_multitu_subobject_pointer_walk_shared.h"

int main(void) {
    struct axis19_grid grid = {{{2u, 3u, 5u, 7u}, {11u, 13u, 17u, 19u},
                                {23u, 29u, 31u, 37u}}};
    unsigned first = axis19_walk(&grid, 7u);

    grid.rows[1][2] = 41u;
    printf("axis19-subobject=%u,%u\n", first, axis19_walk(&grid, 7u));
    return 0;
}
