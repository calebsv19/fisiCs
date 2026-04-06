typedef int (*BinOp)(int, int);
typedef BinOp BinPair[2];

static int op_add(int a, int b) {
    return a + b;
}

static int op_sub(int a, int b) {
    return a - b;
}

static int op_mul(int a, int b) {
    return a * b;
}

static int op_mix(int a, int b) {
    return (a << 1) ^ (b + 7);
}

static BinPair g_pair0 = { op_add, op_sub };
static BinPair g_pair1 = { op_mul, op_mix };

typedef struct Node Node;
struct Node {
    int seed;
    BinOp blend;
    Node *next;
};

static int fold_chain(Node *head, const int *xs, unsigned n) {
    int acc = 17;
    unsigned i;
    Node *cur = head;
    for (i = 0; i < n; ++i) {
        BinOp f;
        int a;
        int b;

        if (cur == ((Node *)0)) {
            break;
        }
        if (cur->seed & 1) {
            f = g_pair1[(unsigned)(cur->seed & 1)];
        } else {
            f = g_pair0[(unsigned)(cur->seed & 1)];
        }
        a = f(xs[i], cur->seed);
        b = cur->blend(a, (int)i + 3);
        acc = (acc * 131) ^ b;
        cur = cur->next;
    }
    return acc;
}

int main(void) {
    int values[] = { 3, 5, 8, 13, 21 };
    Node n3 = { 4, op_mix, (Node *)0 };
    Node n2 = { 3, op_mul, &n3 };
    Node n1 = { 2, op_sub, &n2 };
    Node n0 = { 1, op_add, &n1 };
    int out = fold_chain(&n0, values, (unsigned)(sizeof(values) / sizeof(values[0])));
    return out & 255;
}
