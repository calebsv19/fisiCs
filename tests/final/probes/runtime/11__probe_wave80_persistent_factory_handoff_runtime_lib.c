typedef long (*wave80_callback_fn)(long value, long generation);

struct wave80_route {
    wave80_callback_fn callback;
    long generation;
};

static long wave80_generation;
static long wave80_accumulator = 20;

static long wave80_add(long value, long generation) {
    wave80_accumulator += value + generation;
    return wave80_accumulator;
}

static long wave80_subtract(long value, long generation) {
    wave80_accumulator -= value - generation;
    return wave80_accumulator;
}

struct wave80_route wave80_factory(int mode) {
    struct wave80_route route;
    wave80_generation += 1;
    route.callback = (mode & 1) == 0 ? wave80_add : wave80_subtract;
    route.generation = wave80_generation;
    return route;
}
