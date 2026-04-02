#include <stdio.h>

static int plus1(int x) { return x + 1; }
static int plus2(int x) { return x + 2; }

int main(void) {
    int (*table[3])(int) = {plus1, 0, plus2};
    int a = table[0] ? table[0](10) : -1;
    int b = table[1] ? table[1](10) : -2;
    int c = table[2] ? table[2](10) : -3;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
