#include <stdio.h>

typedef int (*UnaryFn)(int);
typedef UnaryFn (*ChooserFn)(UnaryFn, UnaryFn, int);
typedef ChooserFn ChooserAlias;

static int inc(int x) {
    return x + 1;
}

static int dec(int x) {
    return x - 1;
}

static int dbl(int x) {
    return x * 2;
}

static UnaryFn choose(UnaryFn a, UnaryFn b, int take_a) {
    return take_a ? a : b;
}

static ChooserAlias chooser_passthrough(ChooserAlias c) {
    return c;
}

int main(void) {
    ChooserAlias c = chooser_passthrough(choose);
    int a = c(inc, dec, 1)(10);
    int b = chooser_passthrough(c)(dec, dbl, 0)(7);
    int d = chooser_passthrough(choose)(dbl, dec, 1)(8);

    printf("%d %d %d\n", a, b, d);
    return 0;
}
