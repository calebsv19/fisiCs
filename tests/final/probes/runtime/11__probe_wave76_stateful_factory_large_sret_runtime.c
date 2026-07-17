#include <stdio.h>

struct wave76_state {
    long accumulator;
    int calls;
};

struct wave76_payload {
    double x;
    double y;
    int lane;
};

struct wave76_result {
    long first;
    long second;
    long third;
    long fourth;
};

typedef struct wave76_result (*wave76_leaf_fn)(struct wave76_state *state,
                                                struct wave76_payload payload,
                                                long salt);

static struct wave76_result wave76_add(struct wave76_state *state,
                                       struct wave76_payload payload,
                                       long salt) {
    struct wave76_result out;
    state->accumulator += salt + payload.lane;
    state->calls += 1;
    out.first = state->accumulator;
    out.second = (long)payload.x * 10 + (long)payload.y;
    out.third = state->calls + payload.lane;
    out.fourth = out.first + out.second;
    return out;
}

static struct wave76_result wave76_subtract(struct wave76_state *state,
                                            struct wave76_payload payload,
                                            long salt) {
    struct wave76_result out;
    state->accumulator -= salt - payload.lane;
    state->calls += 1;
    out.first = state->accumulator;
    out.second = (long)payload.y * 10 - (long)payload.x;
    out.third = state->calls - payload.lane;
    out.fourth = out.first - out.second;
    return out;
}

static wave76_leaf_fn wave76_factory(int mode) {
    return (mode & 1) == 0 ? wave76_add : wave76_subtract;
}

int main(void) {
    struct wave76_state state = {10, 0};
    struct wave76_payload first_payload = {2.0, 3.0, 4};
    struct wave76_payload second_payload = {4.0, 1.0, 7};
    wave76_leaf_fn leaf = wave76_factory(4);
    struct wave76_result first = leaf(&state, first_payload, 5);
    struct wave76_result second = leaf(&state, second_payload, 2);

    printf("%ld %ld %ld %ld | %ld %ld %ld %ld\n",
           first.first, first.second, first.third, first.fourth,
           second.first, second.second, second.third, second.fourth);
    return 0;
}
