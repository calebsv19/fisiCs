typedef int (*RouteFn)(int);

static int route_add2(int value) {
    return value + 2;
}

static int route_mul2(int value) {
    return value * 2;
}

RouteFn fnptr_table_pick(int seed) {
    RouteFn table[2];
    table[0] = route_add2;
    table[1] = route_mul2;
    return table[seed & 1];
}
