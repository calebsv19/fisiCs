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

int main(void) {
    ChooserFn table[2] = {choose, choose};
    int x = (0 ? table[0] : table[1])(inc, dec, 0)(9);
    int y = table[1](dec, inc, 1)(4);
    int z = (1 ? table[0] : table[1])(inc, dec, 1)(2);

    printf("%d %d %d\n", x, y, z);
    return 0;
}
