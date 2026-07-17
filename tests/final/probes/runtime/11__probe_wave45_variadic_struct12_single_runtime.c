#include <stdarg.h>
#include <stdio.h>

struct Wave45Triple {
    int a;
    int b;
    int c;
};

static int wave45_struct12_va_arg(int seed, ...) {
    va_list args;
    struct Wave45Triple item;

    va_start(args, seed);
    item = va_arg(args, struct Wave45Triple);
    va_end(args);

    return seed + item.a * 3 + item.b * 5 + item.c * 7;
}

int main(void) {
    struct Wave45Triple item = { 2, 11, 17 };
    printf("%d\n", wave45_struct12_va_arg(19, item));
    return 0;
}
