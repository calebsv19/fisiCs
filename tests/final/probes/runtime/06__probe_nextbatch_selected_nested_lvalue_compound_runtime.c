#include <stdio.h>

typedef struct {
    int value[3];
    int weight;
} Leaf;

typedef struct {
    Leaf leaf[2];
    int checksum;
} Node;

static int score_node(Node *node, int pick_node, int pick_leaf, int pick_slot) {
    int *selected = &node[pick_node].leaf[pick_leaf].value[pick_slot];
    int before = *selected;
    *(pick_slot ? selected : &node[pick_node].leaf[pick_leaf].weight) +=
        before + node[pick_node].checksum;
    return node[pick_node].leaf[pick_leaf].value[pick_slot] +
           node[pick_node].leaf[pick_leaf].weight +
           node[pick_node].checksum;
}

int main(void) {
    Node nodes[2] = {
        {{{{2, 3, 5}, 7}, {{11, 13, 17}, 19}}, 23},
        {{{{29, 31, 37}, 41}, {{43, 47, 53}, 59}}, 61},
    };

    int flag = nodes[0].leaf[1].value[2] > nodes[1].leaf[0].value[0];
    int *selected = flag ? &nodes[0].leaf[1].value[2] : &nodes[1].leaf[0].value[1];
    *selected += 5;

    int *slot = flag ? &nodes[0].leaf[0].value[1] : &nodes[1].leaf[1].value[0];
    *slot += score_node(nodes, flag ? 0 : 1, flag ? 0 : 1, flag ? 1 : 0);

    int total = nodes[0].leaf[0].value[1] + nodes[0].leaf[1].value[2] +
                nodes[1].leaf[0].value[1] + nodes[1].leaf[1].value[0];
    printf("%d %d %d %d\n",
           nodes[0].leaf[0].value[1],
           nodes[1].leaf[0].value[1],
           nodes[1].leaf[1].value[0],
           total);
    return 0;
}
