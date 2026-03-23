#include <stdio.h>

typedef int (*UnaryFn)(int);
typedef UnaryFn (*ChooserFn)(UnaryFn, UnaryFn, int);
typedef ChooserFn (*FactoryFn)(int);

typedef struct Lane {
    FactoryFn factory;
    int bias;
} Lane;

static int add2(int x) {
    return x + 2;
}

static int sub3(int x) {
    return x - 3;
}

static UnaryFn choose(UnaryFn a, UnaryFn b, int take_a) {
    return take_a ? a : b;
}

static ChooserFn make_chooser(int mode) {
    return mode ? choose : choose;
}

int main(void) {
    Lane lanes[2];
    lanes[0].factory = make_chooser;
    lanes[0].bias = 1;
    lanes[1].factory = make_chooser;
    lanes[1].bias = 0;

    int acc = 0;
    for (int i = 0; i < 5; ++i) {
        Lane lane = lanes[i & 1];
        ChooserFn chooser = lane.factory(i & 1);
        int pick = (i + lane.bias + (i >> 1)) & 1;
        UnaryFn step = chooser(add2, sub3, pick);
        acc += step(i * 3);
    }

    ChooserFn tail_chooser = (acc & 1) ? choose : make_chooser(0);
    UnaryFn tail_step = tail_chooser(sub3, add2, acc > 20);
    int tail = tail_step(10);
    printf("%d %d\n", acc, tail);
    return 0;
}
