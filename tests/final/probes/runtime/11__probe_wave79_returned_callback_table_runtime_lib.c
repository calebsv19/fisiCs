struct wave79_state {
    long accumulator;
    int calls;
};

struct wave79_payload {
    double x;
    long bias;
    int lane;
};

struct wave79_result {
    long a;
    long b;
    long c;
    long d;
};

typedef struct wave79_result (*wave79_callback_fn)(struct wave79_state *state,
                                                    struct wave79_payload payload,
                                                    long salt);

struct wave79_table {
    wave79_callback_fn first;
    wave79_callback_fn second;
    long epoch;
    struct wave79_state *state;
};

static struct wave79_result wave79_add(struct wave79_state *state,
                                       struct wave79_payload payload,
                                       long salt) {
    struct wave79_result out;
    state->accumulator += salt + payload.bias + payload.lane;
    state->calls += 1;
    out.a = state->accumulator;
    out.b = (long)(payload.x * 10.0);
    out.c = state->calls + payload.lane;
    out.d = out.a + out.b + out.c;
    return out;
}

static struct wave79_result wave79_subtract(struct wave79_state *state,
                                            struct wave79_payload payload,
                                            long salt) {
    struct wave79_result out;
    state->accumulator -= salt + payload.bias - payload.lane;
    state->calls += 1;
    out.a = state->accumulator;
    out.b = (long)(payload.x * 4.0) + payload.bias;
    out.c = state->calls - payload.lane;
    out.d = out.a - out.b + out.c;
    return out;
}

struct wave79_table wave79_make_table(struct wave79_state *state, int mode) {
    struct wave79_table table;
    table.first = (mode & 1) == 0 ? wave79_add : wave79_subtract;
    table.second = (mode & 1) == 0 ? wave79_subtract : wave79_add;
    table.epoch = 100 + mode;
    table.state = state;
    return table;
}
