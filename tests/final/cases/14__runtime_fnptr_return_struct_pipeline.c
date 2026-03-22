#include <stdio.h>

typedef int (*OpFn)(int, int);

typedef struct Node {
    OpFn op;
    int bias;
} Node;

typedef Node (*FactoryFn)(int);

static int op_add(int a, int b) { return a + b; }
static int op_sub(int a, int b) { return a - b; }
static int op_xor(int a, int b) { return a ^ b; }

static Node make_add(int seed) {
    Node n = {op_add, seed};
    return n;
}

static Node make_sub(int seed) {
    Node n = {op_sub, -seed};
    return n;
}

static Node make_xor(int seed) {
    Node n = {op_xor, seed * 2};
    return n;
}

static int eval(Node n, int a, int b) {
    return n.op(a, b) + n.bias;
}

int main(void) {
    FactoryFn factories[3] = {make_add, make_sub, make_xor};
    Node cache[3];
    for (int i = 0; i < 3; ++i) {
        cache[i] = factories[i](i + 3);
    }

    int acc = 0;
    for (int i = 0; i < 8; ++i) {
        FactoryFn f = (i & 1) ? factories[(i + 1) % 3] : factories[i % 3];
        Node n = (i % 3 == 0) ? cache[2] : f(i + 2);
        Node piped = (i == 4) ? factories[0](5) : n;
        acc += eval(piped, i + 7, i + 1);
    }

    int tail = eval((0 ? factories[1] : factories[2])(6), 11, 3);
    printf("%d %d\n", acc, tail);
    return 0;
}

