#include <stdio.h>

typedef int (*Op)(int, int);

static int op_add(int x, int y) {
    return x + y * 2 + 1;
}

static int op_mix(int x, int y) {
    return (x ^ (y * 13 + 5)) + (x & 7);
}

static int op_sub(int x, int y) {
    return x - y * 3 + 9;
}

struct Engine {
    int bias;
    Op ops[3];
};

static int run_engine(struct Engine *engine, int seed) {
    int value = seed;

    for (int i = 0; i < 9; ++i) {
        int idx = (value + i + engine->bias) % 3;
        if (idx < 0) {
            idx += 3;
        }
        Op op = engine->ops[idx];
        if ((i & 1) == 0) {
            op = (value & 1) ? engine->ops[2] : op;
        }
        value = op(value, engine->bias + i);
        if (value < 0) {
            value = -value + 7;
        }
        value %= 1000;
        engine->bias = (engine->bias * 5 + i + 3) & 15;
    }

    return value + engine->bias;
}

int main(void) {
    struct Engine engine = {
        4,
        {op_add, op_mix, op_sub},
    };

    printf("%d\n", run_engine(&engine, 9));
    return 0;
}
