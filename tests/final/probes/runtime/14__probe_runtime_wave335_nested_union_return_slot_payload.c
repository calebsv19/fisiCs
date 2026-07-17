#include <stdio.h>

typedef union LeafPayload {
    unsigned char bytes[8];
    unsigned short half[4];
} LeafPayload;

typedef struct Leaf {
    unsigned short key;
    LeafPayload payload;
} Leaf;

typedef union BranchPayload {
    Leaf leaves[2];
    unsigned char raw[sizeof(Leaf) * 2u];
} BranchPayload;

typedef struct Branch {
    BranchPayload payload;
    unsigned short guard;
} Branch;

static Leaf seed_leaf(unsigned seed) {
    Leaf leaf;
    unsigned i;

    leaf.key = (unsigned short)(0x2100u + seed * 29u);
    for (i = 0u; i < 8u; ++i) {
        leaf.payload.bytes[i] = (unsigned char)(0x19u + seed * 7u + i * 11u);
    }
    return leaf;
}

static Branch make_branch(unsigned seed) {
    Branch out;

    out.payload.leaves[0] = seed_leaf(seed + 1u);
    out.payload.leaves[1] = seed_leaf(seed + 5u);
    out.guard = (unsigned short)(0x6c00u + seed * 13u);
    return out;
}

static Branch rewrite_return_slot(Branch in, unsigned salt) {
    Branch out = in;
    Leaf hold = out.payload.leaves[0];

    out.payload.leaves[0] = out.payload.leaves[1];
    out.payload.leaves[1] = hold;
    out.payload.leaves[0].payload.half[2] =
        (unsigned short)(out.payload.leaves[0].payload.half[2] + salt);
    out.payload.leaves[1].payload.bytes[5] =
        (unsigned char)(out.payload.leaves[1].payload.bytes[5] ^ (unsigned char)(salt >> 1));
    out.guard = (unsigned short)(out.guard + out.payload.raw[3] + out.payload.raw[sizeof(out.payload.raw) - 2u]);
    return out;
}

static unsigned fold_branch(Branch value) {
    unsigned acc = (unsigned)value.guard * 193u;
    unsigned i;

    for (i = 0u; i < sizeof(value.payload.raw); ++i) {
        acc = acc * 131u + (unsigned)value.payload.raw[i] + i;
    }
    return acc;
}

int main(void) {
    Branch branch = make_branch(9u);
    Branch copy = rewrite_return_slot(rewrite_return_slot(branch, 0x2du), 0x41u);
    unsigned typed = 0u;

    typed += (unsigned)copy.payload.leaves[0].key * 3u;
    typed += (unsigned)copy.payload.leaves[0].payload.bytes[6] * 5u;
    typed += (unsigned)copy.payload.leaves[1].payload.half[1] * 7u;
    typed += (unsigned)copy.guard;

    printf("%u %u %u %u %u %u\n",
           (unsigned)copy.payload.leaves[0].key,
           (unsigned)copy.payload.leaves[0].payload.bytes[6],
           (unsigned)copy.payload.leaves[1].payload.half[1],
           (unsigned)copy.guard,
           typed,
           fold_branch(copy));
    return 0;
}
