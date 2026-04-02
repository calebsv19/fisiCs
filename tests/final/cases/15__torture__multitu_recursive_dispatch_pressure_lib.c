typedef unsigned (*OpFn)(unsigned, unsigned);

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

static OpFn g_ops[4][4] = {
    {op_add, op_mul, op_xor, op_mix},
    {op_mix, op_add, op_mul, op_xor},
    {op_xor, op_mix, op_add, op_mul},
    {op_mul, op_xor, op_mix, op_add},
};

static unsigned g_state = 0xa341316cu;

unsigned mt27_dispatch(unsigned depth, unsigned lhs, unsigned rhs) {
    unsigned mix = lhs * 131u + rhs * 197u + depth * 17u + g_state;
    OpFn fn = g_ops[(depth + lhs) & 3u][(rhs + (g_state >> 5u)) & 3u];
    unsigned next_l = rhs ^ (mix >> 3u);
    unsigned next_r = lhs + (mix & 255u);

    g_state = fn(g_state ^ mix, lhs + (rhs << 1u));
    if (depth == 0u) {
        return g_state ^ mix;
    }
    return mt27_dispatch(depth - 1u, next_l, next_r) ^ g_state ^ (depth * 97u);
}

unsigned mt27_terminal(void) {
    return g_state;
}
