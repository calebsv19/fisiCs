#include <stdio.h>

typedef struct {
    int left;
    int right;
} Pair;

static Pair make_low(int seed) {
    Pair p = {seed - 1, seed + 2};
    return p;
}

static Pair make_high(int seed) {
    Pair p = {seed + 3, seed + 4};
    return p;
}

static Pair choose_pair(int flag, int seed) {
    Pair low = make_low(seed);
    Pair high = make_high(seed);
    return flag ? low : high;
}

int main(void) {
    Pair acc = {1, 2};
    int total = 0;

    for (int i = 0; i < 4; ++i) {
        Pair current = choose_pair((i + acc.left) & 1, i + 2);
        Pair merged = (current.right > acc.right) ? current : acc;
        acc = merged;
        total += acc.left * 2 + acc.right;
    }

    printf("%d %d %d\n", acc.left, acc.right, total);
    return 0;
}
