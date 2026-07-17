#include <stdio.h>
#include <stdlib.h>

typedef union SortPayload {
    unsigned char bytes[6];
    unsigned short half[3];
} SortPayload;

typedef struct SortNode {
    int rank;
    SortPayload payload;
    unsigned short guard;
} SortNode;

static SortNode make_node(int rank, unsigned seed) {
    SortNode node;
    unsigned i;

    node.rank = rank;
    for (i = 0u; i < 6u; ++i) {
        node.payload.bytes[i] = (unsigned char)(0x23u + seed * 13u + i * 7u);
    }
    node.guard = (unsigned short)(0x4300u + seed * 19u);
    return node;
}

static int compare_nodes(const void *lhs, const void *rhs) {
    const SortNode *a = (const SortNode *)lhs;
    const SortNode *b = (const SortNode *)rhs;
    return (a->rank > b->rank) - (a->rank < b->rank);
}

static unsigned fold_nodes(const SortNode *nodes, unsigned count) {
    unsigned acc = 17u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < count; ++i) {
        acc = acc * 109u + (unsigned)(nodes[i].rank + 97);
        acc = acc * 109u + (unsigned)nodes[i].guard;
        for (k = 0u; k < 6u; ++k) {
            acc = acc * 109u + (unsigned)nodes[i].payload.bytes[k] + k;
        }
    }
    return acc;
}

int main(void) {
    SortNode nodes[5];
    unsigned typed = 0u;

    nodes[0] = make_node(42, 1u);
    nodes[1] = make_node(17, 4u);
    nodes[2] = make_node(33, 7u);
    nodes[3] = make_node(9, 3u);
    nodes[4] = make_node(28, 6u);

    qsort(nodes, 5u, sizeof(nodes[0]), compare_nodes);

    nodes[2].payload.half[1] = (unsigned short)(nodes[2].payload.half[1] + nodes[0].payload.bytes[4]);
    nodes[4].payload.bytes[5] = (unsigned char)(nodes[4].payload.bytes[5] ^ nodes[1].payload.bytes[0]);
    nodes[3].guard = (unsigned short)(nodes[3].guard + nodes[2].payload.bytes[3]);

    typed += (unsigned)(nodes[0].rank * 3);
    typed += (unsigned)nodes[2].payload.half[1] * 5u;
    typed += (unsigned)nodes[4].payload.bytes[5] * 7u;
    typed += (unsigned)nodes[3].guard;

    printf("%d %d %d %u %u %u %u\n",
           nodes[0].rank,
           nodes[2].rank,
           nodes[4].rank,
           (unsigned)nodes[2].payload.half[1],
           (unsigned)nodes[4].payload.bytes[5],
           typed,
           fold_nodes(nodes, 5u));
    return 0;
}
