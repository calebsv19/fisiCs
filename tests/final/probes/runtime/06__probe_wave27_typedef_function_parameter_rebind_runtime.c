#include <stdio.h>

typedef int Wave27Function(int);

static int wave27_add_two(int value) {
    return value + 2;
}

static int wave27_triple(int value) {
    return value * 3;
}

static int wave27_rebind_function(Wave27Function callback,
                                  Wave27Function replacement) {
    int first = callback(4);
    callback = replacement;
    return first * 10 + callback(4);
}

int main(void) {
    printf("%d\n", wave27_rebind_function(wave27_add_two, wave27_triple));
    return 0;
}
