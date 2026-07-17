#include <stdio.h>

static int wave26_add_three(int value) {
    return value + 3;
}

static int wave26_double(int value) {
    return value * 2;
}

static int wave26_reassign_callback(int callback(int), int replacement(int)) {
    int first = callback(5);
    callback = replacement;
    return first * 10 + callback(5);
}

int main(void) {
    printf("%d\n", wave26_reassign_callback(wave26_add_three, wave26_double));
    return 0;
}
