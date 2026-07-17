#include <stdio.h>

typedef union WordBag {
    unsigned char byte[4];
    unsigned short half[2];
} WordBag;

typedef struct Leaf {
    unsigned short id;
    WordBag bag;
} Leaf;

typedef struct Tree {
    Leaf leaf[5];
    unsigned short stamp;
} Tree;

typedef Tree (*TreeMap)(Tree, unsigned);

static Tree seed_tree(unsigned seed) {
    Tree out;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 5u; ++i) {
        out.leaf[i].id = (unsigned short)(0x2200u + seed * 3u + i * 0x19u);
        for (k = 0u; k < 4u; ++k) {
            out.leaf[i].bag.byte[k] = (unsigned char)(0x2Bu + seed + i * 11u + k * 9u);
        }
    }
    out.stamp = (unsigned short)(0x6800u + seed * 23u);
    return out;
}

static Tree map_xor(Tree in, unsigned salt) {
    Tree out = in;

    out.leaf[1].bag.half[0] = (unsigned short)(out.leaf[1].bag.half[0] ^ (unsigned short)(salt * 13u));
    out.leaf[3].bag.byte[2] = (unsigned char)(out.leaf[3].bag.byte[2] + out.leaf[0].bag.byte[1]);
    out.stamp = (unsigned short)(out.stamp + out.leaf[1].bag.byte[0] + out.leaf[3].bag.byte[2]);
    return out;
}

static Tree map_rotate(Tree in, unsigned salt) {
    Tree out = in;
    Leaf keep = out.leaf[4];

    out.leaf[4] = out.leaf[2];
    out.leaf[2] = out.leaf[0];
    out.leaf[0] = keep;
    out.leaf[2].id = (unsigned short)(out.leaf[2].id + salt + out.leaf[4].bag.byte[3]);
    out.stamp = (unsigned short)(out.stamp ^ (unsigned short)(salt * 37u));
    return out;
}

static TreeMap choose_map(Tree value, unsigned round) {
    TreeMap maps[2];

    maps[0] = map_xor;
    maps[1] = map_rotate;
    return maps[(value.leaf[round % 5u].bag.byte[round & 3u] + value.stamp) & 1u];
}

static unsigned checksum(Tree value) {
    unsigned acc = (unsigned)value.stamp * 211u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 5u; ++i) {
        acc = acc * 113u + (unsigned)value.leaf[i].id;
        for (k = 0u; k < 4u; ++k) {
            acc = acc * 113u + (unsigned)value.leaf[i].bag.byte[k] + k;
        }
    }
    return acc;
}

int main(void) {
    Tree tree = seed_tree(7u);
    unsigned round;
    unsigned typed = 0u;

    for (round = 0u; round < 6u; ++round) {
        TreeMap map = choose_map(tree, round);
        tree = map(tree, 0x18u + round * 5u);
    }

    typed += (unsigned)tree.stamp;
    typed += (unsigned)tree.leaf[0].id * 3u + (unsigned)tree.leaf[1].bag.byte[0] * 5u;
    typed += (unsigned)tree.leaf[2].bag.half[1] * 7u + (unsigned)tree.leaf[4].bag.byte[3] * 11u;

    printf("%u %u %u %u %u %u %u %u\n",
           (unsigned)tree.leaf[0].id,
           (unsigned)tree.leaf[1].bag.byte[0],
           (unsigned)tree.leaf[2].bag.byte[3],
           (unsigned)tree.leaf[2].bag.half[1],
           (unsigned)tree.leaf[4].bag.byte[3],
           (unsigned)tree.stamp,
           typed,
           checksum(tree));
    return 0;
}
