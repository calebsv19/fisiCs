#include "aligned_union_abi.h"

union MixedAlignedUnion make_aligned_union(int seed) {
    union MixedAlignedUnion value = {.value = seed + 2};
    return value;
}

int consume_aligned_union(union MixedAlignedUnion value) {
    return value.value + 5;
}
