#include <stdio.h>

enum {
    X = 3,
    Y = 9
};

static int g_value = (X * Y) - (X + 1);
static int g_table[3] = {
    X + Y,
    (Y / X) + 4,
    (1 << X)
};

int main(void) {
    int sum = g_value + g_table[0] + g_table[1] + g_table[2];
    printf("%d\n", sum);
    return 0;
}
