#include "11__include_small_struct_return_multitu_runtime.h"

ProbePairInc probe_make_pair_inc(int seed) {
    ProbePairInc pair;
    pair.left = seed * 2;
    pair.right = pair.left + 1;
    return pair;
}
