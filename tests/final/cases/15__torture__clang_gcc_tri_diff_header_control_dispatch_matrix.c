#include <stdio.h>
#include "15__torture__clang_gcc_tri_diff_header_control_dispatch_matrix.h"

int main(void) {
    static const CtrlStep steps[] = {
        {0u, 3, 1},
        {1u, 9, 4},
        {2u, 2, 5},
        {3u, 11, 6},
        {0u, 8, 7},
        {2u, 5, 3},
    };
    unsigned acc = 0x51f15e5u;
    int sum = 0;
    unsigned i;

    for (i = 0u; i < (unsigned)(sizeof(steps) / sizeof(steps[0])); ++i) {
        int v = apply_ctrl_step(steps[i], i);
        sum += v;
        acc = fold_u(acc, (unsigned)(v + (int)(i * 13u)));
    }

    printf("%u %d\n", acc, (int)(acc ^ (unsigned)(sum * 17)));
    return 0;
}
