#include <stdio.h>

typedef int (*StateRouteFn)(int *, int);

StateRouteFn fnptr_state_route_pick(int seed);

static int run_state_route(StateRouteFn fn, int *state, int value) {
    return fn(state, value);
}

int main(void) {
    int state = 5;
    int total = 0;
    total += run_state_route(fnptr_state_route_pick(2), &state, 3);
    total += run_state_route(fnptr_state_route_pick(5), &state, 4);
    if (state != 13) return 1;
    if (total != 29) return 2;
    printf("%d %d\n", total, state);
    return 0;
}
