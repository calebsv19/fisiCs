#include <stdio.h>

static int bump(int* state, int delta) {
    *state += delta;
    return *state;
}

static int choose(int condition, int* state) {
    return condition ? bump(state, 3) : bump(state, 30);
}

int main(void) {
    int state = 0;
    int total = 0;
    int i = 0;

    for (i = 0; i < 4; ++i) {
        total += (i % 2 == 0) ? choose(1, &state) : choose(0, &state);
        total += (bump(&state, 1), state);
        if (i == 2) {
            total += (state > 20) ? 5 : 2;
        }
    }

    printf("%d %d\n", state, total);
    return 0;
}
