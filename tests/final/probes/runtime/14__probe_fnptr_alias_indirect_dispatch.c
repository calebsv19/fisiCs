#include <stdio.h>

typedef int (*OpFn)(int, int);
typedef OpFn (*ChooserFn)(int);

typedef struct Lane {
    ChooserFn chooser;
    int bias;
} Lane;

static int add_lane(int a, int b) {
    return a + b;
}

static int sub_lane(int a, int b) {
    return a - b;
}

static int xor_lane(int a, int b) {
    return a ^ b;
}

static OpFn pick(int code) {
    if ((code & 3) == 0) return add_lane;
    if ((code & 3) == 1) return sub_lane;
    return xor_lane;
}

static int run(const Lane* lane, int x, int y) {
    OpFn op = lane->chooser(x + y + lane->bias);
    return op(x + lane->bias, y - lane->bias);
}

int main(void) {
    Lane lanes[3];
    lanes[0].chooser = pick;
    lanes[0].bias = 2;
    lanes[1].chooser = pick;
    lanes[1].bias = -3;
    lanes[2].chooser = pick;
    lanes[2].bias = 5;

    int acc = 0;
    for (int i = 0; i < 7; ++i) {
        const Lane* lane = &lanes[i % 3];
        acc += run(lane, i + 10, (i * 3) + 1);
    }

    OpFn tail = lanes[1].chooser(acc);
    int out = tail(acc & 31, lanes[2].bias);
    printf("%d %d\n", acc, out);
    return 0;
}
