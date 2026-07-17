#include <stdio.h>

typedef struct {
    int lane;
    int cells[3];
} Wave47Leaf;

typedef struct {
    Wave47Leaf left;
    Wave47Leaf right;
    int bias;
} Wave47Node;

static int checksum(Wave47Node node) {
    return node.bias
        + node.left.lane * 2
        - node.right.lane * 3
        + node.left.cells[0] * 5
        - node.left.cells[1] * 7
        + node.left.cells[2] * 11
        + node.right.cells[0] * 13
        - node.right.cells[1] * 17
        + node.right.cells[2] * 19;
}

int main(void) {
    Wave47Node node = {
        {1, {3, 5, 8}},
        {2, {13, 21, 34}},
        4
    };
    int total = 0;
    int i;

    for (i = 0; i < 10; ++i) {
        if ((checksum(node) + i) % 3 == 0) {
            node = (Wave47Node){
                {node.right.lane + i, {node.bias + i, node.left.cells[1] - i, node.right.cells[2] + 1}},
                {node.left.lane - i, {node.right.cells[0] + i * 2, node.left.cells[2] - 3, node.bias - i}},
                node.bias + i + 5
            };
        } else if ((node.bias ^ i) & 1) {
            Wave47Node copy = (Wave47Node){
                {i + 4, {node.left.cells[2], node.right.cells[1], checksum(node) & 31}},
                node.left,
                node.bias - i
            };
            node = copy;
            node.right.cells[i % 3] += node.left.lane;
        } else {
            node.left = (Wave47Leaf){node.left.lane + 2, {node.right.cells[2] - i, node.bias + i, node.left.cells[0] + node.right.lane}};
            node.bias += node.left.cells[(i + 1) % 3] - node.right.lane;
        }
        total += checksum(node);
    }

    printf("%d %d %d %d %d\n", node.left.lane, node.right.lane, node.bias, checksum(node), total);
    return 0;
}
