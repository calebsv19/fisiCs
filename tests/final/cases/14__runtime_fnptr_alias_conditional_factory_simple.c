#include <stdio.h>

typedef int (*UnaryFn)(int);
typedef UnaryFn (*SelectFn)(int);
typedef SelectFn (*FactoryFn)(int);

typedef struct Ctx {
    FactoryFn factory;
    int skew;
} Ctx;

static int plus1(int x) {
    return x + 1;
}

static int times2(int x) {
    return x * 2;
}

static UnaryFn select_lane(int mode) {
    return (mode & 1) ? plus1 : times2;
}

static SelectFn make_select(int seed) {
    return seed ? select_lane : select_lane;
}

int main(void) {
    Ctx lanes[2];
    lanes[0].factory = make_select;
    lanes[0].skew = 0;
    lanes[1].factory = make_select;
    lanes[1].skew = 1;

    int acc = 0;
    for (int i = 0; i < 6; ++i) {
        Ctx lane = lanes[i & 1];
        SelectFn sel = lane.factory(i + lane.skew);
        UnaryFn fn = sel(i + lane.skew);
        acc += fn(i + 3);
    }

    int tail = ((acc & 1) ? lanes[0].factory(0) : lanes[1].factory(1))(2)(7);
    printf("%d %d\n", acc, tail);
    return 0;
}
