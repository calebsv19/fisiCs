#include <stdio.h>

typedef int (*StateCallback)(int *, int);

int fnptr_callback_state_fold(int *state, int count, StateCallback cb);

static int callback_state_mix(int *state, int step) {
    *state += step + 2;
    return (*state) * (step + 1);
}

int main(void) {
    int state = 3;
    int total = fnptr_callback_state_fold(&state, 3, callback_state_mix);
    if (state != 12) return 1;
    if (total != 57) return 2;
    printf("%d %d\n", total, state);
    return 0;
}
