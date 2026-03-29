#include <stdio.h>

static volatile int g_state = 5;

static int op_add(int x) {
    g_state = g_state + x + 1;
    return g_state;
}

static int op_mul(int x) {
    g_state = g_state * 2 + x;
    return g_state;
}

static int op_xor(int x) {
    g_state = (g_state ^ (x * 3)) + 7;
    return g_state;
}

static int run_chain(int n, int (*ops[3])(int), const int vals[8]) {
    int acc = 0;
    for (int i = 0; i < n; ++i) {
        int sel = (int)((g_state + i) % 3);
        int step = ops[sel](vals[i]);
        acc += step + sel;
    }
    return acc;
}

int main(void) {
    int (*ops[3])(int) = {op_add, op_mul, op_xor};
    const int vals[8] = {2, 1, 4, 3, 6, 5, 8, 7};
    int acc = run_chain(8, ops, vals);
    printf("%d %d\n", acc, (int)g_state);
    return 0;
}
