#include <stdio.h>

typedef int (*OpFn)(int, int);

typedef struct Stage {
    OpFn fn;
    int bias;
} Stage;

static int op_add(int a, int b) {
    return a + b;
}

static int op_sub(int a, int b) {
    return a - b;
}

static int op_xor(int a, int b) {
    return a ^ b;
}

static int op_mix(int a, int b) {
    return (a * 3 + b * 5) & 127;
}

static Stage make_stage(int seed) {
    Stage s;
    if ((seed & 3) == 0) s.fn = op_add;
    else if ((seed & 3) == 1) s.fn = op_sub;
    else if ((seed & 3) == 2) s.fn = op_xor;
    else s.fn = op_mix;
    s.bias = seed * 2 - 3;
    return s;
}

static int run_stage(Stage s, int x, int y) {
    return s.fn(x, y) + s.bias;
}

int main(void) {
    Stage table[4] = {
        make_stage(2),
        make_stage(3),
        make_stage(4),
        make_stage(5)
    };

    int acc = 0;
    for (int i = 0; i < 10; ++i) {
        Stage s = (i % 3 == 0) ? make_stage(i + 6) : table[(i + 1) & 3];
        int y = (acc & 7) + 1;
        acc += run_stage(s, i + 3, y);
    }

    int tail = run_stage((0 ? make_stage(9) : table[2]), 9, 4);
    printf("%d %d\n", acc, tail);
    return 0;
}

