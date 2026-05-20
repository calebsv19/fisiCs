#include <stdio.h>

enum BiasStage {
    BIAS_ZERO = 0,
    BIAS_ONE = 1,
    BIAS_TWO = 2,
    BIAS_ONE_ALIAS = 1
};

static int score(enum BiasStage stage) {
    switch (stage) {
        case BIAS_ZERO:
            return 4;
        case BIAS_ONE:
            return 7;
        case BIAS_TWO:
            return 9;
        default:
            return 1;
    }
}

int main(void) {
    enum BiasStage inputs[] = {BIAS_ZERO, BIAS_ONE_ALIAS, BIAS_TWO, (enum BiasStage)9};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 4; ++i) {
        acc = acc * 10 + score(inputs[i]);
    }

    printf("%d\n", acc);
    return 0;
}
