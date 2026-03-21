#include <stdio.h>

typedef int (*UnaryFn)(int);
typedef UnaryFn (*ChooserFn)(UnaryFn, UnaryFn, int);

static int inc(int x) {
    return x + 1;
}

static int dec(int x) {
    return x - 1;
}

static UnaryFn choose(UnaryFn a, UnaryFn b, int take_a) {
    return take_a ? a : b;
}

static ChooserFn chooser_id(ChooserFn in) {
    return in;
}

int main(void) {
    ChooserFn c = chooser_id(choose);
    int a = c(inc, dec, 0)(7);
    int b = chooser_id(choose)(dec, inc, 1)(9);

    printf("%d %d\n", a, b);
    return 0;
}
