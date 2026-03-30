typedef int (*OpFn)(int, int);

static int op_add(int a, int b) {
    return (a + b + 3) % 257;
}

static int op_xor(int a, int b) {
    return ((a ^ b) + 5) % 257;
}

static int op_mix(int a, int b) {
    return (a * 3 + b * 7 + 1) % 257;
}

static int op_fold(int a, int b) {
    int delta = a - b;
    if (delta < 0) {
        delta = -delta;
    }
    return (delta + 11) % 257;
}

static int op_rot(int a, int b) {
    unsigned ua = (unsigned)(a & 255);
    unsigned ub = (unsigned)(b & 255);
    unsigned rot = ((ua << 1) | (ua >> 7)) & 255u;
    return (int)((rot + (ub & 31u) + 13u) % 257u);
}

static OpFn g_dispatch[4][5] = {
    {op_add, op_xor, op_mix, op_fold, op_rot},
    {op_mix, op_add, op_rot, op_xor, op_fold},
    {op_fold, op_rot, op_add, op_mix, op_xor},
    {op_xor, op_fold, op_mix, op_rot, op_add},
};

static int g_row_seed[6] = {5, 9, 2, 8, 1, 7};
static int g_col_seed[7] = {3, 4, 6, 1, 2, 8, 5};

OpFn lib_pick_op(int row, int col) {
    return g_dispatch[row & 3][col % 5];
}

int lib_seed(int row, int col) {
    int base = g_row_seed[row % 6] * 17 + g_col_seed[col % 7] * 11;
    return (base + row * 3 + col * 5) % 257;
}
