#include <stdio.h>

static int wave85_same_block(void) {
    int retained = 17;

    goto wave85_done;

    int bypassed = 91;

wave85_done:
    return retained + 18;
}

int main(void) {
    int value = wave85_same_block();
    printf("%d\n", value);
    return value != 35;
}
