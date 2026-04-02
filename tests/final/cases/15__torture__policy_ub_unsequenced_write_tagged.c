#include <stdio.h>

static int run_lane(int seed) {
    int x = seed;
    int sum = (x = 7) + (x = 7);
    return (x * 31) + sum;
}

int main(void) {
    int a = run_lane(1);
    int b = run_lane(9);
    printf("%d %d\n", a, b);
    return 0;
}
