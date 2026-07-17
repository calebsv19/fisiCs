#include <stdalign.h>

union MixedAlignedUnion {
    alignas(32) int value;
    unsigned char bytes[32];
    long long scalar;
};

union MixedAlignedUnion make_aligned_union(int seed);
int consume_aligned_union(union MixedAlignedUnion value);
