typedef int (*ReseedRouteFn)(int, int *);

static int route_reseed_add(int value, int *cursor) {
    *cursor += 3;
    return value + *cursor;
}

static int route_reseed_mul(int value, int *cursor) {
    *cursor += 2;
    return value * 2 + *cursor;
}

ReseedRouteFn fnptr_table_reseed_pick(int seed, int *cursor) {
    ReseedRouteFn table[2];
    table[0] = route_reseed_add;
    table[1] = route_reseed_mul;
    *cursor += seed & 1;
    return table[(seed + *cursor) & 1];
}
