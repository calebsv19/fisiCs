typedef int (*StateRouteFn)(int *, int);

static int state_route_add(int *state, int value) {
    *state += value;
    return *state;
}

static int state_route_mix(int *state, int value) {
    *state += value + 1;
    return value * 2 + *state;
}

StateRouteFn fnptr_state_route_pick(int seed) {
    StateRouteFn table[2];
    table[0] = state_route_add;
    table[1] = state_route_mix;
    return table[seed & 1];
}
