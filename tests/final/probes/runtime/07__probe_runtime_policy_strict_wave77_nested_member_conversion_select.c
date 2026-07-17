#include <stdio.h>

enum Rank {
    RANK_LOW = -4,
    RANK_MID = 7,
    RANK_HIGH = 132
};

struct Leaf {
    enum Rank rank;
    unsigned char raw;
    signed char delta;
};

struct Node {
    struct Leaf leaves[3];
    unsigned short bias;
};

struct Root {
    struct Node nodes[2];
    unsigned char selector;
};

static int read_leaf(struct Root *root, int node_index, enum Rank expected) {
    struct Node *node = &root->nodes[(unsigned int)(unsigned char)node_index & 1u];
    int leaf_index = ((int)(unsigned char)(node->leaves[0].raw + root->selector) + (int)expected) % 3;
    struct Leaf *leaf = &node->leaves[leaf_index < 0 ? leaf_index + 3 : leaf_index];
    unsigned char wrapped = (unsigned char)(leaf->raw + (unsigned char)node->bias);
    int promoted = (int)leaf->rank + (int)leaf->delta;
    return (int)wrapped + (int)(unsigned char)promoted + (leaf->rank == expected ? 19 : -5);
}

int main(void) {
    struct Root root = {
        {
            {{{RANK_LOW, 250u, -6}, {RANK_MID, 17u, 9}, {RANK_HIGH, 201u, -11}}, 14u},
            {{{RANK_HIGH, 93u, 12}, {RANK_LOW, 44u, -3}, {RANK_MID, 211u, 5}}, 29u}
        },
        5u
    };

    int first = read_leaf(&root, 0, RANK_LOW);
    int second = read_leaf(&root, 1, RANK_HIGH);
    int third = read_leaf(&root, 2, RANK_MID);
    printf("%d %d %d %d\n", first, second, third, first + second + third);
    return 0;
}
