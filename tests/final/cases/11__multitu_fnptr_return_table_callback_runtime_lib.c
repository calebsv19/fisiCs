typedef int (*BinaryCb)(int, int);
typedef int (*RouteCbFn)(int, BinaryCb);

static int route_forward(int value, BinaryCb cb) {
    return cb(value, 2);
}

static int route_split(int value, BinaryCb cb) {
    return cb(value, 1) + cb(value, 2);
}

RouteCbFn fnptr_route_callback_pick(int seed) {
    RouteCbFn table[2];
    table[0] = route_forward;
    table[1] = route_split;
    return table[seed & 1];
}
