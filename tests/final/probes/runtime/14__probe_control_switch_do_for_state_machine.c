#include <stdio.h>

static int step(int state, int lane) {
    switch ((state + lane) & 3) {
        case 0: return state + lane * 3 + 1;
        case 1: return state ^ (lane * 5 + 7);
        case 2: return state - lane * 2 + 9;
        default: return state + lane + 13;
    }
}

int main(void) {
    int state = 5;

    for (int i = 0; i < 12; ++i) {
        int j = 0;
        do {
            state = step(state + i, j);
            if ((state & 7) == 3) {
                ++j;
                continue;
            }
            state ^= (i + j * 11);
            ++j;
        } while (j < 4);
    }

    printf("%d\n", state);
    return 0;
}
