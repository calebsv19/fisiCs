#include <stdio.h>

struct InnerPair {
    int a;
    int b;
};

struct OuterNode {
    struct InnerPair inner;
    long long tail;
};

struct OuterNode abi_outer_step(struct OuterNode node, int mul);
long long abi_outer_score(struct OuterNode node);

int main(void) {
    struct OuterNode node;
    struct OuterNode stepped;
    long long score;

    node.inner.a = 2;
    node.inner.b = 5;
    node.tail = 40LL;

    stepped = abi_outer_step(node, 3);
    score = abi_outer_score(stepped);
    printf("%d %d %lld %lld\n", stepped.inner.a, stepped.inner.b, stepped.tail, score);
    return 0;
}
