typedef unsigned (*StepFn)(unsigned, unsigned);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned op_add(unsigned s, unsigned x) {
    return rotl32(s + x + 0x9e3779b9u, 7u);
}

static unsigned op_mul(unsigned s, unsigned x) {
    unsigned y = (x | 1u) * 0x45d9f3bu;
    return rotl32((s ^ y) + 0x27d4eb2du, 11u);
}

static unsigned op_xor(unsigned s, unsigned x) {
    return rotl32((s ^ (x * 0x85ebca6bu)) + 0xc2b2ae35u, 13u);
}

static unsigned op_mix(unsigned s, unsigned x) {
    unsigned y = s + x * 0x9e3779b1u + 0x7f4a7c15u;
    y ^= y >> 16u;
    return rotl32(y, 5u);
}

static StepFn g_table[4][4] = {
    {op_add, op_mul, op_xor, op_mix},
    {op_mix, op_add, op_mul, op_xor},
    {op_xor, op_mix, op_add, op_mul},
    {op_mul, op_xor, op_mix, op_add},
};

static unsigned g_state = 0x6d2b79f5u;

unsigned w25_fn_step(unsigned row, unsigned col, unsigned seed) {
    StepFn fn = g_table[(row + seed) & 3u][(col + (seed >> 1u)) & 3u];
    unsigned salt = row * 97u + col * 131u + 0x3du;
    g_state = fn(g_state ^ salt, seed + row + col);
    return g_state;
}

unsigned w25_fn_state(void) {
    return g_state;
}
