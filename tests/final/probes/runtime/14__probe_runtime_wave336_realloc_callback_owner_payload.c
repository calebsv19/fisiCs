#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef union OwnerBytes {
    unsigned char bytes[6];
    unsigned short half[3];
} OwnerBytes;

typedef struct OwnerNode {
    OwnerBytes payload;
    unsigned short guard;
} OwnerNode;

typedef unsigned (*OwnerStep)(OwnerNode *, unsigned);

static unsigned fold_owner(OwnerNode *nodes, unsigned count) {
    unsigned acc = 31u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < count; ++i) {
        acc = acc * 131u + (unsigned)nodes[i].guard;
        for (k = 0u; k < 6u; ++k) {
            acc = acc * 131u + (unsigned)nodes[i].payload.bytes[k];
        }
    }
    return acc;
}

static unsigned mutate_owner(OwnerNode *nodes, unsigned count) {
    unsigned i;
    for (i = 0u; i < count; ++i) {
        nodes[i].payload.bytes[(i + 2u) % 6u] =
            (unsigned char)(nodes[i].payload.bytes[(i + 2u) % 6u] + 3u + i);
        nodes[i].guard = (unsigned short)(nodes[i].guard + nodes[i].payload.bytes[i]);
    }
    return fold_owner(nodes, count);
}

int main(void) {
    OwnerNode *nodes = (OwnerNode *)malloc(2u * sizeof(*nodes));
    OwnerNode saved;
    OwnerStep step = mutate_owner;
    unsigned i;
    unsigned before;
    unsigned after;

    if (!nodes) {
        return 1;
    }
    for (i = 0u; i < 2u; ++i) {
        unsigned k;
        nodes[i].guard = (unsigned short)(0x2100u + i * 0x31u);
        for (k = 0u; k < 6u; ++k) {
            nodes[i].payload.bytes[k] = (unsigned char)(0x18u + i * 17u + k * 5u);
        }
    }
    saved = nodes[1];
    before = fold_owner(nodes, 2u);
    nodes = (OwnerNode *)realloc(nodes, 4u * sizeof(*nodes));
    if (!nodes) {
        return 2;
    }
    memcpy(&nodes[2], &saved, sizeof(saved));
    nodes[3] = nodes[0];
    nodes[3].payload.half[1] = (unsigned short)(nodes[3].payload.half[1] + nodes[2].payload.bytes[4]);
    after = step(nodes, 4u);
    printf("realloc-owner %u %u %u %u %u\n", before, after,
           (unsigned)nodes[2].guard, (unsigned)nodes[3].payload.bytes[2],
           (unsigned)nodes[3].payload.half[1]);
    free(nodes);
    return 0;
}
