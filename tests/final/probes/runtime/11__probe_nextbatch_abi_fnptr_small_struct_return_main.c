#include <stdio.h>

struct NextAbiPair {
    int x;
    int y;
};

typedef struct NextAbiPair (*NextAbiPairFn)(int, int);

NextAbiPairFn nextbatch_pick_pair_fn(int selector);
int nextbatch_pair_fold(struct NextAbiPair pair, int salt);

static int local_bias(int value) {
    return value * 3 + 7;
}

int main(void) {
    NextAbiPairFn first = nextbatch_pick_pair_fn(5);
    NextAbiPairFn second = nextbatch_pick_pair_fn(8);
    struct NextAbiPair a = first(11, local_bias(2));
    struct NextAbiPair b = second(a.y, a.x - 4);
    int total = nextbatch_pair_fold(a, 19) + nextbatch_pair_fold(b, -3);

    printf("%d %d %d\n", a.x + b.x, a.y - b.y, total);
    return 0;
}
