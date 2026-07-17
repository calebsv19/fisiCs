#include "aligned_union_abi.h"

int main(void) {
    union MixedAlignedUnion value = make_aligned_union(35);
    return alignof(union MixedAlignedUnion) == 32 &&
                   sizeof(union MixedAlignedUnion) == 32 &&
                   value.value == 37 && consume_aligned_union(value) == 42
               ? 0
               : 1;
}
