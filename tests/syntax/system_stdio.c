#include <stdio.h>

int main(void) {
    struct Pair { int a; int b; } p = { .a = 1, .b = 2 };
    printf("pair: %d %d\n", p.a, p.b);
    return 0;
}
