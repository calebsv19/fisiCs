#include <stdio.h>

static int g_values[5] = {1, 2};
static int g_seed = 7;

int main(void) {
    int sum = 0;

    for (int i = 0; i < 5; ++i) {
        sum += g_values[i];
    }

    printf("%d %d\n", sum, g_seed);
    return 0;
}
