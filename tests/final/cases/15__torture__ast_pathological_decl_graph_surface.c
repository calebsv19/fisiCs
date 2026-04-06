#include <stdio.h>

typedef int (*Op)(int, int);
typedef Op OpVec[3];

typedef struct Layer Layer;

typedef struct {
    int seed;
    OpVec ops;
    Layer *next;
} Frame;

struct Layer {
    Frame frame;
    int bias;
};

static int op_add(int a, int b) {
    return a + b;
}

static int op_sub(int a, int b) {
    return a - b;
}

static int op_mix(int a, int b) {
    return (a << 1) ^ (b * 3 + 7);
}

static int op_fold(int a, int b) {
    return (a * 5) + (b ^ 11);
}

static int run_graph(const Layer *head, const int *values, unsigned n) {
    int acc = 17;
    const Layer *cur = head;

    for (unsigned i = 0u; i < n; ++i) {
        unsigned lane;
        Op f0;
        Op f1;
        int x;
        int y;

        if (cur == (const Layer *)0) {
            break;
        }

        lane = (unsigned)((cur->frame.seed ^ (int)i) % 3);
        f0 = cur->frame.ops[lane];
        f1 = cur->frame.ops[(lane + 1u) % 3u];
        x = f0(values[i], cur->frame.seed + cur->bias);
        y = f1(x, (int)i + cur->bias);
        acc = (acc * 131) ^ (y + cur->bias * 7);

        if ((i & 1u) != 0u) {
            cur = cur->frame.next;
        }
    }

    return acc;
}

int main(void) {
    int values[] = {3, 5, 8, 13, 21, 34, 55};

    Layer l3 = {
        {4, {op_mix, op_fold, op_add}, (Layer *)0},
        9
    };
    Layer l2 = {
        {3, {op_sub, op_add, op_mix}, &l3},
        7
    };
    Layer l1 = {
        {2, {op_fold, op_sub, op_add}, &l2},
        5
    };
    Layer l0 = {
        {1, {op_add, op_mix, op_fold}, &l1},
        3
    };

    int a = run_graph(&l0, values, (unsigned)(sizeof(values) / sizeof(values[0])));
    int b = run_graph(&l1, values + 1, (unsigned)(sizeof(values) / sizeof(values[0]) - 1u));
    printf("%d %d\n", a, a ^ (b * 3 + 17));
    return 0;
}
