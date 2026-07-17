#include <stdio.h>

typedef struct {
    int lo;
    int hi;
} Wave46Pair;

typedef struct {
    Wave46Pair pairs[3];
    int bias;
} Wave46Bundle;

static Wave46Bundle make_bundle(int seed, int flip) {
    Wave46Bundle bundle;
    int i;
    bundle.bias = seed * 5 - flip;
    for (i = 0; i < 3; ++i) {
        bundle.pairs[i].lo = seed + i * 2 + flip;
        bundle.pairs[i].hi = seed * (i + 2) - flip - i;
    }
    return bundle;
}

static Wave46Bundle adjust_bundle(Wave46Bundle input, int step) {
    Wave46Bundle out = input;
    out.bias += step * 3;
    out.pairs[step % 3].lo += input.bias - step;
    out.pairs[(step + 1) % 3].hi -= input.pairs[0].lo + step;
    return out;
}

static int score_bundle(Wave46Bundle bundle) {
    int total = bundle.bias;
    int i;
    for (i = 0; i < 3; ++i) {
        total += bundle.pairs[i].lo * (i + 2) - bundle.pairs[i].hi * (i + 1);
    }
    return total;
}

int main(void) {
    Wave46Bundle current = make_bundle(4, 1);
    int total = 0;
    int i;

    for (i = 0; i < 10; ++i) {
        Wave46Bundle made = make_bundle(i + 2, current.bias & 3);
        Wave46Bundle adjusted = adjust_bundle(current, i);
        current = (score_bundle(made) > score_bundle(adjusted) - i) ? made : adjusted;
        total += score_bundle(current);
    }

    printf("%d %d %d %d\n", current.bias, current.pairs[1].lo, score_bundle(current), total);
    return 0;
}
