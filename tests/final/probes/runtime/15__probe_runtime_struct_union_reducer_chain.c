#include <stdio.h>

typedef union Payload {
    int i;
    unsigned u;
} Payload;

typedef struct Node {
    int tag;
    Payload payload;
} Node;

typedef int (*ReduceStep)(Node *, int);

static int step_add(Node *node, int input) {
    node->payload.i = node->payload.i + input + node->tag * 3;
    return node->payload.i;
}

static int step_mix(Node *node, int input) {
    node->payload.u = (unsigned)(node->payload.i * 9 + input * 5 + node->tag + 17);
    node->payload.i = (int)(node->payload.u % 100003u);
    return node->payload.i;
}

static int step_rot(Node *node, int input) {
    unsigned v = (unsigned)(node->payload.i & 0x7FFFFFFF);
    unsigned s = (unsigned)(input & 31);
    unsigned left = (v << s) & 0x7FFFFFFFu;
    unsigned right = v >> ((32u - s) & 31u);
    node->payload.i = (int)((left | right) % 100003u);
    return node->payload.i;
}

static int run_chain(Node *node, const int *values, int n) {
    ReduceStep table[3] = {step_add, step_mix, step_rot};
    int acc = 0;
    for (int i = 0; i < n; ++i) {
        ReduceStep step = table[(i + node->tag) % 3];
        acc += step(node, values[i]) + i * 7;
    }
    return acc + node->payload.i;
}

int main(void) {
    int in_a[8] = {11, 7, 19, 5, 23, 3, 29, 2};
    int in_b[8] = {4, 8, 15, 16, 23, 42, 6, 9};
    Node a = {1, {.i = 41}};
    Node b = {2, {.i = 73}};

    int out_a = run_chain(&a, in_a, 8);
    int out_b = run_chain(&b, in_b, 8);
    int final = out_a * 3 + out_b * 5 + a.payload.i - b.payload.i;

    printf("%d %d %d\n", out_a, out_b, final);
    return 0;
}
