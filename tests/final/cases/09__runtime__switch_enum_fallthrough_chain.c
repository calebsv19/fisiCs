#include <stdio.h>

enum Stage {
    STAGE_ZERO = 0,
    STAGE_ONE = 1,
    STAGE_TWO = 2,
    STAGE_OTHER = 9
};

static int score(enum Stage stage) {
    int out = 0;

    switch (stage) {
        case STAGE_ZERO:
            out += 1;
            /* fallthrough */
        case STAGE_ONE:
            out += 10;
            break;
        case STAGE_TWO:
            out += 100;
            break;
        default:
            out += 1000;
            break;
    }

    return out;
}

int main(void) {
    enum Stage sequence[] = {STAGE_ZERO, STAGE_ONE, STAGE_TWO, STAGE_OTHER};
    int total = 0;
    int i = 0;

    for (i = 0; i < 4; ++i) {
        total += score(sequence[i]);
    }

    printf("%d\n", total);
    return 0;
}
