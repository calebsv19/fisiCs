#include <stdio.h>

typedef struct {
    int x;
    int y;
} Pair;

static int score(const int *base, Pair pair) {
    return base[0] * pair.x + base[1] * pair.y + base[2];
}

int main(void) {
    int values[3];
    Pair pair;

    values[0] = 2;
    values[1] = 3;
    values[2] = 5;
    pair.x = 7;
    pair.y = 11;

    printf("%d\n", score(values, pair));
    return 0;
}
