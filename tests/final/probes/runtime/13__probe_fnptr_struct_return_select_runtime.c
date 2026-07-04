#include <stdio.h>

typedef struct {
    int left;
    int right;
} Pair;

static Pair make_up(int x) {
    Pair p = {x, x + 3};
    return p;
}

static Pair make_down(int x) {
    Pair p = {x - 2, x + 5};
    return p;
}

static int fold_pair(Pair p) {
    return p.left * 10 + p.right;
}

int main(void) {
    int seed = 4;
    Pair (*pick)(int) = (seed & 1) ? make_up : make_down;
    Pair first = pick(seed);
    Pair second = ((first.right - first.left) > 4 ? make_up : make_down)(first.left + 1);
    Pair out = (second.left < second.right) ? second : first;

    out.right += first.left;
    printf("%d %d %d\n", first.left, second.right, fold_pair(out));
    return 0;
}
