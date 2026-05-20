#include <stdio.h>

#include "11__include_small_struct_return_multitu_runtime.h"

int main(void) {
    ProbePairInc pair = probe_make_pair_inc(4);
    if (pair.left != 8) return 1;
    if (pair.right != 9) return 2;
    printf("%d %d\n", pair.left, pair.right);
    return 0;
}
