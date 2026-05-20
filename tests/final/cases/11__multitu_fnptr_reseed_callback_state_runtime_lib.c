typedef int (*StatefulCbFn)(int *, int);

static int state_forward(int *cursor, int value) {
    *cursor += value;
    return *cursor + value;
}

static int state_twist(int *cursor, int value) {
    *cursor += value + 1;
    return *cursor + value * 2;
}

StatefulCbFn fnptr_reseed_state_pick(int seed, int *cursor) {
    StatefulCbFn table[2];
    table[0] = state_forward;
    table[1] = state_twist;
    *cursor += seed & 1;
    return table[(*cursor + seed) & 1];
}
