#include <stdio.h>

struct Wave23AddressLeaf {
    int value;
    int delta[2];
};

struct Wave23AddressNode {
    struct Wave23AddressLeaf leaf[2];
    volatile int bias;
};

static int wave23_addressed_member_compound(void) {
    struct Wave23AddressNode nodes[2] = {
        {{{3, {5, 7}}, {11, {13, 17}}}, 19},
        {{{23, {29, 31}}, {37, {41, 43}}}, 47},
    };

    int pick = nodes[1].leaf[1].value > nodes[0].leaf[1].value;
    struct Wave23AddressNode *node = pick ? &nodes[1] : &nodes[0];
    int *delta = &node->leaf[pick ? 1 : 0].delta[0];
    int *value = &node->leaf[pick ? 1 : 0].value;

    *delta += node->bias;
    *value += *delta;
    node->bias += *value;

    return *delta + *value + node->bias;
}

int main(void) {
    printf("%d\n", wave23_addressed_member_compound());
    return 0;
}
