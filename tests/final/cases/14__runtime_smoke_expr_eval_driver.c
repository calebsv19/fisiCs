#include <stdio.h>

enum OpCode {
    OP_ADD = 0,
    OP_SUB = 1,
    OP_MUL = 2,
    OP_XOR = 3
};

struct Expr {
    enum OpCode op;
    int a;
    int b;
};

static int eval_expr(struct Expr e) {
    switch (e.op) {
        case OP_ADD: return e.a + e.b;
        case OP_SUB: return e.a - e.b;
        case OP_MUL: return e.a * e.b;
        case OP_XOR: return (e.a ^ e.b) + 5;
        default: return 0;
    }
}

int main(void) {
    struct Expr program[] = {
        {OP_ADD, 7, 3},
        {OP_MUL, 2, 5},
        {OP_SUB, 40, 9},
        {OP_XOR, 11, 6},
        {OP_ADD, -3, 8},
        {OP_MUL, 4, -2}
    };

    int acc = 13;
    int trace = 0;
    int i;
    for (i = 0; i < (int)(sizeof(program) / sizeof(program[0])); ++i) {
        int v = eval_expr(program[i]);
        acc = acc * 3 + v;
        trace += v * (i + 1);
    }

    printf("%d %d\n", acc, trace);
    return 0;
}
