typedef int wave41_nested_t;

static int wave41_increment(int wave41_nested_t) {
    return wave41_nested_t + 1;
}

static int wave41_route(int (*callback)(int wave41_nested_t),
                        wave41_nested_t value);

static int wave41_route(int (*callback)(int wave41_nested_t),
                        wave41_nested_t value) {
    return callback(value);
}

int main(void) {
    return wave41_route(wave41_increment, 41) == 42 ? 0 : 1;
}
