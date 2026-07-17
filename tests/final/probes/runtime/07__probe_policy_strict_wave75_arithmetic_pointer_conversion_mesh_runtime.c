#include <stddef.h>
#include <stdio.h>

enum Kind {
    KIND_LOW = -2,
    KIND_HIGH = 5
};

struct Node {
    enum Kind kind;
    unsigned char lane;
    signed char delta;
};

static int node_score(struct Node *nodes, int index, unsigned int salt) {
    struct Node *node = nodes + index;
    unsigned int promoted = (unsigned int)(unsigned char)(node->lane + (unsigned char)salt);
    int signed_part = (int)(signed char)(node->delta + (signed char)node->kind);
    ptrdiff_t distance = node - nodes;
    return (int)(promoted ^ (unsigned int)(unsigned char)signed_part) + (int)distance;
}

int main(void) {
    struct Node nodes[4] = {
        {KIND_LOW, 250u, -8},
        {KIND_HIGH, 12u, 9},
        {KIND_LOW, 177u, 4},
        {KIND_HIGH, 33u, -11}
    };

    struct Node *base = nodes;
    struct Node *selected = 1 ? base + 2 : base;
    void *opaque = (void *)selected;
    struct Node *roundtrip = (struct Node *)opaque;
    int first = node_score(base, 0, 19u);
    int second = node_score(roundtrip - 1, 1, 23u);
    int third = node_score(base, (int)(roundtrip - base), 31u);
    printf("%d %d %d %ld\n", first, second, third, (long)(roundtrip - base));
    return 0;
}
