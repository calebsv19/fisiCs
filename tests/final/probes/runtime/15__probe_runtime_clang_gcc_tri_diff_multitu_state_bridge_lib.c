typedef int (*StepFn)(int, int);

static int op_add(int s, int x) {
    return (s + x + 23) % 1009;
}

static int op_mix(int s, int x) {
    return (s * 5 + x * 3 + 17) % 1009;
}

static int op_xor(int s, int x) {
    unsigned us = (unsigned)(s & 1023);
    unsigned ux = (unsigned)(x & 1023);
    return (int)((us ^ (ux * 11u + 7u)) % 1009u);
}

static StepFn g_steps[3][3] = {
    {op_add, op_mix, op_xor},
    {op_xor, op_add, op_mix},
    {op_mix, op_xor, op_add},
};

static int g_state = 59;

int mt19_step(int row, int col, int x) {
    StepFn fn = g_steps[row % 3][col % 3];
    g_state = fn(g_state, x);
    return g_state;
}

int mt19_state(void) {
    return g_state;
}
