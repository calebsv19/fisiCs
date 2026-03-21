#include <stdio.h>

typedef int (*UnaryFn)(int);
typedef UnaryFn (*ChooserFn)(UnaryFn, UnaryFn, int);

static int inc(int x) {
    return x + 1;
}

static int dec(int x) {
    return x - 1;
}

static int neg(int x) {
    return -x;
}

static UnaryFn choose(UnaryFn a, UnaryFn b, int take_a) {
    return take_a ? a : b;
}

static ChooserFn select_chooser(int use_primary) {
    return use_primary ? choose : choose;
}

int main(void) {
    ChooserFn c = select_chooser(1);
    int a = c(inc, dec, 0)(7);
    int b = (0 ? select_chooser(0) : select_chooser(1))(dec, neg, 1)(5);
    int cval = select_chooser(1)(neg, inc, 0)(-9);

    printf("%d %d %d\n", a, b, cval);
    return 0;
}
