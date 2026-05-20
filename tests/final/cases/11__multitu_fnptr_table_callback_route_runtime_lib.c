typedef int (*BinaryCallback)(int, int);
typedef int (*CallbackRouteFn)(int, BinaryCallback);

static int route_apply_once(int value, BinaryCallback cb) {
    return cb(value, 3);
}

static int route_apply_twice(int value, BinaryCallback cb) {
    return cb(value, 2) + cb(value, 1);
}

CallbackRouteFn fnptr_callback_route_pick(int seed) {
    CallbackRouteFn table[2];
    table[0] = route_apply_once;
    table[1] = route_apply_twice;
    return table[seed & 1];
}
