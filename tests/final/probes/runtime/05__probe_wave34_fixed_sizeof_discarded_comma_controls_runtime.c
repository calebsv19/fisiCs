#include <stdio.h>

enum {
    WAVE34_FIXED_SIZE = (int)sizeof(int[3]),
    WAVE34_FIXED_DISCARDED = 1 ? 7 : sizeof(int[3]),
    WAVE34_COMMA_DISCARDED = 1 ? 9 : (0, 4)
};

static int wave34_select(int value) {
    switch (value) {
        case WAVE34_FIXED_DISCARDED:
            return 70;
        case 1 || (0, 1):
            return 10;
        default:
            return 0;
    }
}

int main(void) {
    int ok = WAVE34_FIXED_SIZE == (int)sizeof(int[3]) &&
             WAVE34_COMMA_DISCARDED == 9 &&
             wave34_select(7) == 70 &&
             wave34_select(1) == 10;
    printf("%d\n", ok);
    return ok ? 0 : 1;
}
