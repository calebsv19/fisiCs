typedef int (*wave48_leaf_old_t)();
typedef int (*wave48_leaf_int_t)(int);
typedef wave48_leaf_old_t (*wave48_factory_old_t)(wave48_leaf_old_t);
typedef wave48_leaf_int_t (*wave48_factory_int_t)(wave48_leaf_int_t);

wave48_factory_old_t wave48_route();
wave48_factory_int_t wave48_route(void);

static int wave48_increment(int value) {
    return value + 1;
}

static wave48_leaf_int_t wave48_passthrough(wave48_leaf_int_t callback) {
    return callback;
}

wave48_factory_int_t wave48_route(void) {
    return wave48_passthrough;
}

int main(void) {
    return wave48_route()(wave48_increment)(41) == 42 ? 0 : 1;
}
