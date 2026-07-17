#include <stdio.h>

typedef struct {
    int cells[3];
    volatile int tick;
} Leaf;

typedef struct {
    Leaf leaves[2];
    int bias;
} Node;

int main(void) {
    Node nodes[2] = {
        {{{{1, 3, 5}, 7}, {{9, 11, 13}, 15}}, 17},
        {{{{19, 21, 23}, 25}, {{27, 29, 31}, 33}}, 35},
    };

    int pick = nodes[1].leaves[0].cells[0] > nodes[0].leaves[1].cells[2];
    Leaf *selected_leaf = pick ? &nodes[1].leaves[0] : &nodes[0].leaves[1];
    int *slot = &(*selected_leaf).cells[pick ? 2 : 0];
    *slot += nodes[pick ? 0 : 1].bias;

    int *via_addr = &(*(pick ? &nodes[0].leaves[1] : &nodes[1].leaves[1])).cells[1];
    ++*via_addr;
    selected_leaf->tick += *via_addr;

    int total = *slot + *via_addr + selected_leaf->tick + nodes[0].bias + nodes[0].leaves[1].cells[2];
    printf("%d %d %d %d\n", *slot, *via_addr, selected_leaf->tick, total);
    return 0;
}
