#include <stdio.h>

typedef union {
    struct {
        int x;
        int y;
    } pair;
    int raw[2];
} Wave46Payload;

typedef struct {
    int tag;
    Wave46Payload payload;
    int tail;
} Wave46Node;

static Wave46Node make_node(int tag, int seed) {
    Wave46Node node;
    node.tag = tag;
    if (tag == 1) {
        node.payload.pair.x = seed + 3;
        node.payload.pair.y = seed * 2 - 1;
    } else {
        node.payload.raw[0] = seed * 4 + 5;
        node.payload.raw[1] = seed - 6;
    }
    node.tail = seed * 7 + tag;
    return node;
}

static int node_score(Wave46Node node) {
    int total = node.tag * 19 + node.tail;
    if (node.tag == 1) {
        total += node.payload.pair.x * 2 - node.payload.pair.y * 3;
    } else {
        total += node.payload.raw[0] - node.payload.raw[1] * 5;
    }
    return total;
}

int main(void) {
    Wave46Node node = make_node(1, 5);
    int total = 0;
    int i;

    for (i = 0; i < 12; ++i) {
        Wave46Node candidate = (i & 1) ? make_node(2, i + node.tag) : make_node(1, node.tail % 11 + i);
        candidate.tail += node_score(node) % 17;
        candidate.payload.raw[i & 1] += node.payload.raw[(i + 1) & 1] - i;

        if ((node_score(candidate) > node_score(node)) || ((i % 5) == 4)) {
            node = candidate;
        } else {
            node.payload.raw[(i + 1) & 1] -= i + node.tag;
            node.tail += i * 2;
        }

        total += node_score(node);
    }

    printf("%d %d %d %d %d\n", node.tag, node.payload.raw[0], node.payload.raw[1], node_score(node), total);
    return 0;
}
