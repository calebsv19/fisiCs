#include <stdio.h>

typedef struct {
    int v[2];
    int mark;
} Wave48Leaf;

typedef struct {
    Wave48Leaf left;
    Wave48Leaf right;
    int tag;
} Wave48Tree;

static Wave48Leaf make_leaf(int seed, int twist) {
    Wave48Leaf leaf;
    leaf.v[0] = seed * 2 + twist;
    leaf.v[1] = seed - twist * 3;
    leaf.mark = seed * seed + twist;
    return leaf;
}

static Wave48Tree make_tree(int seed, int twist) {
    Wave48Tree tree;
    tree.left = make_leaf(seed + 1, twist);
    tree.right = make_leaf(seed + 2, twist + 1);
    tree.tag = seed * 7 - twist;
    return tree;
}

static int checksum(Wave48Tree tree) {
    return tree.tag
        + tree.left.v[0] * 3
        - tree.left.v[1] * 5
        + tree.left.mark * 7
        - tree.right.v[0] * 11
        + tree.right.v[1] * 13
        - tree.right.mark * 17;
}

int main(void) {
    Wave48Tree tree = make_tree(2, 1);
    int total = 0;
    int i;

    for (i = 0; i < 9; ++i) {
        Wave48Tree a = make_tree(i + 3, tree.tag & 5);
        Wave48Tree b = (Wave48Tree){tree.right, a.left, tree.tag + i};
        Wave48Tree c = ((checksum(a) ^ checksum(tree)) & 1) ? a : b;
        if ((checksum(c) + total) % 3 == 0) {
            tree = (Wave48Tree){c.right, tree.left, c.tag - i};
            tree.left.v[i & 1] += tree.right.mark;
        } else {
            tree = (Wave48Tree){tree.right, c.left, tree.tag + c.tag};
            tree.right.v[(i + 1) & 1] -= tree.left.mark - i;
        }
        total += checksum(tree);
    }

    printf("%d %d %d %d\n", tree.tag, tree.left.mark, checksum(tree), total);
    return 0;
}
