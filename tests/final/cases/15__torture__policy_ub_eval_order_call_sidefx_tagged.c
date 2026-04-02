#include <stdio.h>

static int blend(int a, int b, int c) {
    return a * 17 + b * 31 + c * 43;
}

static int lane(int seed) {
    int x = seed;
    int score = blend((x = 11), (x = 11), (x = 11));
    return x * 19 + score;
}

int main(void) {
    int a = lane(2);
    int b = lane(9);
    printf("%d %d\n", a, b);
    return 0;
}
