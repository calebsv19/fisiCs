#include <stdio.h>

static int g[8] = {
    [0] = 1,
    [3] = 7,
    [7] = 9
};

int main(void) {
    printf("%d %d %d %d\n", g[0], g[1], g[3], g[7]);
    return 0;
}
