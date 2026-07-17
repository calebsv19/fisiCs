typedef int (*wave48_reverse_leaf_old_t)();
typedef int (*wave48_reverse_leaf_int_t)(int);
typedef wave48_reverse_leaf_old_t (*wave48_reverse_factory_old_t)(wave48_reverse_leaf_old_t);
typedef wave48_reverse_leaf_int_t (*wave48_reverse_factory_int_t)(wave48_reverse_leaf_int_t);

wave48_reverse_factory_int_t wave48_reverse_route(void);
wave48_reverse_factory_old_t wave48_reverse_route();

static int wave48_reverse_increment(int value) {
    return value + 2;
}

static wave48_reverse_leaf_int_t wave48_reverse_passthrough(wave48_reverse_leaf_int_t callback) {
    return callback;
}

wave48_reverse_factory_int_t wave48_reverse_route(void) {
    return wave48_reverse_passthrough;
}

int main(void) {
    return wave48_reverse_route()(wave48_reverse_increment)(40) == 42 ? 0 : 1;
}
