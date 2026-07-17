#include <stdio.h>

static int wave26_increment(int value) {
    return value + 1;
}

static int wave26_triple(int value) {
    return value * 3;
}

int main(void) {
    int (*callback)(int) = wave26_increment;
    int first = callback(6);

    callback = wave26_triple;
    printf("%d %d\n", first, callback(6));
    return 0;
}
